/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "EffectInstance.h"
#include "EffectInstancePrivate.h"

#include <map>
#include <sstream>
#include <algorithm> // min, max
#include <stdexcept>
#include <fstream>

#include <QtConcurrentMap>
#include <QReadWriteLock>
#include <QCoreApplication>
#include <QtConcurrentRun>
#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /usr/local/include/boost/bind/arg.hpp:37:9: warning: unused typedef 'boost_static_assert_typedef_37' [-Wunused-local-typedef]
#include <boost/bind.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif
#include <SequenceParsing.h>

#include "Global/MemoryInfo.h"
#include "Global/QtCompat.h"

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/BlockingBackgroundRender.h"
#include "Engine/DiskCacheNode.h"
#include "Engine/Image.h"
#include "Engine/ImageParams.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/Log.h"
#include "Engine/Node.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/PluginMemory.h"
#include "Engine/Project.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoDrawableItem.h"
#include "Engine/Settings.h"
#include "Engine/ThreadStorage.h"
#include "Engine/Timer.h"
#include "Engine/Transform.h"
#include "Engine/ViewerInstance.h"

//#define NATRON_ALWAYS_ALLOCATE_FULL_IMAGE_BOUNDS


using namespace Natron;


class KnobFile;
class KnobOutputFile;


EffectPointerThreadProperty_RAII::EffectPointerThreadProperty_RAII(EffectInstance* effect)
{
    QThread::currentThread()->setProperty(kNatronTLSEffectPointerProperty, QVariant::fromValue((QObject*)effect));
}

EffectPointerThreadProperty_RAII::~EffectPointerThreadProperty_RAII()
{
    QThread::currentThread()->setProperty(kNatronTLSEffectPointerProperty, QVariant::fromValue((void*)0));
}

void
EffectInstance::addThreadLocalInputImageTempPointer(int inputNb,
                                                    const boost::shared_ptr<Natron::Image> & img)
{
    _imp->addInputImageTempPointer(inputNb, img);
}

EffectInstance::EffectInstance(boost::shared_ptr<Node> node)
    : NamedKnobHolder(node ? node->getApp() : NULL)
    , _node(node)
    , _imp( new Implementation(this) )
{
}

EffectInstance::~EffectInstance()
{
    clearPluginMemoryChunks();
}

void
EffectInstance::resetTotalTimeSpentRendering()
{
    resetTimeSpentRenderingInfos();
}

void
EffectInstance::lock(const boost::shared_ptr<Natron::Image> & entry)
{
    boost::shared_ptr<Node> n = _node.lock();

    n->lock(entry);
}

bool
EffectInstance::tryLock(const boost::shared_ptr<Natron::Image> & entry)
{
    boost::shared_ptr<Node> n = _node.lock();

    return n->tryLock(entry);
}

void
EffectInstance::unlock(const boost::shared_ptr<Natron::Image> & entry)
{
    boost::shared_ptr<Node> n = _node.lock();

    n->unlock(entry);
}

void
EffectInstance::clearPluginMemoryChunks()
{
    int toRemove;
    {
        QMutexLocker l(&_imp->pluginMemoryChunksMutex);
        toRemove = (int)_imp->pluginMemoryChunks.size();
    }

    while (toRemove > 0) {
        PluginMemory* mem;
        {
            QMutexLocker l(&_imp->pluginMemoryChunksMutex);
            mem = ( *_imp->pluginMemoryChunks.begin() );
        }
        delete mem;
        --toRemove;
    }
}

void
EffectInstance::setParallelRenderArgsTLS(int time,
                                         int view,
                                         bool isRenderUserInteraction,
                                         bool isSequential,
                                         bool canAbort,
                                         U64 nodeHash,
                                         U64 renderAge,
                                         const boost::shared_ptr<Natron::Node> & treeRoot,
                                         const boost::shared_ptr<NodeFrameRequest> & nodeRequest,
                                         int textureIndex,
                                         const TimeLine* timeline,
                                         bool isAnalysis,
                                         bool isDuringPaintStrokeCreation,
                                         const std::list<boost::shared_ptr<Natron::Node> > & rotoPaintNodes,
                                         Natron::RenderSafetyEnum currentThreadSafety,
                                         bool doNanHandling,
                                         bool draftMode,
                                         bool viewerProgressReportEnabled,
                                         const boost::shared_ptr<RenderStats> & stats)
{
    ParallelRenderArgs & args = _imp->frameRenderArgs.localData();

    args.time = time;
    args.timeline = timeline;
    args.view = view;
    args.isRenderResponseToUserInteraction = isRenderUserInteraction;
    args.isSequentialRender = isSequential;
    args.request = nodeRequest;
    if (nodeRequest) {
        args.nodeHash = nodeRequest->nodeHash;
    } else {
        args.nodeHash = nodeHash;
    }
    args.canAbort = canAbort;
    args.renderAge = renderAge;
    args.treeRoot = treeRoot;
    args.textureIndex = textureIndex;
    args.isAnalysis = isAnalysis;
    args.isDuringPaintStrokeCreation = isDuringPaintStrokeCreation;
    args.currentThreadSafety = currentThreadSafety;
    args.rotoPaintNodes = rotoPaintNodes;
    args.doNansHandling = doNanHandling;
    args.draftMode = draftMode;
    args.tilesSupported = getNode()->getCurrentSupportTiles();
    args.viewerProgressReportEnabled = viewerProgressReportEnabled;
    args.stats = stats;
    ++args.validArgs;
}

bool
EffectInstance::getThreadLocalRotoPaintTreeNodes(std::list<boost::shared_ptr<Natron::Node> >* nodes) const
{
    if ( !_imp->frameRenderArgs.hasLocalData() ) {
        return false;
    }
    const ParallelRenderArgs & tls = _imp->frameRenderArgs.localData();
    if (!tls.validArgs) {
        return false;
    }
    *nodes = tls.rotoPaintNodes;

    return true;
}

void
EffectInstance::setDuringPaintStrokeCreationThreadLocal(bool duringPaintStroke)
{
    ParallelRenderArgs & args = _imp->frameRenderArgs.localData();

    args.isDuringPaintStrokeCreation = duringPaintStroke;
}

void
EffectInstance::setParallelRenderArgsTLS(const ParallelRenderArgs & args)
{
    assert(args.validArgs);
    ParallelRenderArgs & tls = _imp->frameRenderArgs.localData();
    int curValid = tls.validArgs;
    tls = args;
    tls.validArgs = curValid + 1;
}

void
EffectInstance::invalidateParallelRenderArgsTLS()
{
    if ( _imp->frameRenderArgs.hasLocalData() ) {
        ParallelRenderArgs & args = _imp->frameRenderArgs.localData();
        --args.validArgs;
        if (args.validArgs < 0) {
            args.validArgs = 0;
        }

        for (NodeList::iterator it = args.rotoPaintNodes.begin(); it != args.rotoPaintNodes.end(); ++it) {
            (*it)->getLiveInstance()->invalidateParallelRenderArgsTLS();
        }
    } else {
        //qDebug() << "Frame render args thread storage not set, this is probably because the graph changed while rendering.";
    }
}

const ParallelRenderArgs*
EffectInstance::getParallelRenderArgsTLS() const
{
    if ( _imp->frameRenderArgs.hasLocalData() ) {
        return &_imp->frameRenderArgs.localData();
    } else {
        //qDebug() << "Frame render args thread storage not set, this is probably because the graph changed while rendering.";
        return 0;
    }
}

bool
EffectInstance::isCurrentRenderInAnalysis() const
{
    if ( _imp->frameRenderArgs.hasLocalData() ) {
        const ParallelRenderArgs & args = _imp->frameRenderArgs.localData();

        return args.validArgs && args.isAnalysis;
    }

    return false;
}

U64
EffectInstance::getHash() const
{
    boost::shared_ptr<Node> n = _node.lock();

    return n->getHashValue();
}


U64
EffectInstance::getRenderHash() const
{
    if ( !_imp->frameRenderArgs.hasLocalData() ) {
        return getHash();
    } else {
        ParallelRenderArgs & args = _imp->frameRenderArgs.localData();
        if (!args.validArgs) {
            return getHash();
        } else {
            if (args.request) {
                return args.request->nodeHash;
            }

            return args.nodeHash;
        }
    }
}

bool
EffectInstance::aborted() const
{
    if ( !_imp->frameRenderArgs.hasLocalData() ) {
        ///No local data, we're either not rendering or calling this from a thread not controlled by Natron
        return false;
    } else {
        ParallelRenderArgs & args = _imp->frameRenderArgs.localData();
        if (!args.validArgs) {
            ///No valid args, probably not rendering
            return false;
        } else {
            if (args.isRenderResponseToUserInteraction) {
                //Don't allow the plug-in to abort while in analysis.
                //Need to work on this for the tracker in order to be abortable.
                if (args.isAnalysis) {
                    return false;
                }
                ViewerInstance* isViewer  = 0;
                if (args.treeRoot) {
                    isViewer = dynamic_cast<ViewerInstance*>( args.treeRoot->getLiveInstance() );
                    //If the viewer is already doing a sequential render, abort
                    if ( isViewer && isViewer->isDoingSequentialRender() ) {
                        return true;
                    }
                }

                if (args.canAbort) {
                    if ( isViewer && isViewer->isRenderAbortable(args.textureIndex, args.renderAge) ) {
                        return true;
                    }

                    ///Rendering issued by RenderEngine::renderCurrentFrame, if time or hash changed, abort
                    bool ret = !getNode()->isActivated();

                    return ret;
                } else {
                    bool deactivated = !getNode()->isActivated();

                    return deactivated;
                }
            } else {
                ///Rendering is playback or render on disk, we rely on the flag set on the node that requested the render
                if (args.treeRoot) {
                    Natron::OutputEffectInstance* effect = dynamic_cast<Natron::OutputEffectInstance*>( args.treeRoot->getLiveInstance() );
                    assert(effect);

                    return effect->isSequentialRenderBeingAborted();
                } else {
                    return false;
                }
            }
        }
    }
} // EffectInstance::aborted

bool
EffectInstance::shouldCacheOutput(bool isFrameVaryingOrAnimated) const
{
    boost::shared_ptr<Node> n = _node.lock();

    return n->shouldCacheOutput(isFrameVaryingOrAnimated);
}

U64
EffectInstance::getKnobsAge() const
{
    return getNode()->getKnobsAge();
}

void
EffectInstance::setKnobsAge(U64 age)
{
    getNode()->setKnobsAge(age);
}

const std::string &
EffectInstance::getScriptName() const
{
    return getNode()->getScriptName();
}

std::string
EffectInstance::getScriptName_mt_safe() const
{
    return getNode()->getScriptName_mt_safe();
}

void
EffectInstance::getRenderFormat(Format *f) const
{
    assert(f);
    getApp()->getProject()->getProjectDefaultFormat(f);
}

int
EffectInstance::getRenderViewsCount() const
{
    return getApp()->getProject()->getProjectViewsCount();
}

bool
EffectInstance::hasOutputConnected() const
{
    return getNode()->hasOutputConnected();
}

EffectInstance*
EffectInstance::getInput(int n) const
{
    boost::shared_ptr<Natron::Node> inputNode = getNode()->getInput(n);

    if (inputNode) {
        return inputNode->getLiveInstance();
    }

    return NULL;
}

std::string
EffectInstance::getInputLabel(int inputNb) const
{
    std::string out;

    out.append( 1, (char)(inputNb + 65) );

    return out;
}

bool
EffectInstance::retrieveGetImageDataUponFailure(const double time,
                                                const int view,
                                                const RenderScale & scale,
                                                const RectD* optionalBoundsParam,
                                                U64* nodeHash_p,
                                                bool* isIdentity_p,
                                                double* identityTime,
                                                int* identityInputNb_p,
                                                bool* duringPaintStroke_p,
                                                RectD* rod_p,
                                                RoIMap* inputRois_p, //!< output, only set if optionalBoundsParam != NULL
                                                RectD* optionalBounds_p) //!< output, only set if optionalBoundsParam != NULL
{
    /////Update 09/02/14
    /// We now AUTHORIZE GetRegionOfDefinition and isIdentity and getRegionsOfInterest to be called recursively.
    /// It didn't make much sense to forbid them from being recursive.

//#ifdef DEBUG
//    if (QThread::currentThread() != qApp->thread()) {
//        ///This is a bad plug-in
//        qDebug() << getNode()->getScriptName_mt_safe().c_str() << " is trying to call clipGetImage during an unauthorized time. "
//        "Developers of that plug-in should fix it. \n Reminder from the OpenFX spec: \n "
//        "Images may be fetched from an attached clip in the following situations... \n"
//        "- in the kOfxImageEffectActionRender action\n"
//        "- in the kOfxActionInstanceChanged and kOfxActionEndInstanceChanged actions with a kOfxPropChangeReason or kOfxChangeUserEdited";
//    }
//#endif

    ///Try to compensate for the mistake

    *nodeHash_p = getHash();
    *duringPaintStroke_p = getNode()->isDuringPaintStrokeCreation();
    const U64 & nodeHash = *nodeHash_p;

    {
        RECURSIVE_ACTION();
        Natron::StatusEnum stat = getRegionOfDefinition(nodeHash, time, scale, view, rod_p);
        if (stat == eStatusFailed) {
            return false;
        }
    }
    const RectD & rod = *rod_p;

    ///OptionalBoundsParam is the optional rectangle passed to getImage which may be NULL, in which case we use the RoD.
    if (!optionalBoundsParam) {
        ///// We cannot recover the RoI, we just assume the plug-in wants to render the full RoD.
        *optionalBounds_p = rod;
        ifInfiniteApplyHeuristic(nodeHash, time, scale, view, optionalBounds_p);
        const RectD & optionalBounds = *optionalBounds_p;

        /// If the region parameter is not set to NULL, then it will be clipped to the clip's
        /// Region of Definition for the given time. The returned image will be m at m least as big as this region.
        /// If the region parameter is not set, then the region fetched will be at least the Region of Interest
        /// the effect has previously specified, clipped the clip's Region of Definition.
        /// (renderRoI will do the clipping for us).


        ///// This code is wrong but executed ONLY IF THE PLUG-IN DOESN'T RESPECT THE SPECIFICATIONS. Recursive actions
        ///// should never happen.
        getRegionsOfInterest(time, scale, optionalBounds, optionalBounds, 0, inputRois_p);
    }

    assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(scale.x == 1. && scale.y == 1.) ) );
    RectI pixelRod;
    rod.toPixelEnclosing(scale, getPreferredAspectRatio(), &pixelRod);
    try {
        *isIdentity_p = isIdentity_public(true, nodeHash, time, scale, pixelRod, view, identityTime, identityInputNb_p);
    } catch (...) {
        return false;
    }

    return true;
} // EffectInstance::retrieveGetImageDataUponFailure

void
EffectInstance::getThreadLocalInputImages(EffectInstance::InputImagesMap* images) const
{
    if ( _imp->inputImages.hasLocalData() ) {
        *images = _imp->inputImages.localData();
    }
}

bool
EffectInstance::getThreadLocalRegionsOfInterests(RoIMap & roiMap) const
{
    if ( !_imp->renderArgs.hasLocalData() ) {
        return false;
    }
    RenderArgs & renderArgs = _imp->renderArgs.localData();
    if (!renderArgs._validArgs) {
        return false;
    }
    roiMap = renderArgs._regionOfInterestResults;

    return true;
}

