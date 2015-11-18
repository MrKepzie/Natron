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

#include "Gui/KnobGui.h"
#include "Gui/KnobGuiPrivate.h"

#include <cassert>

#include "Gui/KnobUndoCommand.h" // SetExpressionCommand...

using namespace Natron;



void
KnobGui::onCreateMasterOnGroupActionTriggered()
{
    KnobHolder* holder = getKnob()->getHolder();
    EffectInstance* isEffect = dynamic_cast<EffectInstance*>(holder);
    assert(isEffect);
    if (!isEffect) {
        throw std::logic_error("");
    }
    boost::shared_ptr<NodeCollection> collec = isEffect->getNode()->getGroup();
    NodeGroup* isCollecGroup = dynamic_cast<NodeGroup*>(collec.get());
    assert(isCollecGroup);
    if (isCollecGroup) {
        createDuplicateOnNode(isCollecGroup, true);
    }
}

void
KnobGui::onUnlinkActionTriggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (!action) {
        return;
    }
    int dim = action->data().toInt();
    boost::shared_ptr<KnobI> thisKnob = getKnob();
    int dims = thisKnob->getDimension();
    
    thisKnob->beginChanges();
    for (int i = 0; i < dims; ++i) {
        if (dim == -1 || i == dim) {
            std::pair<int,boost::shared_ptr<KnobI> > other = thisKnob->getMaster(i);
            thisKnob->onKnobUnSlaved(i);
            onKnobSlavedChanged(i, false);
        }
    }
    thisKnob->endChanges();
    getKnob()->getHolder()->getApp()->triggerAutoSave();
}

void
KnobGui::onSetExprActionTriggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    assert(action);
    
    int dim = action->data().toInt();
    
  
    EditExpressionDialog* dialog = new EditExpressionDialog(dim,this,_imp->container);
    dialog->create(getKnob()->getExpression(dim == -1 ? 0 : dim).c_str(), true);
    QObject::connect( dialog,SIGNAL( accepted() ),this,SLOT( onEditExprDialogFinished() ) );
    QObject::connect( dialog,SIGNAL( rejected() ),this,SLOT( onEditExprDialogFinished() ) );
    
    dialog->show();
    
}

void
KnobGui::onClearExprActionTriggered()
{
    QAction* act = qobject_cast<QAction*>(sender());
    assert(act);
    int dim = act->data().toInt();
    pushUndoCommand(new SetExpressionCommand(getKnob(),false,dim,""));
}

void
KnobGui::onEditExprDialogFinished()
{
    
    EditExpressionDialog* dialog = dynamic_cast<EditExpressionDialog*>( sender() );
    
    if (dialog) {
        QDialog::DialogCode ret = (QDialog::DialogCode)dialog->result();
        
        switch (ret) {
            case QDialog::Accepted: {
                bool hasRetVar;
                QString expr = dialog->getExpression(&hasRetVar);
                std::string stdExpr = expr.toStdString();
                int dim = dialog->getDimension();
                std::string existingExpr = getKnob()->getExpression(dim);
                if (existingExpr != stdExpr) {
                    pushUndoCommand(new SetExpressionCommand(getKnob(),hasRetVar,dim,stdExpr));
                }
            } break;
            case QDialog::Rejected:
                break;
        }
        
        dialog->deleteLater();

    }
}

void
KnobGui::setSecret()
{
    bool showit = !isSecretRecursive();
    if (showit) {
        show(); //
    } else {
        hide();
    }
    KnobGuiGroup* isGrp = dynamic_cast<KnobGuiGroup*>(this);
    if (isGrp) {
        const std::list<KnobGui*>& children = isGrp->getChildren();
        for (std::list<KnobGui*>::const_iterator it = children.begin(); it != children.end(); ++it) {
            (*it)->setSecret();
        }
    }
}

