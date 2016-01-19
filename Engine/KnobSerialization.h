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

#ifndef KNOBSERIALIZATION_H
#define KNOBSERIALIZATION_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <map>
#include <vector>
#include "Global/Macros.h"
#ifndef Q_MOC_RUN
GCC_DIAG_OFF(unused-parameter)
GCC_DIAG_OFF(sign-compare)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>
GCC_DIAG_ON(unused-parameter)
GCC_DIAG_ON(sign-compare)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#include "Engine/Variant.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/CurveSerialization.h"
#include "Engine/StringAnimationManager.h"
#include <SequenceParsing.h>
#include "Engine/EngineFwd.h"


#define KNOB_SERIALIZATION_INTRODUCES_SLAVED_TRACKS 2
#define KNOB_SERIALIZATION_INTRODUCES_SLAVED_TRACKS_OFFSET 3
#define KNOB_SERIALIZATION_INTRODUCES_CHOICE_LABEL 4
#define KNOB_SERIALIZATION_INTRODUCES_USER_KNOB 5
#define KNOB_SERIALIZATION_NODE_SCRIPT_NAME 6
#define KNOB_SERIALIZATION_INTRODUCES_NATIVE_OVERLAYS 7
#define KNOB_SERIALIZATION_INTRODUCES_CHOICE_HELP_STRINGS 8
#define KNOB_SERIALIZATION_INTRODUCES_DEFAULT_VALUES 9
#define KNOB_SERIALIZATION_INTRODUCES_DISPLAY_MIN_MAX 10
#define KNOB_SERIALIZATION_INTRODUCES_ALIAS 11
#define KNOB_SERIALIZATION_VERSION KNOB_SERIALIZATION_INTRODUCES_ALIAS

#define VALUE_SERIALIZATION_INTRODUCES_CHOICE_LABEL 2
#define VALUE_SERIALIZATION_INTRODUCES_EXPRESSIONS 3
#define VALUE_SERIALIZATION_REMOVES_EXTRA_DATA 4
#define VALUE_SERIALIZATION_INTRODUCES_EXPRESSIONS_RESULTS 5
#define VALUE_SERIALIZATION_REMOVES_EXPRESSIONS_RESULTS 6
#define VALUE_SERIALIZATION_VERSION VALUE_SERIALIZATION_REMOVES_EXPRESSIONS_RESULTS

NATRON_NAMESPACE_ENTER;

struct MasterSerialization
{
    int masterDimension;
    std::string masterNodeName;
    std::string masterKnobName;

    MasterSerialization()
        : masterDimension(-1)
          , masterNodeName()
          , masterKnobName()
    {
    }

    template<class Archive>
    void serialize(Archive & ar,
                   const unsigned int version)
    {
        Q_UNUSED(version);
        ar & ::boost::serialization::make_nvp("MasterDimension",masterDimension);
        ar & ::boost::serialization::make_nvp("MasterNodeName",masterNodeName);
        ar & ::boost::serialization::make_nvp("MasterKnobName",masterKnobName);
    }
};

class TypeExtraData
{
public:
    
    TypeExtraData() {}
    
    virtual ~TypeExtraData() {}
};

class ChoiceExtraData : public TypeExtraData
{
public:
    
    
    
    ChoiceExtraData() : TypeExtraData(), _choiceString() , _entries() , _helpStrings() {}
    
    virtual ~ChoiceExtraData() {}
    
    std::string _choiceString;
    std::vector<std::string> _entries;
    std::vector<std::string> _helpStrings;
};

class FileExtraData: public TypeExtraData
{
    public:
    FileExtraData() : TypeExtraData() , useSequences(false) {}
    
    bool useSequences;
};

class PathExtraData: public TypeExtraData
{
    public:
    PathExtraData() : TypeExtraData(), multiPath(false) {}
    
    bool multiPath;
};

class TextExtraData : public TypeExtraData
{
public:
    TextExtraData() : TypeExtraData(), label(false),  multiLine(false), richText(false) {}
    
    bool label;
    bool multiLine;
    bool richText;
};

class ValueExtraData: public TypeExtraData
{
public:
    ValueExtraData()
    : TypeExtraData()
    , min(0.)
    , max(0.)
    , dmin(0.)
    , dmax(0.)
    {
    }

    double min;
    double max;
    double dmin;
    double dmax;
};

class KnobSerializationBase;
struct ValueSerialization
{
    KnobSerializationBase* _serialization;
    boost::shared_ptr<KnobI> _knob;
    int _dimension;
    MasterSerialization _master;
    std::string _expression;
    bool _exprHasRetVar;
    
    ///Load
    ValueSerialization(KnobSerializationBase* serialization,
                       const boost::shared_ptr<KnobI> & knob,
                       int dimension);
    
    ///Save
    ValueSerialization(const boost::shared_ptr<KnobI> & knob,
                       int dimension,
                       bool exprHasRetVar,
                       const std::string& expr);

    void setChoiceExtraLabel(const std::string& label);
    
    
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        Knob<int>* isInt = dynamic_cast<Knob<int>*>( _knob.get() );
        Knob<bool>* isBool = dynamic_cast<Knob<bool>*>( _knob.get() );
        Knob<double>* isDouble = dynamic_cast<Knob<double>*>( _knob.get() );
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>( _knob.get() );
        Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>( _knob.get() );
        KnobParametric* isParametric = dynamic_cast<KnobParametric*>(_knob.get());