ImagePtr
EffectInstance::getImage(int inputNb,
                         const double time,
                         const RenderScale & scale,
                         const int view,
                         const RectD *optionalBoundsParam, //!< optional region in canonical coordinates
                         const ImageComponents & comp,
                         const Natron::ImageBitDepthEnum depth,
                         const double par,
                         const bool dontUpscale,
                         RectI* roiPixel)
{
    ///The input we want the image from
    EffectInstance* n = getInput(inputNb);

    ///Is this input a mask or not
    bool isMask = isInputMask(inputNb);

    ///If the input is a mask, this is the channel index in the layer of the mask channel
    int channelForMask = -1;

    ///Is this node a roto node or not. If so, find out if this input is the roto-brush
    boost::shared_ptr<RotoContext> roto;
    boost::shared_ptr<RotoDrawableItem> attachedStroke = getNode()->getAttachedRotoItem();

    if (attachedStroke) {
        roto = attachedStroke->getContext();
    } else {
        roto = getNode()->getRotoContext();
    }
    bool useRotoInput = false;
    if (roto) {
        useRotoInput = isMask || isInputRotoBrush(inputNb);
    }

    ///This is the actual layer that we are fetching in input, note that it is different than "comp" which is pointing to Alpha
    ImageComponents maskComps;
    //if (isMask) {
    if ( !isMaskEnabled(inputNb) ) {
        ///This is last resort, the plug-in should've checked getConnected() before, which would have returned false.
        return ImagePtr();
    }
    NodePtr maskInput;
    channelForMask = getMaskChannel(inputNb, &maskComps, &maskInput);
    if ( maskInput && ( channelForMask != -1) ) {
        n = maskInput->getLiveInstance();
    }

    //Invalid mask
    if ( isMask && ( ( channelForMask == -1) || ( maskComps.getNumComponents() == 0) ) ) {
        return ImagePtr();
    }
    //}


    if ( ( !roto || (roto && !useRotoInput) ) && !n ) {
        //Disconnected input
        return ImagePtr();
    }

    std::list<ImageComponents> outputClipPrefComps;
    ImageBitDepthEnum outputDepth;
    getPreferredDepthAndComponents(inputNb, &outputClipPrefComps, &outputDepth);
    assert(outputClipPrefComps.size() >= 1);
    const ImageComponents & prefComps = outputClipPrefComps.front();

    ///If optionalBounds have been set, use this for the RoI instead of the data int the TLS
    RectD optionalBounds;
    if (optionalBoundsParam) {
        optionalBounds = *optionalBoundsParam;
    }

    /*
     * These are the data fields stored in the TLS from the on-going render action or instance changed action
     */
    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);
    RoIMap inputsRoI;
    RectD rod;
    bool isIdentity;
    int inputNbIdentity;
    double inputIdentityTime;
    U64 nodeHash;
    bool duringPaintStroke;
    /// Never by-pass the cache here because we already computed the image in renderRoI and by-passing the cache again can lead to
    /// re-computing of the same image many many times
    bool byPassCache = false;

    ///The caller thread MUST be a thread owned by Natron. It cannot be a thread from the multi-thread suite.
    ///A call to getImage is forbidden outside an action running in a thread launched by Natron.

    /// From http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ImageEffectsImagesAndClipsUsingClips
    //    Images may be fetched from an attached clip in the following situations...
    //    in the kOfxImageEffectActionRender action
    //    in the kOfxActionInstanceChanged and kOfxActionEndInstanceChanged actions with a kOfxPropChangeReason of kOfxChangeUserEdited
    RectD roi;
    bool roiWasInRequestPass = false;

    if ( !_imp->renderArgs.hasLocalData() || !_imp->frameRenderArgs.hasLocalData() ) {
        if ( !retrieveGetImageDataUponFailure(time, view, scale, optionalBoundsParam, &nodeHash, &isIdentity, &inputIdentityTime, &inputNbIdentity, &duringPaintStroke, &rod, &inputsRoI, &optionalBounds) ) {
            return ImagePtr();
        }
    } else {
        RenderArgs & renderArgs = _imp->renderArgs.localData();
        ParallelRenderArgs & frameRenderArgs = _imp->frameRenderArgs.localData();

        if (!renderArgs._validArgs || !frameRenderArgs.validArgs) {
            if ( !retrieveGetImageDataUponFailure(time, view, scale, optionalBoundsParam, &nodeHash, &isIdentity, &inputIdentityTime, &inputNbIdentity, &duringPaintStroke, &rod, &inputsRoI, &optionalBounds) ) {
                return ImagePtr();
            }
        } else {
            if (n) {
                const ParallelRenderArgs* inputFrameArgs = n->getParallelRenderArgsTLS();
                const FrameViewRequest* request = 0;
                if (inputFrameArgs && inputFrameArgs->request) {
                    request = inputFrameArgs->request->getFrameViewRequest(time, view);
                }
                if (request) {
                    roiWasInRequestPass = true;
                    roi = request->finalData.finalRoi;
                }
            }
            if (!roiWasInRequestPass) {
                inputsRoI = renderArgs._regionOfInterestResults;
            }
            rod = renderArgs._rod;
            isIdentity = renderArgs._isIdentity;
            inputIdentityTime = renderArgs._identityTime;
            inputNbIdentity = renderArgs._identityInputNb;
            nodeHash = frameRenderArgs.nodeHash;
            duringPaintStroke = frameRenderArgs.isDuringPaintStrokeCreation;
        }
    }

    if (optionalBoundsParam) {
        roi = optionalBounds;
    } else if (!roiWasInRequestPass) {
        EffectInstance* inputToFind = 0;
        if (useRotoInput) {
            if ( getNode()->getRotoContext() ) {
                inputToFind = this;
            } else {
                assert(attachedStroke);
                inputToFind = attachedStroke->getContext()->getNode()->getLiveInstance();
            }
        } else {
            inputToFind = n;
        }
        RoIMap::iterator found = inputsRoI.find(inputToFind);
        if ( found != inputsRoI.end() ) {
            ///RoI is in canonical coordinates since the results of getRegionsOfInterest is in canonical coords.
            roi = found->second;
        } else {
            ///Oops, we didn't find the roi in the thread-storage... use  the RoD instead...
            roi = rod;
        }
    }

    if ( roi.isNull() ) {
        return ImagePtr();
    }


    if (isIdentity) {
        assert(inputNbIdentity != -2);
        ///If the effect is an identity but it didn't ask for the effect's image of which it is identity
        ///return a null image
        if (inputNbIdentity != inputNb) {
            ImagePtr();
        }
    }


    ///Does this node supports images at a scale different than 1
    bool renderFullScaleThenDownscale = (!supportsRenderScale() && mipMapLevel != 0);

    ///Do we want to render the graph upstream at scale 1 or at the requested render scale ? (user setting)
    bool renderScaleOneUpstreamIfRenderScaleSupportDisabled = false;
    unsigned int renderMappedMipMapLevel = mipMapLevel;
    if (renderFullScaleThenDownscale) {
        renderScaleOneUpstreamIfRenderScaleSupportDisabled = getNode()->useScaleOneImagesWhenRenderScaleSupportIsDisabled();
        if (renderScaleOneUpstreamIfRenderScaleSupportDisabled) {
            renderMappedMipMapLevel = 0;
        }
    }

    ///Both the result of getRegionsOfInterest and optionalBounds are in canonical coordinates, we have to convert in both cases
    ///Convert to pixel coordinates
    RectI pixelRoI;
    roi.toPixelEnclosing(renderScaleOneUpstreamIfRenderScaleSupportDisabled ? 0 : mipMapLevel, par, &pixelRoI);

    ///Try to find in the input images thread local storage if we already pre-computed the image
    InputImagesMap inputImagesThreadLocal;
    if ( _imp->inputImages.hasLocalData() ) {
        inputImagesThreadLocal = _imp->inputImages.localData();
    }

    ImagePtr inputImg;

    ///For the roto brush, we do things separatly and render the mask with the RotoContext.
    if (useRotoInput) {
        ///Usage of roto outside of the rotopaint node is no longer handled
        assert(attachedStroke);
        if (attachedStroke) {
            if (duringPaintStroke) {
                inputImg = getNode()->getOrRenderLastStrokeImage(mipMapLevel, pixelRoI, par, prefComps, depth);
            } else {
                inputImg = roto->renderMaskFromStroke(attachedStroke, pixelRoI, prefComps,
                                                      time, view, depth, mipMapLevel);
                if ( roto->isDoingNeatRender() ) {
                    getNode()->updateStrokeImage(inputImg);
                }
            }
        }
        if (roiPixel) {
            *roiPixel = pixelRoI;
        }

        if ( !pixelRoI.intersects( inputImg->getBounds() ) ) {
            //The RoI requested does not intersect with the bounds of the input image, return a NULL image.
#ifdef DEBUG
            qDebug() << getNode()->getScriptName_mt_safe().c_str() << ": The RoI requested to the roto mask does not intersect with the bounds of the input image";
#endif

            return ImagePtr();
        }

        if ( inputImagesThreadLocal.empty() ) {
            ///If the effect is analysis (e.g: Tracker) there's no input images in the tread local storage, hence add it
            _imp->addInputImageTempPointer(inputNb, inputImg);
        }

        return inputImg;
    }


    /// The node is connected.
    assert(n);

    std::list<ImageComponents> requestedComps;
    requestedComps.push_back(isMask ? maskComps : comp);
    ImageList inputImages;
    RenderRoIRetCode retCode = n->renderRoI(RenderRoIArgs(time,
                                                          scale,
                                                          renderMappedMipMapLevel,
                                                          view,
                                                          byPassCache,
                                                          pixelRoI,
                                                          RectD(),
                                                          requestedComps,
                                                          depth,
                                                          this,
                                                          inputImagesThreadLocal), &inputImages);

    if ( inputImages.empty() || (retCode != eRenderRoIRetCodeOk) ) {
        return ImagePtr();
    }
    assert(inputImages.size() == 1);

    inputImg = inputImages.front();

    if ( !pixelRoI.intersects( inputImg->getBounds() ) ) {
        //The RoI requested does not intersect with the bounds of the input image, return a NULL image.
#ifdef DEBUG
        qDebug() << getNode()->getScriptName_mt_safe().c_str() << ": The RoI requested to" << n->getScriptName_mt_safe().c_str() << "does not intersect with the bounds of the input image";
#endif

        return ImagePtr();
    }

    /*
     * From now on this is the generic part. We first call renderRoI and then convert to the appropriate scale/components if needed.
     * Note that since the image has been pre-rendered before by the recursive nature of the algorithm, the call to renderRoI will be
     * instantaneous thanks to the image cache.
     */

    ///Check that the rendered image contains what we requested.
    assert( (!isMask && inputImg->getComponents() == comp) || (isMask && inputImg->getComponents() == maskComps) );

    if (roiPixel) {
        *roiPixel = pixelRoI;
    }
    unsigned int inputImgMipMapLevel = inputImg->getMipMapLevel();

    ///If the plug-in doesn't support the render scale, but the image is downscaled, up-scale it.
    ///Note that we do NOT cache it because it is really low def!
    if ( !dontUpscale  && renderFullScaleThenDownscale && (inputImgMipMapLevel != 0) ) {
        assert(inputImgMipMapLevel != 0);
        ///Resize the image according to the requested scale
        Natron::ImageBitDepthEnum bitdepth = inputImg->getBitDepth();
        RectI bounds;
        inputImg->getRoD().toPixelEnclosing(0, par, &bounds);
        ImagePtr rescaledImg( new Natron::Image(inputImg->getComponents(), inputImg->getRoD(),
                                                bounds, 0, par, bitdepth) );
        inputImg->upscaleMipMap( inputImg->getBounds(), inputImgMipMapLevel, 0, rescaledImg.get() );
        if (roiPixel) {
            RectD canonicalPixelRoI;
            pixelRoI.toCanonical(inputImgMipMapLevel, par, rod, &canonicalPixelRoI);
            canonicalPixelRoI.toPixelEnclosing(0, par, roiPixel);
        }

        inputImg = rescaledImg;
    }


    if ( prefComps.getNumComponents() != inputImg->getComponents().getNumComponents() ) {
        ImagePtr remappedImg;
        {
            Image::ReadAccess acc = inputImg->getReadRights();

            remappedImg.reset( new Image(prefComps, inputImg->getRoD(), inputImg->getBounds(), inputImg->getMipMapLevel(), inputImg->getPixelAspectRatio(), inputImg->getBitDepth(), false) );

            Natron::ViewerColorSpaceEnum colorspace = getApp()->getDefaultColorSpaceForBitDepth( inputImg->getBitDepth() );
            bool unPremultIfNeeded = getOutputPremultiplication() == eImagePremultiplicationPremultiplied &&
                                     inputImg->getComponents().getNumComponents() == 4 && prefComps.getNumComponents() == 3;
            inputImg->convertToFormat( inputImg->getBounds(),
                                       colorspace, colorspace,
                                       channelForMask, false, unPremultIfNeeded, remappedImg.get() );
        }
        inputImg = remappedImg;
    }


    if ( inputImagesThreadLocal.empty() ) {
        ///If the effect is analysis (e.g: Tracker) there's no input images in the tread local storage, hence add it
        _imp->addInputImageTempPointer(inputNb, inputImg);
    }

    return inputImg;
} // getImage

void
EffectInstance::calcDefaultRegionOfDefinition(U64 /*hash*/,
                                              double /*time*/,
                                              int /*view*/,
                                              const RenderScale & /*scale*/,
                                              RectD *rod)
{
    Format projectDefault;

    getRenderFormat(&projectDefault);
    *rod = RectD( projectDefault.left(), projectDefault.bottom(), projectDefault.right(), projectDefault.top() );
}

Natron::StatusEnum
EffectInstance::getRegionOfDefinition(U64 hash,
                                      double time,
                                      const RenderScale & scale,
                                      int view,
                                      RectD* rod) //!< rod is in canonical coordinates
{
    bool firstInput = true;
    RenderScale renderMappedScale = scale;

    assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(scale.x == 1. && scale.y == 1.) ) );

    for (int i = 0; i < getMaxInputCount(); ++i) {
        if ( isInputMask(i) ) {
            continue;
        }
        Natron::EffectInstance* input = getInput(i);
        if (input) {
            RectD inputRod;
            bool isProjectFormat;
            StatusEnum st = input->getRegionOfDefinition_public(hash, time, renderMappedScale, view, &inputRod, &isProjectFormat);
            assert(inputRod.x2 >= inputRod.x1 && inputRod.y2 >= inputRod.y1);
            if (st == eStatusFailed) {
                return st;
            }

            if (firstInput) {
                *rod = inputRod;
                firstInput = false;
            } else {
                rod->merge(inputRod);
            }
            assert(rod->x1 <= rod->x2 && rod->y1 <= rod->y2);
        }
    }

    // if rod was not set, return default, else return OK
    return firstInput ? eStatusReplyDefault : eStatusOK;
}

bool
EffectInstance::ifInfiniteApplyHeuristic(U64 hash,
                                         double time,
                                         const RenderScale & scale,
                                         int view,
                                         RectD* rod) //!< input/output
{
    /*If the rod is infinite clip it to the project's default*/

    Format projectFormat;

    getRenderFormat(&projectFormat);
    RectD projectDefault = projectFormat.toCanonicalFormat();
    /// FIXME: before removing the assert() (I know you are tempted) please explain (here: document!) if the format rectangle can be empty and in what situation(s)
    assert( !projectDefault.isNull() );

    assert(rod);
    assert(rod->x1 <= rod->x2 && rod->y1 <= rod->y2);
    bool x1Infinite = rod->x1 <= kOfxFlagInfiniteMin;
    bool y1Infinite = rod->y1 <= kOfxFlagInfiniteMin;
    bool x2Infinite = rod->x2 >= kOfxFlagInfiniteMax;
    bool y2Infinite = rod->y2 >= kOfxFlagInfiniteMax;

    ///Get the union of the inputs.
    RectD inputsUnion;

    ///Do the following only if one coordinate is infinite otherwise we wont need the RoD of the input
    if (x1Infinite || y1Infinite || x2Infinite || y2Infinite) {
        // initialize with the effect's default RoD, because inputs may not be connected to other effects (e.g. Roto)
        calcDefaultRegionOfDefinition(hash, time, view, scale, &inputsUnion);
        bool firstInput = true;
        for (int i = 0; i < getMaxInputCount(); ++i) {
            Natron::EffectInstance* input = getInput(i);
            if (input) {
                RectD inputRod;
                bool isProjectFormat;
                RenderScale inputScale = scale;
                if (input->supportsRenderScaleMaybe() == eSupportsNo) {
                    inputScale.x = inputScale.y = 1.;
                }
                StatusEnum st = input->getRegionOfDefinition_public(hash, time, inputScale, view, &inputRod, &isProjectFormat);
                if (st != eStatusFailed) {
                    if (firstInput) {
                        inputsUnion = inputRod;
                        firstInput = false;
                    } else {
                        inputsUnion.merge(inputRod);
                    }
                }
            }
        }
    }
    ///If infinite : clip to inputsUnion if not null, otherwise to project default

    // BE CAREFUL:
    // std::numeric_limits<int>::infinity() does not exist (check std::numeric_limits<int>::has_infinity)
    bool isProjectFormat = false;
    if (x1Infinite) {
        if ( !inputsUnion.isNull() ) {
            rod->x1 = std::min(inputsUnion.x1, projectDefault.x1);
        } else {
            rod->x1 = projectDefault.x1;
            isProjectFormat = true;
        }
        rod->x2 = std::max(rod->x1, rod->x2);
    }
    if (y1Infinite) {
        if ( !inputsUnion.isNull() ) {
            rod->y1 = std::min(inputsUnion.y1, projectDefault.y1);
        } else {
            rod->y1 = projectDefault.y1;
            isProjectFormat = true;
        }
        rod->y2 = std::max(rod->y1, rod->y2);
    }
    if (x2Infinite) {
        if ( !inputsUnion.isNull() ) {
            rod->x2 = std::max(inputsUnion.x2, projectDefault.x2);
        } else {
            rod->x2 = projectDefault.x2;
            isProjectFormat = true;
        }
        rod->x1 = std::min(rod->x1, rod->x2);
    }
    if (y2Infinite) {
        if ( !inputsUnion.isNull() ) {
            rod->y2 = std::max(inputsUnion.y2, projectDefault.y2);
        } else {
            rod->y2 = projectDefault.y2;
            isProjectFormat = true;
        }
        rod->y1 = std::min(rod->y1, rod->y2);
    }
    if ( isProjectFormat && !isGenerator() ) {
        isProjectFormat = false;
    }
    assert(rod->x1 <= rod->x2 && rod->y1 <= rod->y2);

    return isProjectFormat;
} // ifInfiniteApplyHeuristic

void
EffectInstance::getRegionsOfInterest(double /*time*/,
                                     const RenderScale & /*scale*/,
                                     const RectD & /*outputRoD*/, //!< the RoD of the effect, in canonical coordinates
                                     const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                     int /*view*/,
                                     RoIMap* ret)
{
    for (int i = 0; i < getMaxInputCount(); ++i) {
        Natron::EffectInstance* input = getInput(i);
        if (input) {
            ret->insert( std::make_pair(input, renderWindow) );
        }
    }
}

FramesNeededMap
EffectInstance::getFramesNeeded(double time,
                                int view)
{
    FramesNeededMap ret;
    RangeD defaultRange;

    defaultRange.min = defaultRange.max = time;
    std::vector<RangeD> ranges;
    ranges.push_back(defaultRange);
    std::map<int, std::vector<RangeD> > defViewRange;
    defViewRange.insert( std::make_pair(view, ranges) );
    for (int i = 0; i < getMaxInputCount(); ++i) {
        if ( isInputRotoBrush(i) ) {
            ret.insert( std::make_pair(i, defViewRange) );
        } else {
            Natron::EffectInstance* input = getInput(i);
            if (input) {
                ret.insert( std::make_pair(i, defViewRange) );
            }
        }
    }

    return ret;
}

void
EffectInstance::getFrameRange(double *first,
                              double *last)
{
    // default is infinite if there are no non optional input clips
    *first = INT_MIN;
    *last = INT_MAX;
    for (int i = 0; i < getMaxInputCount(); ++i) {
        Natron::EffectInstance* input = getInput(i);
        if (input) {
            double inpFirst, inpLast;
            input->getFrameRange(&inpFirst, &inpLast);
            if (i == 0) {
                *first = inpFirst;
                *last = inpLast;
            } else {
                if (inpFirst < *first) {
                    *first = inpFirst;
                }
                if (inpLast > *last) {
                    *last = inpLast;
                }
            }
        }
    }
}

EffectInstance::NotifyRenderingStarted_RAII::NotifyRenderingStarted_RAII(Node* node)
    : _node(node)
{
    _didEmit = node->notifyRenderingStarted();
}

EffectInstance::NotifyRenderingStarted_RAII::~NotifyRenderingStarted_RAII()
{
    if (_didEmit) {
        _node->notifyRenderingEnded();
    }
}

EffectInstance::NotifyInputNRenderingStarted_RAII::NotifyInputNRenderingStarted_RAII(Node* node,
                                                                                     int inputNumber)
    : _node(node)
    , _inputNumber(inputNumber)
{
    _didEmit = node->notifyInputNIsRendering(inputNumber);
}

EffectInstance::NotifyInputNRenderingStarted_RAII::~NotifyInputNRenderingStarted_RAII()
{
    if (_didEmit) {
        _node->notifyInputNIsFinishedRendering(_inputNumber);
    }
}

static void
getOrCreateFromCacheInternal(const ImageKey & key,
                             const boost::shared_ptr<ImageParams> & params,
                             bool useCache,
                             bool useDiskCache,
                             ImagePtr* image)
{
    if (useCache) {
        !useDiskCache ? Natron::getImageFromCacheOrCreate(key, params, image) :
        Natron::getImageFromDiskCacheOrCreate(key, params, image);

        if (!*image) {
            std::stringstream ss;
            ss << "Failed to allocate an image of ";
            ss << printAsRAM( params->getElementsCount() * sizeof(Image::data_t) ).toStdString();
            Natron::errorDialog( QObject::tr("Out of memory").toStdString(), ss.str() );

            return;
        }

        /*
         * Note that at this point the image is already exposed to other threads and another one might already have allocated it.
         * This function does nothing if it has been reallocated already.
         */
        (*image)->allocateMemory();


        /*
         * Another thread might have allocated the same image in the cache but with another RoI, make sure
         * it is big enough for us, or resize it to our needs.
         */


        (*image)->ensureBounds( params->getBounds() );
    } else {
        image->reset( new Image(key, params) );
    }
}

