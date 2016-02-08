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

#ifndef NATRON_ENGINE_OFXPARAMINSTANCE_H
#define NATRON_ENGINE_OFXPARAMINSTANCE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"
#include <map>
#include <string>
#include <vector>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif
CLANG_DIAG_OFF(deprecated)
#include <QStringList>
#include <QMutex>
CLANG_DIAG_ON(deprecated)


#include "Global/GlobalDefines.h"
//ofx
// ofxhPropertySuite.h:565:37: warning: 'this' pointer cannot be null in well-defined C++ code; comparison may be assumed to always evaluate to true [-Wtautological-undefined-compare]
CLANG_DIAG_OFF(unknown-pragmas)
CLANG_DIAG_OFF(tautological-undefined-compare) // appeared in clang 3.5
#include <ofxhImageEffect.h>
CLANG_DIAG_ON(tautological-undefined-compare)
CLANG_DIAG_ON(unknown-pragmas)
#include "ofxCore.h"
#include "ofxhParametricParam.h"

#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

/*This file contains the classes that connect the knobs to the OpenFX params.
   Note that all the get(...) and set(...) functions are called BY PLUGIN and you should
   never call them. When the user interact with a knob, the onInstanceChanged() slot
   is called. In turn, the plug-in will fetch the value that has changed by calling get(...).
 */


class OfxParamToKnob;

class PropertyModified_RAII
{
    OfxParamToKnob* _h;
public:
    
    PropertyModified_RAII(OfxParamToKnob* h);
    
    ~PropertyModified_RAII();
};

#define SET_DYNAMIC_PROPERTY_EDITED() PropertyModified_RAII dynamic_prop_edited_raii(this)

#define DYNAMIC_PROPERTY_CHECK() if (isDynamicPropertyBeingModified()) { \
                                    return; \
                                 } \
                                 SET_DYNAMIC_PROPERTY_EDITED();

class OfxParamToKnob : public QObject
{
    
    Q_OBJECT

    friend class PropertyModified_RAII;
    
    OFX::Host::Interact::Descriptor interactDesc;
    mutable QMutex dynamicPropModifiedMutex;
    bool _dynamicPropModified;
    
    
public:
   

    OfxParamToKnob()
    : dynamicPropModifiedMutex()
    , _dynamicPropModified(false)
    {
    }

    virtual KnobPtr getKnob() const = 0;
    virtual OFX::Host::Param::Instance* getOfxParam() = 0;
    
    OFX::Host::Interact::Descriptor & getInteractDesc()
    {
        return interactDesc;
    }
    
    
    
    void connectDynamicProperties();
    
    //these are per ofxparam thread-local data
    struct OfxParamTLSData
    {
        //only for string-param for now
        std::string str;
    };

public Q_SLOTS:
    
    /*
     These are called when the properties are changed on the Natron side
     */
    void onEvaluateOnChangeChanged(bool evaluate);
    void onSecretChanged();
    void onEnabledChanged();
    void onLabelChanged();
    void onDisplayMinMaxChanged(double min,double max, int index);
    void onMinMaxChanged(double min,double max, int index);
    void onKnobAnimationLevelChanged(ViewIdx view,int dim,int lvl);
    
protected:
    
    void setDynamicPropertyModified(bool dynamicPropModified)
    {
        QMutexLocker k(&dynamicPropModifiedMutex);
        _dynamicPropModified = dynamicPropModified;
    }
    
    bool isDynamicPropertyBeingModified() const
    {
        QMutexLocker k(&dynamicPropModifiedMutex);
        return _dynamicPropModified;
    }
    
    virtual bool hasDoubleMinMaxProps() const { return true; }
    
};





class OfxPushButtonInstance
    : public OFX::Host::Param::PushbuttonInstance, public OfxParamToKnob
{
public:
    OfxPushButtonInstance(const boost::shared_ptr<OfxEffectInstance>& node,
                          OFX::Host::Param::Descriptor & descriptor);

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;
    virtual KnobPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

private:
    boost::weak_ptr<KnobButton> _knob;
};


class OfxIntegerInstance
    :  public OfxParamToKnob, public OFX::Host::Param::IntegerInstance
{

public:

    OfxIntegerInstance(const boost::shared_ptr<OfxEffectInstance>& node,
                       OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(int &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time, int &) OVERRIDE FINAL;
    virtual OfxStatus set(int) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time, int) OVERRIDE FINAL;

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;
    virtual void setDisplayRange() OVERRIDE FINAL;
    virtual void setRange() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;
    
    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual KnobPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }


private:
    
    virtual bool hasDoubleMinMaxProps() const OVERRIDE FINAL { return false; }
    
    boost::weak_ptr<KnobInt> _knob;
};

class OfxDoubleInstance
:   public OfxParamToKnob, public OFX::Host::Param::DoubleInstance
{

public:
    OfxDoubleInstance(const boost::shared_ptr<OfxEffectInstance>& node,
                      OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(double &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time, double &) OVERRIDE FINAL;
    virtual OfxStatus set(double) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time, double) OVERRIDE FINAL;
    virtual OfxStatus derive(OfxTime time, double &) OVERRIDE FINAL;
    virtual OfxStatus integrate(OfxTime time1, OfxTime time2, double &) OVERRIDE FINAL;

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;
    virtual void setDisplayRange() OVERRIDE FINAL;
    virtual void setRange() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;


    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual KnobPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

    bool isAnimated() const;


private:
    
    boost::weak_ptr<KnobDouble> _knob;
};

class OfxBooleanInstance
:   public OfxParamToKnob, public OFX::Host::Param::BooleanInstance
{

public:
    OfxBooleanInstance(const boost::shared_ptr<OfxEffectInstance>& node,
                       OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(bool &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time, bool &) OVERRIDE FINAL;
    virtual OfxStatus set(bool) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time, bool) OVERRIDE FINAL;


    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual KnobPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }


private:
    
    virtual bool hasDoubleMinMaxProps() const OVERRIDE FINAL { return false; }
    
    boost::weak_ptr<KnobBool> _knob;
};

class OfxChoiceInstance
    : public OfxParamToKnob, public OFX::Host::Param::ChoiceInstance
{

public:
    OfxChoiceInstance(const boost::shared_ptr<OfxEffectInstance>& node,
                      OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(int &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time, int &) OVERRIDE FINAL;
    virtual OfxStatus set(int) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time, int) OVERRIDE FINAL;

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;

    // callback which should set option as appropriate
    virtual void setOption(int num) OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual KnobPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

private:
    
    virtual bool hasDoubleMinMaxProps() const OVERRIDE FINAL { return false; }
    
    boost::weak_ptr<KnobChoice> _knob;
};

class OfxRGBAInstance
    :  public OfxParamToKnob, public OFX::Host::Param::RGBAInstance
{

public:
    OfxRGBAInstance(const boost::shared_ptr<OfxEffectInstance>& node,
                    OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(double &,double &,double &,double &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time, double &,double &,double &,double &) OVERRIDE FINAL;
    virtual OfxStatus set(double,double,double,double) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time, double,double,double,double) OVERRIDE FINAL;
    virtual OfxStatus derive(OfxTime time, double &, double &, double &, double &) OVERRIDE FINAL;
    virtual OfxStatus integrate(OfxTime time1, OfxTime time2, double &, double &, double &, double &) OVERRIDE FINAL;

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual KnobPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

    bool isAnimated(int dimension) const;
    bool isAnimated() const;

private:
    boost::weak_ptr<KnobColor> _knob;
};


class OfxRGBInstance
    :  public OfxParamToKnob,  public OFX::Host::Param::RGBInstance
{

public:
    OfxRGBInstance(const boost::shared_ptr<OfxEffectInstance>& node,
                   OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(double &,double &,double &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time, double &,double &,double &) OVERRIDE FINAL;
    virtual OfxStatus set(double,double,double) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time, double,double,double) OVERRIDE FINAL;
    virtual OfxStatus derive(OfxTime time, double &, double &, double &) OVERRIDE FINAL;
    virtual OfxStatus integrate(OfxTime time1, OfxTime time2, double &, double &, double &) OVERRIDE FINAL;

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual KnobPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

    bool isAnimated(int dimension) const;
    bool isAnimated() const;

private:
    boost::weak_ptr<KnobColor> _knob;
};