        KnobPage* isPage = dynamic_cast<KnobPage*>(_knob.get());
        KnobGroup* isGrp = dynamic_cast<KnobGroup*>(_knob.get());
        KnobSeparator* isSep = dynamic_cast<KnobSeparator*>(_knob.get());
        KnobButton* btn = dynamic_cast<KnobButton*>(_knob.get());
        
        bool enabled = _knob->isEnabled(_dimension);
        ar & ::boost::serialization::make_nvp("Enabled",enabled);
        bool hasAnimation = _knob->isAnimated(_dimension);
        ar & ::boost::serialization::make_nvp("HasAnimation",hasAnimation);

        if (hasAnimation) {
            ar & ::boost::serialization::make_nvp("Curve",*( _knob->getCurve(_dimension,true) ));
        }

        if (isInt && !isChoice) {
            int v = isInt->getValue(_dimension);
            ar & ::boost::serialization::make_nvp("Value",v);
        } else if (isBool && !isPage && !isGrp && !isSep && !btn) {
            bool v = isBool->getValue(_dimension);
            ar & ::boost::serialization::make_nvp("Value",v);
        } else if (isDouble && !isParametric) {
            double v = isDouble->getValue(_dimension);
            ar & ::boost::serialization::make_nvp("Value",v);
        } else if (isChoice) {
            int v = isChoice->getValue(_dimension);
            std::vector<std::string> entries = isChoice->getEntries_mt_safe();
            std::string label ;
            if (v < (int)entries.size() && v >= 0) {
                label = entries[v];
            }
            ar & ::boost::serialization::make_nvp("Value", v);
       
        } else if (isString) {
            std::string v = isString->getValue(_dimension);
            ar & ::boost::serialization::make_nvp("Value",v);
        }

        bool hasMaster = _knob->isSlave(_dimension);
        ar & ::boost::serialization::make_nvp("HasMaster",hasMaster);
        if (hasMaster) {
            ar & ::boost::serialization::make_nvp("Master",_master);
        }
        
        ar & ::boost::serialization::make_nvp("Expression",_expression);
        ar & ::boost::serialization::make_nvp("ExprHasRet",_exprHasRetVar);

        
    } // save

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        Knob<int>* isInt = dynamic_cast<Knob<int>*>( _knob.get() );
        Knob<bool>* isBool = dynamic_cast<Knob<bool>*>( _knob.get() );
        Knob<double>* isDouble = dynamic_cast<Knob<double>*>( _knob.get() );
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>( _knob.get() );
        Knob<std::string>* isString = dynamic_cast<Knob<std::string>*>( _knob.get() );
        KnobFile* isFile = dynamic_cast<KnobFile*>(_knob.get());
        KnobParametric* isParametric = dynamic_cast<KnobParametric*>(_knob.get());
        KnobPage* isPage = dynamic_cast<KnobPage*>(_knob.get());
        KnobGroup* isGrp = dynamic_cast<KnobGroup*>(_knob.get());
        KnobSeparator* isSep = dynamic_cast<KnobSeparator*>(_knob.get());
        KnobButton* btn = dynamic_cast<KnobButton*>(_knob.get());

        
        bool enabled;
        ar & ::boost::serialization::make_nvp("Enabled",enabled);

        _knob->setEnabled(_dimension, enabled);

        bool hasAnimation;
        ar & ::boost::serialization::make_nvp("HasAnimation",hasAnimation);
        bool convertOldFileKeyframesToPattern = false;
        if (hasAnimation) {
            assert(_knob->canAnimate());
            Curve c;
            ar & ::boost::serialization::make_nvp("Curve",c);
            ///This is to overcome the change to the animation of file params: They no longer hold keyframes
            ///Don't try to load keyframes
            convertOldFileKeyframesToPattern = isFile && isFile->getName() == kOfxImageEffectFileParamName;
            if (!convertOldFileKeyframesToPattern) {
                boost::shared_ptr<Curve> curve = _knob->getCurve(_dimension);
                assert(curve);
                if (curve) {
                    _knob->getCurve(_dimension)->clone(c);
                }
            }
        }

        if (isInt && !isChoice) {
            int v;
            ar & ::boost::serialization::make_nvp("Value",v);
            isInt->setValue(v,_dimension);
        } else if (isBool && !isGrp && !isPage && !isSep && !btn) {
            bool v;
            ar & ::boost::serialization::make_nvp("Value",v);
            isBool->setValue(v,_dimension);
        } else if (isDouble && !isParametric) {
            double v;
            ar & ::boost::serialization::make_nvp("Value",v);
            isDouble->setValue(v,_dimension);
        } else if (isChoice) {
            int v;
            ar & ::boost::serialization::make_nvp("Value", v);
            assert(v >= 0);
            if (version >= VALUE_SERIALIZATION_INTRODUCES_CHOICE_LABEL) {
                
                if (version < VALUE_SERIALIZATION_REMOVES_EXTRA_DATA) {
                    std::string label;
                    ar & ::boost::serialization::make_nvp("Label", label);
                    setChoiceExtraLabel(label);
                }
                
            }
            isChoice->setValue(v, _dimension);

        } else if (isString && !isFile) {
            std::string v;
            ar & ::boost::serialization::make_nvp("Value",v);
            isString->setValue(v,_dimension);
        } else if (isFile) {
            std::string v;
            ar & ::boost::serialization::make_nvp("Value",v);

            ///Convert the old keyframes stored in the file parameter by analysing one keyframe
            ///and deducing the pattern from it and setting it as a value instead
            if (convertOldFileKeyframesToPattern) {
                SequenceParsing::FileNameContent content(v);
                content.generatePatternWithFrameNumberAtIndex(content.getPotentialFrameNumbersCount() - 1,
                                                              content.getNumPrependingZeroes() + 1,
                                                              &v);
            }
            isFile->setValue(v,_dimension);
        }

        ///We cannot restore the master yet. It has to be done in another pass.
        bool hasMaster;
        ar & ::boost::serialization::make_nvp("HasMaster",hasMaster);
        if (hasMaster) {
            ar & ::boost::serialization::make_nvp("Master",_master);
        }
        
        if (version >= VALUE_SERIALIZATION_INTRODUCES_EXPRESSIONS) {
            ar & ::boost::serialization::make_nvp("Expression",_expression);
            ar & ::boost::serialization::make_nvp("ExprHasRet",_exprHasRetVar);
        }
        
        if (version >= VALUE_SERIALIZATION_INTRODUCES_EXPRESSIONS_RESULTS && version < VALUE_SERIALIZATION_REMOVES_EXPRESSIONS_RESULTS) {
            if (isInt) {
                std::map<SequenceTime,int> exprValues;
                ar & ::boost::serialization::make_nvp("ExprResults",exprValues);
            } else if (isBool) {
                std::map<SequenceTime,bool> exprValues;
                ar & ::boost::serialization::make_nvp("ExprResults",exprValues);
            } else if (isDouble) {
                std::map<SequenceTime,double> exprValues;
                ar & ::boost::serialization::make_nvp("ExprResults",exprValues);
            } else if (isString) {
                std::map<SequenceTime,std::string> exprValues;
                ar & ::boost::serialization::make_nvp("ExprResults",exprValues);
            }

        }
    } // load

    BOOST_SERIALIZATION_SPLIT_MEMBER()
};