void
EffectInstance::getImageFromCacheAndConvertIfNeeded(bool useCache,
                                                    bool useDiskCache,
                                                    const Natron::ImageKey & key,
                                                    unsigned int mipMapLevel,
                                                    const RectI* boundsParam,
                                                    const RectD* rodParam,
                                                    Natron::ImageBitDepthEnum bitdepth,
                                                    const Natron::ImageComponents & components,
                                                    Natron::ImageBitDepthEnum nodePrefDepth,
                                                    const Natron::ImageComponents & nodePrefComps,
                                                    const EffectInstance::InputImagesMap & inputImages,
                                                    const boost::shared_ptr<RenderStats> & stats,
                                                    boost::shared_ptr<Natron::Image>* image)
{
    ImageList cachedImages;
    bool isCached = false;

    ///Find first something in the input images list
    if ( !inputImages.empty() ) {
        for (EffectInstance::InputImagesMap::const_iterator it = inputImages.begin(); it != inputImages.end(); ++it) {
            for (ImageList::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
                if ( !it2->get() ) {
                    continue;
                }
                const ImageKey & imgKey = (*it2)->getKey();
                if (imgKey == key) {
                    cachedImages.push_back(*it2);
                    isCached = true;
                }
            }
        }
    }

    if (!isCached) {
        isCached = !useDiskCache ? Natron::getImageFromCache(key, &cachedImages) : Natron::getImageFromDiskCache(key, &cachedImages);
    }

    if (stats && stats->isInDepthProfilingEnabled() && !isCached) {
        stats->addCacheInfosForNode(getNode(), true, false);
    }

    if (isCached) {
        ///A ptr to a higher resolution of the image or an image with different comps/bitdepth
        ImagePtr imageToConvert;

        for (ImageList::iterator it = cachedImages.begin(); it != cachedImages.end(); ++it) {
            unsigned int imgMMlevel = (*it)->getMipMapLevel();
            const Natron::ImageComponents & imgComps = (*it)->getComponents();
            ImageBitDepthEnum imgDepth = (*it)->getBitDepth();

            if ( (*it)->getParams()->isRodProjectFormat() ) {
                ////If the image was cached with a RoD dependent on the project format, but the project format changed,
                ////just discard this entry
                Format projectFormat;
                getRenderFormat(&projectFormat);
                RectD canonicalProject = projectFormat.toCanonicalFormat();
                if ( canonicalProject != (*it)->getRoD() ) {
                    appPTR->removeFromNodeCache(*it);
                    continue;
                }
            }

            ///Throw away images that are not even what the node want to render
            if ( ( imgComps.isColorPlane() && nodePrefComps.isColorPlane() && (imgComps != nodePrefComps) ) || (imgDepth != nodePrefDepth) ) {
                appPTR->removeFromNodeCache(*it);
                continue;
            }

            bool convertible = imgComps.isConvertibleTo(components);
            if ( (imgMMlevel == mipMapLevel) && convertible &&
                 ( getSizeOfForBitDepth(imgDepth) >= getSizeOfForBitDepth(bitdepth) ) /* && imgComps == components && imgDepth == bitdepth*/ ) {
                ///We found  a matching image

                *image = *it;
                break;
            } else {
                if ( (imgMMlevel >= mipMapLevel) || !convertible ||
                     ( getSizeOfForBitDepth(imgDepth) < getSizeOfForBitDepth(bitdepth) ) ) {
                    ///Either smaller resolution or not enough components or bit-depth is not as deep, don't use the image
                    continue;
                }

                assert(imgMMlevel < mipMapLevel);

                if (!imageToConvert) {
                    imageToConvert = *it;
                } else {
                    ///We found an image which scale is closer to the requested mipmap level we want, use it instead
                    if ( imgMMlevel > imageToConvert->getMipMapLevel() ) {
                        imageToConvert = *it;
                    }
                }
            }
        } //end for

        if (imageToConvert && !*image) {
            ///Ensure the image is allocated
            (imageToConvert)->allocateMemory();


            if (imageToConvert->getMipMapLevel() != mipMapLevel) {
                boost::shared_ptr<ImageParams> oldParams = imageToConvert->getParams();

                assert(imageToConvert->getMipMapLevel() < mipMapLevel);

                RectI imgToConvertBounds = imageToConvert->getBounds();
                const RectD & rod = rodParam ? *rodParam : oldParams->getRoD();
                RectD imgToConvertCanonical;
                imgToConvertBounds.toCanonical(imageToConvert->getMipMapLevel(), imageToConvert->getPixelAspectRatio(), rod, &imgToConvertCanonical);
                RectI downscaledBounds;

                imgToConvertCanonical.toPixelEnclosing(imageToConvert->getMipMapLevel(), imageToConvert->getPixelAspectRatio(), &imgToConvertBounds);
                imgToConvertCanonical.toPixelEnclosing(mipMapLevel, imageToConvert->getPixelAspectRatio(), &downscaledBounds);

                if (boundsParam) {
                    downscaledBounds.merge(*boundsParam);
                }

                RectI pixelRoD;
                rod.toPixelEnclosing(mipMapLevel, oldParams->getPixelAspectRatio(), &pixelRoD);
                downscaledBounds.intersect(pixelRoD, &downscaledBounds);

                boost::shared_ptr<ImageParams> imageParams = Image::makeParams( oldParams->getCost(),
                                                                                rod,
                                                                                downscaledBounds,
                                                                                oldParams->getPixelAspectRatio(),
                                                                                mipMapLevel,
                                                                                oldParams->isRodProjectFormat(),
                                                                                oldParams->getComponents(),
                                                                                oldParams->getBitDepth(),
                                                                                oldParams->getFramesNeeded() );


                imageParams->setMipMapLevel(mipMapLevel);


                boost::shared_ptr<Image> img;
                getOrCreateFromCacheInternal(key, imageParams, useCache, useDiskCache, &img);
                if (!img) {
                    return;
                }

                if (imgToConvertBounds.area() > 1) {
                    imageToConvert->downscaleMipMap( rod,
                                                     imgToConvertBounds,
                                                     imageToConvert->getMipMapLevel(), img->getMipMapLevel(),
                                                     useCache && imageToConvert->usesBitMap(),
                                                     img.get() );
                } else {
                    img->pasteFrom(*imageToConvert, imgToConvertBounds);
                }

                imageToConvert = img;
            }

            *image = imageToConvert;
            //assert(imageToConvert->getBounds().contains(bounds));

            if ( stats && stats->isInDepthProfilingEnabled() ) {
                stats->addCacheInfosForNode(getNode(), false, true);
            }
        } else if (*image) { //  else if (imageToConvert && !*image)
            ///Ensure the image is allocated
            (*image)->allocateMemory();

            if ( stats && stats->isInDepthProfilingEnabled() ) {
                stats->addCacheInfosForNode(getNode(), false, false);
            }
        } else {
            if ( stats && stats->isInDepthProfilingEnabled() ) {
                stats->addCacheInfosForNode(getNode(), true, false);
            }
        }
    } // isCached
} // EffectInstance::getImageFromCacheAndConvertIfNeeded

void
EffectInstance::tryConcatenateTransforms(double time,
                                         int view,
                                         const RenderScale & scale,
                                         InputMatrixMap* inputTransforms)
{
    bool canTransform = getCanTransform();

    //An effect might not be able to concatenate transforms but can still apply a transform (e.g CornerPinMasked)
    std::list<int> inputHoldingTransforms;
    bool canApplyTransform = getInputsHoldingTransform(&inputHoldingTransforms);

    assert(inputHoldingTransforms.empty() || canApplyTransform);

    Transform::Matrix3x3 thisNodeTransform;
    Natron::EffectInstance* inputToTransform = 0;
    bool getTransformSucceeded = false;

    if (canTransform) {
        /*
         * If getting the transform does not succeed, then this effect is treated as any other ones.
         */
        Natron::StatusEnum stat = getTransform_public(time, scale, view, &inputToTransform, &thisNodeTransform);
        if (stat == eStatusOK) {
            getTransformSucceeded = true;
        }
    }


    if ( (canTransform && getTransformSucceeded) || ( !canTransform && canApplyTransform && !inputHoldingTransforms.empty() ) ) {
        for (std::list<int>::iterator it = inputHoldingTransforms.begin(); it != inputHoldingTransforms.end(); ++it) {
            EffectInstance* input = getInput(*it);
            if (!input) {
                continue;
            }
            std::list<Transform::Matrix3x3> matricesByOrder; // from downstream to upstream
            InputMatrix im;
            im.newInputEffect = input;
            im.newInputNbToFetchFrom = *it;


            // recursion upstream
            bool inputCanTransform = false;
            bool inputIsDisabled  =  input->getNode()->isNodeDisabled();

            if (!inputIsDisabled) {
                inputCanTransform = input->getCanTransform();
            }


            while ( input && (inputCanTransform || inputIsDisabled) ) {
                //input is either disabled, or identity or can concatenate a transform too
                if (inputIsDisabled) {
                    int prefInput;
                    input = input->getNearestNonDisabled();
                    prefInput = input ? input->getNode()->getPreferredInput() : -1;
                    if (prefInput == -1) {
                        break;
                    }

                    if (input) {
                        im.newInputNbToFetchFrom = prefInput;
                        im.newInputEffect = input;
                    }
                } else if (inputCanTransform) {
                    Transform::Matrix3x3 m;
                    inputToTransform = 0;
                    Natron::StatusEnum stat = input->getTransform_public(time, scale, view, &inputToTransform, &m);
                    if (stat == eStatusOK) {
                        matricesByOrder.push_back(m);
                        if (inputToTransform) {
                            im.newInputNbToFetchFrom = input->getInputNumber(inputToTransform);
                            im.newInputEffect = input;
                            input = inputToTransform;
                        }
                    } else {
                        break;
                    }
                } else {
                    assert(false);
                }

                if (input) {
                    inputIsDisabled = input->getNode()->isNodeDisabled();
                    if (!inputIsDisabled) {
                        inputCanTransform = input->getCanTransform();
                    }
                }
            }

            if ( input && !matricesByOrder.empty() ) {
                assert(im.newInputEffect);

                ///Now actually concatenate matrices together
                im.cat.reset(new Transform::Matrix3x3);
                std::list<Transform::Matrix3x3>::iterator it2 = matricesByOrder.begin();
                *im.cat = *it2;
                ++it2;
                while ( it2 != matricesByOrder.end() ) {
                    *im.cat = Transform::matMul(*im.cat, *it2);
                    ++it2;
                }

                inputTransforms->insert( std::make_pair(*it, im) );
            }
        } //  for (std::list<int>::iterator it = inputHoldingTransforms.begin(); it != inputHoldingTransforms.end(); ++it)
    } // if ((canTransform && getTransformSucceeded) || (canApplyTransform && !inputHoldingTransforms.empty()))
} // EffectInstance::tryConcatenateTransforms


bool
EffectInstance::allocateImagePlane(const ImageKey & key,
                                   const RectD & rod,
                                   const RectI & downscaleImageBounds,
                                   const RectI & fullScaleImageBounds,
                                   bool isProjectFormat,
                                   const FramesNeededMap & framesNeeded,
                                   const Natron::ImageComponents & components,
                                   Natron::ImageBitDepthEnum depth,
                                   double par,
                                   unsigned int mipmapLevel,
                                   bool renderFullScaleThenDownscale,
                                   bool renderScaleOneUpstreamIfRenderScaleSupportDisabled,
                                   bool useDiskCache,
                                   bool createInCache,
                                   boost::shared_ptr<Natron::Image>* fullScaleImage,
                                   boost::shared_ptr<Natron::Image>* downscaleImage)
{
    //Controls whether images are stored on disk or in RAM, 0 = RAM, 1 = mmap
    int cost = useDiskCache ? 1 : 0;

    //If we're rendering full scale and with input images at full scale, don't cache the downscale image since it is cheap to
    //recreate, instead cache the full-scale image
    if (renderFullScaleThenDownscale && renderScaleOneUpstreamIfRenderScaleSupportDisabled) {
        downscaleImage->reset( new Natron::Image(components, rod, downscaleImageBounds, mipmapLevel, par, depth, true) );
    } else {
        ///Cache the image with the requested components instead of the remapped ones
        boost::shared_ptr<Natron::ImageParams> cachedImgParams = Natron::Image::makeParams(cost,
                                                                                           rod,
                                                                                           downscaleImageBounds,
                                                                                           par,
                                                                                           mipmapLevel,
                                                                                           isProjectFormat,
                                                                                           components,
                                                                                           depth,
                                                                                           framesNeeded);

        //Take the lock after getting the image from the cache or while allocating it
        ///to make sure a thread will not attempt to write to the image while its being allocated.
        ///When calling allocateMemory() on the image, the cache already has the lock since it added it
        ///so taking this lock now ensures the image will be allocated completetly

        getOrCreateFromCacheInternal(key, cachedImgParams, createInCache, useDiskCache, fullScaleImage);
        if (!*fullScaleImage) {
            return false;
        }


        *downscaleImage = *fullScaleImage;
    }

    if (renderFullScaleThenDownscale) {
        if (!renderScaleOneUpstreamIfRenderScaleSupportDisabled) {
            ///The upscaled image will be rendered using input images at lower def... which means really crappy results, don't cache this image!
            fullScaleImage->reset( new Natron::Image(components, rod, fullScaleImageBounds, 0, par, depth, true) );
        } else {
            boost::shared_ptr<Natron::ImageParams> upscaledImageParams = Natron::Image::makeParams(cost,
                                                                                                   rod,
                                                                                                   fullScaleImageBounds,
                                                                                                   par,
                                                                                                   0,
                                                                                                   isProjectFormat,
                                                                                                   components,
                                                                                                   depth,
                                                                                                   framesNeeded);

            //The upscaled image will be rendered with input images at full def, it is then the best possibly rendered image so cache it!

            fullScaleImage->reset();
            getOrCreateFromCacheInternal(key, upscaledImageParams, createInCache, useDiskCache, fullScaleImage);

            if (!*fullScaleImage) {
                return false;
            }
        }
    }

    return true;
} // EffectInstance::allocateImagePlane


void
EffectInstance::transformInputRois(Natron::EffectInstance* self,
                                   const InputMatrixMap & inputTransforms,
                                   double par,
                                   const RenderScale & scale,
                                   RoIMap* inputsRoi,
                                   std::map<int, EffectInstance*>* reroutesMap)
{
    //Transform the RoIs by the inverse of the transform matrix (which is in pixel coordinates)
    for (InputMatrixMap::const_iterator it = inputTransforms.begin(); it != inputTransforms.end(); ++it) {
        RectD transformedRenderWindow;
        Natron::EffectInstance* effectInTransformInput = self->getInput(it->first);
        assert(effectInTransformInput);


        RoIMap::iterator foundRoI = inputsRoi->find(effectInTransformInput);
        if ( foundRoI == inputsRoi->end() ) {
            //There might be no RoI because it was null
            continue;
        }

        // invert it
        Transform::Matrix3x3 invertTransform;
        double det = Transform::matDeterminant(*it->second.cat);
        if (det != 0.) {
            invertTransform = Transform::matInverse(*it->second.cat, det);
        }

        Transform::Matrix3x3 canonicalToPixel = Transform::matCanonicalToPixel(par, scale.x,
                                                                               scale.y, false);
        Transform::Matrix3x3 pixelToCanonical = Transform::matPixelToCanonical(par,  scale.x,
                                                                               scale.y, false);

        invertTransform = Transform::matMul(Transform::matMul(pixelToCanonical, invertTransform), canonicalToPixel);
        Transform::transformRegionFromRoD(foundRoI->second, invertTransform, transformedRenderWindow);

        //Replace the original RoI by the transformed RoI
        inputsRoi->erase(foundRoI);
        inputsRoi->insert( std::make_pair(it->second.newInputEffect->getInput(it->second.newInputNbToFetchFrom), transformedRenderWindow) );
        reroutesMap->insert( std::make_pair(it->first, it->second.newInputEffect) );
    }
}

EffectInstance::RenderRoIRetCode
EffectInstance::renderInputImagesForRoI(const FrameViewRequest* request,
                                        bool useTransforms,
                                        double time,
                                        int view,
                                        double par,
                                        const RectD & rod,
                                        const RectD & canonicalRenderWindow,
                                        const InputMatrixMap & inputTransforms,
                                        unsigned int mipMapLevel,
                                        const RenderScale & scale,
                                        const RenderScale & renderMappedScale,
                                        bool useScaleOneInputImages,
                                        bool byPassCache,
                                        const FramesNeededMap & framesNeeded,
                                        const ComponentsNeededMap & neededComps,
                                        InputImagesMap *inputImages,
                                        RoIMap* inputsRoi)
{
    if (!request) {
        getRegionsOfInterest_public(time, renderMappedScale, rod, canonicalRenderWindow, view, inputsRoi);
    }
#ifdef DEBUG
    if ( !inputsRoi->empty() && framesNeeded.empty() && !isReader() && !isRotoPaintNode() ) {
        qDebug() << getNode()->getScriptName_mt_safe().c_str() << ": getRegionsOfInterestAction returned 1 or multiple input RoI(s) but returned "
                 << "an empty list with getFramesNeededAction";
    }
#endif

    std::map<int, EffectInstance*> reroutesMap;
    if (!request) {
        transformInputRois(this, inputTransforms, par, scale, inputsRoi, &reroutesMap);
    } else {
        reroutesMap = request->globalData.reroutesMap;
    }

    return treeRecurseFunctor(true,
                              getNode(),
                              framesNeeded,
                              *inputsRoi,
                              reroutesMap,
                              useTransforms,
                              mipMapLevel,
                              NodePtr(),
                              0,
                              inputImages,
                              &neededComps,
                              useScaleOneInputImages,
                              byPassCache);
}


EffectInstance::RenderingFunctorRetEnum
EffectInstance::tiledRenderingFunctor( TiledRenderingFunctorArgs & args,
                                       const RectToRender & specificData,
                                       const QThread* callingThread)
{
    return tiledRenderingFunctor(callingThread,
                                 args.frameArgs,
                                 specificData,
                                 args.frameTLS,
                                 args.renderFullScaleThenDownscale,
                                 args.renderUseScaleOneInputs,
                                 args.isSequentialRender,
                                 args.isRenderResponseToUserInteraction,
                                 args.firstFrame,
                                 args.lastFrame,
                                 args.preferredInput,
                                 args.mipMapLevel,
                                 args.renderMappedMipMapLevel,
                                 args.rod,
                                 args.time,
                                 args.view,
                                 args.par,
                                 args.byPassCache,
                                 args.outputClipPrefDepth,
                                 args.outputClipPrefsComps,
                                 args.processChannels,
                                 args.planes);
}

