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

#include "Gui.h"

#include <cassert>
#include <sstream> // stringstream
#include <algorithm> // min, max
#include <map>
#include <list>
#include <utility>

#include "Global/Macros.h"

#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <QApplication> // qApp
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QMenuBar>
#include <QToolButton>
#include <QProgressDialog>
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QTabBar>
#include <QTextEdit>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QTreeView>

#include "Engine/GroupOutput.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h" // NodeList, NodeCollection
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewerInstance.h"

#include "Gui/ActionShortcuts.h"
#include "Gui/CurveEditor.h"
#include "Gui/CurveWidget.h"
#include "Gui/ComboBox.h"
#include "Gui/DockablePanel.h"
#include "Gui/DopeSheetEditor.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h" // appPTR
#include "Gui/GuiPrivate.h"
#include "Gui/LogWindow.h"
#include "Gui/NodeGraph.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/RightClickableWidget.h"
#include "Gui/RotoGui.h"
#include "Gui/ScriptEditor.h"
#include "Gui/ShortCutEditor.h"
#include "Gui/SpinBox.h"
#include "Gui/TabWidget.h"
#include "Gui/ViewerGL.h"
#include "Gui/ViewerTab.h"
#include "Gui/Histogram.h"

using namespace Natron;


void
Gui::setUndoRedoStackLimit(int limit)
{
    _imp->_nodeGraphArea->setUndoRedoStackLimit(limit);
}

void
Gui::showOfxLog()
{
    QString log = appPTR->getOfxLog_mt_safe();
    LogWindow lw(log, this);

    lw.setWindowTitle( tr("Error Log") );
    ignore_result( lw.exec() );
}

void
Gui::createNewTrackerInterface(NodeGui* n)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->createTrackerInterface(n);
    }
}

void
Gui::removeTrackerInterface(NodeGui* n,
                            bool permanently)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->removeTrackerInterface(n, permanently, false);
    }
}

void
Gui::onRotoSelectedToolChanged(int tool)
{
    RotoGui* roto = qobject_cast<RotoGui*>( sender() );

    if (!roto) {
        return;
    }
    QMutexLocker l(&_imp->_viewerTabsMutex);
    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->updateRotoSelectedTool(tool, roto);
    }
}

void
Gui::createNewRotoInterface(NodeGui* n)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->createRotoInterface(n);
    }
}

void
Gui::removeRotoInterface(NodeGui* n,
                         bool permanently)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->removeRotoInterface(n, permanently, false);
    }
}

void
Gui::setRotoInterface(NodeGui* n)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->setRotoInterface(n);
    }
}

void
Gui::onViewerRotoEvaluated(ViewerTab* viewer)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        if (*it != viewer) {
            (*it)->getViewer()->redraw();
        }
    }
}

void
Gui::progressStart(KnobHolder* effect,
                   const std::string &message,
                   const std::string &/*messageid*/,
                   bool canCancel)
{
    if (!effect) {
        return;
    }
    if ( QThread::currentThread() != qApp->thread() ) {
        qDebug() << "Progress bars called from a thread different than the main-thread is not supported at the moment.";

        return;
    }

    QProgressDialog* dialog = new QProgressDialog(message.c_str(), tr("Cancel"), 0, 100, this);
    if (!canCancel) {
        dialog->setCancelButton(0);
    }
    dialog->setModal(false);
    dialog->setRange(0, 100);
    dialog->setMinimumWidth(250);
    NamedKnobHolder* isNamed = dynamic_cast<NamedKnobHolder*>(effect);
    if (isNamed) {
        dialog->setWindowTitle( isNamed->getScriptName_mt_safe().c_str() );
    }
    std::map<KnobHolder*, QProgressDialog*>::iterator found = _imp->_progressBars.find(effect);

    ///If a second dialog was asked for whilst another is still active, the first dialog will not be
    ///able to be canceled.
    if ( found != _imp->_progressBars.end() ) {
        _imp->_progressBars.erase(found);
    }

    _imp->_progressBars.insert( std::make_pair(effect, dialog) );
    dialog->show();
    //dialog->exec();
}

