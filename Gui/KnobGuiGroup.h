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

#ifndef _Gui_KnobGuiGroup_h_
#define _Gui_KnobGuiGroup_h_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <vector> // KnobGuiInt
#include <list>
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QObject>
#include <QStyledItemDelegate>
#include <QTextEdit>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"

#include "Engine/Singleton.h"
#include "Engine/Knob.h"
#include "Engine/ImageComponents.h"

#include "Gui/CurveSelection.h"
#include "Gui/KnobGui.h"
#include "Gui/AnimatedCheckBox.h"
#include "Gui/Label.h"

//Define this if you want the spinbox to clamp to the plugin defined range
//#define SPINBOX_TAKE_PLUGIN_RANGE_INTO_ACCOUNT

// Qt
class QString;
class QFrame;
class QHBoxLayout;
class QTreeWidget;
class QTreeWidgetItem;
class QScrollArea;
class QFontComboBox;

// Engine
class KnobI;
class KnobInt;
class KnobBool;
class KnobDouble;
class KnobButton;
class KnobSeparator;
class KnobGroup;
class KnobParametric;
class KnobColor;
class KnobChoice;
class KnobString;

// Gui
class DockablePanel;
class LineEdit;
class Button;
class SpinBox;
class ComboBox;
class ScaleSliderQWidget;
class CurveWidget;
class KnobCurveGui;
class TabGroup;

// private classes, defined in KnobGuiTypes.cpp
namespace Natron {
class GroupBoxLabel;
class ClickableLabel;
}
class AnimatedCheckBox;

namespace Natron {
class Node;
}


class KnobGuiGroup
    : public KnobGui
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    static KnobGui * BuildKnobGui(boost::shared_ptr<KnobI> knob,
                                  DockablePanel *container)
    {
        return new KnobGuiGroup(knob, container);
    }

    KnobGuiGroup(boost::shared_ptr<KnobI> knob,
                  DockablePanel *container);

    virtual ~KnobGuiGroup() OVERRIDE;

    virtual void removeSpecificGui() OVERRIDE FINAL;
    
    void addKnob(KnobGui *child);
    
    const std::list<KnobGui*>& getChildren() const { return _children; }

    bool isChecked() const;

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;
    
    TabGroup* getOrCreateTabWidget();
    
    void removeTabWidget();

public Q_SLOTS:
    void setChecked(bool b);

private:
    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void _show() OVERRIDE FINAL;
    virtual void setEnabled()  OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;
    virtual void setDirty(bool /*dirty*/) OVERRIDE FINAL
    {
    }

    virtual bool eventFilter(QObject *target, QEvent* e) OVERRIDE FINAL;
    virtual void setReadOnly(bool /*readOnly*/,
                             int /*dimension*/) OVERRIDE FINAL
    {
    }

private:
    bool _checked;
    Natron::GroupBoxLabel *_button;
    std::list<KnobGui*> _children;
    std::vector< std::pair<KnobGui*,std::vector<int> > > _childrenToEnable; //< when re-enabling a group, what are the children that we should set
    TabGroup* _tabGroup;
    //enabled too
    boost::weak_ptr<KnobGroup> _knob;
};


#endif // _Gui_KnobGuiGroup_h_