class OfxDouble2DInstance
    :  public OfxParamToKnob, public OFX::Host::Param::Double2DInstance
{

public:
    OfxDouble2DInstance(const boost::shared_ptr<OfxEffectInstance>& node,
                        OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(double &,double &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time,double &,double &) OVERRIDE FINAL;
    virtual OfxStatus set(double,double) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time,double,double) OVERRIDE FINAL;
    virtual OfxStatus derive(OfxTime time, double &, double &) OVERRIDE FINAL;
    virtual OfxStatus integrate(OfxTime time1, OfxTime time2, double &, double &) OVERRIDE FINAL;

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;
    virtual void setDisplayRange() OVERRIDE FINAL;
    virtual void setRange() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual KnobPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

    bool isAnimated(int dimension) const;
    bool isAnimated() const;

private:
    boost::weak_ptr<KnobDouble> _knob;
};


class OfxInteger2DInstance
    :  public OfxParamToKnob, public OFX::Host::Param::Integer2DInstance
{

public:
    OfxInteger2DInstance(const boost::shared_ptr<OfxEffectInstance>& node,
                         OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(int &,int &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time,int &,int &) OVERRIDE FINAL;
    virtual OfxStatus set(int,int) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time,int,int) OVERRIDE FINAL;

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;
    
    virtual void setRange() OVERRIDE FINAL;
    virtual void setDisplayRange() OVERRIDE FINAL;

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual KnobPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

private:
    
    virtual bool hasDoubleMinMaxProps() const OVERRIDE FINAL { return false; }
    
    boost::weak_ptr<KnobInt> _knob;
};

class OfxDouble3DInstance
    :  public OfxParamToKnob, public OFX::Host::Param::Double3DInstance
{

public:
    OfxDouble3DInstance(const boost::shared_ptr<OfxEffectInstance>& node,
                        OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(double &,double &,double &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time,double &,double &,double &) OVERRIDE FINAL;
    virtual OfxStatus set(double,double,double) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time,double,double,double) OVERRIDE FINAL;
    virtual OfxStatus derive(OfxTime time, double &, double &, double &) OVERRIDE FINAL;
    virtual OfxStatus integrate(OfxTime time1, OfxTime time2, double &, double &, double &) OVERRIDE FINAL;

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;

    virtual void setLabel() OVERRIDE FINAL;
    
    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;
    
    virtual void setDisplayRange() OVERRIDE FINAL;
    virtual void setRange() OVERRIDE FINAL;

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual KnobPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

    bool isAnimated(int dimension) const;
    bool isAnimated() const;

private:
    boost::weak_ptr<KnobDouble> _knob;
};

class OfxInteger3DInstance
    :  public OfxParamToKnob, public OFX::Host::Param::Integer3DInstance
{

public:
    OfxInteger3DInstance(const boost::shared_ptr<OfxEffectInstance>& node,
                         OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(int &,int &,int &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time,int &,int &,int &) OVERRIDE FINAL;
    virtual OfxStatus set(int,int,int) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time,int,int,int) OVERRIDE FINAL;

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;

    virtual void setLabel() OVERRIDE FINAL;
    
    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;
    
    virtual void setRange() OVERRIDE FINAL;
    virtual void setDisplayRange() OVERRIDE FINAL;

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual KnobPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

private:
    
    virtual bool hasDoubleMinMaxProps() const OVERRIDE FINAL { return false; }
    
    boost::weak_ptr<KnobInt> _knob;
};

class OfxGroupInstance
    : public OFX::Host::Param::GroupInstance, public OfxParamToKnob
{
public:

    OfxGroupInstance(const boost::shared_ptr<OfxEffectInstance>& node,
                     OFX::Host::Param::Descriptor & descriptor);

    void addKnob(KnobPtr k);

    virtual KnobPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

    virtual ~OfxGroupInstance()
    {
    }

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;

private:
    boost::weak_ptr<KnobGroup> _groupKnob;
};

class OfxPageInstance
    : public OFX::Host::Param::PageInstance, public OfxParamToKnob
{
public:


    OfxPageInstance(const boost::shared_ptr<OfxEffectInstance>& node,
                    OFX::Host::Param::Descriptor & descriptor);

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;
    virtual KnobPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }
private:
    boost::weak_ptr<KnobPage> _pageKnob;
};