class KnobSerializationBase
{

public:
    
    KnobSerializationBase() {}
    
    virtual ~KnobSerializationBase() {}
    
    
    virtual const std::string& getName() const = 0;
    
    virtual boost::shared_ptr<KnobI> getKnob() const = 0;
    
    virtual void setChoiceExtraString(const std::string& /*label*/) {}
    
    
};

class KnobSerialization : public KnobSerializationBase
{
    boost::shared_ptr<KnobI> _knob; //< used when serializing
    std::string _typeName;
    int _dimension;
    std::list<MasterSerialization> _masters; //< used when deserializating, we can't restore it before all knobs have been restored.
    bool _masterIsAlias;
    std::vector<std::pair<std::string,bool> > _expressions; //< used when deserializing, we can't restore it before all knobs have been restored.
    std::list< Curve > parametricCurves;
    std::list<KnobDouble::SerializedTrack> slavedTracks; //< same as for master, can't be used right away when deserializing
    
    mutable TypeExtraData* _extraData;
    
    bool _isUserKnob;
    std::string _label;
    bool _triggerNewLine;
    bool _evaluatesOnChange;
    bool _isPersistent;
    bool _animationEnabled;
    std::string _tooltip;
    bool _useHostOverlay;

    
    virtual void setChoiceExtraString(const std::string& label) OVERRIDE FINAL;
    
    friend class ::boost::serialization::access;
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        assert(_knob);
        AnimatingKnobStringHelper* isString = dynamic_cast<AnimatingKnobStringHelper*>( _knob.get() );
        KnobParametric* isParametric = dynamic_cast<KnobParametric*>( _knob.get() );
        KnobDouble* isDouble = dynamic_cast<KnobDouble*>( _knob.get() );
     
        
        std::string name = _knob->getName();
        ar & ::boost::serialization::make_nvp("Name",name);
        ar & ::boost::serialization::make_nvp("Type",_typeName);
        ar & ::boost::serialization::make_nvp("Dimension",_dimension);
        bool secret = _knob->getIsSecret();
        ar & ::boost::serialization::make_nvp("Secret",secret);

        ar & ::boost::serialization::make_nvp("MasterIsAlias",_masterIsAlias);
        
        for (int i = 0; i < _knob->getDimension(); ++i) {
            ValueSerialization vs(_knob,i,_expressions[i].second,_expressions[i].first);
            ar & ::boost::serialization::make_nvp("item",vs);
        }

