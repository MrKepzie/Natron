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

#include "OutputEffectInstance.h"

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


OutputEffectInstance::OutputEffectInstance(boost::shared_ptr<Node> node)
    : Natron::EffectInstance(node)
    , _writerCurrentFrame(0)
    , _writerFirstFrame(0)
    , _writerLastFrame(0)
    , _outputEffectDataLock(new QMutex)
    , _renderController(0)
    , _engine(0)
    , _timeSpentPerFrameRendered()
{
}

OutputEffectInstance::~OutputEffectInstance()
{
    if (_engine) {
        ///Thread must have been killed before.
        assert( !_engine->hasThreadsAlive() );
    }
    delete _engine;
    delete _outputEffectDataLock;
}

void
OutputEffectInstance::renderCurrentFrame(bool canAbort)
{
    _engine->renderCurrentFrame(getApp()->isRenderStatsActionChecked(), canAbort);
}

void
OutputEffectInstance::renderCurrentFrameWithRenderStats(bool canAbort)
{
    _engine->renderCurrentFrame(true, canAbort);
}

void
OutputEffectInstance::renderFromCurrentFrameUsingCurrentDirection(bool enableRenderStats)
{
    _engine->renderFromCurrentFrameUsingCurrentDirection(enableRenderStats);
}

bool
OutputEffectInstance::ifInfiniteclipRectToProjectDefault(RectD* rod) const
{
    if ( !getApp()->getProject() ) {
        return false;
    }
    /*If the rod is infinite clip it to the project's default*/
    Format projectDefault;
    getRenderFormat(&projectDefault);
    // BE CAREFUL:
    // std::numeric_limits<int>::infinity() does not exist (check std::numeric_limits<int>::has_infinity)
    // an int can not be equal to (or compared to) std::numeric_limits<double>::infinity()
    bool isRodProjctFormat = false;
    if (rod->left() <= kOfxFlagInfiniteMin) {
        rod->set_left( projectDefault.left() );
        isRodProjctFormat = true;
    }
    if (rod->bottom() <= kOfxFlagInfiniteMin) {
        rod->set_bottom( projectDefault.bottom() );
        isRodProjctFormat = true;
    }
    if (rod->right() >= kOfxFlagInfiniteMax) {
        rod->set_right( projectDefault.right() );
        isRodProjctFormat = true;
    }
    if (rod->top() >= kOfxFlagInfiniteMax) {
        rod->set_top( projectDefault.top() );
        isRodProjctFormat = true;
    }

    return isRodProjctFormat;
}

void
OutputEffectInstance::renderFullSequence(bool enableRenderStats,
                                         BlockingBackgroundRender* renderController,
                                         int first,
                                         int last)
{
    _renderController = renderController;

    ///Make sure that the file path exists
    boost::shared_ptr<KnobI> fileParam = getKnobByName(kOfxImageEffectFileParamName);
    if (fileParam) {
        Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>( fileParam.get() );
        if (isString) {
            std::string pattern = isString->getValue();
            std::string path = SequenceParsing::removePath(pattern);
            std::map<std::string, std::string> env;
            getApp()->getProject()->getEnvironmentVariables(env);
            Project::expandVariable(env, path);
            QDir().mkpath( path.c_str() );
        }
    }
    ///If you want writers to render backward (from last to first), just change the flag in parameter here
    _engine->renderFrameRange(enableRenderStats, first, last, OutputSchedulerThread::eRenderDirectionForward);
}

void
OutputEffectInstance::notifyRenderFinished()
{
    if (_renderController) {
        _renderController->notifyFinished();
        _renderController = 0;
    }
}

int
OutputEffectInstance::getCurrentFrame() const
{
    QMutexLocker l(_outputEffectDataLock);

    return _writerCurrentFrame;
}

void
OutputEffectInstance::setCurrentFrame(int f)
{
    QMutexLocker l(_outputEffectDataLock);

    _writerCurrentFrame = f;
}

void
OutputEffectInstance::incrementCurrentFrame()
{
    QMutexLocker l(_outputEffectDataLock);

    ++_writerCurrentFrame;
}