bool
KnobGui::isSecretRecursive() const
{
    // If the Knob is within a group, only show it if the group is unfolded!
    // To test it:
    // try TuttlePinning: fold all groups, then switch from perspective to affine to perspective.
    //  VISIBILITY is different from SECRETNESS. The code considers that both things are equivalent, which is wrong.
    // Of course, this check has to be *recursive* (in case the group is within a folded group)
    boost::shared_ptr<KnobI> knob = getKnob();
    bool showit = !knob->getIsSecret();
    boost::shared_ptr<KnobI> parentKnob = knob->getParentKnob();
    
    while (showit && parentKnob && parentKnob->typeName() == "Group") {
        KnobGuiGroup* parentGui = dynamic_cast<KnobGuiGroup*>( _imp->container->getKnobGui(parentKnob) );
        assert(parentGui);
        // check for secretness and visibility of the group
        if ( parentKnob->getIsSecret() || ( parentGui && !parentGui->isChecked() ) ) {
            showit = false; // one of the including groups is folder, so this item is hidden
        }
        // prepare for next loop iteration
        parentKnob = parentKnob->getParentKnob();
    }
    return !showit;
}

void
KnobGui::showAnimationMenu()
{
    createAnimationMenu(_imp->animationMenu,-1);
    _imp->animationMenu->exec( _imp->animationButton->mapToGlobal( QPoint(0,0) ) );
}

void
KnobGui::onShowInCurveEditorActionTriggered()
{
    boost::shared_ptr<KnobI> knob = getKnob();

    assert( knob->getHolder()->getApp() );
    getGui()->setCurveEditorOnTop();
    std::vector<boost::shared_ptr<Curve> > curves;
    for (int i = 0; i < knob->getDimension(); ++i) {
        boost::shared_ptr<Curve> c = getCurve(i);
        if ( c->isAnimated() ) {
            curves.push_back(c);
        }
    }
    if ( !curves.empty() ) {
        getGui()->getCurveEditor()->centerOn(curves);
    }
}

void
KnobGui::onRemoveAnimationActionTriggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    assert(action);
    int dim = action->data().toInt();
    
    boost::shared_ptr<KnobI> knob = getKnob();
    std::map<boost::shared_ptr<CurveGui> , std::vector<KeyFrame > > toRemove;
    
    
    for (int i = 0; i < knob->getDimension(); ++i) {
        
        if (dim == -1 || dim == i) {
            std::list<boost::shared_ptr<CurveGui> > curves = getGui()->getCurveEditor()->findCurve(this, i);
            for (std::list<boost::shared_ptr<CurveGui> >::iterator it = curves.begin(); it != curves.end(); ++it) {
                KeyFrameSet keys = (*it)->getInternalCurve()->getKeyFrames_mt_safe();
                
                std::vector<KeyFrame > vect;
                for (KeyFrameSet::const_iterator it2 = keys.begin(); it2 != keys.end(); ++it2) {
                    vect.push_back(*it2);
                }
                toRemove.insert(std::make_pair(*it, vect));
            }
            
        }
    }
    pushUndoCommand( new RemoveKeysCommand(getGui()->getCurveEditor()->getCurveWidget(),
                                           toRemove) );
    //refresh the gui so it doesn't indicate the parameter is animated anymore
    updateGUI(dim);
}

void
KnobGui::setInterpolationForDimensions(const std::vector<int> & dimensions,
                                       Natron::KeyframeTypeEnum interp)
{
    boost::shared_ptr<KnobI> knob = getKnob();
    
    for (U32 i = 0; i < dimensions.size(); ++i) {
        boost::shared_ptr<Curve> c = knob->getCurve(dimensions[i]);
        if (c) {
            int kfCount = c->getKeyFramesCount();
            for (int j = 0; j < kfCount; ++j) {
                c->setKeyFrameInterpolation(interp, j);
            }
            boost::shared_ptr<Curve> guiCurve = getCurve(dimensions[i]);
            if (guiCurve) {
                guiCurve->clone(*c);
            }
        }
    }
    if (knob->getHolder()) {
        knob->getHolder()->evaluate_public(knob.get(), knob->getEvaluateOnChange(), Natron::eValueChangedReasonNatronGuiEdited);
    }
    Q_EMIT keyInterpolationChanged();

}

void
KnobGui::onConstantInterpActionTriggered()
{
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<int> dims;

    for (int i = 0; i < knob->getDimension(); ++i) {
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims, Natron::eKeyframeTypeConstant);
}

void
KnobGui::onLinearInterpActionTriggered()
{
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<int> dims;

    for (int i = 0; i < knob->getDimension(); ++i) {
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims, Natron::eKeyframeTypeLinear);
}

void
KnobGui::onSmoothInterpActionTriggered()
{
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<int> dims;

    for (int i = 0; i < knob->getDimension(); ++i) {
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims, Natron::eKeyframeTypeSmooth);
}