void
Gui::progressEnd(KnobHolder* effect)
{
    if ( QThread::currentThread() != qApp->thread() ) {
        qDebug() << "Progress bars called from a thread different than the main-thread is not supported at the moment.";

        return;
    }

    std::map<KnobHolder*, QProgressDialog*>::iterator found = _imp->_progressBars.find(effect);
    if ( found == _imp->_progressBars.end() ) {
        return;
    }


    found->second->close();
    _imp->_progressBars.erase(found);
}

bool
Gui::progressUpdate(KnobHolder* effect,
                    double t)
{
    if ( QThread::currentThread() != qApp->thread() ) {
        qDebug() << "Progress bars called from a thread different than the main-thread is not supported at the moment.";

        return true;
    }

    std::map<KnobHolder*, QProgressDialog*>::iterator found = _imp->_progressBars.find(effect);
    if ( found == _imp->_progressBars.end() ) {
        NamedKnobHolder* isNamed = dynamic_cast<NamedKnobHolder*>(effect);
        if (isNamed) {
            qDebug() << isNamed->getScriptName_mt_safe().c_str() <<  " called progressUpdate but didn't called progressStart first.";
        }
    } else {
        if ( found->second->wasCanceled() ) {
            return false;
        }
        found->second->setValue(t * 100);
    }
    //QCoreApplication::processEvents();

    return true;
}

void
Gui::addVisibleDockablePanel(DockablePanel* panel)
{
    
    std::list<DockablePanel*>::iterator it = std::find(_imp->openedPanels.begin(), _imp->openedPanels.end(), panel);
    putSettingsPanelFirst(panel);
    if ( it == _imp->openedPanels.end() ) {
        assert(panel);
        int maxPanels = appPTR->getCurrentSettings()->getMaxPanelsOpened();
        if ( ( (int)_imp->openedPanels.size() == maxPanels ) && (maxPanels != 0) ) {
            std::list<DockablePanel*>::iterator it = _imp->openedPanels.begin();
            (*it)->closePanel();
        }
        _imp->openedPanels.push_back(panel);
    }
}

void
Gui::removeVisibleDockablePanel(DockablePanel* panel)
{
    std::list<DockablePanel*>::iterator it = std::find(_imp->openedPanels.begin(), _imp->openedPanels.end(), panel);

    if ( it != _imp->openedPanels.end() ) {
        _imp->openedPanels.erase(it);
    }
}

const std::list<DockablePanel*>&
Gui::getVisiblePanels() const
{
    return _imp->openedPanels;
}

void
Gui::onMaxVisibleDockablePanelChanged(int maxPanels)
{
    assert(maxPanels >= 0);
    if (maxPanels == 0) {
        return;
    }
    while ( (int)_imp->openedPanels.size() > maxPanels ) {
        std::list<DockablePanel*>::iterator it = _imp->openedPanels.begin();
        (*it)->closePanel();
    }
    _imp->_maxPanelsOpenedSpinBox->setValue(maxPanels);
}

void
Gui::onMaxPanelsSpinBoxValueChanged(double val)
{
    appPTR->getCurrentSettings()->setMaxPanelsOpened( (int)val );
}

void
Gui::clearAllVisiblePanels()
{
    while ( !_imp->openedPanels.empty() ) {
        std::list<DockablePanel*>::iterator it = _imp->openedPanels.begin();
        if ( !(*it)->isFloating() ) {
            (*it)->setClosed(true);
        }

        bool foundNonFloating = false;
        for (std::list<DockablePanel*>::iterator it2 = _imp->openedPanels.begin(); it2 != _imp->openedPanels.end(); ++it2) {
            if ( !(*it2)->isFloating() ) {
                foundNonFloating = true;
                break;
            }
        }
        ///only floating windows left
        if (!foundNonFloating) {
            break;
        }
    }
    getApp()->redrawAllViewers();
}