        ////restore extra datas
        if (isParametric) {
            std::list< Curve > curves;
            isParametric->saveParametricCurves(&curves);
            ar & ::boost::serialization::make_nvp("ParametricCurves",curves);
        } else if (isString) {
            std::map<int,std::string> extraDatas;
            isString->getAnimation().save(&extraDatas);
            ar & ::boost::serialization::make_nvp("StringsAnimation",extraDatas);
        } else if ( isDouble && (isDouble->getName() == "center") && (isDouble->getDimension() == 2) ) {
            std::list<KnobDouble::SerializedTrack> tracks;
            isDouble->serializeTracks(&tracks);
            int count = (int)tracks.size();
            ar & ::boost::serialization::make_nvp("SlavePtsNo",count);
            for (std::list<KnobDouble::SerializedTrack>::iterator it = tracks.begin(); it != tracks.end(); ++it) {
                ar & ::boost::serialization::make_nvp("SlavePtNodeName",it->rotoNodeName);
                ar & ::boost::serialization::make_nvp("SlavePtBezier",it->bezierName);
                ar & ::boost::serialization::make_nvp("SlavePtIndex",it->cpIndex);
                ar & ::boost::serialization::make_nvp("SlavePtIsFeather",it->isFeather);
                ar & ::boost::serialization::make_nvp("OffsetTime",it->offsetTime);
            }
        }
        
        ChoiceExtraData* cdata = dynamic_cast<ChoiceExtraData*>(_extraData);
        if (cdata) {
            ar & ::boost::serialization::make_nvp("ChoiceLabel",cdata->_choiceString);
        }
        
        ar & ::boost::serialization::make_nvp("UserKnob",_isUserKnob);
        if (_isUserKnob) {
            ar & ::boost::serialization::make_nvp("Label",_label);
            ar & ::boost::serialization::make_nvp("Help",_tooltip);
            ar & ::boost::serialization::make_nvp("NewLine",_triggerNewLine);
            ar & ::boost::serialization::make_nvp("Evaluate",_evaluatesOnChange);
            ar & ::boost::serialization::make_nvp("Animates",_animationEnabled);
            ar & ::boost::serialization::make_nvp("Persistent",_isPersistent);
            if (_extraData) {
                ValueExtraData* vdata = dynamic_cast<ValueExtraData*>(_extraData);
                TextExtraData* tdata = dynamic_cast<TextExtraData*>(_extraData);
                FileExtraData* fdata = dynamic_cast<FileExtraData*>(_extraData);
                PathExtraData* pdata = dynamic_cast<PathExtraData*>(_extraData);
                
                if (cdata) {
                    ar & ::boost::serialization::make_nvp("Entries",cdata->_entries);
                    ar & ::boost::serialization::make_nvp("Helps",cdata->_helpStrings);
                } else if (vdata) {
                    ar & ::boost::serialization::make_nvp("Min",vdata->min);
                    ar & ::boost::serialization::make_nvp("Max",vdata->max);
                    ar & ::boost::serialization::make_nvp("DMin",vdata->dmin);
                    ar & ::boost::serialization::make_nvp("DMax",vdata->dmax);
                } else if (fdata) {
                    ar & ::boost::serialization::make_nvp("Sequences",fdata->useSequences);
                } else if (pdata) {
                    ar & ::boost::serialization::make_nvp("MultiPath",pdata->multiPath);
                } else if (tdata) {
                    ar & ::boost::serialization::make_nvp("IsLabel",tdata->label);
                    ar & ::boost::serialization::make_nvp("IsMultiLine",tdata->multiLine);
                    ar & ::boost::serialization::make_nvp("UseRichText",tdata->richText);
                }
            }
            
            if (isDouble && isDouble->getDimension() == 2) {
                bool useOverlay = isDouble->getHasHostOverlayHandle();
                ar & ::boost::serialization::make_nvp("HasOverlayHandle",useOverlay);
            }
            
            Knob<double>* isDbl = dynamic_cast<Knob<double>*>(_knob.get());
            Knob<int>* isInt = dynamic_cast<Knob<int>*>(_knob.get());
            KnobBool* isBool = dynamic_cast<KnobBool*>(_knob.get());
            Knob<std::string>* isStr = dynamic_cast<Knob<std::string>*>(_knob.get());
            
            for (int i = 0; i < _knob->getDimension(); ++i) {
                if (isDbl) {
                    double def = isDbl->getDefaultValue(i);
                    ar & ::boost::serialization::make_nvp("DefaultValue",def);
                } else if (isInt) {
                    int def = isInt->getDefaultValue(i);
                    ar & ::boost::serialization::make_nvp("DefaultValue",def);
                } else if (isBool) {
                    bool def = isBool->getDefaultValue(i);
                    ar & ::boost::serialization::make_nvp("DefaultValue",def);
                } else if (isStr) {
                    std::string def = isStr->getDefaultValue(i);
                    ar & ::boost::serialization::make_nvp("DefaultValue",def);
                }
            }
        }
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        assert(!_knob);
        std::string name;
        ar & ::boost::serialization::make_nvp("Name",name);
        ar & ::boost::serialization::make_nvp("Type",_typeName);
        ar & ::boost::serialization::make_nvp("Dimension",_dimension);
        boost::shared_ptr<KnobI> created = createKnob(_typeName, _dimension);
        if (!created) {
            return;
        } else {
            _knob = created;
        }
        _knob->setName(name);

        bool secret;
        ar & ::boost::serialization::make_nvp("Secret",secret);
        _knob->setSecret(secret);
        
        if (version >= KNOB_SERIALIZATION_INTRODUCES_ALIAS) {
            ar & ::boost::serialization::make_nvp("MasterIsAlias",_masterIsAlias);
        } else {
            _masterIsAlias = false;
        }