void
KnobGui::onCatmullromInterpActionTriggered()
{
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<int> dims;

    for (int i = 0; i < knob->getDimension(); ++i) {
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims, Natron::eKeyframeTypeCatmullRom);
}

void
KnobGui::onCubicInterpActionTriggered()
{
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<int> dims;

    for (int i = 0; i < knob->getDimension(); ++i) {
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims, Natron::eKeyframeTypeCubic);
}

void
KnobGui::onHorizontalInterpActionTriggered()
{
    boost::shared_ptr<KnobI> knob = getKnob();
    std::vector<int> dims;

    for (int i = 0; i < knob->getDimension(); ++i) {
        dims.push_back(i);
    }
    setInterpolationForDimensions(dims, Natron::eKeyframeTypeHorizontal);
}

void
KnobGui::setKeyframe(double time,
                     int dimension)
{
    boost::shared_ptr<KnobI> knob = getKnob();

    assert( knob->getHolder()->getApp() );
    
    bool keyAdded = knob->onKeyFrameSet(time, dimension);
    
    Q_EMIT keyFrameSet();
    
    if ( !knob->getIsSecret() && keyAdded && knob->isDeclaredByPlugin()) {
        knob->getHolder()->getApp()->getTimeLine()->addKeyframeIndicator(time);
    }
}

void
KnobGui::setKeyframe(double time,const KeyFrame& key,int dimension)
{
    boost::shared_ptr<KnobI> knob = getKnob();
    
    assert( knob->getHolder()->getApp() );
    
    bool keyAdded = knob->onKeyFrameSet(time, key, dimension);
    
    Q_EMIT keyFrameSet();
    if ( !knob->getIsSecret() && keyAdded && knob->isDeclaredByPlugin() ) {
        knob->getHolder()->getApp()->getTimeLine()->addKeyframeIndicator(time);
    }
}

void
KnobGui::onSetKeyActionTriggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    assert(action);
    int dim = action->data().toInt();

    boost::shared_ptr<KnobI> knob = getKnob();

    assert( knob->getHolder()->getApp() );
    //get the current time on the global timeline
    SequenceTime time = knob->getHolder()->getApp()->getTimeLine()->currentFrame();

    AddKeysCommand::KeysToAddList toAdd;
    
    for (int i = 0; i < knob->getDimension(); ++i) {
        
        if (dim == -1 || i == dim) {
            
            std::list<boost::shared_ptr<CurveGui> > curves = getGui()->getCurveEditor()->findCurve(this, i);
            for (std::list<boost::shared_ptr<CurveGui> >::iterator it = curves.begin(); it != curves.end(); ++it) {
                KeyFrame kf;
                kf.setTime(time);
                Knob<int>* isInt = dynamic_cast<Knob<int>*>( knob.get() );
                Knob<bool>* isBool = dynamic_cast<Knob<bool>*>( knob.get() );
                AnimatingKnobStringHelper* isString = dynamic_cast<AnimatingKnobStringHelper*>( knob.get() );
                Knob<double>* isDouble = dynamic_cast<Knob<double>*>( knob.get() );
                
                if (isInt) {
                    kf.setValue( isInt->getValue(i) );
                } else if (isBool) {
                    kf.setValue( isBool->getValue(i) );
                } else if (isDouble) {
                    kf.setValue( isDouble->getValue(i) );
                } else if (isString) {
                    std::string v = isString->getValue(i);
                    double dv;
                    isString->stringToKeyFrameValue(time, v, &dv);
                    kf.setValue(dv);
                }
                
                std::vector<KeyFrame> kvec;
                kvec.push_back(kf);
                toAdd.insert(std::make_pair(*it,kvec));
            }
        }
    }
    pushUndoCommand( new AddKeysCommand(getGui()->getCurveEditor()->getCurveWidget(), toAdd) );
}

void
KnobGui::removeKeyFrame(double time,
                        int dimension)
{
    boost::shared_ptr<KnobI> knob = getKnob();
    knob->onKeyFrameRemoved(time, dimension);
    Q_EMIT keyFrameRemoved();
    

    assert( knob->getHolder()->getApp() );
    if ( !knob->getIsSecret() ) {
        knob->getHolder()->getApp()->getTimeLine()->removeKeyFrameIndicator(time);
    }
    updateGUI(dimension);
}

