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

#ifndef Gui_KnobGuiColor_h
#define Gui_KnobGuiColor_h

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

class KnobGuiColor;
class ColorPickerLabel
    : public Natron::Label
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    ColorPickerLabel(KnobGuiColor* knob,QWidget* parent = NULL);

    virtual ~ColorPickerLabel() OVERRIDE
    {
    }
    
    bool isPickingEnabled() const
    {
        return _pickingEnabled;
    }

    void setColor(const QColor & color);

    const QColor& getCurrentColor() const
    {
        return _currentColor;
    }

    void setPickingEnabled(bool enabled);
    
    void setEnabledMode(bool enabled);

Q_SIGNALS:

    void pickingEnabled(bool);

private:

    virtual void enterEvent(QEvent*) OVERRIDE FINAL;
    virtual void leaveEvent(QEvent*) OVERRIDE FINAL;
    virtual void mousePressEvent(QMouseEvent*) OVERRIDE FINAL;

private:

    bool _pickingEnabled;
    QColor _currentColor;
    KnobGuiColor* _knob;
};


class KnobGuiColor
    : public KnobGui
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new KnobGuiColor(knob, container);
    }

    KnobGuiColor(boost::shared_ptr<KnobI> knob,
                  DockablePanel *container);

    virtual ~KnobGuiColor() OVERRIDE;
    
    virtual void removeSpecificGui() OVERRIDE FINAL;

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

    bool getAllDimensionsVisible() const;

    int getDimensionForSpinBox(const SpinBox* spinbox) const;
    
public Q_SLOTS:

    void onColorChanged();
    void onMinMaxChanged(double mini, double maxi, int index);
    void onDisplayMinMaxChanged(double mini, double maxi, int index);

    void showColorDialog();

    void setPickingEnabled(bool enabled);


    void onPickingEnabled(bool enabled);

    void onDimensionSwitchClicked();

    void onSliderValueChanged(double v);
    
    void onSliderEditingFinished(bool hasMovedOnce);

    void onMustShowAllDimension();

    void onDialogCurrentColorChanged(const QColor & color);

Q_SIGNALS:

    void dimensionSwitchToggled(bool b);

private:

    void expandAllDimensions();
    void foldAllDimensions();

    virtual bool shouldAddStretch() const OVERRIDE { return false; }
    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setReadOnly(bool readOnly,int dimension) OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;
    virtual void setDirty(bool dirty) OVERRIDE FINAL;
    virtual void reflectAnimationLevel(int dimension,Natron::AnimationLevelEnum level) OVERRIDE FINAL;
    virtual void reflectExpressionState(int dimension,bool hasExpr) OVERRIDE FINAL;
    virtual void updateToolTip() OVERRIDE FINAL;
    virtual void reflectModificationsState() OVERRIDE FINAL;
    
    void updateLabel(double r, double g, double b, double a);

private:
    QWidget *mainContainer;
    QHBoxLayout *mainLayout;
    QWidget *boxContainers;
    QHBoxLayout *boxLayout;
    QWidget *colorContainer;
    QHBoxLayout *colorLayout;
    Natron::Label *_rLabel;
    Natron::Label *_gLabel;
    Natron::Label *_bLabel;
    Natron::Label *_aLabel;
    SpinBox *_rBox;
    SpinBox *_gBox;
    SpinBox *_bBox;
    SpinBox *_aBox;
    ColorPickerLabel *_colorLabel;
    Button *_colorDialogButton;
    Button *_dimensionSwitchButton;
    ScaleSliderQWidget* _slider;
    int _dimension;
    boost::weak_ptr<KnobColor> _knob;
    std::vector<double> _lastColor;
};

#endif // Gui_KnobGuiColor_h