        AnimatingKnobStringHelper* isStringAnimated = dynamic_cast<AnimatingKnobStringHelper*>( _knob.get() );
        KnobFile* isFile = dynamic_cast<KnobFile*>( _knob.get() );
        KnobParametric* isParametric = dynamic_cast<KnobParametric*>( _knob.get() );
        KnobDouble* isDouble = dynamic_cast<KnobDouble*>( _knob.get() );
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>( _knob.get() );
        if (isChoice && !_extraData) {
            _extraData = new ChoiceExtraData;
            
        }
        
        for (int i = 0; i < _knob->getDimension(); ++i) {
            ValueSerialization vs(this,_knob,i);
            ar & ::boost::serialization::make_nvp("item",vs);
            _masters.push_back(vs._master);
            _expressions.push_back(std::make_pair(vs._expression,vs._exprHasRetVar));
        }

        ////restore extra datas
        if (isParametric) {
            std::list< Curve > curves;
            ar & ::boost::serialization::make_nvp("ParametricCurves",curves);
            isParametric->loadParametricCurves(curves);
        } else if (isStringAnimated) {
            std::map<int,std::string> extraDatas;
            ar & ::boost::serialization::make_nvp("StringsAnimation",extraDatas);
            ///Don't load animation for input image files: they no longer hold keyframes
            // in the Reader context, the script name must be kOfxImageEffectFileParamName, @see kOfxImageEffectContextReader
            if ( !isFile || ( isFile && (isFile->getName() != kOfxImageEffectFileParamName) ) ) {
                isStringAnimated->loadAnimation(extraDatas);
            }
        }
        if ( (version >= KNOB_SERIALIZATION_INTRODUCES_SLAVED_TRACKS) &&
                    isDouble && ( isDouble->getName() == "center") && ( isDouble->getDimension() == 2) ) {
            int count;
            ar & ::boost::serialization::make_nvp("SlavePtsNo",count);
            for (int i = 0; i < count; ++i) {
                KnobDouble::SerializedTrack t;
                ar & ::boost::serialization::make_nvp("SlavePtNodeName",t.rotoNodeName);
                if (version >= KNOB_SERIALIZATION_NODE_SCRIPT_NAME) {
                    t.rotoNodeName = Natron::makeNameScriptFriendly(t.rotoNodeName);
                }
                ar & ::boost::serialization::make_nvp("SlavePtBezier",t.bezierName);
                ar & ::boost::serialization::make_nvp("SlavePtIndex",t.cpIndex);
                ar & ::boost::serialization::make_nvp("SlavePtIsFeather",t.isFeather);
                if (version >= KNOB_SERIALIZATION_INTRODUCES_SLAVED_TRACKS_OFFSET) {
                    ar & ::boost::serialization::make_nvp("OffsetTime",t.offsetTime);
                }
                slavedTracks.push_back(t);
            }
        }
        