void
KnobGui::setKeyframes(const std::vector<KeyFrame>& keys, int dimension)
{
    boost::shared_ptr<KnobI> knob = getKnob();
    
    assert( knob->getHolder()->getApp() );
    
    std::list<SequenceTime> times;
    for (std::size_t i = 0; i < keys.size(); ++i) {
        bool keyAdded = knob->onKeyFrameSet(keys[i].getTime(), keys[i], dimension);
        if (keyAdded) {
            times.push_back(keys[i].getTime());
        }
    }
    Q_EMIT keyFrameSet();
    if ( !knob->getIsSecret() && knob->isDeclaredByPlugin() ) {
        knob->getHolder()->getApp()->getTimeLine()->addMultipleKeyframeIndicatorsAdded(times, true);
    }
}

void
KnobGui::removeKeyframes(const std::vector<KeyFrame>& keys, int dimension)
{
    boost::shared_ptr<KnobI> knob = getKnob();
    for (std::size_t i = 0; i < keys.size(); ++i) {
        knob->onKeyFrameRemoved(keys[i].getTime(), dimension);
    }
    
    /*assert( knob->getHolder()->getApp() );
    if ( !knob->getIsSecret() ) {
        std::list<SequenceTime> times;
        for (std::size_t i = 0; i < keys.size(); ++i) {
            times.push_back(keys[i].getTime());
        }
        knob->getHolder()->getApp()->getTimeLine()->removeMultipleKeyframeIndicator(times, true);
    }*/

    Q_EMIT keyFrameRemoved();
    updateGUI(dimension);
}

QString
KnobGui::getScriptNameHtml() const
{
    return  QString("<font size = 4><b>%1</b></font>").arg( getKnob()->getName().c_str() );
}

QString
KnobGui::toolTip() const
{
    boost::shared_ptr<KnobI> knob = getKnob();
    KnobChoice* isChoice = dynamic_cast<KnobChoice*>(knob.get());
    QString tt = getScriptNameHtml();
    
    QString realTt;
    if (!isChoice) {
        realTt.append( knob->getHintToolTip().c_str() );
    } else {
        realTt.append( isChoice->getHintToolTipFull().c_str() );
    }
    
    std::vector<std::string> expressions;
    bool exprAllSame = true;
    for (int i = 0; i < knob->getDimension(); ++i) {
        expressions.push_back(knob->getExpression(i));
        if (i > 0 && expressions[i] != expressions[0]) {
            exprAllSame = false;
        }
    }
    
    QString exprTt;
    if (exprAllSame) {
        if (!expressions[0].empty()) {
            exprTt = QString("<br>ret = <b>%1</b></br>").arg(expressions[0].c_str());
        }
    } else {
        for (int i = 0; i < knob->getDimension(); ++i) {
            std::string dimName = knob->getDimensionName(i);
            QString toAppend = QString("<br>%1 = <b>%2</b></br>").arg(dimName.c_str()).arg(expressions[i].c_str());
            exprTt.append(toAppend);
        }
    }
    
    if (!exprTt.isEmpty()) {
        tt.append(exprTt);
    }

    if ( !realTt.isEmpty() ) {
        realTt = Natron::convertFromPlainText(realTt.trimmed(), Qt::WhiteSpaceNormal);
        tt.append(realTt);
    }

    return tt;
}

bool
KnobGui::hasToolTip() const
{
    //Always true now that we display the script name in the tooltip
    return true; //!getKnob()->getHintToolTip().empty();
}

void
KnobGui::onRemoveKeyActionTriggered()
{
    QAction* action = qobject_cast<QAction*>(sender());
    assert(action);
    int dim = action->data().toInt();
    
    boost::shared_ptr<KnobI> knob = getKnob();
    
    assert( knob->getHolder()->getApp() );
    //get the current time on the global timeline
    SequenceTime time = knob->getHolder()->getApp()->getTimeLine()->currentFrame();
    std::map<boost::shared_ptr<CurveGui> , std::vector<KeyFrame > > toRemove;
    for (int i = 0; i < knob->getDimension(); ++i) {
        
        if (dim == -1 || i == dim) {
            std::list<boost::shared_ptr<CurveGui> > curves = getGui()->getCurveEditor()->findCurve(this, i);
            for (std::list<boost::shared_ptr<CurveGui> >::iterator it = curves.begin(); it != curves.end(); ++it) {
                
                KeyFrame kf;
                bool foundKey = knob->getCurve(i)->getKeyFrameWithTime(time, &kf);
                
                if (foundKey) {
                    std::vector<KeyFrame > vect;
                    vect.push_back(kf);
                    toRemove.insert( std::make_pair(*it,vect) );
                }
            }
            
        }
    }
    pushUndoCommand( new RemoveKeysCommand(getGui()->getCurveEditor()->getCurveWidget(),
                                           toRemove) );
}