void
OutputEffectInstance::decrementCurrentFrame()
{
    QMutexLocker l(_outputEffectDataLock);

    --_writerCurrentFrame;
}

int
OutputEffectInstance::getFirstFrame() const
{
    QMutexLocker l(_outputEffectDataLock);

    return _writerFirstFrame;
}

bool
OutputEffectInstance::isSequentialRenderBeingAborted() const
{
    return _engine ? _engine->isSequentialRenderBeingAborted() : false;
}

bool
OutputEffectInstance::isDoingSequentialRender() const
{
    return _engine ? _engine->isDoingSequentialRender() : false;
}

void
OutputEffectInstance::setFirstFrame(int f)
{
    QMutexLocker l(_outputEffectDataLock);

    _writerFirstFrame = f;
}

int
OutputEffectInstance::getLastFrame() const
{
    QMutexLocker l(_outputEffectDataLock);

    return _writerLastFrame;
}

void
OutputEffectInstance::setLastFrame(int f)
{
    QMutexLocker l(_outputEffectDataLock);

    _writerLastFrame = f;
}

void
OutputEffectInstance::initializeData()
{
    _engine = createRenderEngine();
}

RenderEngine*
OutputEffectInstance::createRenderEngine()
{
    return new RenderEngine(this);
}

void
OutputEffectInstance::updateRenderTimeInfos(double lastTimeSpent,
                                            double *averageTimePerFrame,
                                            double *totalTimeSpent)
{
    assert(totalTimeSpent && averageTimePerFrame);

    *totalTimeSpent = 0;

    QMutexLocker k(_outputEffectDataLock);
    _timeSpentPerFrameRendered.push_back(lastTimeSpent);

    for (std::list<double>::iterator it = _timeSpentPerFrameRendered.begin(); it != _timeSpentPerFrameRendered.end(); ++it) {
        *totalTimeSpent += *it;
    }
    size_t c = _timeSpentPerFrameRendered.size();
    *averageTimePerFrame = (c == 0 ? 0 : *totalTimeSpent / c);
}

void
OutputEffectInstance::resetTimeSpentRenderingInfos()
{
    QMutexLocker k(_outputEffectDataLock);

    _timeSpentPerFrameRendered.clear();
}