        if (version >= KNOB_SERIALIZATION_INTRODUCES_USER_KNOB) {
            
            KnobChoice* isChoice = dynamic_cast<KnobChoice*>( _knob.get() );
            if (isChoice) {
                //ChoiceExtraData* cData = new ChoiceExtraData;
                assert(_extraData);
                ChoiceExtraData* cData = dynamic_cast<ChoiceExtraData*>(_extraData);
                ar & ::boost::serialization::make_nvp("ChoiceLabel",cData->_choiceString);
                //_extraData = cData;
            }
            
            
            ar & ::boost::serialization::make_nvp("UserKnob",_isUserKnob);
            if (_isUserKnob) {
                ar & ::boost::serialization::make_nvp("Label",_label);
                ar & ::boost::serialization::make_nvp("Help",_tooltip);
                ar & ::boost::serialization::make_nvp("NewLine",_triggerNewLine);
                ar & ::boost::serialization::make_nvp("Evaluate",_evaluatesOnChange);
                ar & ::boost::serialization::make_nvp("Animates",_animationEnabled);
                ar & ::boost::serialization::make_nvp("Persistent",_isPersistent);
                
                if (isChoice) {
                    assert(_extraData);
                    ChoiceExtraData* data = dynamic_cast<ChoiceExtraData*>(_extraData);
                    assert(data);
                    ar & ::boost::serialization::make_nvp("Entries",data->_entries);
                    if (version >= KNOB_SERIALIZATION_INTRODUCES_CHOICE_HELP_STRINGS) {
                        ar & ::boost::serialization::make_nvp("Helps",data->_helpStrings);
                    }
                }
                
                KnobString* isString = dynamic_cast<KnobString*>(_knob.get());
                if (isString) {
                    TextExtraData* tdata = new TextExtraData;
                    ar & ::boost::serialization::make_nvp("IsLabel",tdata->label);
                    ar & ::boost::serialization::make_nvp("IsMultiLine",tdata->multiLine);
                    ar & ::boost::serialization::make_nvp("UseRichText",tdata->richText);
                    _extraData = tdata;
                }
                KnobDouble* isDbl = dynamic_cast<KnobDouble*>(_knob.get());
                KnobInt* isInt = dynamic_cast<KnobInt*>(_knob.get());
                KnobColor* isColor = dynamic_cast<KnobColor*>(_knob.get());
                if (isDbl || isInt || isColor) {
                    ValueExtraData* extraData = new ValueExtraData;
                    ar & ::boost::serialization::make_nvp("Min",extraData->min);
                    ar & ::boost::serialization::make_nvp("Max",extraData->max);
                    if (version >= KNOB_SERIALIZATION_INTRODUCES_DISPLAY_MIN_MAX) {
                        ar & ::boost::serialization::make_nvp("DMin",extraData->dmin);
                        ar & ::boost::serialization::make_nvp("DMax",extraData->dmax);
                    }
                    _extraData = extraData;
                }
                
                KnobFile* isFile = dynamic_cast<KnobFile*>(_knob.get());
                KnobOutputFile* isOutFile = dynamic_cast<KnobOutputFile*>(_knob.get());
                if (isFile || isOutFile) {
                    FileExtraData* extraData = new FileExtraData;
                    ar & ::boost::serialization::make_nvp("Sequences",extraData->useSequences);
                    _extraData = extraData;
                }
                
                KnobPath* isPath = dynamic_cast<KnobPath*>(_knob.get());
                if (isPath) {
                    PathExtraData* extraData = new PathExtraData;
                    ar & ::boost::serialization::make_nvp("MultiPath",extraData->multiPath);
                    _extraData = extraData;
                }
                
                
                if (isDbl && version >= KNOB_SERIALIZATION_INTRODUCES_NATIVE_OVERLAYS && isDbl->getDimension() == 2) {
                    ar & ::boost::serialization::make_nvp("HasOverlayHandle",_useHostOverlay);
                    
                }
                
                if (version >= KNOB_SERIALIZATION_INTRODUCES_DEFAULT_VALUES) {
                    Knob<double>* isDoubleVal = dynamic_cast<Knob<double>*>(_knob.get());
                    Knob<int>* isIntVal = dynamic_cast<Knob<int>*>(_knob.get());
                    KnobBool* isBool = dynamic_cast<KnobBool*>(_knob.get());
                    Knob<std::string>* isStr = dynamic_cast<Knob<std::string>*>(_knob.get());
                    
                    for (int i = 0; i < _knob->getDimension(); ++i) {
                        if (isDoubleVal) {
                            double def;
                            ar & ::boost::serialization::make_nvp("DefaultValue",def);
                            isDoubleVal->setDefaultValueWithoutApplying(def,i);
                        } else if (isIntVal) {
                            int def;
                            ar & ::boost::serialization::make_nvp("DefaultValue",def);
                            isIntVal->setDefaultValueWithoutApplying(def,i);
                        } else if (isBool) {
                            bool def;
                            ar & ::boost::serialization::make_nvp("DefaultValue",def);
                            isBool->setDefaultValueWithoutApplying(def,i);
                        } else if (isStr) {
                            std::string def;
                            ar & ::boost::serialization::make_nvp("DefaultValue",def);
                            isStr->setDefaultValueWithoutApplying(def,i);
                        }
                    }
                }
                
            }
        }
        
    } // load
    
    BOOST_SERIALIZATION_SPLIT_MEMBER()

