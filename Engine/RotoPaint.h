/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#ifndef ROTOPAINT_H
#define ROTOPAINT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Engine/EffectInstance.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

struct RotoPaintPrivate;
class RotoPaint : public EffectInstance
{
public:
    
    static EffectInstance* BuildEffect(NodePtr n)
    {
        return new RotoPaint(n, true);
    }
    
    RotoPaint(NodePtr node, bool isPaintByDefault);
    
    virtual ~RotoPaint();
    
    bool isDefaultBehaviourPaintContext() const;
    
    virtual bool isRotoPaintNode() const OVERRIDE FINAL WARN_UNUSED_RETURN  { return true; }
    
    virtual int getMajorVersion() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return 1;
    }
    
    virtual int getMinorVersion() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return 0;
    }
    
    virtual int getMaxInputCount() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return 11;
    }
    
    virtual bool getCanTransform() const OVERRIDE FINAL WARN_UNUSED_RETURN { return false; }
    
    virtual std::string getPluginID() const OVERRIDE WARN_UNUSED_RETURN;

    virtual std::string getPluginLabel() const OVERRIDE WARN_UNUSED_RETURN;

    virtual std::string getPluginDescription() const OVERRIDE WARN_UNUSED_RETURN;

    virtual void getPluginGrouping(std::list<std::string>* grouping) const OVERRIDE FINAL
    {
        grouping->push_back(PLUGIN_GROUP_PAINT);
    }

    virtual std::string getInputLabel (int inputNb) const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual bool isInputMask(int inputNb) const OVERRIDE FINAL WARN_UNUSED_RETURN;  

    virtual bool isInputOptional(int /*inputNb*/) const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual void addAcceptedComponents(int inputNb,std::list<ImageComponents>* comps) OVERRIDE FINAL;
    virtual void addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const OVERRIDE FINAL;

    ///Doesn't really matter here since it won't be used (this effect is always an identity)
    virtual RenderSafetyEnum renderThreadSafety() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return eRenderSafetyFullySafeFrame;
    }

    virtual bool supportsTiles() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual bool supportsMultiResolution() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return true;
    }

    virtual bool isOutput() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual void initializeKnobs() OVERRIDE FINAL;

    virtual StatusEnum getPreferredMetaDatas(NodeMetadata& metadata) OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual void onInputChanged(int inputNb) OVERRIDE FINAL;

    virtual void clearLastRenderedImage() OVERRIDE FINAL;

    virtual bool isHostMaskingEnabled() const OVERRIDE FINAL WARN_UNUSED_RETURN { return true; }
    virtual bool isHostMixingEnabled() const OVERRIDE FINAL WARN_UNUSED_RETURN  { return true; }
    virtual bool getCreateChannelSelectorKnob() const OVERRIDE FINAL WARN_UNUSED_RETURN { return false; }

    virtual bool isHostChannelSelectorSupported(bool* defaultR,bool* defaultG, bool* defaultB, bool* defaultA) const OVERRIDE WARN_UNUSED_RETURN;
    
private:
    
    virtual void knobChanged(KnobI* k,
                             ValueChangedReasonEnum reason,
                             ViewSpec view,
                             double time,
                             bool originatedFromMainThread) OVERRIDE FINAL;

    virtual StatusEnum
    getRegionOfDefinition(U64 hash,double time, const RenderScale & scale, ViewIdx view, RectD* rod) OVERRIDE WARN_UNUSED_RETURN;
    
    virtual void getRegionsOfInterest(double time,
                                      const RenderScale & scale,
                                      const RectD & outputRoD, //!< the RoD of the effect, in canonical coordinates
                                      const RectD & renderWindow, //!< the region to be rendered in the output image, in Canonical Coordinates
                                      ViewIdx view,
                                      RoIMap* ret) OVERRIDE FINAL;
    
    virtual FramesNeededMap getFramesNeeded(double time, ViewIdx view) OVERRIDE FINAL;

    virtual bool isIdentity(double time,
                        const RenderScale & scale,
                        const RectI & roi,
                        ViewIdx view,
                        double* inputTime,
                        int* inputNb) OVERRIDE FINAL WARN_UNUSED_RETURN;
        


    virtual StatusEnum render(const RenderActionArgs& args) OVERRIDE WARN_UNUSED_RETURN;

    boost::scoped_ptr<RotoPaintPrivate> _imp;

};

/**
 * @brief Same as RotoPaint except that by default RGB checkboxes are unchecked and the default selected tool is not the same
 **/
class RotoNode : public RotoPaint
{
    
public:
    
    static EffectInstance* BuildEffect(NodePtr n)
    {
        return new RotoNode(n);
    }

    
    RotoNode(NodePtr node) : RotoPaint(node, false) {}
    
    virtual std::string getPluginID() const OVERRIDE WARN_UNUSED_RETURN;
    
    virtual std::string getPluginLabel() const OVERRIDE WARN_UNUSED_RETURN;
    
    virtual std::string getPluginDescription() const OVERRIDE WARN_UNUSED_RETURN;
    
    virtual bool isHostChannelSelectorSupported(bool* defaultR,bool* defaultG, bool* defaultB, bool* defaultA) const OVERRIDE WARN_UNUSED_RETURN;
};

NATRON_NAMESPACE_EXIT;

#endif // ROTOPAINT_H
