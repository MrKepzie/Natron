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

#include <cmath> // abs

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QMouseEvent>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON

#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/Settings.h"

#include "Gui/BackDropGui.h"
#include "Gui/Edge.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiMacros.h"
#include "Gui/NodeGui.h"

#include "Global/QtCompat.h"

using namespace Natron;


void
NodeGraph::mouseMoveEvent(QMouseEvent* e)
{
    QPointF newPos = mapToScene( e->pos() );
    
    QPointF lastMousePosScene = mapToScene(_imp->_lastMousePos.x(),_imp->_lastMousePos.y());
    
    double dx = _imp->_root->mapFromScene(newPos).x() - _imp->_root->mapFromScene(lastMousePosScene).x();
    double dy = _imp->_root->mapFromScene(newPos).y() - _imp->_root->mapFromScene(lastMousePosScene).y();

    _imp->_hasMovedOnce = true;
    
    bool mustUpdate = true;
    
    QRectF sceneR = visibleSceneRect();
    if (_imp->_evtState != eEventStateSelectionRect && _imp->_evtState != eEventStateDraggingArrow) {
        ///set cursor
        boost::shared_ptr<NodeGui> selected;
        Edge* selectedEdge = 0;
        {
            bool optionalInputsAutoHidden = areOptionalInputsAutoHidden();
            QMutexLocker l(&_imp->_nodesMutex);
            for (NodeGuiList::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
                QPointF evpt = (*it)->mapFromScene(newPos);
                
                QRectF bbox = (*it)->mapToScene((*it)->boundingRect()).boundingRect();
                if ((*it)->isActive() && bbox.intersects(sceneR)) {
                    if ((*it)->contains(evpt)) {
                        selected = (*it);
                        if (optionalInputsAutoHidden) {
                            (*it)->setOptionalInputsVisible(true);
                        } else {
                            break;
                        }
                    } else {
                        Edge* edge = (*it)->hasEdgeNearbyPoint(newPos);
                        if (edge) {
                            selectedEdge = edge;
                            if (!optionalInputsAutoHidden) {
                                break;
                            }
                        } else if (optionalInputsAutoHidden && !(*it)->getIsSelected()) {
                            (*it)->setOptionalInputsVisible(false);
                        }
                    }
                }
                
            }
        }
        if (selected) {
            setCursor( QCursor(Qt::OpenHandCursor) );
        } else if (selectedEdge) {
        } else if (!selectedEdge && !selected) {
            unsetCursor();
        }
    }

    bool mustUpdateNavigator = false;
    ///Apply actions
    switch (_imp->_evtState) {
    case eEventStateDraggingArrow: {
        QPointF np = _imp->_arrowSelected->mapFromScene(newPos);
        if ( _imp->_arrowSelected->isOutputEdge() ) {
            _imp->_arrowSelected->dragDest(np);
        } else {
            _imp->_arrowSelected->dragSource(np);
        }
        scrollViewIfNeeded(newPos);
        mustUpdate = true;
        break;
    }
    case eEventStateDraggingNode: {
        mustUpdate = true;
        if ( !_imp->_selection.empty() ) {
            
            bool controlDown = modCASIsControl(e);

            std::list<std::pair<NodeGuiPtr,bool> > nodesToMove;
            for (std::list<NodeGuiPtr>::iterator it = _imp->_selection.begin();
                 it != _imp->_selection.end(); ++it) {
            
                const NodeGuiPtr& node = *it;
                nodesToMove.push_back(std::make_pair(node,false));
                
                std::map<NodeGuiPtr,NodeGuiList>::iterator foundBd = _imp->_nodesWithinBDAtPenDown.find(*it);
                if (!controlDown && foundBd != _imp->_nodesWithinBDAtPenDown.end()) {
                    for (NodeGuiList::iterator it2 = foundBd->second.begin();
                         it2 != foundBd->second.end(); ++it2) {
                        ///add it only if it's not already in the list
                        bool found = false;
                        for (std::list<std::pair<NodeGuiPtr,bool> >::iterator it3 = nodesToMove.begin();
                             it3 != nodesToMove.end(); ++it3) {
                            if (it3->first == *it2) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            nodesToMove.push_back(std::make_pair(*it2,true));
                        }
                    }

                }
            }
            
            double dxScene = newPos.x() - lastMousePosScene.x();
            double dyScene = newPos.y() - lastMousePosScene.y();
            
            QPointF newNodesCenter;
            
            bool deltaSet = false;
            for (std::list<std::pair<NodeGuiPtr,bool> >::iterator it = nodesToMove.begin();
                 it != nodesToMove.end(); ++it) {
                QPointF pos = it->first->getPos_mt_safe();
                bool ignoreMagnet = it->second || nodesToMove.size() > 1;
                it->first->refreshPosition(pos.x() + dxScene, pos.y() + dyScene,ignoreMagnet,newPos);
                QPointF newNodePos = it->first->getPos_mt_safe();
                if (!ignoreMagnet) {
                    assert(nodesToMove.size() == 1);
                    _imp->_deltaSinceMousePress.rx() += newNodePos.x() - pos.x();
                    _imp->_deltaSinceMousePress.ry() += newNodePos.y() - pos.y();
                    deltaSet = true;
                }
                newNodePos = it->first->mapToScene(it->first->mapFromParent(newNodePos));
                newNodesCenter.rx() += newNodePos.x();
                newNodesCenter.ry() += newNodePos.y();
                
            }
            size_t c = nodesToMove.size();
            if (c) {
                newNodesCenter.rx() /= c;
                newNodesCenter.ry() /= c;
            }
            
            scrollViewIfNeeded(newNodesCenter);
            
            if (!deltaSet) {
                _imp->_deltaSinceMousePress.rx() += dxScene;
                _imp->_deltaSinceMousePress.ry() += dyScene;
            }


            mustUpdateNavigator = true;
            
        }
        
        if (_imp->_selection.size() == 1) {
            ///try to find a nearby edge
            boost::shared_ptr<NodeGui> selectedNode = _imp->_selection.front();
            
            boost::shared_ptr<Natron::Node> internalNode = selectedNode->getNode();
            
            bool doMergeHints = e->modifiers().testFlag(Qt::ControlModifier) && e->modifiers().testFlag(Qt::ShiftModifier);
            bool doHints = appPTR->getCurrentSettings()->isConnectionHintEnabled();
            
            BackDropGui* isBd = dynamic_cast<BackDropGui*>(selectedNode.get());
            if (isBd) {
                doMergeHints = false;
                doHints = false;
            }
            
            if (!doMergeHints) {
                ///for nodes already connected don't show hint
                if ( ( internalNode->getMaxInputCount() == 0) && internalNode->hasOutputConnected() ) {
                    doHints = false;
                } else if ( ( internalNode->getMaxInputCount() > 0) && internalNode->hasAllInputsConnected() && internalNode->hasOutputConnected() ) {
                    doHints = false;
                }
            }
            
            if (doHints) {
                QRectF selectedNodeBbox = selectedNode->boundingRectWithEdges();//selectedNode->mapToParent( selectedNode->boundingRect() ).boundingRect();
                double tolerance = 10;
                selectedNodeBbox.adjust(-tolerance, -tolerance, tolerance, tolerance);
                
                boost::shared_ptr<NodeGui> nodeToShowMergeRect;
                
                boost::shared_ptr<Natron::Node> selectedNodeInternalNode = selectedNode->getNode();
                bool selectedNodeIsReader = selectedNodeInternalNode->getLiveInstance()->isReader() || selectedNodeInternalNode->getMaxInputCount() == 0;
                Edge* edge = 0;
                {
                    QMutexLocker l(&_imp->_nodesMutex);
                    for (NodeGuiList::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
                        
                        bool isAlreadyAnOutput = false;
                        const std::list<Natron::Node*>& outputs = internalNode->getGuiOutputs();
                        for (std::list<Natron::Node*>::const_iterator it2 = outputs.begin(); it2 != outputs.end(); ++it2) {
                            if (*it2 == (*it)->getNode().get()) {
                                isAlreadyAnOutput = true;
                                break;
                            }
                        }
                        if (isAlreadyAnOutput) {
                            continue;
                        }
                        QRectF nodeBbox = (*it)->boundingRectWithEdges();
                        if ( (*it) != selectedNode && (*it)->isVisible() && nodeBbox.intersects(sceneR)) {
                            
                            if (doMergeHints) {
                                
                                //QRectF nodeRect = (*it)->mapToParent((*it)->boundingRect()).boundingRect();
                                
                                boost::shared_ptr<Natron::Node> internalNode = (*it)->getNode();
                                
                                
                                if (!internalNode->isOutputNode() && nodeBbox.intersects(selectedNodeBbox)) {
                                    
                                    bool nHasInput = internalNode->hasInputConnected();
                                    int nMaxInput = internalNode->getMaxInputCount();
                                    bool selectedHasInput = selectedNodeInternalNode->hasInputConnected();
                                    int selectedMaxInput = selectedNodeInternalNode->getMaxInputCount();
                                    double nPAR = internalNode->getLiveInstance()->getPreferredAspectRatio();
                                    double selectedPAR = selectedNodeInternalNode->getLiveInstance()->getPreferredAspectRatio();
                                    double nFPS = internalNode->getLiveInstance()->getPreferredFrameRate();
                                    double selectedFPS = selectedNodeInternalNode->getLiveInstance()->getPreferredFrameRate();
                                    
                                    bool isValid = true;
                                    
                                    if (selectedPAR != nPAR || std::abs(nFPS - selectedFPS) > 0.01) {
                                        if (nHasInput || selectedHasInput) {
                                            isValid = false;
                                        } else if (!nHasInput && nMaxInput == 0 && !selectedHasInput && selectedMaxInput == 0) {
                                            isValid = false;
                                        }
                                    }
                                    if (isValid) {
                                        nodeToShowMergeRect = *it;
                                    }
                                } else {
                                    (*it)->setMergeHintActive(false);
                                }
                                
                            } else {
                                
                                
                                
                                edge = (*it)->hasEdgeNearbyRect(selectedNodeBbox);
                                
                                ///if the edge input is the selected node don't continue
                                if ( edge && ( edge->getSource() == selectedNode) ) {
                                    edge = 0;
                                }
                                
                                if ( edge && edge->isOutputEdge() ) {
                                    
                                    if (selectedNodeIsReader) {
                                        continue;
                                    }
                                    int prefInput = selectedNodeInternalNode->getPreferredInputForConnection();
                                    if (prefInput == -1) {
                                        edge = 0;
                                    } else {
                                        Natron::Node::CanConnectInputReturnValue ret = selectedNodeInternalNode->canConnectInput(edge->getSource()->getNode(),
                                                                                                                                 prefInput);
                                        if (ret != Natron::Node::eCanConnectInput_ok) {
                                            edge = 0;
                                        }
                                    }
                                }
                                
                                if ( edge && !edge->isOutputEdge() ) {
                                    
                                    if ((*it)->getNode()->getLiveInstance()->isReader() ||
                                        (*it)->getNode()->getMaxInputCount() == 0) {
                                        edge = 0;
                                        continue;
                                    }
                                    
                                    if ((*it)->getNode()->getLiveInstance()->isInputRotoBrush(edge->getInputNumber())) {
                                        edge = 0;
                                        continue;
                                    }
                                    
                                    Natron::Node::CanConnectInputReturnValue ret = edge->getDest()->getNode()->canConnectInput(selectedNodeInternalNode, edge->getInputNumber());
                                    if (ret == Natron::Node::eCanConnectInput_inputAlreadyConnected &&
                                        !selectedNodeIsReader) {
                                        ret = Natron::Node::eCanConnectInput_ok;
                                    }
                                    
                                    if (ret != Natron::Node::eCanConnectInput_ok) {
                                        edge = 0;
                                    }
                                    
                                }
                                
                                if (edge) {
                                    edge->setUseHighlight(true);
                                    break;
                                }
                            }
                        }
                    }
                } // QMutexLocker l(&_imp->_nodesMutex);
                
                if ( _imp->_highLightedEdge && ( _imp->_highLightedEdge != edge) ) {
                    _imp->_highLightedEdge->setUseHighlight(false);
                    _imp->_hintInputEdge->hide();
                    _imp->_hintOutputEdge->hide();
                }

                _imp->_highLightedEdge = edge;

                if ( edge && edge->getSource() && edge->getDest() ) {
                    ///setup the hints edge

                    ///find out if the node is already connected to what the edge is connected
                    bool alreadyConnected = false;
                    const std::vector<boost::shared_ptr<Natron::Node> > & inpNodes = selectedNode->getNode()->getGuiInputs();
                    for (U32 i = 0; i < inpNodes.size(); ++i) {
                        if ( inpNodes[i] == edge->getSource()->getNode() ) {
                            alreadyConnected = true;
                            break;
                        }
                    }

                    if ( !_imp->_hintInputEdge->isVisible() ) {
                        if (!alreadyConnected) {
                            int prefInput = selectedNode->getNode()->getPreferredInputForConnection();
                            _imp->_hintInputEdge->setInputNumber(prefInput != -1 ? prefInput : 0);
                            _imp->_hintInputEdge->setSourceAndDestination(edge->getSource(), selectedNode);
                            _imp->_hintInputEdge->setVisible(true);
                        }
                        _imp->_hintOutputEdge->setInputNumber( edge->getInputNumber() );
                        _imp->_hintOutputEdge->setSourceAndDestination( selectedNode, edge->getDest() );
                        _imp->_hintOutputEdge->setVisible(true);
                    } else {
                        if (!alreadyConnected) {
                            _imp->_hintInputEdge->initLine();
                        }
                        _imp->_hintOutputEdge->initLine();
                    }
                } else if (edge) {
                    ///setup only 1 of the hints edge

                    if ( _imp->_highLightedEdge && !_imp->_hintInputEdge->isVisible() ) {
                        if ( edge->isOutputEdge() ) {
                            int prefInput = selectedNode->getNode()->getPreferredInputForConnection();
                            _imp->_hintInputEdge->setInputNumber(prefInput != -1 ? prefInput : 0);
                            _imp->_hintInputEdge->setSourceAndDestination(edge->getSource(), selectedNode);
                        } else {
                            _imp->_hintInputEdge->setInputNumber( edge->getInputNumber() );
                            _imp->_hintInputEdge->setSourceAndDestination( selectedNode,edge->getDest() );
                        }
                        _imp->_hintInputEdge->setVisible(true);
                    } else if ( _imp->_highLightedEdge && _imp->_hintInputEdge->isVisible() ) {
                        _imp->_hintInputEdge->initLine();
                    }
                } else if (nodeToShowMergeRect) {
                    nodeToShowMergeRect->setMergeHintActive(true);
                    selectedNode->setMergeHintActive(true);
                    _imp->_mergeHintNode = nodeToShowMergeRect;
                } else {
                    selectedNode->setMergeHintActive(false);
                    _imp->_mergeHintNode.reset();
                }
                
                
            } // if (doHints) {
        } //  if (_imp->_selection.nodes.size() == 1) {
        setCursor( QCursor(Qt::ClosedHandCursor) );
        break;
    }
    case eEventStateMovingArea: {
        mustUpdateNavigator = true;
        _imp->_root->moveBy(dx, dy);
        setCursor( QCursor(Qt::SizeAllCursor) );
        mustUpdate = true;
        break;
    }
    case eEventStateResizingBackdrop: {
        mustUpdateNavigator = true;
        assert(_imp->_backdropResized);
        QPointF p = _imp->_backdropResized->scenePos();
        int w = newPos.x() - p.x();
        int h = newPos.y() - p.y();
        scrollViewIfNeeded(newPos);
        mustUpdate = true;
        pushUndoCommand( new ResizeBackDropCommand(_imp->_backdropResized,w,h) );
        break;
    }
    case eEventStateSelectionRect: {
        QPointF lastSelectionScene = mapToScene(_imp->_lastSelectionStartPoint);
        QPointF startDrag = _imp->_selectionRect->mapFromScene(lastSelectionScene);
        QPointF cur = _imp->_selectionRect->mapFromScene(newPos);
        double xmin = std::min( cur.x(),startDrag.x() );
        double xmax = std::max( cur.x(),startDrag.x() );
        double ymin = std::min( cur.y(),startDrag.y() );
        double ymax = std::max( cur.y(),startDrag.y() );
        scrollViewIfNeeded(newPos);
        _imp->_selectionRect->setRect(xmin,ymin,xmax - xmin,ymax - ymin);
        _imp->_selectionRect->show();
        mustUpdate = true;
        break;
    }
    case eEventStateDraggingNavigator: {
        QPointF mousePosSceneCoordinates;
        bool insideNavigator = isNearbyNavigator(e->pos(), mousePosSceneCoordinates);
        if (insideNavigator) {
            _imp->_refreshOverlays = true;
            centerOn(mousePosSceneCoordinates);
            _imp->_lastMousePos = e->pos();
            update();
            return;
        }
        
    } break;
    case eEventStateZoomingArea: {
        int delta = 2*((e->x() - _imp->_lastMousePos.x()) - (e->y() - _imp->_lastMousePos.y()));
        setTransformationAnchor(QGraphicsView::AnchorViewCenter);
        wheelEventInternal(modCASIsControl(e),delta);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        mustUpdate = true;
    } break;
    default:
            mustUpdate = false;
        break;
    } // switch

    
    _imp->_lastMousePos = e->pos();

    if (mustUpdateNavigator) {
        _imp->_refreshOverlays = true;
        mustUpdate = true;
    }
    
    if (mustUpdate) {
        update();
    }
    QGraphicsView::mouseMoveEvent(e);
} // mouseMoveEvent