EffectInstance::RenderingFunctorRetEnum
EffectInstance::tiledRenderingFunctor(const QThread* callingThread,
                                      const ParallelRenderArgs & frameArgs,
                                      const RectToRender & rectToRender,
                                      const std::map<boost::shared_ptr<Natron::Node>, ParallelRenderArgs > & frameTLS,
                                      bool renderFullScaleThenDownscale,
                                      bool renderUseScaleOneInputs,
                                      bool isSequentialRender,
                                      bool isRenderResponseToUserInteraction,
                                      int firstFrame,
                                      int lastFrame,
                                      int preferredInput,
                                      unsigned int mipMapLevel,
                                      unsigned int renderMappedMipMapLevel,
                                      const RectD & rod,
                                      double time,
                                      int view,
                                      const double par,
                                      bool byPassCache,
                                      Natron::ImageBitDepthEnum outputClipPrefDepth,
                                      const std::list<Natron::ImageComponents> & outputClipPrefsComps,
                                      bool* processChannels,
                                      ImagePlanesToRender & planes) // when MT, planes is a copy so there's is no data race
{
    assert( !rectToRender.rect.isNull() );

    bool outputUseImage = renderFullScaleThenDownscale && renderUseScaleOneInputs;

    ///Make the thread-storage live as long as the render action is called if we're in a newly launched thread in eRenderSafetyFullySafeFrame mode
    boost::shared_ptr<ParallelRenderArgsSetter> scopedFrameArgs;
    if ( !frameTLS.empty() && ( callingThread != QThread::currentThread() ) ) {
        scopedFrameArgs.reset( new ParallelRenderArgsSetter(frameTLS) );
    }

    ///We hold our input images in thread-storage, so that the getImage function can find them afterwards, even if the node doesn't cache its output.
    boost::shared_ptr<InputImagesHolder_RAII> inputImagesHolder;
    if ( !rectToRender.imgs.empty() ) {
        inputImagesHolder.reset( new InputImagesHolder_RAII(rectToRender.imgs, &_imp->inputImages) );
    }

    /*
     * downscaledRectToRender is in bitmap mipmap level. If outputUseImage,
     * then mipmapLevel = 0, otherwise mipmapLevel = mipMapLevel (arg)
     */
    RectI downscaledRectToRender = rectToRender.rect;

    /*
     * renderMappedRectToRender is in renderMappedMipMapLevel
     */
    RectI renderMappedRectToRender = downscaledRectToRender;

    ///Upscale the RoI to a region in the full scale image so it is in canonical coordinates
    RectD canonicalRectToRender;
    downscaledRectToRender.toCanonical(mipMapLevel, par, rod, &canonicalRectToRender);
    if ( !outputUseImage && (mipMapLevel > 0) && (renderMappedMipMapLevel != mipMapLevel) ) {
        canonicalRectToRender.toPixelEnclosing(renderMappedMipMapLevel, par, &renderMappedRectToRender);
    }

    const PlaneToRender & firstPlaneToRender = planes.planes.begin()->second;
    // at this point, it may be unnecessary to call render because it was done a long time ago => check the bitmap here!
# ifndef NDEBUG
    RectI renderBounds = firstPlaneToRender.renderMappedImage->getBounds();
    assert(renderBounds.x1 <= renderMappedRectToRender.x1 && renderMappedRectToRender.x2 <= renderBounds.x2 &&
           renderBounds.y1 <= renderMappedRectToRender.y1 && renderMappedRectToRender.y2 <= renderBounds.y2);
# endif

    bool isBeingRenderedElseWhere = false;
    ///At this point if we're in eRenderSafetyFullySafeFrame mode, we are a thread that might have been launched way after
    ///the time renderRectToRender was computed. We recompute it to update the portion to render.
    ///Note that if it is bigger than the initial rectangle, we don't render the bigger rectangle since we cannot
    ///now make the preliminaries call to handle that region (getRoI etc...) so just stick with the old rect to render

    // check the bitmap!
    if (frameArgs.tilesSupported) {
        if (outputUseImage) {
            //The renderMappedImage is cached , read bitmap from it
            canonicalRectToRender.toPixelEnclosing(0, par, &downscaledRectToRender);
            downscaledRectToRender.intersect(firstPlaneToRender.renderMappedImage->getBounds(), &downscaledRectToRender);

            RectI initialRenderRect = downscaledRectToRender;

#if NATRON_ENABLE_TRIMAP
            if (!frameArgs.canAbort && frameArgs.isRenderResponseToUserInteraction) {
                downscaledRectToRender = firstPlaneToRender.renderMappedImage->getMinimalRect_trimap(downscaledRectToRender, &isBeingRenderedElseWhere);
            } else {
                downscaledRectToRender = firstPlaneToRender.renderMappedImage->getMinimalRect(downscaledRectToRender);
            }
#else
            reducedDownscaledRectToRender = renderMappedImage->getMinimalRect(renderRectToRender);
#endif

            ///If the new rect after getMinimalRect is bigger (maybe because another thread as grown the image)
            ///we stick to what was requested
            if ( !initialRenderRect.contains(downscaledRectToRender) ) {
                downscaledRectToRender = initialRenderRect;
            }

            assert( downscaledRectToRender.isNull() || (renderBounds.x1 <= downscaledRectToRender.x1 && downscaledRectToRender.x2 <= renderBounds.x2 &&
                                                        renderBounds.y1 <= downscaledRectToRender.y1 && downscaledRectToRender.y2 <= renderBounds.y2) );
            renderMappedRectToRender = downscaledRectToRender;
        } else {
            //The downscaled image is cached, read bitmap from it
#if NATRON_ENABLE_TRIMAP
            RectI downscaledRectToRenderMinimal;
            if (!frameArgs.canAbort && frameArgs.isRenderResponseToUserInteraction) {
                downscaledRectToRenderMinimal = firstPlaneToRender.downscaleImage->getMinimalRect_trimap(downscaledRectToRender, &isBeingRenderedElseWhere);
            } else {
                downscaledRectToRenderMinimal = firstPlaneToRender.downscaleImage->getMinimalRect(downscaledRectToRender);
            }
#else
            const RectI downscaledRectToRenderMinimal = downscaledImage->getMinimalRect(downscaledRectToRender);
#endif

            assert( downscaledRectToRenderMinimal.isNull() || (renderBounds.x1 <= downscaledRectToRenderMinimal.x1 && downscaledRectToRenderMinimal.x2 <= renderBounds.x2 &&
                                                               renderBounds.y1 <= downscaledRectToRenderMinimal.y1 && downscaledRectToRenderMinimal.y2 <= renderBounds.y2) );


            if (renderFullScaleThenDownscale) {
                ///If the new rect after getMinimalRect is bigger (maybe because another thread as grown the image)
                ///we stick to what was requested
                RectD canonicalrenderRectToRender;
                if ( downscaledRectToRender.contains(downscaledRectToRenderMinimal) ) {
                    downscaledRectToRenderMinimal.toCanonical(mipMapLevel, par, rod, &canonicalrenderRectToRender);
                    downscaledRectToRender = downscaledRectToRenderMinimal;
                } else {
                    downscaledRectToRender.toCanonical(mipMapLevel, par, rod, &canonicalrenderRectToRender);
                }
                canonicalrenderRectToRender.toPixelEnclosing(0, par, &renderMappedRectToRender);
                renderMappedRectToRender.intersect(firstPlaneToRender.renderMappedImage->getBounds(), &renderMappedRectToRender);
            } else {
                ///If the new rect after getMinimalRect is bigger (maybe because another thread as grown the image)
                ///we stick to what was requested
                if ( downscaledRectToRender.contains(downscaledRectToRenderMinimal) ) {
                    downscaledRectToRender = downscaledRectToRenderMinimal;
                    renderMappedRectToRender = downscaledRectToRender;
                }
            }
        }
    } // tilesSupported
      ///It might have been already rendered now
    if ( downscaledRectToRender.isNull() ) {
        return isBeingRenderedElseWhere ? eRenderingFunctorRetTakeImageLock : eRenderingFunctorRetOK;
    }

    ///There cannot be the same thread running 2 concurrent instances of renderRoI on the same effect.
    assert(!_imp->renderArgs.hasLocalData() || !_imp->renderArgs.localData()._validArgs);


    Implementation::ScopedRenderArgs scopedArgs(&_imp->renderArgs);
    scopedArgs.setArgs_firstPass(rod,
                                 renderMappedRectToRender,
                                 time,
                                 view,
                                 false, //< if we reached here the node is not an identity!
                                 0.,
                                 -1);


    ///The scoped args will maintain the args set for this thread during the
    ///whole time the render action is called, so they can be fetched in the
    ///getImage() call.
    /// @see EffectInstance::getImage
    scopedArgs.setArgs_secondPass(rectToRender.inputRois, firstFrame, lastFrame);
    RenderArgs & args = scopedArgs.getLocalData();
    ImagePtr originalInputImage, maskImage;
    Natron::ImagePremultiplicationEnum originalImagePremultiplication;
    InputImagesMap::const_iterator foundPrefInput = rectToRender.imgs.find(preferredInput);
    InputImagesMap::const_iterator foundMaskInput = rectToRender.imgs.end();

    if ( isHostMaskingEnabled() ) {
        foundMaskInput = rectToRender.imgs.find(getMaxInputCount() - 1);
    }
    if ( ( foundPrefInput != rectToRender.imgs.end() ) && !foundPrefInput->second.empty() ) {
        originalInputImage = foundPrefInput->second.front();
    }
    std::map<int, Natron::ImagePremultiplicationEnum>::const_iterator foundPrefPremult = planes.inputPremult.find(preferredInput);
    if ( ( foundPrefPremult != planes.inputPremult.end() ) && originalInputImage ) {
        originalImagePremultiplication = foundPrefPremult->second;
    } else {
        originalImagePremultiplication = Natron::eImagePremultiplicationOpaque;
    }


    if ( ( foundMaskInput != rectToRender.imgs.end() ) && !foundMaskInput->second.empty() ) {
        maskImage = foundMaskInput->second.front();
    }

#ifndef NDEBUG
    RenderScale scale;
    scale.x = Image::getScaleFromMipMapLevel(mipMapLevel);
    scale.y = scale.x;
    // check the dimensions of all input and output images
    const RectD & dstRodCanonical = firstPlaneToRender.renderMappedImage->getRoD();
    RectI dstBounds;
    dstRodCanonical.toPixelEnclosing(firstPlaneToRender.renderMappedImage->getMipMapLevel(), par, &dstBounds); // compute dstRod at level 0
    for (InputImagesMap::const_iterator it = rectToRender.imgs.begin();
         it != rectToRender.imgs.end();
         ++it) {
        for (ImageList::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            assert(outputUseImage || (*it2)->getMipMapLevel() == mipMapLevel);
            const RectD & srcRodCanonical = (*it2)->getRoD();
            RectI srcBounds;
            srcRodCanonical.toPixelEnclosing( (*it2)->getMipMapLevel(), (*it2)->getPixelAspectRatio(), &srcBounds ); // compute srcRod at level 0

            if (!frameArgs.tilesSupported) {
                // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsTiles
                //  If a clip or plugin does not support tiled images, then the host should supply full RoD images to the effect whenever it fetches one.

                ///Note: The renderRoI() function returns an image according to the mipMapLevel given in parameters.
                ///For effects that DO NOT SUPPORT TILES they are expected an input image to be the full RoD.
                ///Hence the resulting image of the renderRoI call made on the input has to be upscaled to its full RoD.
                ///The reason why this upscale is done externally to renderRoI is because renderRoI is "local" to an effect:
                ///The effect has no way to know that the caller (downstream effect) doesn't support tiles. We would have to
                ///pass this in parameters to the renderRoI function and would make it less clear to the caller.
                ///
                ///Another point is that we don't cache the resulting upscaled image (@see getImage()).
                ///The reason why we don't do this is because all images in the NodeCache have a key identifying them.
                ///Part of the key is the mipmapLevel of the image, hence
                ///2 images with different mipmapLevels have different keys. Now if we were to put those "upscaled" images in the cache
                ///they would take the same priority as the images that were REALLY rendered at scale 1. But those upcaled images have poor
                ///quality compared to the images rendered at scale 1, hence we don't cache them.
                ///If we were to cache them, we would need to change the way the cache works and return a list of potential images instead.
                ///This way we could add a "quality" identifier to images and pick the best one from the list returned by the cache.
                RectI srcRealBounds = (*it2)->getBounds();
                RectI dstRealBounds = firstPlaneToRender.renderMappedImage->getBounds();

                assert(srcRealBounds.x1 == srcBounds.x1);
                assert(srcRealBounds.x2 == srcBounds.x2);
                assert(srcRealBounds.y1 == srcBounds.y1);
                assert(srcRealBounds.y2 == srcBounds.y2);
                assert(dstRealBounds.x1 == dstBounds.x1);
                assert(dstRealBounds.x2 == dstBounds.x2);
                assert(dstRealBounds.y1 == dstBounds.y1);
                assert(dstRealBounds.y2 == dstBounds.y2);
            }
            if ( !supportsMultiResolution() ) {
                // http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxImageEffectPropSupportsMultiResolution
                //   Multiple resolution images mean...
                //    input and output images can be of any size
                //    input and output images can be offset from the origin
                assert(srcBounds.x1 == 0);
                assert(srcBounds.y1 == 0);
                assert(srcBounds.x1 == dstBounds.x1);
                assert(srcBounds.x2 == dstBounds.x2);
                assert(srcBounds.y1 == dstBounds.y1);
                assert(srcBounds.y2 == dstBounds.y2);
            }
        } // end for
    } //end for

    if (supportsRenderScaleMaybe() == eSupportsNo) {
        assert(firstPlaneToRender.renderMappedImage->getMipMapLevel() == 0);
        assert(renderMappedMipMapLevel == 0);
    }
#     endif // DEBUG

    RenderingFunctorRetEnum handlerRet =  renderHandler(args,
                                                        frameArgs,
                                                        rectToRender.imgs,
                                                        rectToRender.isIdentity,
                                                        rectToRender.identityTime,
                                                        rectToRender.identityInput,
                                                        renderFullScaleThenDownscale,
                                                        renderUseScaleOneInputs,
                                                        isSequentialRender,
                                                        isRenderResponseToUserInteraction,
                                                        renderMappedRectToRender,
                                                        downscaledRectToRender,
                                                        byPassCache,
                                                        outputClipPrefDepth,
                                                        outputClipPrefsComps,
                                                        processChannels,
                                                        originalInputImage,
                                                        maskImage,
                                                        originalImagePremultiplication,
                                                        planes);
    if (handlerRet == eRenderingFunctorRetOK) {
        if (isBeingRenderedElseWhere) {
            return eRenderingFunctorRetTakeImageLock;
        } else {
            return eRenderingFunctorRetOK;
        }
    } else {
        return handlerRet;
    }
} // EffectInstance::tiledRenderingFunctor

EffectInstance::RenderingFunctorRetEnum
EffectInstance::renderHandler(RenderArgs & args,
                              const ParallelRenderArgs & frameArgs,
                              const InputImagesMap & inputImages,
                              bool identity,
                              double identityTime,
                              Natron::EffectInstance* identityInput,
                              bool renderFullScaleThenDownscale,
                              bool renderUseScaleOneInputs,
                              bool isSequentialRender,
                              bool isRenderResponseToUserInteraction,
                              const RectI & renderMappedRectToRender,
                              const RectI & downscaledRectToRender,
                              bool byPassCache,
                              Natron::ImageBitDepthEnum outputClipPrefDepth,
                              const std::list<Natron::ImageComponents> & outputClipPrefsComps,
                              bool* processChannels,
                              const boost::shared_ptr<Natron::Image> & originalInputImage,
                              const boost::shared_ptr<Natron::Image> & maskImage,
                              Natron::ImagePremultiplicationEnum originalImagePremultiplication,
                              ImagePlanesToRender & planes)
{
    boost::shared_ptr<TimeLapse> timeRecorder;

    if (frameArgs.stats) {
        timeRecorder.reset( new TimeLapse() );
    }

    const PlaneToRender & firstPlane = planes.planes.begin()->second;
    const double time = args._time;
    int mipMapLevel = firstPlane.downscaleImage->getMipMapLevel();
    const int view = args._view;

    // at this point, it may be unnecessary to call render because it was done a long time ago => check the bitmap here!
# ifndef NDEBUG
    RectI renderBounds = firstPlane.renderMappedImage->getBounds();
    assert(renderBounds.x1 <= renderMappedRectToRender.x1 && renderMappedRectToRender.x2 <= renderBounds.x2 &&
           renderBounds.y1 <= renderMappedRectToRender.y1 && renderMappedRectToRender.y2 <= renderBounds.y2);
# endif

    bool outputUseImage = renderUseScaleOneInputs && renderFullScaleThenDownscale;
    RenderActionArgs actionArgs;
    actionArgs.byPassCache = byPassCache;
    for (int i = 0; i < 4; ++i) {
        actionArgs.processChannels[i] = processChannels[i];
    }
    actionArgs.mappedScale.x = actionArgs.mappedScale.y = Image::getScaleFromMipMapLevel( firstPlane.renderMappedImage->getMipMapLevel() );
    assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(actionArgs.mappedScale.x == 1. && actionArgs.mappedScale.y == 1.) ) );
    actionArgs.originalScale.x = firstPlane.downscaleImage->getScale();
    actionArgs.originalScale.y = actionArgs.originalScale.x;
    actionArgs.draftMode = frameArgs.draftMode;

    std::list<std::pair<ImageComponents, ImagePtr> > tmpPlanes;
    bool multiPlanar = isMultiPlanar();

    actionArgs.roi = renderMappedRectToRender;

    assert( !outputClipPrefsComps.empty() );

    if (identity) {
        std::list<Natron::ImageComponents> comps;
        for (std::map<Natron::ImageComponents, PlaneToRender>::iterator it = planes.planes.begin(); it != planes.planes.end(); ++it) {
            comps.push_back( it->second.renderMappedImage->getComponents() );
        }
        assert( !comps.empty() );
        ImageList identityPlanes;
        RenderRoIArgs renderArgs(identityTime,
                                 actionArgs.originalScale,
                                 mipMapLevel,
                                 view,
                                 false,
                                 downscaledRectToRender,
                                 RectD(),
                                 comps,
                                 outputClipPrefDepth,
                                 this);
        if (!identityInput) {
            for (std::map<Natron::ImageComponents, PlaneToRender>::iterator it = planes.planes.begin(); it != planes.planes.end(); ++it) {
                if (outputUseImage) {
                    it->second.fullscaleImage->fillZero(downscaledRectToRender);
                    it->second.fullscaleImage->markForRendered(downscaledRectToRender);
                } else {
                    it->second.downscaleImage->fillZero(downscaledRectToRender);
                    it->second.downscaleImage->markForRendered(downscaledRectToRender);
                }
                if ( frameArgs.stats && frameArgs.stats->isInDepthProfilingEnabled() ) {
                    frameArgs.stats->addRenderInfosForNode( getNode(),  NodePtr(), it->first.getComponentsGlobalName(), renderMappedRectToRender, timeRecorder->getTimeSinceCreation() );
                }
            }

            return eRenderingFunctorRetOK;
        } else {
            EffectInstance::RenderRoIRetCode renderOk = identityInput->renderRoI(renderArgs, &identityPlanes);
            if (renderOk == eRenderRoIRetCodeAborted) {
                return eRenderingFunctorRetAborted;
            } else if (renderOk == eRenderRoIRetCodeFailed) {
                return eRenderingFunctorRetFailed;
            } else if ( identityPlanes.empty() ) {
                for (std::map<Natron::ImageComponents, PlaneToRender>::iterator it = planes.planes.begin(); it != planes.planes.end(); ++it) {
                    if (outputUseImage) {
                        it->second.fullscaleImage->fillZero(downscaledRectToRender);
                        it->second.fullscaleImage->markForRendered(downscaledRectToRender);
                    } else {
                        it->second.downscaleImage->fillZero(downscaledRectToRender);
                        it->second.downscaleImage->markForRendered(downscaledRectToRender);
                    }
                    if ( frameArgs.stats && frameArgs.stats->isInDepthProfilingEnabled() ) {
                        frameArgs.stats->addRenderInfosForNode( getNode(),  identityInput->getNode(), it->first.getComponentsGlobalName(), renderMappedRectToRender, timeRecorder->getTimeSinceCreation() );
                    }
                }

                return eRenderingFunctorRetOK;
            } else {
                assert( identityPlanes.size() == planes.planes.size() );

                ImageList::iterator idIt = identityPlanes.begin();
                for (std::map<Natron::ImageComponents, PlaneToRender>::iterator it = planes.planes.begin(); it != planes.planes.end(); ++it, ++idIt) {
                    if ( outputUseImage && ( (*idIt)->getMipMapLevel() > it->second.fullscaleImage->getMipMapLevel() ) ) {
                        if ( !(*idIt)->getBounds().contains(downscaledRectToRender) ) {
                            ///Fill the RoI with 0's as the identity input image might have bounds contained into the RoI
                            it->second.fullscaleImage->fillZero(downscaledRectToRender);
                        }

                        ///Convert format first if needed
                        ImagePtr sourceImage;
                        if ( ( it->second.fullscaleImage->getComponents() != (*idIt)->getComponents() ) || ( it->second.fullscaleImage->getBitDepth() != (*idIt)->getBitDepth() ) ) {
                            sourceImage.reset( new Image(it->second.fullscaleImage->getComponents(), (*idIt)->getRoD(), (*idIt)->getBounds(),
                                                         (*idIt)->getMipMapLevel(), (*idIt)->getPixelAspectRatio(), it->second.fullscaleImage->getBitDepth(), false) );
                            Natron::ViewerColorSpaceEnum colorspace = getApp()->getDefaultColorSpaceForBitDepth( (*idIt)->getBitDepth() );
                            Natron::ViewerColorSpaceEnum dstColorspace = getApp()->getDefaultColorSpaceForBitDepth( it->second.fullscaleImage->getBitDepth() );
                            (*idIt)->convertToFormat( (*idIt)->getBounds(), colorspace, dstColorspace, 3, false, false, sourceImage.get() );
                        } else {
                            sourceImage = *idIt;
                        }

                        ///then upscale
                        const RectD & rod = sourceImage->getRoD();
                        RectI bounds;
                        rod.toPixelEnclosing(it->second.renderMappedImage->getMipMapLevel(), it->second.renderMappedImage->getPixelAspectRatio(), &bounds);
                        ImagePtr inputPlane( new Image(it->first, rod, bounds, it->second.renderMappedImage->getMipMapLevel(),
                                                       it->second.renderMappedImage->getPixelAspectRatio(), it->second.renderMappedImage->getBitDepth(), false) );
                        sourceImage->upscaleMipMap( sourceImage->getBounds(), sourceImage->getMipMapLevel(), inputPlane->getMipMapLevel(), inputPlane.get() );
                        it->second.fullscaleImage->pasteFrom(*inputPlane, downscaledRectToRender, false);
                        it->second.fullscaleImage->markForRendered(downscaledRectToRender);
                    } else {
                        if ( !(*idIt)->getBounds().contains(downscaledRectToRender) ) {
                            ///Fill the RoI with 0's as the identity input image might have bounds contained into the RoI
                            it->second.downscaleImage->fillZero(downscaledRectToRender);
                        }

                        ///Convert format if needed or copy
                        if ( ( it->second.downscaleImage->getComponents() != (*idIt)->getComponents() ) || ( it->second.downscaleImage->getBitDepth() != (*idIt)->getBitDepth() ) ) {
                            Natron::ViewerColorSpaceEnum colorspace = getApp()->getDefaultColorSpaceForBitDepth( (*idIt)->getBitDepth() );
                            Natron::ViewerColorSpaceEnum dstColorspace = getApp()->getDefaultColorSpaceForBitDepth( it->second.fullscaleImage->getBitDepth() );
                            RectI convertWindow;
                            (*idIt)->getBounds().intersect(downscaledRectToRender, &convertWindow);
                            (*idIt)->convertToFormat( convertWindow, colorspace, dstColorspace, 3, false, false, it->second.downscaleImage.get() );
                        } else {
                            it->second.downscaleImage->pasteFrom(**idIt, downscaledRectToRender, false);
                        }
                        it->second.downscaleImage->markForRendered(downscaledRectToRender);
                    }

                    if ( frameArgs.stats && frameArgs.stats->isInDepthProfilingEnabled() ) {
                        frameArgs.stats->addRenderInfosForNode( getNode(),  identityInput->getNode(), it->first.getComponentsGlobalName(), renderMappedRectToRender, timeRecorder->getTimeSinceCreation() );
                    }
                }

                return eRenderingFunctorRetOK;
            } // if (renderOk == eRenderRoIRetCodeAborted) {
        }  //  if (!identityInput) {
    } // if (identity) {

    args._outputPlanes = planes.planes;
    for (std::map<Natron::ImageComponents, PlaneToRender>::iterator it = args._outputPlanes.begin(); it != args._outputPlanes.end(); ++it) {
        /*
         * When using the cache, allocate a local temporary buffer onto which the plug-in will render, and then safely
         * copy this buffer to the shared (among threads) image.
         * This is also needed if the plug-in does not support the number of components of the renderMappedImage
         */
        Natron::ImageComponents prefComp;
        if (multiPlanar) {
            prefComp = getNode()->findClosestSupportedComponents( -1, it->second.renderMappedImage->getComponents() );
        } else {
            prefComp = Node::findClosestInList(it->second.renderMappedImage->getComponents(), outputClipPrefsComps, multiPlanar);
        }

        if ( ( it->second.renderMappedImage->usesBitMap() || ( prefComp != it->second.renderMappedImage->getComponents() ) ||
               ( outputClipPrefDepth != it->second.renderMappedImage->getBitDepth() ) ) && !isPaintingOverItselfEnabled() ) {
            it->second.tmpImage.reset( new Image(prefComp,
                                                 it->second.renderMappedImage->getRoD(),
                                                 actionArgs.roi,
                                                 it->second.renderMappedImage->getMipMapLevel(),
                                                 it->second.renderMappedImage->getPixelAspectRatio(),
                                                 outputClipPrefDepth,
                                                 false) ); //< no bitmap
        } else {
            it->second.tmpImage = it->second.renderMappedImage;
        }
        tmpPlanes.push_back( std::make_pair(it->second.renderMappedImage->getComponents(), it->second.tmpImage) );
    }