public:

    ///Constructor used to serialize
    explicit KnobSerialization(const boost::shared_ptr<KnobI> & knob)
        : _knob()
        , _dimension(0)
        , _masterIsAlias(false)
        , _extraData(NULL)
        , _isUserKnob(false)
        , _label()
        , _triggerNewLine(false)
        , _evaluatesOnChange(false)
        , _isPersistent(false)
        , _animationEnabled(false)
        , _tooltip()
        , _useHostOverlay(false)
    {
        initialize(knob);
    }

    ///Doing the empty param constructor + this function is the same
    ///as calling the constructore above
    void initialize(const boost::shared_ptr<KnobI> & knob)
    {
        
        _knob = knob;
        
        _typeName = knob->typeName();
        _dimension = knob->getDimension();
        
        for (int i = 0; i < _dimension ; ++i) {
            _expressions.push_back(std::make_pair(knob->getExpression(i),knob->isExpressionUsingRetVariable(i)));
        }
        
        _masterIsAlias = knob->getAliasMaster().get() != 0;
        
        _isUserKnob = knob->isUserKnob();
        _label = knob->getLabel();
        _triggerNewLine = knob->isNewLineActivated();
        _evaluatesOnChange = knob->getEvaluateOnChange();
        _isPersistent = knob->getIsPersistant();
        _animationEnabled = knob->isAnimationEnabled();
        _tooltip = knob->getHintToolTip();
        
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>( _knob.get() );
        if (isChoice) {
            ChoiceExtraData* extraData = new ChoiceExtraData;
            extraData->_entries = isChoice->getEntries_mt_safe();
            extraData->_helpStrings = isChoice->getEntriesHelp_mt_safe();
            int idx = isChoice->getValue();
            if (idx >= 0 && idx < (int)extraData->_entries.size()) {
                extraData->_choiceString = extraData->_entries[idx];
            }
            _extraData = extraData;
        }
        if (_isUserKnob) {
            KnobString* isString = dynamic_cast<KnobString*>(_knob.get());
            if (isString) {
                TextExtraData* extraData = new TextExtraData;
                extraData->label = isString->isLabel();
                extraData->multiLine = isString->isMultiLine();
                extraData->richText = isString->usesRichText();
                _extraData = extraData;
            }
            KnobDouble* isDbl = dynamic_cast<KnobDouble*>(_knob.get());
            KnobInt* isInt = dynamic_cast<KnobInt*>(_knob.get());
            KnobColor* isColor = dynamic_cast<KnobColor*>(_knob.get());
            if (isDbl || isInt || isColor) {
                ValueExtraData* extraData = new ValueExtraData;
                if (isDbl) {
                    extraData->min = isDbl->getMinimum();
                    extraData->max = isDbl->getMaximum();
                    extraData->dmin = isDbl->getDisplayMinimum();
                    extraData->dmax = isDbl->getDisplayMaximum();
                } else if (isInt) {
                    extraData->min = isInt->getMinimum();
                    extraData->max = isInt->getMaximum();
                    extraData->dmin = isInt->getDisplayMinimum();
                    extraData->dmax = isInt->getDisplayMaximum();
                } else if (isColor) {
                    extraData->min = isColor->getMinimum();
                    extraData->max = isColor->getMaximum();
                    extraData->dmin = isColor->getDisplayMinimum();
                    extraData->dmax = isColor->getDisplayMaximum();
                }
                _extraData = extraData;
            }
            
            KnobFile* isFile = dynamic_cast<KnobFile*>(_knob.get());
            KnobOutputFile* isOutFile = dynamic_cast<KnobOutputFile*>(_knob.get());
            if (isFile || isOutFile) {
                FileExtraData* extraData = new FileExtraData;
                extraData->useSequences = isFile ? isFile->isInputImageFile() : isOutFile->isOutputImageFile();
                _extraData = extraData;
            }
            
            KnobPath* isPath = dynamic_cast<KnobPath*>(_knob.get());
            if (isPath) {
                PathExtraData* extraData = new PathExtraData;
                extraData->multiPath = isPath->isMultiPath();
                _extraData = extraData;
            }
        }
    }

    ///Constructor used to deserialize: It will try to deserialize the next knob in the archive
    ///into a knob of the holder. If it couldn't find a knob with the same name as it was serialized
    ///this the deserialization will not succeed.
    KnobSerialization()
        : _knob()
        , _dimension(0)
        , _extraData(NULL)
        , _isUserKnob(false)
        , _label()
        , _triggerNewLine(false)
        , _evaluatesOnChange(false)
        , _isPersistent(false)
        , _animationEnabled(false)
        , _tooltip()
        , _useHostOverlay(false)
    {
    }

    ~KnobSerialization() { delete _extraData; }
    
    /**
     * @brief This function cannot be called until all knobs of the project have been created.
     **/
    void restoreKnobLinks(const boost::shared_ptr<KnobI> & knob,
                          const std::list<boost::shared_ptr<Node> > & allNodes,
                          const std::map<std::string,std::string>& oldNewScriptNamesMapping);
    
    /**
     * @brief This function cannot be called until all knobs of the project have been created.
     **/
    void restoreExpressions(const boost::shared_ptr<KnobI> & knob,
                            const std::map<std::string,std::string>& oldNewScriptNamesMapping);

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL
    {
        return _knob;
    }

    virtual const std::string& getName() const OVERRIDE FINAL
    {
        return _knob->getName();
    }

    static boost::shared_ptr<KnobI> createKnob(const std::string & typeName,int dimension);

    void restoreTracks(const boost::shared_ptr<KnobI> & knob,const std::list<boost::shared_ptr<Node> > & allNodes);

    const TypeExtraData* getExtraData() const { return _extraData; }
    
    bool isPersistent() const {
        return _isPersistent;
    }
    
    bool getEvaluatesOnChange() const {
        return _evaluatesOnChange;
    }
    
    bool isAnimationEnabled() const {
        return _animationEnabled;
    }
    
    bool triggerNewLine() const {
        return _triggerNewLine;
    }
    
    bool isUserKnob() const {
        return _isUserKnob;
    }
    
    std::string getLabel() const {
        return _label;
    }
    
    std::string getHintToolTip() const {
        return _tooltip;
    }
    
    bool getUseHostOverlayHandle() const
    {
        return _useHostOverlay;
    }
    
private:
};



template <typename T>
boost::shared_ptr<KnobSerialization> createDefaultValueForParam(const std::string& paramName,const T& value)
{
    boost::shared_ptr< Knob<T> > knob(new Knob<T>(NULL, paramName, 1, false));
    knob->populate();
    knob->setName(paramName);
    knob->setValue(value,0);
    boost::shared_ptr<KnobSerialization> ret(new KnobSerialization(knob));
    return ret;
}

///Used by Groups and Pages for serialization of User knobs, to maintain their layout.
class GroupKnobSerialization : public KnobSerializationBase
{
    
    boost::shared_ptr<KnobI> _knob;
    std::list <boost::shared_ptr<KnobSerializationBase> > _children;
    std::string _name,_label;
    bool _secret;
    bool _isSetAsTab; //< only valid for groups
    bool _isOpened; //< only for groups
public:
    