struct OfxStringInstancePrivate;
class OfxStringInstance
    : public OfxParamToKnob, public OFX::Host::Param::StringInstance
{

public:
    OfxStringInstance(const boost::shared_ptr<OfxEffectInstance>& node,
                      OFX::Host::Param::Descriptor & descriptor);

    virtual ~OfxStringInstance();
    
    virtual OfxStatus get(std::string &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time, std::string &) OVERRIDE FINAL;
    virtual OfxStatus set(const char*) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time, const char*) OVERRIDE FINAL;

    /// implementation of var args function
    /// Be careful: the char* is only valid until next API call
    /// see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ArchitectureStrings
    virtual OfxStatus getV(va_list arg) OVERRIDE FINAL;

    /// implementation of var args function
    /// Be careful: the char* is only valid until next API call
    /// see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ArchitectureStrings
    virtual OfxStatus getV(OfxTime time, va_list arg) OVERRIDE FINAL;


    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual KnobPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }
    
private:
    
    /**
     * @brief Expand any project path contained in str
     **/
    void projectEnvVar_getProxy(std::string& str) const;
    
    /**
     * @brief Find any project path contained in str and replace it by the associated name
     **/
    void projectEnvVar_setProxy(std::string& str) const;
   
    boost::scoped_ptr<OfxStringInstancePrivate> _imp;
};

struct OfxCustomInstancePrivate;
class OfxCustomInstance
    : public OfxParamToKnob, public OFX::Host::Param::CustomInstance
{

public:
    OfxCustomInstance(const boost::shared_ptr<OfxEffectInstance>& node,
                      OFX::Host::Param::Descriptor & descriptor);

    virtual ~OfxCustomInstance();

    
    virtual OfxStatus get(std::string &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time, std::string &) OVERRIDE FINAL;
    virtual OfxStatus set(const char*) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time, const char*) OVERRIDE FINAL;

    /// implementation of var args function
    /// Be careful: the char* is only valid until next API call
    /// see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ArchitectureStrings
    virtual OfxStatus getV(va_list arg) OVERRIDE FINAL;

    /// implementation of var args function
    /// Be careful: the char* is only valid until next API call
    /// see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ArchitectureStrings
    virtual OfxStatus getV(OfxTime time, va_list arg) OVERRIDE FINAL;


    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;
    virtual KnobPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }
    
    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;

    typedef OfxStatus (*customParamInterpolationV1Entry_t)(const void*            handleRaw,
                                                           OfxPropertySetHandle inArgsRaw,
                                                           OfxPropertySetHandle outArgsRaw);

private:

    void getCustomParamAtTime(double time,std::string &str) const;

 
    boost::scoped_ptr<OfxCustomInstancePrivate> _imp;
    
};


class OfxParametricInstance
    : public OfxParamToKnob, public OFX::Host::ParametricParam::ParametricInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    explicit OfxParametricInstance(const boost::shared_ptr<OfxEffectInstance>& node,
                                   OFX::Host::Param::Descriptor & descriptor);

    virtual ~OfxParametricInstance();

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;

    /// callback which should update label
    virtual void setLabel() OVERRIDE FINAL;
    

    /// callback which should set
    virtual void setDisplayRange() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;

    void onCurvesDefaultInitialized();
    
    ///derived from CurveHolder
    virtual OfxStatus getValue(int curveIndex,OfxTime time,double parametricPosition,double *returnValue) OVERRIDE FINAL;
    virtual OfxStatus getNControlPoints(int curveIndex,double time,int *returnValue) OVERRIDE FINAL;
    virtual OfxStatus getNthControlPoint(int curveIndex,
                                         double time,
                                         int nthCtl,
                                         double *key,
                                         double *value) OVERRIDE FINAL;
    virtual OfxStatus setNthControlPoint(int curveIndex,
                                         double time,
                                         int nthCtl,
                                         double key,
                                         double value,
                                         bool addAnimationKey) OVERRIDE FINAL;
    virtual OfxStatus addControlPoint(int curveIndex,
                                      double time,
                                      double key,
                                      double value,
                                      bool addAnimationKey) OVERRIDE FINAL;
    virtual OfxStatus  deleteControlPoint(int curveIndex, int nthCtl) OVERRIDE FINAL;
    virtual OfxStatus  deleteAllControlPoints(int curveIndex) OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual KnobPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }
    
public Q_SLOTS:

    void onCustomBackgroundDrawingRequested();

    void initializeInteract(OverlaySupport* widget);


private:

    OfxOverlayInteract* _overlayInteract;
    boost::weak_ptr<KnobParametric> _knob;
};

NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_OFXPARAMINSTANCE_H

