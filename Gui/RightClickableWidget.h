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

#ifndef _Gui_RightClickableWidget_h_
#define _Gui_RightClickableWidget_h_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <map>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QFrame>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif
#include <QTabWidget>
#include <QDialog>
#include "Global/GlobalDefines.h"

#include "Engine/DockablePanelI.h"

class KnobI;
class KnobGui;
class KnobHolder;
class NodeGui;
class Gui;
class KnobPage;
class QVBoxLayout;
class Button;
class QUndoStack;
class QUndoCommand;
class QGridLayout;
class RotoPanel;
class MultiInstancePanel;
class QTabWidget;
class KnobGroup;
class DockablePanel;

class RightClickableWidget : public QWidget
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    DockablePanel* panel;
    
public:
    
    
    RightClickableWidget(DockablePanel* panel,QWidget* parent)
    : QWidget(parent)
    , panel(panel)
    {
        setObjectName("SettingsPanel");
    }
    
    virtual ~RightClickableWidget() {}
    
    DockablePanel* getPanel() const { return panel; }
    
Q_SIGNALS:
    
    void rightClicked(const QPoint& p);
    void escapePressed();
    
private:
    
    virtual void enterEvent(QEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    
};

#endif // _Gui_RightClickableWidget_h_