    GroupKnobSerialization(const boost::shared_ptr<KnobI>& knob)
    : _knob(knob)
    , _children()
    , _name()
    , _label()
    , _secret(false)
    , _isSetAsTab(false)
    , _isOpened(false)
    {
        KnobGroup* isGrp = dynamic_cast<KnobGroup*>(knob.get());
        KnobPage* isPage = dynamic_cast<KnobPage*>(knob.get());
        assert(isGrp || isPage);
        
        _name = knob->getName();
        _label = knob->getLabel();
        _secret = _knob->getIsSecret();
        
        if (isGrp) {
            _isSetAsTab = isGrp->isTab();
            _isOpened = isGrp->getValue();
        }
        
        std::vector<boost::shared_ptr<KnobI> > children;
        
        if (isGrp) {
            children = isGrp->getChildren();
        } else {
            children = isPage->getChildren();
        }
        for (U32 i = 0; i < children.size(); ++i) {
            
            if (isPage) {
                ///If page, check that the child is a top level child and not child of a sub-group
                ///otherwise let the sub group register the child
                boost::shared_ptr<KnobI> parent = children[i]->getParentKnob();
                if (parent.get() != isPage) {
                    continue;
                }
            }
            boost::shared_ptr<KnobGroup> isGrp = boost::dynamic_pointer_cast<KnobGroup>(children[i]);
            if (isGrp) {
                boost::shared_ptr<GroupKnobSerialization> serialisation(new GroupKnobSerialization(isGrp));
                _children.push_back(serialisation);
            } else {
                //KnobChoice* isChoice = dynamic_cast<KnobChoice*>(children[i].get());
                //bool copyKnob = false;//isChoice != NULL;
                boost::shared_ptr<KnobSerialization> serialisation(new KnobSerialization(children[i]));
                _children.push_back(serialisation);
            }
        }
    }
    
    GroupKnobSerialization()
    : _knob()
    , _children()
    , _name()
    , _label()
    , _secret(false)
    , _isSetAsTab(false)
    , _isOpened(false)
    {
        
    }
    
    ~GroupKnobSerialization()
    {
        
    }
    
    const std::list <boost::shared_ptr<KnobSerializationBase> >& getChildren() const
    {
        return _children;
    }
    
    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL
    {
        return _knob;
    }

    
    virtual const std::string& getName() const  OVERRIDE FINAL {
        return _name;
    }
    
    const std::string& getLabel() const {
        return _label;
    }
    
    bool isSecret() const
    {
        return _secret;
    }
    
    bool isOpened() const {
        return _isOpened;
    }
    
    bool isSetAsTab() const
    {
        return _isSetAsTab;
    }
    
private:
    
    friend class ::boost::serialization::access;
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        assert(_knob);
        ar & ::boost::serialization::make_nvp("Name",_name);
        ar & ::boost::serialization::make_nvp("Label",_label);
        ar & ::boost::serialization::make_nvp("Secret",_secret);
        ar & ::boost::serialization::make_nvp("IsTab",_isSetAsTab);
        ar & ::boost::serialization::make_nvp("IsOpened",_isOpened);
        
        int nbChildren = (int)_children.size();
        ar & ::boost::serialization::make_nvp("NbChildren",nbChildren);
        for (std::list <boost::shared_ptr<KnobSerializationBase> >::const_iterator it = _children.begin();
             it != _children.end(); ++it) {
            GroupKnobSerialization* isGrp = dynamic_cast<GroupKnobSerialization*>(it->get());
            KnobSerialization* isRegularKnob = dynamic_cast<KnobSerialization*>(it->get());
            assert(isGrp || isRegularKnob);
            
            std::string type;
            if (isGrp) {
                type = "Group";
            } else {
                type = "Regular";
            }
            ar & ::boost::serialization::make_nvp("Type",type);
            if (isGrp) {
                ar & ::boost::serialization::make_nvp("item",*isGrp);
            } else {
                ar & ::boost::serialization::make_nvp("item",*isRegularKnob);
            }
        }
        
    }
    
    template<class Archive>
    void load(Archive & ar,
              const unsigned int /*version*/)
    {
        assert(!_knob);
        ar & ::boost::serialization::make_nvp("Name",_name);
        ar & ::boost::serialization::make_nvp("Label",_label);
        ar & ::boost::serialization::make_nvp("Secret",_secret);
        ar & ::boost::serialization::make_nvp("IsTab",_isSetAsTab);
        ar & ::boost::serialization::make_nvp("IsOpened",_isOpened);
        
        int nbChildren;
        ar & ::boost::serialization::make_nvp("NbChildren",nbChildren);
        for (int i = 0; i < nbChildren; ++i) {
    
            std::string type;
            ar & ::boost::serialization::make_nvp("Type",type);
            
            if (type == "Group") {
                boost::shared_ptr<GroupKnobSerialization> knob(new GroupKnobSerialization);
                ar & ::boost::serialization::make_nvp("item",*knob);
                _children.push_back(knob);
            } else {
                boost::shared_ptr<KnobSerialization> knob(new KnobSerialization);
                ar & ::boost::serialization::make_nvp("item",*knob);
                _children.push_back(knob);
            }
        }

        
        
    } // load
    
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

NATRON_NAMESPACE_EXIT;

BOOST_CLASS_VERSION(NATRON_NAMESPACE::KnobSerialization, KNOB_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(NATRON_NAMESPACE::ValueSerialization, VALUE_SERIALIZATION_VERSION)

#endif // KNOBSERIALIZATION_H
