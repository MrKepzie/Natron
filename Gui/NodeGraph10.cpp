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

#include "NodeGraph.h"
#include "NodeGraphPrivate.h"

#include <stdexcept>

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QMouseEvent>
#include <QString>
#include <QAction>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON

#include "Engine/Node.h"
#include "Engine/Project.h"

#include "Gui/BackDropGui.h"
#include "Gui/Edge.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiMacros.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"

#include "Global/QtCompat.h"

using namespace Natron;


void
NodeGraph::mousePressEvent(QMouseEvent* e)
{
    assert(e);
    
    takeClickFocus();
    
    _imp->_hasMovedOnce = false;
    _imp->_deltaSinceMousePress = QPointF(0,0);
    if ( buttonDownIsMiddle(e) ) {
        _imp->_evtState = eEventStateMovingArea;

        return;
    }
   
    bool didSomething = false;

    _imp->_lastMousePos = e->pos();
    QPointF lastMousePosScene = mapToScene(_imp->_lastMousePos.x(),_imp->_lastMousePos.y());

    if (((e->buttons() & Qt::MiddleButton) &&
         (buttonMetaAlt(e) == Qt::AltModifier || (e->buttons() & Qt::LeftButton))) ||
        ((e->buttons() & Qt::LeftButton) &&
         (buttonMetaAlt(e) == (Qt::AltModifier|Qt::MetaModifier)))) {
        // Alt + middle or Left + middle or Crtl + Alt + Left = zoom
        _imp->_evtState = eEventStateZoomingArea;
        return;
    }
    
    
    boost::shared_ptr<NodeCollection> collection = getGroup();
    NodeGroup* isGroup = dynamic_cast<NodeGroup*>(collection.get());
    bool isGroupEditable = true;
    bool groupEdited = true;
    if (isGroup) {
        isGroupEditable = isGroup->isSubGraphEditable();
        groupEdited = isGroup->getNode()->hasPyPlugBeenEdited();
    }
    
    if (!groupEdited) {
        
        if (isGroupEditable) {
            ///check if user clicked unlock
            int iw = _imp->unlockIcon.width();
            int ih = _imp->unlockIcon.height();
            int w = width();
            int offset = 20;
            if (e->x() >= (w - iw - 10 - offset) && e->x() <= (w - 10 + offset) &&
                e->y() >= (10 - offset) && e->y() <= (10 + ih + offset)) {
                assert(isGroup);
                isGroup->getNode()->setPyPlugEdited(true);
                NodeList nodes = isGroup->getNodes();
                for (NodeList::iterator it = nodes.begin(); it!=nodes.end(); ++it) {
                    boost::shared_ptr<NodeGui> nodeUi = boost::dynamic_pointer_cast<NodeGui>((*it)->getNodeGui());
                    if (nodeUi) {
                        NodeSettingsPanel* panel = nodeUi->getSettingPanel();
                        if (panel) {
                            panel->setEnabled(true);
                        }
                    }
                }
            }
            getGui()->getApp()->triggerAutoSave();
            update();
        }
    }

    
    NodeGuiPtr selected;
    Edge* selectedEdge = 0;
    Edge* selectedBendPoint = 0;
    {
        
        QMutexLocker l(&_imp->_nodesMutex);
        
        ///Find matches, sorted by depth
        std::map<double,NodeGuiPtr> matches;
        for (NodeGuiList::reverse_iterator it = _imp->_nodes.rbegin(); it != _imp->_nodes.rend(); ++it) {
            QPointF evpt = (*it)->mapFromScene(lastMousePosScene);
            if ( (*it)->isVisible() && (*it)->isActive() ) {
                
                BackDropGui* isBd = dynamic_cast<BackDropGui*>(it->get());
                if (isBd) {
                    if (isBd->isNearbyNameFrame(evpt)) {
                        matches.insert(std::make_pair((*it)->zValue(),*it));
                    } else if (isBd->isNearbyResizeHandle(evpt) && groupEdited) {
                        selected = *it;
                        _imp->_backdropResized = *it;
                        _imp->_evtState = eEventStateResizingBackdrop;
                        break;
                    }
                } else {
                    if ((*it)->contains(evpt)) {
                        matches.insert(std::make_pair((*it)->zValue(),*it));
                    }
                }
                
            }
        }
        if (!matches.empty() && _imp->_evtState != eEventStateResizingBackdrop) {
            selected = matches.rbegin()->second;
        }
        if (!selected) {
            ///try to find a selected edge
            for (NodeGuiList::reverse_iterator it = _imp->_nodes.rbegin(); it != _imp->_nodes.rend(); ++it) {
                Edge* bendPointEdge = (*it)->hasBendPointNearbyPoint(lastMousePosScene);

                if (bendPointEdge) {
                    selectedBendPoint = bendPointEdge;
                    break;
                }
                Edge* edge = (*it)->hasEdgeNearbyPoint(lastMousePosScene);
                if (edge) {
                    selectedEdge = edge;
                }
                
            }
        }
    }
    
    
    if (selected) {
        didSomething = true;
        if ( buttonDownIsLeft(e) ) {
            
            BackDropGui* isBd = dynamic_cast<BackDropGui*>(selected.get());
            if (!isBd) {
                _imp->_magnifiedNode = selected;
            }
            
            if ( !selected->getIsSelected() ) {
                selectNode( selected, modCASIsShift(e) );
            } else if ( modCASIsShift(e) ) {
                NodeGuiList::iterator it = std::find(_imp->_selection.begin(),
                                                     _imp->_selection.end(),selected);
                if ( it != _imp->_selection.end() ) {
                    (*it)->setUserSelected(false);
                    _imp->_selection.erase(it);
                }
            }
            if (groupEdited && _imp->_evtState != eEventStateResizingBackdrop) {
                _imp->_evtState = eEventStateDraggingNode;
            }
            ///build the _nodesWithinBDAtPenDown map
            _imp->_nodesWithinBDAtPenDown.clear();
            for (NodeGuiList::iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
                BackDropGui* isBd = dynamic_cast<BackDropGui*>(it->get());
                if (isBd) {
                    NodeGuiList nodesWithin = getNodesWithinBackDrop(*it);
                    _imp->_nodesWithinBDAtPenDown.insert(std::make_pair(*it,nodesWithin));
                }
            }

            _imp->_lastNodeDragStartPoint = selected->getPos_mt_safe();
        } else if ( buttonDownIsRight(e) ) {
            if ( !selected->getIsSelected() ) {
                selectNode(selected,true); ///< don't wipe the selection
            }
        }
    } else if (selectedBendPoint && groupEdited) {
        _imp->setNodesBendPointsVisible(false);
        
        ///Now connect the node to the edge input
        boost::shared_ptr<Natron::Node> inputNode = selectedBendPoint->getSource()->getNode();
        assert(inputNode);
        ///disconnect previous connection
        boost::shared_ptr<Natron::Node> outputNode = selectedBendPoint->getDest()->getNode();
        assert(outputNode);
        int inputNb = outputNode->inputIndex( inputNode.get() );
        if (inputNb == -1) {
            return;
        }
        
        CreateNodeArgs args(PLUGINID_NATRON_DOT,
                            std::string(),
                            -1,
                            -1,
                            false, //< don't autoconnect
                            INT_MIN,
                            INT_MIN,
                            false, //<< don't push an undo command
                            true,
                            true,
                            QString(),
                            CreateNodeArgs::DefaultValuesList(),
                            _imp->group.lock());
        boost::shared_ptr<Natron::Node> dotNode = getGui()->getApp()->createNode(args);
        assert(dotNode);
        boost::shared_ptr<NodeGuiI> dotNodeGui_i = dotNode->getNodeGui();
        boost::shared_ptr<NodeGui> dotNodeGui = boost::dynamic_pointer_cast<NodeGui>(dotNodeGui_i);
        assert(dotNodeGui);
        
        std::list<boost::shared_ptr<NodeGui> > nodesList;
        nodesList.push_back(dotNodeGui);
        
       
        
        
        assert(getGui());
        GuiAppInstance* guiApp = getGui()->getApp();
        assert(guiApp);
        boost::shared_ptr<Natron::Project> project = guiApp->getProject();
        assert(project);
        bool ok = project->disconnectNodes(inputNode.get(), outputNode.get());
        if (!ok) {
            throw std::logic_error("disconnectNodes failed");
        }
        
        ok = project->connectNodes(0, inputNode, dotNode.get());
        if (!ok) {
            throw std::logic_error("connectNodes failed");
        }

        ok = project->connectNodes(inputNb,dotNode,outputNode.get());
        if (!ok) {
            throw std::logic_error("connectNodes failed");
        }

        QPointF pos = dotNodeGui->mapToParent( dotNodeGui->mapFromScene(lastMousePosScene) );

        dotNodeGui->refreshPosition( pos.x(), pos.y() );
        if ( !dotNodeGui->getIsSelected() ) {
            selectNode( dotNodeGui, modCASIsShift(e) );
        }
        pushUndoCommand( new AddMultipleNodesCommand( this,nodesList) );
        
        
        _imp->_evtState = eEventStateDraggingNode;
        _imp->_lastNodeDragStartPoint = dotNodeGui->getPos_mt_safe();
        didSomething = true;
    } else if (selectedEdge && groupEdited) {
        _imp->_arrowSelected = selectedEdge;
        didSomething = true;
        _imp->_evtState = eEventStateDraggingArrow;
    }
    
    ///Test if mouse is inside the navigator
    {
        QPointF mousePosSceneCoordinates;
        bool insideNavigator = isNearbyNavigator(e->pos(), mousePosSceneCoordinates);
        if (insideNavigator) {
            updateNavigator();
            _imp->_refreshOverlays = true;
            centerOn(mousePosSceneCoordinates);
            _imp->_evtState = eEventStateDraggingNavigator;
            didSomething = true;
        }
    }

    ///Don't forget to reset back to null the _backdropResized pointer
    if (_imp->_evtState != eEventStateResizingBackdrop) {
        _imp->_backdropResized.reset();
    }

    if ( buttonDownIsRight(e) && groupEdited) {
        showMenu( mapToGlobal( e->pos() ) );
        didSomething = true;
    }
    if (!didSomething) {
        if ( buttonDownIsLeft(e) ) {
            if ( !modCASIsShift(e) ) {
                deselect();
            }
            _imp->_evtState = eEventStateSelectionRect;
            _imp->_lastSelectionStartPoint = _imp->_lastMousePos;
            QPointF clickPos = _imp->_selectionRect->mapFromScene(lastMousePosScene);
            _imp->_selectionRect->setRect(clickPos.x(), clickPos.y(), 0, 0);
            //_imp->_selectionRect->show();
        } else if ( buttonDownIsMiddle(e) ) {
            _imp->_evtState = eEventStateMovingArea;
            QGraphicsView::mousePressEvent(e);
        }
    }
} // mousePressEvent