void
Gui::minimizeMaximizeAllPanels(bool clicked)
{
    for (std::list<DockablePanel*>::iterator it = _imp->openedPanels.begin(); it != _imp->openedPanels.end(); ++it) {
        if (clicked) {
            if ( !(*it)->isMinimized() ) {
                (*it)->minimizeOrMaximize(true);
            }
        } else {
            if ( (*it)->isMinimized() ) {
                (*it)->minimizeOrMaximize(false);
            }
        }
    }
    getApp()->redrawAllViewers();
}

void
Gui::connectViewersToViewerCache()
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->connectToViewerCache();
    }
}

void
Gui::disconnectViewersFromViewerCache()
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->disconnectFromViewerCache();
    }
}

void
Gui::moveEvent(QMoveEvent* e)
{
    QMainWindow::moveEvent(e);
    QPoint p = pos();

    setMtSafePosition( p.x(), p.y() );
}


#if 0
bool
Gui::event(QEvent* e)
{
    switch (e->type()) {
        case QEvent::TabletEnterProximity:
        case QEvent::TabletLeaveProximity:
        case QEvent::TabletMove:
        case QEvent::TabletPress:
        case QEvent::TabletRelease:
        {
            QTabletEvent *tEvent = dynamic_cast<QTabletEvent *>(e);
            const std::list<ViewerTab*>& viewers = getViewersList();
            for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it!=viewers.end(); ++it) {
                QPoint widgetPos = (*it)->mapToGlobal((*it)->mapFromParent((*it)->pos()));
                QRect r(widgetPos.x(),widgetPos.y(),(*it)->width(),(*it)->height());
                if (r.contains(tEvent->globalPos())) {
                    QTabletEvent te(tEvent->type()
                                    , mapFromGlobal(tEvent->pos())
                                    , tEvent->globalPos()
                                    , tEvent->hiResGlobalPos()
                                    , tEvent->device()
                                    , tEvent->pointerType()
                                    , tEvent->pressure()
                                    , tEvent->xTilt()
                                    , tEvent->yTilt()
                                    , tEvent->tangentialPressure()
                                    , tEvent->rotation()
                                    , tEvent->z()
                                    , tEvent->modifiers()
                                    , tEvent->uniqueId());
                    qApp->sendEvent((*it)->getViewer(), &te);
                    return true;
                }
            }
            break;
        }
        default:
            break;
    }
    return QMainWindow::event(e);
}
#endif
void
Gui::resizeEvent(QResizeEvent* e)
{
    QMainWindow::resizeEvent(e);

    setMtSafeWindowSize( width(), height() );
}

static RightClickableWidget* isParentSettingsPanelRecursive(QWidget* w)
{
    if (!w) {
        return 0;
    }
    RightClickableWidget* panel = qobject_cast<RightClickableWidget*>(w);
    if (panel) {
        return panel;
    } else {
        return isParentSettingsPanelRecursive(w->parentWidget());
    }
}

void
Gui::keyPressEvent(QKeyEvent* e)
{
    if (_imp->currentPanelFocusEventRecursion > 0) {
        return;
    }
    
    QWidget* w = qApp->widgetAt( QCursor::pos() );

    
    Qt::Key key = (Qt::Key)e->key();
    Qt::KeyboardModifiers modifiers = e->modifiers();

    if (key == Qt::Key_Escape) {
        
        RightClickableWidget* panel = isParentSettingsPanelRecursive(w);
        if (panel) {
            panel->getPanel()->closePanel();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevious, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->previousFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerForward, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->toggleStartForward();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerBackward, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->toggleStartBackward();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerStop, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->abortRendering();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNext, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->nextFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerFirst, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->firstFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerLast, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->lastFrame();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevIncr, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->previousIncrement();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextIncr, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->nextIncrement();
        }
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevKF, modifiers, key) ) {
        getApp()->getTimeLine()->goToPreviousKeyframe();
    } else if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextKF, modifiers, key) ) {
        getApp()->getTimeLine()->goToNextKeyframe();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphDisableNodes, modifiers, key) ) {
        _imp->_nodeGraphArea->toggleSelectedNodesEnabled();
    } else if ( isKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphFindNode, modifiers, key) ) {
        _imp->_nodeGraphArea->popFindDialog();
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput1, modifiers, key) ) {
        connectInput(0);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput2, modifiers, key) ) {
        connectInput(1);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput3, modifiers, key) ) {
        connectInput(2);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput4, modifiers, key) ) {
        connectInput(3);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput5, modifiers, key) ) {
        connectInput(4);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput6, modifiers, key) ) {
        connectInput(5);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput7, modifiers, key) ) {
        connectInput(6);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput8, modifiers, key) ) {
        connectInput(7);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput9, modifiers, key) ) {
        connectInput(8);
    } else if (isKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput10, modifiers, key) ) {
        connectInput(9);
    } else {
        if (_imp->currentPanelFocus) {
            
            ++_imp->currentPanelFocusEventRecursion;
            //If a panel as the click focus, try to send the event to it
            QWidget* curFocusWidget = _imp->currentPanelFocus->getWidget();
            assert(curFocusWidget);
            QKeyEvent* ev = new QKeyEvent(QEvent::KeyPress, key, modifiers);
            qApp->notify(curFocusWidget,ev);
            --_imp->currentPanelFocusEventRecursion;
        } else {
            QMainWindow::keyPressEvent(e);
        }
    }
}

