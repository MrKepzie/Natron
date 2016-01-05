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

#ifndef Gui_KnobGuiInt_h
#define Gui_KnobGuiInt_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector> // KnobGuiInt
#include <list>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QObject>
#include <QStyledItemDelegate>
#include <QTextEdit>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/Singleton.h"
#include "Engine/Knob.h"
#include "Engine/ImageComponents.h"
#include "Engine/EngineFwd.h"

#include "Gui/CurveSelection.h"
#include "Gui/KnobGui.h"
#include "Gui/AnimatedCheckBox.h"
#include "Gui/Label.h"
#include "Gui/GuiFwd.h"


//================================

class KnobGuiInt
    : public KnobGui
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new KnobGuiInt(knob, container);
    }

    KnobGuiInt(boost::shared_ptr<KnobI> knob,
                DockablePanel *container);

    virtual ~KnobGuiInt() OVERRIDE;
    
    virtual void removeSpecificGui() OVERRIDE FINAL;

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

    bool getAllDimensionsVisible() const;
    
    int getDimensionForSpinBox(const SpinBox* spinbox) const;
    
public Q_SLOTS:

    void onSpinBoxValueChanged();

    void onSliderValueChanged(double);
    void onSliderEditingFinished(bool hasMovedOnce);

#ifdef SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT
    void onMinMaxChanged(double mini, double maxi, int index = 0);
#endif
    
    void onDisplayMinMaxChanged(double mini,double maxi,int index = 0);

    void onIncrementChanged(int incr, int index = 0);
    
    void onDimensionSwitchClicked();
    
private:
    void expandAllDimensions();
    void foldAllDimensions();
    
    void sliderEditingEnd(double d);

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;

    void setMaximum(int);

    void setMinimum(int);

    virtual bool shouldAddStretch() const OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;
    virtual void setDirty(bool dirty) OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;
    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevelEnum level) OVERRIDE FINAL;
    virtual void reflectExpressionState(int dimension,bool hasExpr) OVERRIDE FINAL;
    virtual void updateToolTip() OVERRIDE FINAL;
    virtual void reflectModificationsState() OVERRIDE FINAL;

private:
    std::vector<std::pair<SpinBox *, Natron::Label *> > _spinBoxes;
    QWidget *_container;
    ScaleSliderQWidget *_slider;
    Button *_dimensionSwitchButton;
    boost::weak_ptr<KnobInt> _knob;
};


#endif // Gui_KnobGuiInt_h