int
KnobGui::getKnobsCountOnSameLine() const
{
    return _imp->knobsOnSameLine.size();
}

void
KnobGui::hide()
{
    if (!_imp->customInteract) {
        _hide();
    } else {
        _imp->customInteract->hide();
    }
    if (_imp->animationButton) {
        _imp->animationButton->hide();
    }
    //also  hide the curve from the curve editor if there's any and the knob is not inside a group
    if ( getKnob()->getHolder()->getApp() ) {
        boost::shared_ptr<KnobI> parent = getKnob()->getParentKnob();
        bool isSecret = true;
        while (parent) {
            if (!parent->getIsSecret()) {
                isSecret = false;
                break;
            }
            parent = parent->getParentKnob();
        }
        if (isSecret) {
            getGui()->getCurveEditor()->hideCurves(this);
        }
    }

    ////In order to remove the row of the layout we have to make sure ALL the knobs on the row
    ////are hidden.
    bool shouldRemoveWidget = true;
    for (U32 i = 0; i < _imp->knobsOnSameLine.size(); ++i) {
        KnobGui* sibling = _imp->container->getKnobGui(_imp->knobsOnSameLine[i].lock());
        if ( sibling && !sibling->isSecretRecursive() ) {
            shouldRemoveWidget = false;
        }
    }

    if (shouldRemoveWidget) {
        _imp->field->hide();
        if (_imp->container) {
            _imp->container->refreshTabWidgetMaxHeight();
        }
    } else {
        if (!_imp->field->isVisible()) {
            _imp->field->show();
        }
    }
    if (_imp->descriptionLabel) {
        _imp->descriptionLabel->hide();
    }
    
}

void
KnobGui::show(int /*index*/)
{
    if (!getGui()) {
        return;
    }
    if (!_imp->customInteract) {
        _show();
    } else {
        _imp->customInteract->show();
    }
    if (_imp->animationButton) {
        _imp->animationButton->show();
    }
    //also show the curve from the curve editor if there's any
    if ( getKnob()->getHolder()->getApp() ) {
        getGui()->getCurveEditor()->showCurves(this);
    }

    if (_imp->isOnNewLine) {
        _imp->field->show();
        if (_imp->container) {
            _imp->container->refreshTabWidgetMaxHeight();
        }
    }
    
    if (_imp->descriptionLabel) {
        _imp->descriptionLabel->show();
    }
 
}

int
KnobGui::getActualIndexInLayout() const
{
    if (!_imp->containerLayout) {
        return -1;
    }
    for (int i = 0; i < _imp->containerLayout->rowCount(); ++i) {
        QLayoutItem* item = _imp->containerLayout->itemAtPosition(i, 1);
        if ( item && (item->widget() == _imp->field) ) {
            return i;
        }
    }

    return -1;
}

bool
KnobGui::isOnNewLine() const
{
    return _imp->isOnNewLine;
}

void
KnobGui::setEnabledSlot()
{
    if (!getGui()) {
        return;
    }
    if (!_imp->customInteract) {
        setEnabled();
    }
    boost::shared_ptr<KnobI> knob = getKnob();
    if (_imp->descriptionLabel) {
        _imp->descriptionLabel->setReadOnly( !knob->isEnabled(0) );
    }
    if ( knob->getHolder()->getApp() ) {
        for (int i = 0; i < knob->getDimension(); ++i) {
            if ( !knob->isEnabled(i) ) {
                getGui()->getCurveEditor()->hideCurve(this,i);
            } else {
                getGui()->getCurveEditor()->showCurve(this,i);
            }
        }
    }
}

QWidget*
KnobGui::getFieldContainer() const
{
    return _imp->field;
}