TabWidget*
Gui::getAnchor() const
{
    QMutexLocker l(&_imp->_panesMutex);

    for (std::list<TabWidget*>::const_iterator it = _imp->_panes.begin(); it != _imp->_panes.end(); ++it) {
        if ( (*it)->isAnchor() ) {
            return *it;
        }
    }

    return NULL;
}

bool
Gui::isGUIFrozen() const
{
    QMutexLocker k(&_imp->_isGUIFrozenMutex);

    return _imp->_isGUIFrozen;
}

void
Gui::onFreezeUIButtonClicked(bool clicked)
{
    {
        QMutexLocker k(&_imp->_isGUIFrozenMutex);
        if (_imp->_isGUIFrozen == clicked) {
            return;
        }
        _imp->_isGUIFrozen = clicked;
    }
    {
        QMutexLocker k(&_imp->_viewerTabsMutex);
        for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
            (*it)->setTurboButtonDown(clicked);
            if (!clicked) {
                (*it)->getViewer()->redraw(); //< overlays were disabled while frozen, redraw to make them re-appear
            }
        }
    }
    _imp->_nodeGraphArea->onGuiFrozenChanged(clicked);
    for (std::list<NodeGraph*>::iterator it = _imp->_groups.begin(); it != _imp->_groups.end(); ++it) {
        (*it)->onGuiFrozenChanged(clicked);
    }
}

bool
Gui::hasShortcutEditorAlreadyBeenBuilt() const
{
    return _imp->shortcutEditor != NULL;
}

void
Gui::addShortcut(BoundAction* action)
{
    assert(_imp->shortcutEditor);
    _imp->shortcutEditor->addShortcut(action);
}

void
Gui::getNodesEntitledForOverlays(std::list<boost::shared_ptr<Natron::Node> > & nodes) const
{
    int layoutItemsCount = _imp->_layoutPropertiesBin->count();

    for (int i = 0; i < layoutItemsCount; ++i) {
        QLayoutItem* item = _imp->_layoutPropertiesBin->itemAt(i);
        NodeSettingsPanel* panel = dynamic_cast<NodeSettingsPanel*>( item->widget() );
        if (!panel) {
            continue;
        }
        boost::shared_ptr<NodeGui> node = panel->getNode();
        boost::shared_ptr<Natron::Node> internalNode = node->getNode();
        if (node && internalNode) {
            boost::shared_ptr<MultiInstancePanel> multiInstance = node->getMultiInstancePanel();
            if (multiInstance) {
                if ( internalNode->hasOverlay() &&
                    !internalNode->isNodeDisabled() &&
                    node->isSettingsPanelVisible() &&
                    !node->isSettingsPanelMinimized() ) {
                    nodes.push_back( node->getNode() );
                }
            } else {
                if ( ( internalNode->hasOverlay() || internalNode->getRotoContext() ) &&
                    !internalNode->isNodeDisabled() &&
                    !internalNode->getParentMultiInstance() &&
                    internalNode->isActivated() &&
                    node->isSettingsPanelVisible() &&
                    !node->isSettingsPanelMinimized() ) {
                    nodes.push_back( node->getNode() );
                }
            }
        }
        
    }
}