void
OutputEffectInstance::reportStats(int time,
                                  int view,
                                  double wallTime,
                                  const std::map<boost::shared_ptr<Natron::Node>, NodeRenderStats > & stats)
{
    std::string filename;
    boost::shared_ptr<KnobI> fileKnob = getKnobByName(kOfxImageEffectFileParamName);

    if (fileKnob) {
        KnobOutputFile* strKnob = dynamic_cast<KnobOutputFile*>( fileKnob.get() );
        if  (strKnob) {
            QString qfileName( SequenceParsing::generateFileNameFromPattern(strKnob->getValue(0), time, view).c_str() );
            Natron::removeFileExtension(qfileName);
            qfileName.append("-stats.txt");
            filename = qfileName.toStdString();
        }
    }

    //If there's no filename knob, do not write anything
    if ( filename.empty() ) {
        std::cout << QObject::tr("Cannot write render statistics file: ").toStdString() << getScriptName_mt_safe() <<
            QObject::tr(" does not seem to have a parameter named \"filename\" to determine the location where to write the stats file.").toStdString() << std::endl;

        return;
    }

    std::ofstream ofile;
    ofile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    try {
        ofile.open(filename.c_str(), std::ofstream::out);
    } catch (const std::ios_base::failure & e) {
        std::cout << QObject::tr("Failure to write render statistics file: ").toStdString() << e.what() << std::endl;

        return;
    }

    if ( !ofile.good() ) {
        std::cout << QObject::tr("Failure to write render statistics file.").toStdString() << std::endl;

        return;
    }

    ofile << "Time spent to render frame (wall clock time): " << Timer::printAsTime(wallTime, false).toStdString() << std::endl;
    for (std::map<boost::shared_ptr<Natron::Node>, NodeRenderStats >::const_iterator it = stats.begin(); it != stats.end(); ++it) {
        ofile << "------------------------------- " << it->first->getScriptName_mt_safe() << "------------------------------- " << std::endl;
        ofile << "Time spent rendering: " << Timer::printAsTime(it->second.getTotalTimeSpentRendering(), false).toStdString() << std::endl;
        const RectD & rod = it->second.getRoD();
        ofile << "Region of definition: x1 = " << rod.x1  << " y1 = " << rod.y1 << " x2 = " << rod.x2 << " y2 = " << rod.y2 << std::endl;
        ofile << "Is Identity to Effect? ";
        NodePtr identity = it->second.getInputImageIdentity();
        if (identity) {
            ofile << "Yes, to " << identity->getScriptName_mt_safe() << std::endl;
        } else {
            ofile << "No" << std::endl;
        }
        ofile << "Has render-scale support? ";
        if ( it->second.isRenderScaleSupportEnabled() ) {
            ofile << "Yes";
        } else {
            ofile << "No";
        }
        ofile << std::endl;
        ofile << "Has tiles support? ";
        if ( it->second.isTilesSupportEnabled() ) {
            ofile << "Yes";
        } else {
            ofile << "No";
        }
        ofile << std::endl;
        ofile << "Channels processed: ";

        bool r, g, b, a;
        it->second.getChannelsRendered(&r, &g, &b, &a);
        if (r) {
            ofile << "red ";
        }
        if (g) {
            ofile << "green ";
        }
        if (b) {
            ofile << "blue ";
        }
        if (a) {
            ofile << "alpha";
        }
        ofile << std::endl;

        ofile << "Output alpha premultiplication: ";
        switch ( it->second.getOutputPremult() ) {
        case Natron::eImagePremultiplicationOpaque:
            ofile << "opaque";
            break;
        case Natron::eImagePremultiplicationPremultiplied:
            ofile << "premultiplied";
            break;
        case Natron::eImagePremultiplicationUnPremultiplied:
            ofile << "unpremultiplied";
            break;
        }
        ofile << std::endl;

        ofile << "Mipmap level(s) rendered: ";
        for (std::set<unsigned int>::const_iterator it2 = it->second.getMipMapLevelsRendered().begin(); it2 != it->second.getMipMapLevelsRendered().end(); ++it2) {
            ofile << *it2 << ' ';
        }
        ofile << std::endl;
        int nbCacheMiss, nbCacheHit, nbCacheHitButDownscaled;
        it->second.getCacheAccessInfos(&nbCacheMiss, &nbCacheHit, &nbCacheHitButDownscaled);
        ofile << "Nb cache hit: " << nbCacheMiss << std::endl;
        ofile << "Nb cache miss: " << nbCacheMiss << std::endl;
        ofile << "Nb cache hit requiring mipmap downscaling: " << nbCacheHitButDownscaled << std::endl;

        const std::set<std::string> & planes = it->second.getPlanesRendered();
        ofile << "Plane(s) rendered: ";
        for (std::set<std::string>::const_iterator it2 = planes.begin(); it2 != planes.end(); ++it2) {
            ofile << *it2 << ' ';
        }
        ofile << std::endl;

        std::list<std::pair<RectI, boost::shared_ptr<Natron::Node> > > identityRectangles = it->second.getIdentityRectangles();
        const std::list<RectI> & renderedRectangles = it->second.getRenderedRectangles();

        ofile << "Identity rectangles: " << identityRectangles.size() << std::endl;
        for (std::list<std::pair<RectI, boost::shared_ptr<Natron::Node> > > ::iterator it2 = identityRectangles.begin(); it2 != identityRectangles.end(); ++it2) {
            ofile << "Origin: " << it2->second->getScriptName_mt_safe() << ", rect: x1 = " << it2->first.x1
                  << " y1 = " << it2->first.y1 << " x2 = " << it2->first.x2 << " y2 = " << it2->first.y2 << std::endl;
        }

        ofile << "Rectangles rendered: " << renderedRectangles.size() << std::endl;
        for (std::list<RectI>::const_iterator it2 = renderedRectangles.begin(); it2 != renderedRectangles.end(); ++it2) {
            ofile << "x1 = " << it2->x1 << " y1 = " << it2->y1 << " x2 = " << it2->x2 << " y2 = " << it2->y2 << std::endl;
        }
    }
} // OutputEffectInstance::reportStats