#if NATRON_ENABLE_TRIMAP
    if (!frameArgs.canAbort && frameArgs.isRenderResponseToUserInteraction) {
        for (std::map<Natron::ImageComponents, PlaneToRender>::iterator it = args._outputPlanes.begin(); it != args._outputPlanes.end(); ++it) {
            if (outputUseImage) {
                it->second.fullscaleImage->markForRendering(downscaledRectToRender);
            } else {
                it->second.downscaleImage->markForRendering(downscaledRectToRender);
            }
        }
    }
#endif


    /// Render in the temporary image


    actionArgs.time = time;
    actionArgs.view = view;
    actionArgs.isSequentialRender = isSequentialRender;
    actionArgs.isRenderResponseToUserInteraction = isRenderResponseToUserInteraction;
    actionArgs.inputImages = inputImages;

    std::list< std::list<std::pair<ImageComponents, ImagePtr> > > planesLists;
    if (!multiPlanar) {
        for (std::list<std::pair<ImageComponents, ImagePtr> >::iterator it = tmpPlanes.begin(); it != tmpPlanes.end(); ++it) {
            std::list<std::pair<ImageComponents, ImagePtr> > tmp;
            tmp.push_back(*it);
            planesLists.push_back(tmp);
        }
    } else {
        planesLists.push_back(tmpPlanes);
    }

    bool renderAborted = false;
    std::map<Natron::ImageComponents, PlaneToRender> outputPlanes;
    for (std::list<std::list<std::pair<ImageComponents, ImagePtr> > >::iterator it = planesLists.begin(); it != planesLists.end(); ++it) {
        if (!multiPlanar) {
            assert( !it->empty() );
            args._outputPlaneBeingRendered = it->front().first;
        }
        actionArgs.outputPlanes = *it;

        Natron::StatusEnum st = render_public(actionArgs);

        renderAborted = aborted();

        /*
         * Since new planes can have been allocated on the fly by allocateImagePlaneAndSetInThreadLocalStorage(), refresh
         * the planes map from the thread local storage once the render action is finished
         */
        if ( it == planesLists.begin() ) {
            outputPlanes = args._outputPlanes;
            assert( !outputPlanes.empty() );
        }

        if ( (st != eStatusOK) || renderAborted ) {
#if NATRON_ENABLE_TRIMAP
            if (!frameArgs.canAbort && frameArgs.isRenderResponseToUserInteraction) {
                /*
                   At this point, another thread might have already gotten this image from the cache and could end-up
                   using it while it has still pixels marked to PIXEL_UNAVAILABLE, hence clear the bitmap
                 */
                for (std::map<ImageComponents, PlaneToRender>::const_iterator it = outputPlanes.begin(); it != outputPlanes.end(); ++it) {
                    if (outputUseImage) {
                        it->second.fullscaleImage->clearBitmap(downscaledRectToRender);
                    } else {
                        it->second.downscaleImage->clearBitmap(downscaledRectToRender);
                    }
                }
            }
#endif

            return st != eStatusOK ? eRenderingFunctorRetFailed : eRenderingFunctorRetAborted;
        } // if (st != eStatusOK || renderAborted) {
    } // for (std::list<std::list<std::pair<ImageComponents,ImagePtr> > >::iterator it = planesLists.begin(); it != planesLists.end(); ++it)

    assert(!renderAborted);

    bool unPremultIfNeeded = planes.outputPremult == eImagePremultiplicationPremultiplied;
    bool useMaskMix = isHostMaskingEnabled() || isHostMixingEnabled();
    double mix = useMaskMix ? getNode()->getHostMixingValue(time) : 1.;
    bool doMask = useMaskMix ? getNode()->isMaskEnabled(getMaxInputCount() - 1) : false;

    //Check for NaNs, copy to output image and mark for rendered
    for (std::map<ImageComponents, PlaneToRender>::const_iterator it = outputPlanes.begin(); it != outputPlanes.end(); ++it) {
        bool unPremultRequired = unPremultIfNeeded && it->second.tmpImage->getComponentsCount() == 4 && it->second.renderMappedImage->getComponentsCount() == 3;

        if ( frameArgs.doNansHandling && it->second.tmpImage->checkForNaNs(actionArgs.roi) ) {
            QString warning( getNode()->getScriptName_mt_safe().c_str() );
            warning.append(": ");
            warning.append( tr("rendered rectangle (") );
            warning.append( QString::number(actionArgs.roi.x1) );
            warning.append(',');
            warning.append( QString::number(actionArgs.roi.y1) );
            warning.append(")-(");
            warning.append( QString::number(actionArgs.roi.x2) );
            warning.append(',');
            warning.append( QString::number(actionArgs.roi.y2) );
            warning.append(") ");
            warning.append( tr("contains NaN values. They have been converted to 1.") );
            setPersistentMessage( Natron::eMessageTypeWarning, warning.toStdString() );
        }
        if (it->second.isAllocatedOnTheFly) {
            ///Plane allocated on the fly only have a temp image if using the cache and it is defined over the render window only
            if (it->second.tmpImage != it->second.renderMappedImage) {
                assert(it->second.tmpImage->getBounds() == actionArgs.roi);

                if ( ( it->second.renderMappedImage->getComponents() != it->second.tmpImage->getComponents() ) ||
                     ( it->second.renderMappedImage->getBitDepth() != it->second.tmpImage->getBitDepth() ) ) {
                    it->second.tmpImage->convertToFormat( it->second.tmpImage->getBounds(),
                                                          getApp()->getDefaultColorSpaceForBitDepth( it->second.tmpImage->getBitDepth() ),
                                                          getApp()->getDefaultColorSpaceForBitDepth( it->second.renderMappedImage->getBitDepth() ),
                                                          -1, false, unPremultRequired, it->second.renderMappedImage.get() );
                } else {
                    it->second.renderMappedImage->pasteFrom(*(it->second.tmpImage), it->second.tmpImage->getBounds(), false);
                }
            }
            it->second.renderMappedImage->markForRendered(actionArgs.roi);
        } else {
            if (renderFullScaleThenDownscale) {
                ///copy the rectangle rendered in the full scale image to the downscaled output

                /* If we're using renderUseScaleOneInputs, the full scale image is cached.
                   We might have been asked to render only a portion.
                   Hence we're not sure that the whole part of the image will be downscaled.
                   Instead we do all the downscale at once at the end of renderRoI().
                   If !renderUseScaleOneInputs the image is not cached.
                   Hence we know it will be rendered completly so it is safe to do this here and take advantage of the multi-threading.*/
                if ( (mipMapLevel != 0) && !renderUseScaleOneInputs ) {
                    assert(it->second.fullscaleImage != it->second.downscaleImage && it->second.renderMappedImage == it->second.fullscaleImage);


                    if ( ( it->second.downscaleImage->getComponents() != it->second.tmpImage->getComponents() ) ||
                         ( it->second.downscaleImage->getBitDepth() != it->second.tmpImage->getBitDepth() ) ) {
                        /*
                         * BitDepth/Components conversion required as well as downscaling, do conversion to a tmp buffer
                         */
                        ImagePtr tmp( new Image(it->second.downscaleImage->getComponents(), it->second.tmpImage->getRoD(), it->second.tmpImage->getBounds(), mipMapLevel, it->second.tmpImage->getPixelAspectRatio(), it->second.downscaleImage->getBitDepth(), false) );

                        it->second.tmpImage->convertToFormat( it->second.tmpImage->getBounds(),
                                                              getApp()->getDefaultColorSpaceForBitDepth( it->second.tmpImage->getBitDepth() ),
                                                              getApp()->getDefaultColorSpaceForBitDepth( it->second.downscaleImage->getBitDepth() ),
                                                              -1, false, unPremultRequired, tmp.get() );
                        tmp->downscaleMipMap( it->second.tmpImage->getRoD(),
                                              actionArgs.roi, 0, mipMapLevel, false, it->second.downscaleImage.get() );
                    } else {
                        /*
                         *  Downscaling required only
                         */
                        it->second.tmpImage->downscaleMipMap( it->second.tmpImage->getRoD(),
                                                              actionArgs.roi, 0, mipMapLevel, false, it->second.downscaleImage.get() );
                    }

                    it->second.downscaleImage->copyUnProcessedChannels(downscaledRectToRender, planes.outputPremult, originalImagePremultiplication,  processChannels, originalInputImage);
                    if (useMaskMix) {
                        it->second.downscaleImage->applyMaskMix(downscaledRectToRender, maskImage.get(), originalInputImage.get(), doMask, false, mix);
                    }
                    it->second.downscaleImage->markForRendered(downscaledRectToRender);
                } else { // if (mipMapLevel != 0 && !renderUseScaleOneInputs) {
                    assert(it->second.renderMappedImage == it->second.fullscaleImage);
                    if (it->second.tmpImage != it->second.renderMappedImage) {
                        if ( ( it->second.fullscaleImage->getComponents() != it->second.tmpImage->getComponents() ) ||
                             ( it->second.fullscaleImage->getBitDepth() != it->second.tmpImage->getBitDepth() ) ) {
                            /*
                             * BitDepth/Components conversion required
                             */

                            it->second.tmpImage->copyUnProcessedChannels(it->second.tmpImage->getBounds(), planes.outputPremult, originalImagePremultiplication, processChannels, originalInputImage);
                            if (useMaskMix) {
                                it->second.tmpImage->applyMaskMix(actionArgs.roi, maskImage.get(), originalInputImage.get(), doMask, false, mix);
                            }
                            it->second.tmpImage->convertToFormat( it->second.tmpImage->getBounds(),
                                                                  getApp()->getDefaultColorSpaceForBitDepth( it->second.tmpImage->getBitDepth() ),
                                                                  getApp()->getDefaultColorSpaceForBitDepth( it->second.fullscaleImage->getBitDepth() ),
                                                                  -1, false, unPremultRequired, it->second.fullscaleImage.get() );
                        } else {
                            /*
                             * No conversion required, copy to output
                             */
                            int prefInput = getNode()->getPreferredInput();
                            RectI roiPixel;
                            ImagePtr originalInputImageFullScale;
                            if (prefInput != -1) {
                                originalInputImageFullScale = getImage(prefInput, time, actionArgs.mappedScale, view, NULL, originalInputImage->getComponents(), originalInputImage->getBitDepth(), originalInputImage->getPixelAspectRatio(), false, &roiPixel);
                            }


                            if (originalInputImageFullScale) {
                                it->second.fullscaleImage->copyUnProcessedChannels(actionArgs.roi, planes.outputPremult, originalImagePremultiplication,  processChannels, originalInputImageFullScale);
                                if (useMaskMix) {
                                    ImagePtr originalMaskFullScale = getImage(getMaxInputCount() - 1, time, actionArgs.mappedScale, view, NULL, ImageComponents::getAlphaComponents(), originalInputImage->getBitDepth(), originalInputImage->getPixelAspectRatio(), false, &roiPixel);
                                    if (originalMaskFullScale) {
                                        it->second.fullscaleImage->applyMaskMix(actionArgs.roi, originalMaskFullScale.get(), originalInputImageFullScale.get(), doMask, false, mix);
                                    }
                                }
                            }
                            it->second.fullscaleImage->pasteFrom(*it->second.tmpImage, actionArgs.roi, false);
                        }
                    }
                    it->second.fullscaleImage->markForRendered(actionArgs.roi);
                }
            } else { // if (renderFullScaleThenDownscale) {
                ///Copy the rectangle rendered in the downscaled image
                if (it->second.tmpImage != it->second.downscaleImage) {
                    if ( ( it->second.downscaleImage->getComponents() != it->second.tmpImage->getComponents() ) ||
                         ( it->second.downscaleImage->getBitDepth() != it->second.tmpImage->getBitDepth() ) ) {
                        /*
                         * BitDepth/Components conversion required
                         */


                        it->second.tmpImage->convertToFormat( it->second.tmpImage->getBounds(),
                                                              getApp()->getDefaultColorSpaceForBitDepth( it->second.tmpImage->getBitDepth() ),
                                                              getApp()->getDefaultColorSpaceForBitDepth( it->second.downscaleImage->getBitDepth() ),
                                                              -1, false, unPremultRequired, it->second.downscaleImage.get() );
                    } else {
                        /*
                         * No conversion required, copy to output
                         */

                        it->second.downscaleImage->pasteFrom(*(it->second.tmpImage), it->second.downscaleImage->getBounds(), false);
                    }
                }

                it->second.downscaleImage->copyUnProcessedChannels(actionArgs.roi, planes.outputPremult, originalImagePremultiplication, processChannels, originalInputImage);
                if (useMaskMix) {
                    it->second.downscaleImage->applyMaskMix(actionArgs.roi, maskImage.get(), originalInputImage.get(), doMask, false, mix);
                }
                it->second.downscaleImage->markForRendered(downscaledRectToRender);
            } // if (renderFullScaleThenDownscale) {
        } // if (it->second.isAllocatedOnTheFly) {

        if ( frameArgs.stats && frameArgs.stats->isInDepthProfilingEnabled() ) {
            frameArgs.stats->addRenderInfosForNode( getNode(),  NodePtr(), it->first.getComponentsGlobalName(), renderMappedRectToRender, timeRecorder->getTimeSinceCreation() );
        }
    } // for (std::map<ImageComponents,PlaneToRender>::const_iterator it = outputPlanes.begin(); it != outputPlanes.end(); ++it) {


    return eRenderingFunctorRetOK;
} // tiledRenderingFunctor

ImagePtr
EffectInstance::allocateImagePlaneAndSetInThreadLocalStorage(const Natron::ImageComponents & plane)
{
    /*
     * The idea here is that we may have asked the plug-in to render say motion.forward, but it can only render both fotward
     * and backward at a time.
     * So it needs to allocate motion.backward and store it in the cache for efficiency.
     * Note that when calling this, the plug-in is already in the render action, hence in case of Host frame threading,
     * this function will be called as many times as there were thread used by the host frame threading.
     * For all other planes, there was a local temporary image, shared among all threads for the calls to render.
     * Since we may be in a thread of the host frame threading, only allocate a temporary image of the size of the rectangle
     * to render and mark that we're a plane allocated on the fly so that the tiledRenderingFunctor can know this is a plane
     * to handle specifically.
     */

    if ( _imp->renderArgs.hasLocalData() ) {
        RenderArgs & args = _imp->renderArgs.localData();
        if (args._validArgs) {
            assert( !args._outputPlanes.empty() );

            const PlaneToRender & firstPlane = args._outputPlanes.begin()->second;
            bool useCache = firstPlane.fullscaleImage->usesBitMap() || firstPlane.downscaleImage->usesBitMap();
            const ImagePtr & img = firstPlane.fullscaleImage->usesBitMap() ? firstPlane.fullscaleImage : firstPlane.downscaleImage;
            boost::shared_ptr<ImageParams> params = img->getParams();
            PlaneToRender p;
            bool ok = allocateImagePlane(img->getKey(), img->getRoD(), img->getBounds(), img->getBounds(), false, params->getFramesNeeded(), plane, img->getBitDepth(), img->getPixelAspectRatio(), img->getMipMapLevel(), false, false, false, useCache, &p.fullscaleImage, &p.downscaleImage);
            if (!ok) {
                return ImagePtr();
            } else {
                p.renderMappedImage = p.downscaleImage;
                p.isAllocatedOnTheFly = true;

                /*
                 * Allocate a temporary image for rendering only if using cache
                 */
                if (useCache) {
                    p.tmpImage.reset( new Image(p.renderMappedImage->getComponents(),
                                                p.renderMappedImage->getRoD(),
                                                args._renderWindowPixel,
                                                p.renderMappedImage->getMipMapLevel(),
                                                p.renderMappedImage->getPixelAspectRatio(),
                                                p.renderMappedImage->getBitDepth(),
                                                false) );
                } else {
                    p.tmpImage = p.renderMappedImage;
                }
                args._outputPlanes.insert( std::make_pair(plane, p) );

                return p.downscaleImage;
            }
        } else {
            return ImagePtr();
        }
    } else {
        return ImagePtr();
    }
} // allocateImagePlaneAndSetInThreadLocalStorage