void
Gui::redrawAllViewers()
{
    QMutexLocker k(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::const_iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        if ( (*it)->isVisible() ) {
            (*it)->getViewer()->redraw();
        }
    }
}

void
Gui::renderAllViewers()
{
    assert(QThread::currentThread() == qApp->thread());
    for (std::list<ViewerTab*>::const_iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        if ( (*it)->isVisible() ) {
            (*it)->getInternalNode()->renderCurrentFrame(true);
        }
    }
}

void
Gui::toggleAutoHideGraphInputs()
{
    _imp->_nodeGraphArea->toggleAutoHideInputs(false);
}

void
Gui::centerAllNodeGraphsWithTimer()
{
    QTimer::singleShot( 25, _imp->_nodeGraphArea, SLOT( centerOnAllNodes() ) );

    for (std::list<NodeGraph*>::iterator it = _imp->_groups.begin(); it != _imp->_groups.end(); ++it) {
        QTimer::singleShot( 25, *it, SLOT( centerOnAllNodes() ) );
    }
}

void
Gui::setLastEnteredTabWidget(TabWidget* tab)
{
    _imp->_lastEnteredTabWidget = tab;
}

TabWidget*
Gui::getLastEnteredTabWidget() const
{
    return _imp->_lastEnteredTabWidget;
}

void
Gui::onPrevTabTriggered()
{
    TabWidget* t = getLastEnteredTabWidget();
    
    if (t) {
        t->moveToPreviousTab();
        PanelWidget* pw = t->currentWidget();
        if (pw) {
            pw->takeClickFocus();
        }
    }
    

}

void
Gui::onNextTabTriggered()
{
    TabWidget* t = getLastEnteredTabWidget();

    if (t) {
        t->moveToNextTab();
        PanelWidget* pw = t->currentWidget();
        if (pw) {
            pw->takeClickFocus();
        }
    }
}

void
Gui::onCloseTabTriggered()
{
    TabWidget* t = getLastEnteredTabWidget();

    if (t) {
        t->closeCurrentWidget();
        PanelWidget* pw = t->currentWidget();
        if (pw) {
            pw->takeClickFocus();
        }
    }
}

void
Gui::appendToScriptEditor(const std::string & str)
{
    _imp->_scriptEditor->appendToScriptEditor( str.c_str() );
}

void
Gui::printAutoDeclaredVariable(const std::string & str)
{
    _imp->_scriptEditor->printAutoDeclaredVariable( str.c_str() );
}

void
Gui::exportGroupAsPythonScript(NodeCollection* collection)
{
    assert(collection);
    NodeList nodes = collection->getNodes();
    bool hasOutput = false;
    for (NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if ( (*it)->isActivated() && dynamic_cast<GroupOutput*>( (*it)->getLiveInstance() ) ) {
            hasOutput = true;
            break;
        }
    }

    if (!hasOutput) {
        Natron::errorDialog( tr("Export").toStdString(), tr("To export as group, at least one Ouptut node must exist.").toStdString() );

        return;
    }
    ExportGroupTemplateDialog dialog(collection, this, this);
    ignore_result( dialog.exec() );
}

void
Gui::exportProjectAsGroup()
{
    exportGroupAsPythonScript( getApp()->getProject().get() );
}


void
Gui::onUserCommandTriggered()
{
    QAction* action = qobject_cast<QAction*>( sender() );

    if (!action) {
        return;
    }
    ActionWithShortcut* aws = dynamic_cast<ActionWithShortcut*>(action);
    if (!aws) {
        return;
    }
    std::map<ActionWithShortcut*, std::string>::iterator found = _imp->pythonCommands.find(aws);
    if ( found != _imp->pythonCommands.end() ) {
        std::string err;
        std::string output;
        if ( !Natron::interpretPythonScript(found->second, &err, &output) ) {
            getApp()->appendToScriptEditor(err);
        } else {
            getApp()->appendToScriptEditor(output);
        }
    }
}