void
EffectInstance::openImageFileKnob()
{
    const std::vector< boost::shared_ptr<KnobI> > & knobs = getKnobs();

    for (U32 i = 0; i < knobs.size(); ++i) {
        if ( knobs[i]->typeName() == KnobFile::typeNameStatic() ) {
            boost::shared_ptr<KnobFile> fk = boost::dynamic_pointer_cast<KnobFile>(knobs[i]);
            assert(fk);
            if ( fk->isInputImageFile() ) {
                std::string file = fk->getValue();
                if ( file.empty() ) {
                    fk->open_file();
                }
                break;
            }
        } else if ( knobs[i]->typeName() == KnobOutputFile::typeNameStatic() ) {
            boost::shared_ptr<KnobOutputFile> fk = boost::dynamic_pointer_cast<KnobOutputFile>(knobs[i]);
            assert(fk);
            if ( fk->isOutputImageFile() ) {
                std::string file = fk->getValue();
                if ( file.empty() ) {
                    fk->open_file();
                }
                break;
            }
        }
    }
}

void
EffectInstance::evaluate(KnobI* knob,
                         bool isSignificant,
                         Natron::ValueChangedReasonEnum /*reason*/)
{
    ////If the node is currently modifying its input, to ask for a render
    ////because at then end of the inputChanged handler, it will ask for a refresh
    ////and a rebuild of the inputs tree.
    NodePtr node = getNode();

    if ( node->duringInputChangedAction() ) {
        return;
    }

    if ( getApp()->getProject()->isLoadingProject() ) {
        return;
    }


    KnobButton* button = dynamic_cast<KnobButton*>(knob);

    /*if this is a writer (openfx or built-in writer)*/
    if ( isWriter() ) {
        /*if this is a button and it is a render button,we're safe to assume the plug-ins wants to start rendering.*/
        if (button) {
            if ( button->isRenderButton() ) {
                std::string sequentialNode;
                if ( node->hasSequentialOnlyNodeUpstream(sequentialNode) ) {
                    if (node->getApp()->getProject()->getProjectViewsCount() > 1) {
                        Natron::StandardButtonEnum answer =
                            Natron::questionDialog( QObject::tr("Render").toStdString(),
                                                    sequentialNode + QObject::tr(" can only "
                                                                                 "render in sequential mode. Due to limitations in the "
                                                                                 "OpenFX standard that means that %1"
                                                                                 " will not be able "
                                                                                 "to render all the views of the project. "
                                                                                 "Only the main view of the project will be rendered, you can "
                                                                                 "change the main view in the project settings. Would you like "
                                                                                 "to continue ?").arg(NATRON_APPLICATION_NAME).toStdString(), false );
                        if (answer != Natron::eStandardButtonYes) {
                            return;
                        }
                    }
                }
                AppInstance::RenderWork w;
                w.writer = dynamic_cast<OutputEffectInstance*>(this);
                w.firstFrame = INT_MIN;
                w.lastFrame = INT_MAX;
                std::list<AppInstance::RenderWork> works;
                works.push_back(w);
                getApp()->startWritersRendering(getApp()->isRenderStatsActionChecked(), works);

                return;
            }
        }
    }

    ///increments the knobs age following a change
    if (!button && isSignificant) {
        node->incrementKnobsAge();
    }


    double time = getCurrentTime();
    std::list<ViewerInstance* > viewers;
    node->hasViewersConnected(&viewers);
    for (std::list<ViewerInstance* >::iterator it = viewers.begin();
         it != viewers.end();
         ++it) {
        if (isSignificant) {
            (*it)->renderCurrentFrame(true);
        } else {
            (*it)->redrawViewer();
        }
    }

    getNode()->refreshPreviewsRecursivelyDownstream(time);
} // evaluate

bool
EffectInstance::message(Natron::MessageTypeEnum type,
                        const std::string & content) const
{
    return getNode()->message(type, content);
}

void
EffectInstance::setPersistentMessage(Natron::MessageTypeEnum type,
                                     const std::string & content)
{
    getNode()->setPersistentMessage(type, content);
}

void
EffectInstance::clearPersistentMessage(bool recurse)
{
    getNode()->clearPersistentMessage(recurse);
}

int
EffectInstance::getInputNumber(Natron::EffectInstance* inputEffect) const
{
    for (int i = 0; i < getMaxInputCount(); ++i) {
        if (getInput(i) == inputEffect) {
            return i;
        }
    }

    return -1;
}

/**
 * @brief Does this effect supports rendering at a different scale than 1 ?
 * There is no OFX property for this purpose. The only solution found for OFX is that if a isIdentity
 * with renderscale != 1 fails, the host retries with renderscale = 1 (and upscaled images).
 * If the renderScale support was not set, this throws an exception.
 **/
bool
EffectInstance::supportsRenderScale() const
{
    if (_imp->supportsRenderScale == eSupportsMaybe) {
        qDebug() << "EffectInstance::supportsRenderScale should be set before calling supportsRenderScale(), or use supportsRenderScaleMaybe() instead";
        throw std::runtime_error("supportsRenderScale not set");
    }

    return _imp->supportsRenderScale == eSupportsYes;
}

EffectInstance::SupportsEnum
EffectInstance::supportsRenderScaleMaybe() const
{
    QMutexLocker l(&_imp->supportsRenderScaleMutex);

    return _imp->supportsRenderScale;
}

/// should be set during effect initialization, but may also be set by the first getRegionOfDefinition that succeeds
void
EffectInstance::setSupportsRenderScaleMaybe(EffectInstance::SupportsEnum s) const
{
    {
        QMutexLocker l(&_imp->supportsRenderScaleMutex);

        _imp->supportsRenderScale = s;
    }
    NodePtr node = getNode();

    if (node) {
        node->onSetSupportRenderScaleMaybeSet( (int)s );
    }
}

void
EffectInstance::setOutputFilesForWriter(const std::string & pattern)
{
    if ( !isWriter() ) {
        return;
    }

    const std::vector<boost::shared_ptr<KnobI> > & knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        if ( knobs[i]->typeName() == KnobOutputFile::typeNameStatic() ) {
            boost::shared_ptr<KnobOutputFile> fk = boost::dynamic_pointer_cast<KnobOutputFile>(knobs[i]);
            assert(fk);
            if ( fk->isOutputImageFile() ) {
                fk->setValue(pattern, 0);
                break;
            }
        }
    }
}

PluginMemory*
EffectInstance::newMemoryInstance(size_t nBytes)
{
    PluginMemory* ret = new PluginMemory( getNode()->getLiveInstance() ); //< hack to get "this" as a shared ptr
    bool wasntLocked = ret->alloc(nBytes);

    assert(wasntLocked);
    Q_UNUSED(wasntLocked);

    return ret;
}

void
EffectInstance::addPluginMemoryPointer(PluginMemory* mem)
{
    QMutexLocker l(&_imp->pluginMemoryChunksMutex);

    _imp->pluginMemoryChunks.push_back(mem);
}

void
EffectInstance::removePluginMemoryPointer(PluginMemory* mem)
{
    QMutexLocker l(&_imp->pluginMemoryChunksMutex);
    std::list<PluginMemory*>::iterator it = std::find(_imp->pluginMemoryChunks.begin(), _imp->pluginMemoryChunks.end(), mem);

    if ( it != _imp->pluginMemoryChunks.end() ) {
        _imp->pluginMemoryChunks.erase(it);
    }
}

void
EffectInstance::registerPluginMemory(size_t nBytes)
{
    getNode()->registerPluginMemory(nBytes);
}

void
EffectInstance::unregisterPluginMemory(size_t nBytes)
{
    getNode()->unregisterPluginMemory(nBytes);
}

void
EffectInstance::onAllKnobsSlaved(bool isSlave,
                                 KnobHolder* master)
{
    getNode()->onAllKnobsSlaved(isSlave, master);
}

void
EffectInstance::onKnobSlaved(KnobI* slave,
                             KnobI* master,
                             int dimension,
                             bool isSlave)
{
    getNode()->onKnobSlaved(slave, master, dimension, isSlave);
}

void
EffectInstance::setCurrentViewportForOverlays_public(OverlaySupport* viewport)
{
    getNode()->setCurrentViewportForDefaultOverlays(viewport);
    setCurrentViewportForOverlays(viewport);
}

void
EffectInstance::drawOverlay_public(double time,
                                   double scaleX,
                                   double scaleY)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay() && !getNode()->hasDefaultOverlay() ) {
        return;
    }

    RECURSIVE_ACTION();

    _imp->setDuringInteractAction(true);
    drawOverlay(time, scaleX, scaleY);
    getNode()->drawDefaultOverlay(time, scaleX, scaleY);
    _imp->setDuringInteractAction(false);
}

bool
EffectInstance::onOverlayPenDown_public(double time,
                                        double scaleX,
                                        double scaleY,
                                        const QPointF & viewportPos,
                                        const QPointF & pos,
                                        double pressure)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasDefaultOverlay() ) {
        return false;
    }

    bool ret;
    {
        NON_RECURSIVE_ACTION();
        _imp->setDuringInteractAction(true);
        ret = onOverlayPenDown(time, scaleX, scaleY, viewportPos, pos, pressure);
        if (!ret) {
            ret |= getNode()->onOverlayPenDownDefault(scaleX, scaleY, viewportPos, pos, pressure);
        }
        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayPenMotion_public(double time,
                                          double scaleX,
                                          double scaleY,
                                          const QPointF & viewportPos,
                                          const QPointF & pos,
                                          double pressure)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasDefaultOverlay() ) {
        return false;
    }


    NON_RECURSIVE_ACTION();
    _imp->setDuringInteractAction(true);
    bool ret = onOverlayPenMotion(time, scaleX, scaleY, viewportPos, pos, pressure);
    if (!ret) {
        ret |= getNode()->onOverlayPenMotionDefault(scaleX, scaleY, viewportPos, pos, pressure);
    }
    _imp->setDuringInteractAction(false);
    //Don't chek if render is needed on pen motion, wait for the pen up

    //checkIfRenderNeeded();
    return ret;
}

bool
EffectInstance::onOverlayPenUp_public(double time,
                                      double scaleX,
                                      double scaleY,
                                      const QPointF & viewportPos,
                                      const QPointF & pos,
                                      double pressure)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasDefaultOverlay() ) {
        return false;
    }
    bool ret;
    {
        NON_RECURSIVE_ACTION();
        _imp->setDuringInteractAction(true);
        ret = onOverlayPenUp(time, scaleX, scaleY, viewportPos, pos, pressure);
        if (!ret) {
            ret |= getNode()->onOverlayPenUpDefault(scaleX, scaleY, viewportPos, pos, pressure);
        }
        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayKeyDown_public(double time,
                                        double scaleX,
                                        double scaleY,
                                        Natron::Key key,
                                        Natron::KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasDefaultOverlay() ) {
        return false;
    }

    bool ret;
    {
        NON_RECURSIVE_ACTION();
        _imp->setDuringInteractAction(true);
        ret = onOverlayKeyDown(time, scaleX, scaleY, key, modifiers);
        if (!ret) {
            ret |= getNode()->onOverlayKeyDownDefault(scaleX, scaleY, key, modifiers);
        }
        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayKeyUp_public(double time,
                                      double scaleX,
                                      double scaleY,
                                      Natron::Key key,
                                      Natron::KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasDefaultOverlay() ) {
        return false;
    }

    bool ret;
    {
        NON_RECURSIVE_ACTION();

        _imp->setDuringInteractAction(true);
        ret = onOverlayKeyUp(time, scaleX, scaleY, key, modifiers);
        if (!ret) {
            ret |= getNode()->onOverlayKeyUpDefault(scaleX, scaleY, key, modifiers);
        }
        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayKeyRepeat_public(double time,
                                          double scaleX,
                                          double scaleY,
                                          Natron::Key key,
                                          Natron::KeyboardModifiers modifiers)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay()  && !getNode()->hasDefaultOverlay() ) {
        return false;
    }

    bool ret;
    {
        NON_RECURSIVE_ACTION();
        _imp->setDuringInteractAction(true);
        ret = onOverlayKeyRepeat(time, scaleX, scaleY, key, modifiers);
        if (!ret) {
            ret |= getNode()->onOverlayKeyRepeatDefault(scaleX, scaleY, key, modifiers);
        }
        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayFocusGained_public(double time,
                                            double scaleX,
                                            double scaleY)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay() && !getNode()->hasDefaultOverlay() ) {
        return false;
    }

    bool ret;
    {
        NON_RECURSIVE_ACTION();
        _imp->setDuringInteractAction(true);
        ret = onOverlayFocusGained(time, scaleX, scaleY);
        if (!ret) {
            ret |= getNode()->onOverlayFocusGainedDefault(scaleX, scaleY);
        }
        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::onOverlayFocusLost_public(double time,
                                          double scaleX,
                                          double scaleY)
{
    ///cannot be run in another thread
    assert( QThread::currentThread() == qApp->thread() );
    if ( !hasOverlay() && !getNode()->hasDefaultOverlay() ) {
        return false;
    }
    bool ret;
    {
        NON_RECURSIVE_ACTION();
        _imp->setDuringInteractAction(true);
        ret = onOverlayFocusLost(time, scaleX, scaleY);
        if (!ret) {
            ret |= getNode()->onOverlayFocusLostDefault(scaleX, scaleY);
        }
        _imp->setDuringInteractAction(false);
    }
    checkIfRenderNeeded();

    return ret;
}

bool
EffectInstance::isDoingInteractAction() const
{
    QReadLocker l(&_imp->duringInteractActionMutex);

    return _imp->duringInteractAction;
}

Natron::StatusEnum
EffectInstance::render_public(const RenderActionArgs & args)
{
    NON_RECURSIVE_ACTION();
    EffectPointerThreadProperty_RAII propHolder_raii(this);

    return render(args);
}

Natron::StatusEnum
EffectInstance::getTransform_public(double time,
                                    const RenderScale & renderScale,
                                    int view,
                                    Natron::EffectInstance** inputToTransform,
                                    Transform::Matrix3x3* transform)
{
    RECURSIVE_ACTION();
    assert( getCanTransform() );

    return getTransform(time, renderScale, view, inputToTransform, transform);
}

bool
EffectInstance::isIdentity_public(bool useIdentityCache, // only set to true when calling for the whole image (not for a subrect)
                                  U64 hash,
                                  double time,
                                  const RenderScale & scale,
                                  const RectI & renderWindow,
                                  int view,
                                  double* inputTime,
                                  int* inputNb)
{
    assert( !( (supportsRenderScaleMaybe() == eSupportsNo) && !(scale.x == 1. && scale.y == 1.) ) );

    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);

    if (useIdentityCache) {
        double timeF = 0.;
        bool foundInCache = _imp->actionsCache.getIdentityResult(hash, time, view, mipMapLevel, inputNb, &timeF);
        if (foundInCache) {
            *inputTime = timeF;

            return *inputNb >= 0 || *inputNb == -2;
        }
    }
    ///If this is running on a render thread, attempt to find the info in the thread local storage.
    if ( ( QThread::currentThread() != qApp->thread() ) && _imp->renderArgs.hasLocalData() ) {
        const RenderArgs & args = _imp->renderArgs.localData();
        if (args._validArgs) {
            *inputNb = args._identityInputNb;
            *inputTime = args._identityTime;

            return *inputNb != -1;
        }
    }

    ///EDIT: We now allow isIdentity to be called recursively.
    RECURSIVE_ACTION();


    bool ret = false;
    boost::shared_ptr<RotoDrawableItem> rotoItem = getNode()->getAttachedRotoItem();
    if ( rotoItem && !rotoItem->isActivated(time) ) {
        ret = true;
        *inputNb = getNode()->getPreferredInput();
        *inputTime = time;
    } else if ( appPTR->isBackground() && (dynamic_cast<DiskCacheNode*>(this) != NULL) ) {
        ret = true;
        *inputNb = 0;
        *inputTime = time;
    } else if ( getNode()->isNodeDisabled() || !getNode()->hasAtLeastOneChannelToProcess() ) {
        ret = true;
        *inputTime = time;
        *inputNb = -1;
        *inputNb = getNode()->getPreferredInput();
    } else {
        /// Don't call isIdentity if plugin is sequential only.
        if (getSequentialPreference() != Natron::eSequentialPreferenceOnlySequential) {
            try {
                ret = isIdentity(time, scale, renderWindow, view, inputTime, inputNb);
            } catch (...) {
                throw;
            }
        }
    }
    if (!ret) {
        *inputNb = -1;
        *inputTime = time;
    }

    if (useIdentityCache) {
        _imp->actionsCache.setIdentityResult(hash, time, view, mipMapLevel, *inputNb, *inputTime);
    }

    return ret;
} // EffectInstance::isIdentity_public

void
EffectInstance::onInputChanged(int /*inputNo*/)
{
    if ( !getApp()->getProject()->isLoadingProject() ) {
        RenderScale s;
        s.x = s.y = 1.;
        checkOFXClipPreferences_public(getCurrentTime(), s, kOfxChangeUserEdited, true, true);
    }
}

Natron::StatusEnum
EffectInstance::getRegionOfDefinition_publicInternal(U64 hash,
                                                     double time,
                                                     const RenderScale & scale,
                                                     const RectI & renderWindow,
                                                     bool useRenderWindow,
                                                     int view,
                                                     RectD* rod,
                                                     bool* isProjectFormat)
{
    if ( !isEffectCreated() ) {
        return eStatusFailed;
    }

    unsigned int mipMapLevel = Image::getLevelFromScale(scale.x);

    if (useRenderWindow) {
        double inputTimeIdentity;
        int inputNbIdentity;
        bool isIdentity = isIdentity_public(true, hash, time, scale, renderWindow, view, &inputTimeIdentity, &inputNbIdentity);
        if (isIdentity) {
            if (inputNbIdentity >= 0) {
                Natron::EffectInstance* input = getInput(inputNbIdentity);
                if (input) {
                    return input->getRegionOfDefinition_public(input->getRenderHash(), inputTimeIdentity, scale, view, rod, isProjectFormat);
                }
            } else if (inputNbIdentity == -2) {
                return getRegionOfDefinition_public(hash, inputTimeIdentity, scale, view, rod, isProjectFormat);
            } else {
                return eStatusFailed;
            }
        }
    }

    bool foundInCache = _imp->actionsCache.getRoDResult(hash, time, view, mipMapLevel, rod);
    if (foundInCache) {
        *isProjectFormat = false;
        if ( rod->isNull() ) {
            return Natron::eStatusFailed;
        }

        return Natron::eStatusOK;
    } else {
        ///If this is running on a render thread, attempt to find the RoD in the thread local storage.
        if ( ( QThread::currentThread() != qApp->thread() ) && _imp->renderArgs.hasLocalData() ) {
            const RenderArgs & args = _imp->renderArgs.localData();
            if (args._validArgs) {
                *rod = args._rod;
                *isProjectFormat = false;

                return Natron::eStatusOK;
            }
        }

        Natron::StatusEnum ret;
        RenderScale scaleOne;
        scaleOne.x = scaleOne.y = 1.;
        {
            RECURSIVE_ACTION();


            ret = getRegionOfDefinition(hash, time, supportsRenderScaleMaybe() == eSupportsNo ? scaleOne : scale, view, rod);

            if ( (ret != eStatusOK) && (ret != eStatusReplyDefault) ) {
                // rod is not valid
                //if (!isDuringStrokeCreation) {
                _imp->actionsCache.invalidateAll(hash);
                _imp->actionsCache.setRoDResult( hash, time, view, mipMapLevel, RectD() );

                // }
                return ret;
            }

            if ( rod->isNull() ) {
                //if (!isDuringStrokeCreation) {
                _imp->actionsCache.invalidateAll(hash);
                _imp->actionsCache.setRoDResult( hash, time, view, mipMapLevel, RectD() );

                //}
                return eStatusFailed;
            }

            assert( (ret == eStatusOK || ret == eStatusReplyDefault) && (rod->x1 <= rod->x2 && rod->y1 <= rod->y2) );
        }
        *isProjectFormat = ifInfiniteApplyHeuristic(hash, time, scale, view, rod);
        assert(rod->x1 <= rod->x2 && rod->y1 <= rod->y2);

        //if (!isDuringStrokeCreation) {
        _imp->actionsCache.setRoDResult(hash, time, view,  mipMapLevel, *rod);

        //}
        return ret;
    }
} // EffectInstance::getRegionOfDefinition_publicInternal

Natron::StatusEnum
EffectInstance::getRegionOfDefinitionWithIdentityCheck_public(U64 hash,
                                                              double time,
                                                              const RenderScale & scale,
                                                              const RectI & renderWindow,
                                                              int view,
                                                              RectD* rod,
                                                              bool* isProjectFormat)
{
    return getRegionOfDefinition_publicInternal(hash, time, scale, renderWindow, true, view, rod, isProjectFormat);
}

Natron::StatusEnum
EffectInstance::getRegionOfDefinition_public(U64 hash,
                                             double time,
                                             const RenderScale & scale,
                                             int view,
                                             RectD* rod,
                                             bool* isProjectFormat)
{
    return getRegionOfDefinition_publicInternal(hash, time, scale, RectI(), false, view, rod, isProjectFormat);
}

void
EffectInstance::getRegionsOfInterest_public(double time,
                                            const RenderScale & scale,
                                            const RectD & outputRoD, //!< effect RoD in canonical coordinates
                                            const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                            int view,
                                            RoIMap* ret)
{
    NON_RECURSIVE_ACTION();
    assert(outputRoD.x2 >= outputRoD.x1 && outputRoD.y2 >= outputRoD.y1);
    assert(renderWindow.x2 >= renderWindow.x1 && renderWindow.y2 >= renderWindow.y1);

    getRegionsOfInterest(time, scale, outputRoD, renderWindow, view, ret);
}

FramesNeededMap
EffectInstance::getFramesNeeded_public(U64 hash,
                                       double time,
                                       int view,
                                       unsigned int mipMapLevel)
{
    NON_RECURSIVE_ACTION();
    FramesNeededMap framesNeeded;
    bool foundInCache = _imp->actionsCache.getFramesNeededResult(hash, time, view, mipMapLevel, &framesNeeded);
    if (foundInCache) {
        return framesNeeded;
    }

    framesNeeded = getFramesNeeded(time, view);
    _imp->actionsCache.setFramesNeededResult(hash, time, view, mipMapLevel, framesNeeded);

    return framesNeeded;
}

void
EffectInstance::getFrameRange_public(U64 hash,
                                     double *first,
                                     double *last,
                                     bool bypasscache)
{
    double fFirst = 0., fLast = 0.;
    bool foundInCache = false;

    if (!bypasscache) {
        foundInCache = _imp->actionsCache.getTimeDomainResult(hash, &fFirst, &fLast);
    }
    if (foundInCache) {
        *first = std::floor(fFirst + 0.5);
        *last = std::floor(fLast + 0.5);
    } else {
        ///If this is running on a render thread, attempt to find the info in the thread local storage.
        if ( ( QThread::currentThread() != qApp->thread() ) && _imp->renderArgs.hasLocalData() ) {
            const RenderArgs & args = _imp->renderArgs.localData();
            if (args._validArgs) {
                *first = args._firstFrame;
                *last = args._lastFrame;

                return;
            }
        }

        NON_RECURSIVE_ACTION();
        getFrameRange(first, last);
        _imp->actionsCache.setTimeDomainResult(hash, *first, *last);
    }
}

Natron::StatusEnum
EffectInstance::beginSequenceRender_public(double first,
                                           double last,
                                           double step,
                                           bool interactive,
                                           const RenderScale & scale,
                                           bool isSequentialRender,
                                           bool isRenderResponseToUserInteraction,
                                           bool draftMode,
                                           int view)
{
    NON_RECURSIVE_ACTION();
    {
        if ( !_imp->beginEndRenderCount.hasLocalData() ) {
            _imp->beginEndRenderCount.localData() = 1;
        } else {
            ++_imp->beginEndRenderCount.localData();
        }
    }

    return beginSequenceRender(first, last, step, interactive, scale,
                               isSequentialRender, isRenderResponseToUserInteraction, draftMode, view);
}

Natron::StatusEnum
EffectInstance::endSequenceRender_public(double first,
                                         double last,
                                         double step,
                                         bool interactive,
                                         const RenderScale & scale,
                                         bool isSequentialRender,
                                         bool isRenderResponseToUserInteraction,
                                         bool draftMode,
                                         int view)
{
    NON_RECURSIVE_ACTION();
    {
        assert( _imp->beginEndRenderCount.hasLocalData() );
        --_imp->beginEndRenderCount.localData();
        assert(_imp->beginEndRenderCount.localData() >= 0);
    }

    return endSequenceRender(first, last, step, interactive, scale, isSequentialRender, isRenderResponseToUserInteraction, draftMode, view);
}

bool
EffectInstance::isSupportedComponent(int inputNb,
                                     const Natron::ImageComponents & comp) const
{
    return getNode()->isSupportedComponent(inputNb, comp);
}

Natron::ImageBitDepthEnum
EffectInstance::getBitDepth() const
{
    return getNode()->getBitDepth();
}

bool
EffectInstance::isSupportedBitDepth(Natron::ImageBitDepthEnum depth) const
{
    return getNode()->isSupportedBitDepth(depth);
}

Natron::ImageComponents
EffectInstance::findClosestSupportedComponents(int inputNb,
                                               const Natron::ImageComponents & comp) const
{
    return getNode()->findClosestSupportedComponents(inputNb, comp);
}

void
EffectInstance::getPreferredDepthAndComponents(int inputNb,
                                               std::list<Natron::ImageComponents>* comp,
                                               Natron::ImageBitDepthEnum* depth) const
{
    EffectInstance* inp = 0;
    std::list<Natron::ImageComponents> inputComps;

    if (inputNb != -1) {
        inp = getInput(inputNb);
        if (inp) {
            Natron::ImageBitDepthEnum depth;
            inp->getPreferredDepthAndComponents(-1, &inputComps, &depth);
        }
    } else {
        int index = getNode()->getPreferredInput();
        if (index != -1) {
            Natron::EffectInstance* input = getInput(index);
            if (input) {
                Natron::ImageBitDepthEnum inputDepth;
                input->getPreferredDepthAndComponents(-1, &inputComps, &inputDepth);
            }
        }
    }
    if ( inputComps.empty() ) {
        inputComps.push_back( ImageComponents::getNoneComponents() );
    }
    for (std::list<Natron::ImageComponents>::iterator it = inputComps.begin(); it != inputComps.end(); ++it) {
        comp->push_back( findClosestSupportedComponents(inputNb, *it) );
    }


    ///find deepest bitdepth
    *depth = getBitDepth();
}

void
EffectInstance::clearActionsCache()
{
    _imp->actionsCache.clearAll();
}

void
EffectInstance::setComponentsAvailableDirty(bool dirty)
{
    QMutexLocker k(&_imp->componentsAvailableMutex);

    _imp->componentsAvailableDirty = dirty;
}

void
EffectInstance::getNonMaskInputsAvailableComponents(double time,
                                                    int view,
                                                    bool preferExistingComponents,
                                                    ComponentsAvailableMap* comps,
                                                    std::list<Natron::EffectInstance*>* markedNodes)
{
    NodePtr node  = getNode();

    if (!node) {
        return;
    }
    int preferredInput = node->getPreferredInput();

    //Call recursively on all inputs which are not masks
    int maxInputs = getMaxInputCount();
    for (int i = 0; i < maxInputs; ++i) {
        if ( !isInputMask(i) && !isInputRotoBrush(i) ) {
            EffectInstance* input = getInput(i);
            if (input) {
                ComponentsAvailableMap inputAvailComps;
                input->getComponentsAvailableRecursive(time, view, &inputAvailComps, markedNodes);
                for (ComponentsAvailableMap::iterator it = inputAvailComps.begin(); it != inputAvailComps.end(); ++it) {
                    //If the component is already present in the 'comps' map, only add it if we are the preferred input
                    ComponentsAvailableMap::iterator colorMatch = comps->end();
                    bool found = false;
                    for (ComponentsAvailableMap::iterator it2 = comps->begin(); it2 != comps->end(); ++it2) {
                        if (it2->first == it->first) {
                            if ( (i == preferredInput) && !preferExistingComponents ) {
                                it2->second = node;
                            }
                            found = true;
                            break;
                        } else if ( it2->first.isColorPlane() ) {
                            colorMatch = it2;
                        }
                    }
                    if (!found) {
                        if ( ( colorMatch != comps->end() ) && it->first.isColorPlane() ) {
                            //we found another color components type, skip
                            continue;
                        } else {
                            comps->insert(*it);
                        }
                    }
                }
            }
        }
    }
}

void
EffectInstance::getComponentsAvailableRecursive(double time,
                                                int view,
                                                ComponentsAvailableMap* comps,
                                                std::list<Natron::EffectInstance*>* markedNodes)
{
    if ( std::find(markedNodes->begin(), markedNodes->end(), this) != markedNodes->end() ) {
        return;
    }

    {
        QMutexLocker k(&_imp->componentsAvailableMutex);
        if (!_imp->componentsAvailableDirty) {
            comps->insert( _imp->outputComponentsAvailable.begin(), _imp->outputComponentsAvailable.end() );

            return;
        }
    }


    NodePtr node  = getNode();
    if (!node) {
        return;
    }
    ComponentsNeededMap neededComps;
    SequenceTime ptTime;
    int ptView;
    NodePtr ptInput;
    bool processAll;
    bool processChannels[4];
    getComponentsNeededAndProduced_public(time, view, &neededComps, &processAll, &ptTime, &ptView, processChannels, &ptInput);


    ///If the plug-in is not pass-through, only consider the components processed by the plug-in in output,
    ///so we do not need to recurse.
    PassThroughEnum passThrough = isPassThroughForNonRenderedPlanes();
    if ( (passThrough == ePassThroughPassThroughNonRenderedPlanes) || (passThrough == ePassThroughRenderAllRequestedPlanes) ) {
        bool doHeuristicForPassThrough = false;
        if ( isMultiPlanar() ) {
            if (!ptInput) {
                doHeuristicForPassThrough = true;
            }
        } else {
            doHeuristicForPassThrough = true;
        }

        if (doHeuristicForPassThrough) {
            getNonMaskInputsAvailableComponents(time, view, false, comps, markedNodes);
        } else {
            if (ptInput) {
                ptInput->getLiveInstance()->getComponentsAvailableRecursive(time, view, comps, markedNodes);
            }
        }
    }
    if (processAll) {
        //The node makes available everything available upstream
        for (ComponentsAvailableMap::iterator it = comps->begin(); it != comps->end(); ++it) {
            if ( it->second.lock() ) {
                it->second = node;
            }
        }
    }


    ComponentsNeededMap::iterator foundOutput = neededComps.find(-1);
    if ( foundOutput != neededComps.end() ) {
        ///Foreach component produced by the node at the given (view,time),  try
        ///to add it to the components available. Since we already handled upstream nodes, it is probably
        ///already in there, in which case we mark that this node is producing the component instead
        for (std::vector<Natron::ImageComponents>::iterator it = foundOutput->second.begin();
             it != foundOutput->second.end(); ++it) {
            ComponentsAvailableMap::iterator alreadyExisting = comps->end();

            if ( it->isColorPlane() ) {
                ComponentsAvailableMap::iterator colorMatch = comps->end();

                for (ComponentsAvailableMap::iterator it2 = comps->begin(); it2 != comps->end(); ++it2) {
                    if (it2->first == *it) {
                        alreadyExisting = it2;
                        break;
                    } else if ( it2->first.isColorPlane() ) {
                        colorMatch = it2;
                    }
                }

                if ( ( alreadyExisting == comps->end() ) && ( colorMatch != comps->end() ) ) {
                    comps->erase(colorMatch);
                }
            } else {
                for (ComponentsAvailableMap::iterator it2 = comps->begin(); it2 != comps->end(); ++it2) {
                    if (it2->first == *it) {
                        alreadyExisting = it2;
                        break;
                    }
                }
            }


            if ( alreadyExisting == comps->end() ) {
                comps->insert( std::make_pair(*it, node) );
            } else {
                //If the component already exists from upstream in the tree, mark that we produce it instead
                alreadyExisting->second = node;
            }
        }


        std::list<ImageComponents> userComps;
        node->getUserComponents(&userComps);
        ///Foreach user component, add it as an available component, but use this node only if it is also
        ///in the "needed components" list
        for (std::list<ImageComponents>::iterator it = userComps.begin(); it != userComps.end(); ++it) {
            bool found = false;
            for (std::vector<Natron::ImageComponents>::iterator it2 = foundOutput->second.begin();
                 it2 != foundOutput->second.end(); ++it2) {
                if (*it2 == *it) {
                    found = true;
                    break;
                }
            }


            ComponentsAvailableMap::iterator alreadyExisting = comps->end();

            if ( it->isColorPlane() ) {
                ComponentsAvailableMap::iterator colorMatch = comps->end();

                for (ComponentsAvailableMap::iterator it2 = comps->begin(); it2 != comps->end(); ++it2) {
                    if (it2->first == *it) {
                        alreadyExisting = it2;
                        break;
                    } else if ( it2->first.isColorPlane() ) {
                        colorMatch = it2;
                    }
                }

                if ( ( alreadyExisting == comps->end() ) && ( colorMatch != comps->end() ) ) {
                    comps->erase(colorMatch);
                }
            } else {
                alreadyExisting = comps->find(*it);
            }

            //If the component already exists from above in the tree, do not add it
            if ( alreadyExisting == comps->end() ) {
                comps->insert( std::make_pair( *it, found ? node : NodePtr() ) );
            } else {
                alreadyExisting->second = node;
            }
        }
    }
    markedNodes->push_back(this);


    {
        QMutexLocker k(&_imp->componentsAvailableMutex);
        _imp->componentsAvailableDirty = false;
        _imp->outputComponentsAvailable = *comps;
    }
} // EffectInstance::getComponentsAvailableRecursive

void
EffectInstance::getComponentsAvailable(double time,
                                       ComponentsAvailableMap* comps,
                                       std::list<Natron::EffectInstance*>* markedNodes)
{
    getComponentsAvailableRecursive(time, 0, comps, markedNodes);
}

void
EffectInstance::getComponentsAvailable(double time,
                                       ComponentsAvailableMap* comps)
{
    //int nViews = getApp()->getProject()->getProjectViewsCount();

    ///Union components over all views
    //for (int view = 0; view < nViews; ++view) {
    ///Edit: Just call for 1 view, it should not matter as this should be view agnostic.
    std::list<Natron::EffectInstance*> marks;

    getComponentsAvailableRecursive(time, 0, comps, &marks);

    //}
}

void
EffectInstance::getComponentsNeededAndProduced(double time,
                                               int view,
                                               ComponentsNeededMap* comps,
                                               SequenceTime* passThroughTime,
                                               int* passThroughView,
                                               boost::shared_ptr<Natron::Node>* passThroughInput)
{
    *passThroughTime = time;
    *passThroughView = view;

    std::list<Natron::ImageComponents> outputComp;
    Natron::ImageBitDepthEnum outputDepth;
    getPreferredDepthAndComponents(-1, &outputComp, &outputDepth);

    std::vector<Natron::ImageComponents> outputCompVec;
    for (std::list<Natron::ImageComponents>::iterator it = outputComp.begin(); it != outputComp.end(); ++it) {
        outputCompVec.push_back(*it);
    }


    comps->insert( std::make_pair(-1, outputCompVec) );

    NodePtr firstConnectedOptional;
    for (int i = 0; i < getMaxInputCount(); ++i) {
        NodePtr node = getNode()->getInput(i);
        if (!node) {
            continue;
        }
        if ( isInputRotoBrush(i) ) {
            continue;
        }

        std::list<Natron::ImageComponents> comp;
        Natron::ImageBitDepthEnum depth;
        getPreferredDepthAndComponents(-1, &comp, &depth);

        std::vector<Natron::ImageComponents> compVect;
        for (std::list<Natron::ImageComponents>::iterator it = comp.begin(); it != comp.end(); ++it) {
            compVect.push_back(*it);
        }
        comps->insert( std::make_pair(i, compVect) );

        if ( !isInputOptional(i) ) {
            *passThroughInput = node;
        } else {
            firstConnectedOptional = node;
        }
    }
    if (!*passThroughInput) {
        *passThroughInput = firstConnectedOptional;
    }
}

void
EffectInstance::getComponentsNeededAndProduced_public(double time,
                                                      int view,
                                                      ComponentsNeededMap* comps,
                                                      bool* processAllRequested,
                                                      SequenceTime* passThroughTime,
                                                      int* passThroughView,
                                                      bool* processChannels,
                                                      boost::shared_ptr<Natron::Node>* passThroughInput)

{
    RECURSIVE_ACTION();

    if ( isMultiPlanar() ) {
        processChannels[0] = processChannels[1] = processChannels[2] = processChannels[3] = true;
        getComponentsNeededAndProduced(time, view, comps, passThroughTime, passThroughView, passThroughInput);
        *processAllRequested = false;
    } else {
        *passThroughTime = time;
        *passThroughView = view;
        int idx = getNode()->getPreferredInput();
        *passThroughInput = getNode()->getInput(idx);

        {
            ImageComponents layer;
            std::vector<ImageComponents> compVec;
            bool ok = getNode()->getUserComponents(-1, processChannels, processAllRequested, &layer);
            if (ok) {
                if ( !layer.isColorPlane() && (layer.getNumComponents() != 0) ) {
                    compVec.push_back(layer);
                } else {
                    //Use regular clip preferences
                    ImageBitDepthEnum depth;
                    std::list<ImageComponents> components;
                    getPreferredDepthAndComponents(-1, &components, &depth);
                    for (std::list<ImageComponents>::iterator it = components.begin(); it != components.end(); ++it) {
                        //if (it->isColorPlane()) {
                        compVec.push_back(*it);
                        //}
                    }
                }
            } else {
                //Use regular clip preferences
                ImageBitDepthEnum depth;
                std::list<ImageComponents> components;
                getPreferredDepthAndComponents(-1, &components, &depth);
                for (std::list<ImageComponents>::iterator it = components.begin(); it != components.end(); ++it) {
                    //if (it->isColorPlane()) {
                    compVec.push_back(*it);
                    //}
                }
            }
            comps->insert( std::make_pair(-1, compVec) );
        }

        int maxInput = getMaxInputCount();
        for (int i = 0; i < maxInput; ++i) {
            EffectInstance* input = getInput(i);
            if (input) {
                std::vector<ImageComponents> compVec;
                bool inputProcChannels[4];
                ImageComponents layer;
                bool isAll;
                bool ok = getNode()->getUserComponents(i, inputProcChannels, &isAll, &layer);
                ImageComponents maskComp;
                NodePtr maskInput;
                int channelMask = getNode()->getMaskChannel(i, &maskComp, &maskInput);

                if (ok && !isAll) {
                    if ( !layer.isColorPlane() ) {
                        compVec.push_back(layer);
                    } else {
                        //Use regular clip preferences
                        ImageBitDepthEnum depth;
                        std::list<ImageComponents> components;
                        getPreferredDepthAndComponents(i, &components, &depth);
                        for (std::list<ImageComponents>::iterator it = components.begin(); it != components.end(); ++it) {
                            //if (it->isColorPlane()) {
                            compVec.push_back(*it);
                            //}
                        }
                    }
                } else if ( (channelMask != -1) && (maskComp.getNumComponents() > 0) ) {
                    std::vector<ImageComponents> compVec;
                    compVec.push_back(maskComp);
                    comps->insert( std::make_pair(i, compVec) );
                } else {
                    //Use regular clip preferences
                    ImageBitDepthEnum depth;
                    std::list<ImageComponents> components;
                    getPreferredDepthAndComponents(i, &components, &depth);
                    for (std::list<ImageComponents>::iterator it = components.begin(); it != components.end(); ++it) {
                        //if (it->isColorPlane()) {
                        compVec.push_back(*it);
                        //}
                    }
                }
                comps->insert( std::make_pair(i, compVec) );
            }
        }
    }
} // EffectInstance::getComponentsNeededAndProduced_public

int
EffectInstance::getMaskChannel(int inputNb,
                               Natron::ImageComponents* comps,
                               boost::shared_ptr<Natron::Node>* maskInput) const
{
    return getNode()->getMaskChannel(inputNb, comps, maskInput);
}

bool
EffectInstance::isMaskEnabled(int inputNb) const
{
    return getNode()->isMaskEnabled(inputNb);
}

void
EffectInstance::onKnobValueChanged(KnobI* /*k*/,
                                   Natron::ValueChangedReasonEnum /*reason*/,
                                   SequenceTime /*time*/,
                                   bool /*originatedFromMainThread*/)
{
}

int
EffectInstance::getThreadLocalRenderTime() const
{
    if ( _imp->renderArgs.hasLocalData() ) {
        const RenderArgs & args = _imp->renderArgs.localData();
        if (args._validArgs) {
            return args._time;
        }
    }

    if ( _imp->frameRenderArgs.hasLocalData() ) {
        const ParallelRenderArgs & args = _imp->frameRenderArgs.localData();
        if (args.validArgs) {
            return args.time;
        }
    }

    return getApp()->getTimeLine()->currentFrame();
}

bool
EffectInstance::getThreadLocalRenderedPlanes(std::map<Natron::ImageComponents, PlaneToRender> *outputPlanes,
                                             Natron::ImageComponents* planeBeingRendered,
                                             RectI* renderWindow) const
{
    if ( _imp->renderArgs.hasLocalData() ) {
        const RenderArgs & args = _imp->renderArgs.localData();
        if (args._validArgs) {
            assert( !args._outputPlanes.empty() );
            *planeBeingRendered = args._outputPlaneBeingRendered;
            *outputPlanes = args._outputPlanes;
            *renderWindow = args._renderWindowPixel;

            return true;
        }
    }

    return false;
}

void
EffectInstance::updateThreadLocalRenderTime(double time)
{
    if ( ( QThread::currentThread() != qApp->thread() ) && _imp->renderArgs.hasLocalData() ) {
        RenderArgs & args = _imp->renderArgs.localData();
        if (args._validArgs) {
            args._time = time;
        }
    }
}

bool
EffectInstance::isDuringPaintStrokeCreationThreadLocal() const
{
    if ( _imp->frameRenderArgs.hasLocalData() ) {
        const ParallelRenderArgs & args = _imp->frameRenderArgs.localData();
        if (args.validArgs) {
            return args.isDuringPaintStrokeCreation;
        }
    }

    return getNode()->isDuringPaintStrokeCreation();
}

Natron::RenderSafetyEnum
EffectInstance::getCurrentThreadSafetyThreadLocal() const
{
    if ( _imp->frameRenderArgs.hasLocalData() ) {
        const ParallelRenderArgs & args = _imp->frameRenderArgs.localData();
        if (args.validArgs) {
            return args.currentThreadSafety;
        }
    }

    return getNode()->getCurrentRenderThreadSafety();
}

void
EffectInstance::onKnobValueChanged_public(KnobI* k,
                                          Natron::ValueChangedReasonEnum reason,
                                          SequenceTime time,
                                          bool originatedFromMainThread)
{
    NodePtr node = getNode();

//    if (!node->isNodeCreated()) {
//        return;
//    }

    if ( isReader() && (k->getName() == kOfxImageEffectFileParamName) ) {
        node->computeFrameRangeForReader(k);
    }


    KnobHelper* kh = dynamic_cast<KnobHelper*>(k);
    assert(kh);
    if ( kh && kh->isDeclaredByPlugin() ) {
        ////We set the thread storage render args so that if the instance changed action
        ////tries to call getImage it can render with good parameters.


        ParallelRenderArgsSetter frameRenderArgs( getApp()->getProject().get(),
                                                  time,
                                                  0, /*view*/
                                                  true,
                                                  false,
                                                  false,
                                                  0,
                                                  node,
                                                  0, // request
                                                  0, //texture index
                                                  getApp()->getTimeLine().get(),
                                                  NodePtr(),
                                                  true,
                                                  false,
                                                  false,
                                                  boost::shared_ptr<RenderStats>() );

        RECURSIVE_ACTION();
        EffectPointerThreadProperty_RAII propHolder_raii(this);
        knobChanged(k, reason, /*view*/ 0, time, originatedFromMainThread);
    }

    node->onEffectKnobValueChanged(k, reason);

    ///If there's a knobChanged Python callback, run it
    std::string pythonCB = getNode()->getKnobChangedCallback();

    if ( !pythonCB.empty() ) {
        bool userEdited = reason == eValueChangedReasonNatronGuiEdited ||
                          reason == eValueChangedReasonUserEdited;
        _imp->runChangedParamCallback(k, userEdited, pythonCB);
    }

    ///Refresh the dynamic properties that can be changed during the instanceChanged action
    node->refreshDynamicProperties();

    ///Clear input images pointers that were stored in getImage() for the main-thread.
    ///This is safe to do so because if this is called while in render() it won't clear the input images
    ///pointers for the render thread. This is helpful for analysis effects which call getImage() on the main-thread
    ///and whose render() function is never called.
    _imp->clearInputImagePointers();
} // onKnobValueChanged_public

void
EffectInstance::clearLastRenderedImage()
{
    
}

void
EffectInstance::aboutToRestoreDefaultValues()
{
    ///Invalidate the cache by incrementing the age
    NodePtr node = getNode();

    node->incrementKnobsAge();

    if ( node->areKeyframesVisibleOnTimeline() ) {
        node->hideKeyframesFromTimeline(true);
    }
}

/**
 * @brief Returns a pointer to the first non disabled upstream node.
 * When cycling through the tree, we prefer non optional inputs and we span inputs
 * from last to first.
 **/
Natron::EffectInstance*
EffectInstance::getNearestNonDisabled() const
{
    NodePtr node = getNode();

    if ( !node->isNodeDisabled() ) {
        return node->getLiveInstance();
    } else {
        ///Test all inputs recursively, going from last to first, preferring non optional inputs.
        std::list<Natron::EffectInstance*> nonOptionalInputs;
        std::list<Natron::EffectInstance*> optionalInputs;
        bool useInputA = appPTR->getCurrentSettings()->isMergeAutoConnectingToAInput();

        ///Find an input named A
        std::string inputNameToFind, otherName;
        if (useInputA) {
            inputNameToFind = "A";
            otherName = "B";
        } else {
            inputNameToFind = "B";
            otherName = "A";
        }
        int foundOther = -1;
        int maxinputs = getMaxInputCount();
        for (int i = 0; i < maxinputs; ++i) {
            std::string inputLabel = getInputLabel(i);
            if (inputLabel == inputNameToFind) {
                EffectInstance* inp = getInput(i);
                if (inp) {
                    nonOptionalInputs.push_front(inp);
                    break;
                }
            } else if (inputLabel == otherName) {
                foundOther = i;
            }
        }

        if ( (foundOther != -1) && nonOptionalInputs.empty() ) {
            EffectInstance* inp = getInput(foundOther);
            if (inp) {
                nonOptionalInputs.push_front(inp);
            }
        }

        ///If we found A or B so far, cycle through them
        for (std::list<Natron::EffectInstance*> ::iterator it = nonOptionalInputs.begin(); it != nonOptionalInputs.end(); ++it) {
            Natron::EffectInstance* inputRet = (*it)->getNearestNonDisabled();
            if (inputRet) {
                return inputRet;
            }
        }


        ///We cycle in reverse by default. It should be a setting of the application.
        ///In this case it will return input B instead of input A of a merge for example.
        for (int i = 0; i < maxinputs; ++i) {
            Natron::EffectInstance* inp = getInput(i);
            bool optional = isInputOptional(i);
            if (inp) {
                if (optional) {
                    optionalInputs.push_back(inp);
                } else {
                    nonOptionalInputs.push_back(inp);
                }
            }
        }

        ///Cycle through all non optional inputs first
        for (std::list<Natron::EffectInstance*> ::iterator it = nonOptionalInputs.begin(); it != nonOptionalInputs.end(); ++it) {
            Natron::EffectInstance* inputRet = (*it)->getNearestNonDisabled();
            if (inputRet) {
                return inputRet;
            }
        }

        ///Cycle through optional inputs...
        for (std::list<Natron::EffectInstance*> ::iterator it = optionalInputs.begin(); it != optionalInputs.end(); ++it) {
            Natron::EffectInstance* inputRet = (*it)->getNearestNonDisabled();
            if (inputRet) {
                return inputRet;
            }
        }

        ///We didn't find anything upstream, return
        return NULL;
    }
} // EffectInstance::getNearestNonDisabled

Natron::EffectInstance*
EffectInstance::getNearestNonDisabledPrevious(int* inputNb)
{
    assert( getNode()->isNodeDisabled() );

    ///Test all inputs recursively, going from last to first, preferring non optional inputs.
    std::list<Natron::EffectInstance*> nonOptionalInputs;
    std::list<Natron::EffectInstance*> optionalInputs;
    int localPreferredInput = -1;
    bool useInputA = appPTR->getCurrentSettings()->isMergeAutoConnectingToAInput();
    ///Find an input named A
    std::string inputNameToFind, otherName;
    if (useInputA) {
        inputNameToFind = "A";
        otherName = "B";
    } else {
        inputNameToFind = "B";
        otherName = "A";
    }
    int foundOther = -1;
    int maxinputs = getMaxInputCount();
    for (int i = 0; i < maxinputs; ++i) {
        std::string inputLabel = getInputLabel(i);
        if (inputLabel == inputNameToFind) {
            EffectInstance* inp = getInput(i);
            if (inp) {
                nonOptionalInputs.push_front(inp);
                localPreferredInput = i;
                break;
            }
        } else if (inputLabel == otherName) {
            foundOther = i;
        }
    }

    if ( (foundOther != -1) && nonOptionalInputs.empty() ) {
        EffectInstance* inp = getInput(foundOther);
        if (inp) {
            nonOptionalInputs.push_front(inp);
            localPreferredInput = foundOther;
        }
    }

    ///If we found A or B so far, cycle through them
    for (std::list<Natron::EffectInstance*> ::iterator it = nonOptionalInputs.begin(); it != nonOptionalInputs.end(); ++it) {
        if ( (*it)->getNode()->isNodeDisabled() ) {
            Natron::EffectInstance* inputRet = (*it)->getNearestNonDisabledPrevious(inputNb);
            if (inputRet) {
                return inputRet;
            }
        }
    }


    ///We cycle in reverse by default. It should be a setting of the application.
    ///In this case it will return input B instead of input A of a merge for example.
    for (int i = 0; i < maxinputs; ++i) {
        Natron::EffectInstance* inp = getInput(i);
        bool optional = isInputOptional(i);
        if (inp) {
            if (optional) {
                if (localPreferredInput == -1) {
                    localPreferredInput = i;
                }
                optionalInputs.push_back(inp);
            } else {
                if (localPreferredInput == -1) {
                    localPreferredInput = i;
                }
                nonOptionalInputs.push_back(inp);
            }
        }
    }


    ///Cycle through all non optional inputs first
    for (std::list<Natron::EffectInstance*> ::iterator it = nonOptionalInputs.begin(); it != nonOptionalInputs.end(); ++it) {
        if ( (*it)->getNode()->isNodeDisabled() ) {
            Natron::EffectInstance* inputRet = (*it)->getNearestNonDisabledPrevious(inputNb);
            if (inputRet) {
                return inputRet;
            }
        }
    }

    ///Cycle through optional inputs...
    for (std::list<Natron::EffectInstance*> ::iterator it = optionalInputs.begin(); it != optionalInputs.end(); ++it) {
        if ( (*it)->getNode()->isNodeDisabled() ) {
            Natron::EffectInstance* inputRet = (*it)->getNearestNonDisabledPrevious(inputNb);
            if (inputRet) {
                return inputRet;
            }
        }
    }

    *inputNb = localPreferredInput;

    return this;
} // EffectInstance::getNearestNonDisabledPrevious

Natron::EffectInstance*
EffectInstance::getNearestNonIdentity(double time)
{
    U64 hash = getRenderHash();
    RenderScale scale;

    scale.x = scale.y = 1.;

    RectD rod;
    bool isProjectFormat;
    Natron::StatusEnum stat = getRegionOfDefinition_public(hash, time, scale, 0, &rod, &isProjectFormat);

    ///Ignore the result of getRoD if it failed
    Q_UNUSED(stat);

    double inputTimeIdentity;
    int inputNbIdentity;
    RectI pixelRoi;
    rod.toPixelEnclosing(scale, getPreferredAspectRatio(), &pixelRoi);
    if ( !isIdentity_public(true, hash, time, scale, pixelRoi, 0, &inputTimeIdentity, &inputNbIdentity) ) {
        return this;
    } else {
        if (inputNbIdentity < 0) {
            return this;
        }
        Natron::EffectInstance* effect = getInput(inputNbIdentity);

        return effect ? effect->getNearestNonIdentity(time) : this;
    }
}

void
EffectInstance::restoreClipPreferences()
{
    setSupportsRenderScaleMaybe(eSupportsYes);
}

void
EffectInstance::onNodeHashChanged(U64 hash)
{
    ///Invalidate actions cache
    _imp->actionsCache.invalidateAll(hash);

    const std::vector<boost::shared_ptr<KnobI> > & knobs = getKnobs();
    for (std::vector<boost::shared_ptr<KnobI> >::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        for (int i = 0; i < (*it)->getDimension(); ++i) {
            (*it)->clearExpressionsResults(i);
        }
    }
}

bool
EffectInstance::canSetValue() const
{
    return !getNode()->isNodeRendering() || appPTR->isBackground();
}

void
EffectInstance::abortAnyEvaluation()
{
    ///Just change the hash
    NodePtr node = getNode();

    
    assert(node);
    node->incrementKnobsAge();
    std::list<Natron::OutputEffectInstance*> outputNodes;
    node->hasOutputNodesConnected(&outputNodes);
    for (std::list<Natron::OutputEffectInstance*>::const_iterator it = outputNodes.begin(); it != outputNodes.end(); ++it) {
        ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>(*it);
        if (isViewer) {
            isViewer->markAllOnRendersAsAborted();
        }
        (*it)->getRenderEngine()->abortRendering(true,false);
    }
}

double
EffectInstance::getCurrentTime() const
{
    return getThreadLocalRenderTime();
}

int
EffectInstance::getCurrentView() const
{
    if ( _imp->renderArgs.hasLocalData() ) {
        const RenderArgs & args = _imp->renderArgs.localData();
        if (args._validArgs) {
            return args._view;
        }
    }

    return 0;
}

SequenceTime
EffectInstance::getFrameRenderArgsCurrentTime() const
{
    if ( _imp->frameRenderArgs.hasLocalData() ) {
        const ParallelRenderArgs & args = _imp->frameRenderArgs.localData();
        if (args.validArgs) {
            return args.time;
        }
    }

    return getApp()->getTimeLine()->currentFrame();
}

int
EffectInstance::getFrameRenderArgsCurrentView() const
{
    if ( _imp->frameRenderArgs.hasLocalData() ) {
        const ParallelRenderArgs & args = _imp->frameRenderArgs.localData();
        if (args.validArgs) {
            return args.view;
        }
    }

    return 0;
}

#ifdef DEBUG
void
EffectInstance::checkCanSetValueAndWarn() const
{
    if ( !checkCanSetValue() ) {
        qDebug() << getScriptName_mt_safe().c_str() << ": setValue()/setValueAtTime() was called during an action that is not allowed to call this function.";
    }
}

#endif

static
void
isFrameVaryingOrAnimated_impl(const Natron::EffectInstance* node,
                              bool *ret)
{
    if ( node->isFrameVarying() || node->getHasAnimation() || node->getNode()->getRotoContext() ) {
        *ret = true;
    } else {
        int maxInputs = node->getMaxInputCount();
        for (int i = 0; i < maxInputs; ++i) {
            Natron::EffectInstance* input = node->getInput(i);
            if (input) {
                isFrameVaryingOrAnimated_impl(input, ret);
                if (*ret) {
                    return;
                }
            }
        }
    }
}

bool
EffectInstance::isFrameVaryingOrAnimated_Recursive() const
{
    bool ret = false;

    isFrameVaryingOrAnimated_impl(this, &ret);

    return ret;
}

bool
EffectInstance::isPaintingOverItselfEnabled() const
{
    return isDuringPaintStrokeCreationThreadLocal();
}

double
EffectInstance::getPreferredFrameRate() const
{
    return getApp()->getProjectFrameRate();
}

void
EffectInstance::refreshChannelSelectors_recursiveInternal(Natron::Node* node,
                                                          std::list<Natron::Node*> & markedNodes)
{
    std::list<Natron::Node*>::iterator found = std::find(markedNodes.begin(), markedNodes.end(), node);

    if ( found != markedNodes.end() ) {
        return;
    }
    node->refreshChannelSelectors(false);

    markedNodes.push_back(node);
    std::list<Natron::Node*>  outputs;
    node->getOutputsWithGroupRedirection(outputs);
    for (std::list<Natron::Node*>::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        refreshChannelSelectors_recursiveInternal( (*it), markedNodes );
    }
}

void
EffectInstance::refreshChannelSelectors_recursive()
{
    std::list<Natron::Node*> markedNodes;

    refreshChannelSelectors_recursiveInternal(getNode().get(), markedNodes);
}

void
EffectInstance::checkOFXClipPreferences_recursive(double time,
                                                  const RenderScale & scale,
                                                  const std::string & reason,
                                                  bool forceGetClipPrefAction,
                                                  std::list<Natron::Node*> & markedNodes)
{
    NodePtr node = getNode();
    std::list<Natron::Node*>::iterator found = std::find( markedNodes.begin(), markedNodes.end(), node.get() );

    if ( found != markedNodes.end() ) {
        return;
    }


    checkOFXClipPreferences(time, scale, reason, forceGetClipPrefAction);

    if ( !node->duringInputChangedAction() ) {
        ///The channels selector refreshing is already taken care of in the inputChanged action
        node->refreshChannelSelectors(false);
    }

    markedNodes.push_back( node.get() );

    std::list<Natron::Node*>  outputs;
    node->getOutputsWithGroupRedirection(outputs);
    for (std::list<Natron::Node*>::const_iterator it = outputs.begin(); it != outputs.end(); ++it) {
        (*it)->getLiveInstance()->checkOFXClipPreferences_recursive(time, scale, reason, forceGetClipPrefAction, markedNodes);
    }
}

void
EffectInstance::checkOFXClipPreferences_public(double time,
                                               const RenderScale & scale,
                                               const std::string & reason,
                                               bool forceGetClipPrefAction,
                                               bool recurse)
{
    assert( QThread::currentThread() == qApp->thread() );

    if (recurse) {
        std::list<Natron::Node*> markedNodes;
        checkOFXClipPreferences_recursive(time, scale, reason, forceGetClipPrefAction, markedNodes);
    } else {
        checkOFXClipPreferences(time, scale, reason, forceGetClipPrefAction);
    }
}