void
Gui::addMenuEntry(const QString & menuGrouping,
                  const std::string & pythonFunction,
                  Qt::Key key,
                  const Qt::KeyboardModifiers & modifiers)
{
    QStringList grouping = menuGrouping.split('/');

    if ( grouping.isEmpty() ) {
        getApp()->appendToScriptEditor( tr("Failed to add menu entry for ").toStdString() +
                                       menuGrouping.toStdString() +
                                       tr(": incorrect menu grouping").toStdString() );

        return;
    }

    std::string appID = getApp()->getAppIDString();
    std::string script = "app = " + appID + "\n" + pythonFunction + "()\n";
    QAction* action = _imp->findActionRecursive(0, _imp->menubar, grouping);
    ActionWithShortcut* aws = dynamic_cast<ActionWithShortcut*>(action);
    if (aws) {
        aws->setShortcut( makeKeySequence(modifiers, key) );
        std::map<ActionWithShortcut*, std::string>::iterator found = _imp->pythonCommands.find(aws);
        if ( found != _imp->pythonCommands.end() ) {
            found->second = pythonFunction;
        } else {
            _imp->pythonCommands.insert( std::make_pair(aws, script) );
        }
    }
}

void
Gui::setDopeSheetTreeWidth(int width)
{
    _imp->_dopeSheetEditor->setTreeWidgetWidth(width);
}

void
Gui::setCurveEditorTreeWidth(int width)
{
    _imp->_curveEditor->setTreeWidgetWidth(width);
}

void Gui::setTripleSyncEnabled(bool enabled)
{
    if (_imp->_isTripleSyncEnabled != enabled) {
        _imp->_isTripleSyncEnabled = enabled;
    }
}

bool Gui::isTripleSyncEnabled() const
{
    return _imp->_isTripleSyncEnabled;
}

void Gui::centerOpenedViewersOn(SequenceTime left, SequenceTime right)
{
    const std::list<ViewerTab *> &viewers = getViewersList();

    for (std::list<ViewerTab *>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
        ViewerTab *v = (*it);

        v->centerOn_tripleSync(left, right);
    }
}

#ifdef __NATRON_WIN32__
void
Gui::ddeOpenFile(const QString& filePath)
{
    openProject(filePath.toStdString());
#pragma message WARN("CONTROL FLOW ERROR: should check the return value of openProject, raise an error...")
}
#endif


bool
Gui::isFocusStealingPossible()
{
    assert( qApp && qApp->thread() == QThread::currentThread() );
    QWidget* currentFocus = qApp->focusWidget();
    
    bool focusStealingNotPossible =
    dynamic_cast<QLineEdit*>(currentFocus) ||
    dynamic_cast<QTextEdit*>(currentFocus) ||
    dynamic_cast<QCheckBox*>(currentFocus) ||
    dynamic_cast<ComboBox*>(currentFocus) ||
    dynamic_cast<QComboBox*>(currentFocus);

    return !focusStealingNotPossible;

}

void
Gui::setCurrentPanelFocus(PanelWidget* widget)
{
    assert(QThread::currentThread() == qApp->thread());
    _imp->currentPanelFocus = widget;
}

PanelWidget*
Gui::getCurrentPanelFocus() const
{
    assert(QThread::currentThread() == qApp->thread());
    return _imp->currentPanelFocus;
}

static PanelWidget* isPaneChild(QWidget* w, int recursionLevel)
{
    if (!w) {
        return 0;
    }
    PanelWidget* pw = dynamic_cast<PanelWidget*>(w);
    if (pw && recursionLevel > 0) {
        /*
         Do not return it if recursion is 0, otherwise the focus stealing of the mouse over will actually take click focus
         */
        return pw;
    }
    return isPaneChild(w->parentWidget(), recursionLevel + 1);
}

void
Gui::onFocusChanged(QWidget* /*old*/, QWidget* newFocus)
{
    PanelWidget* pw = isPaneChild(newFocus,0);
    if (pw) {
        pw->takeClickFocus();
    }
}