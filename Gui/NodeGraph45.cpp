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

#include <map>
#include <vector>

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QThread>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QApplication>
#include <QCheckBox>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Node.h"
#include "Engine/NodeSerialization.h"
#include "Engine/OutputSchedulerThread.h" // RenderEngine
#include "Engine/Project.h"
#include "Engine/RotoLayer.h"
#include "Engine/Settings.h"
#include "Engine/ViewerInstance.h"

#include "Gui/BackDropGui.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/KnobGui.h"
#include "Gui/Label.h"
#include "Gui/LineEdit.h"
#include "Gui/NodeClipBoard.h"
#include "Gui/NodeGui.h"
#include "Gui/TabWidget.h"
#include "Gui/ViewerTab.h"

#include "Global/QtCompat.h"

using namespace Natron;


void
NodeGraph::toggleConnectionHints()
{
    appPTR->getCurrentSettings()->setConnectionHintsEnabled( !appPTR->getCurrentSettings()->isConnectionHintEnabled() );
}

void
NodeGraph::toggleAutoHideInputs(bool setSettings)
{
    bool autoHide ;
    if (setSettings) {
        autoHide = !appPTR->getCurrentSettings()->areOptionalInputsAutoHidden();
        appPTR->getCurrentSettings()->setOptionalInputsAutoHidden(autoHide);
    } else {
        autoHide = appPTR->getCurrentSettings()->areOptionalInputsAutoHidden();
    }
    if (!autoHide) {
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
            (*it)->setOptionalInputsVisible(true);
        }
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodesTrash.begin(); it != _imp->_nodesTrash.end(); ++it) {
            (*it)->setOptionalInputsVisible(true);
        }
    } else {
        
        QPointF evpt = mapFromScene(mapToScene(mapFromGlobal(QCursor::pos())));
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
            
            QRectF bbox = (*it)->mapToScene((*it)->boundingRect()).boundingRect();
            if (!(*it)->getIsSelected() && !bbox.contains(evpt)) {
                (*it)->setOptionalInputsVisible(false);
            }
            
        }
        for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodesTrash.begin(); it != _imp->_nodesTrash.end(); ++it) {
            (*it)->setOptionalInputsVisible(false);
        }
    }
}

std::list<boost::shared_ptr<NodeGui> > NodeGraph::getNodesWithinBackDrop(const boost::shared_ptr<NodeGui>& bd) const
{
    BackDropGui* isBd = dynamic_cast<BackDropGui*>(bd.get());
    if (!isBd) {
        return std::list<boost::shared_ptr<NodeGui> >();
    }
    
    QRectF bbox = bd->mapToScene( bd->boundingRect() ).boundingRect();
    std::list<boost::shared_ptr<NodeGui> > ret;
    QMutexLocker l(&_imp->_nodesMutex);

    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        QRectF nodeBbox = (*it)->mapToScene( (*it)->boundingRect() ).boundingRect();
        if ( bbox.contains(nodeBbox) ) {
            ret.push_back(*it);
        }
    }

    return ret;
}

void
NodeGraph::refreshNodesKnobsAtTime(SequenceTime time)
{
    ///Refresh all knobs at the current time
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        (*it)->refreshKnobsAfterTimeChange(time);
    }
}

void
NodeGraph::onTimelineTimeAboutToChange()
{
    assert(QThread::currentThread() == qApp->thread());
    _imp->wasLaskUserSeekDuringPlayback = false;
    const std::list<ViewerTab*>& viewers = _imp->_gui->getViewersList();
    for (std::list<ViewerTab*>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
        RenderEngine* engine = (*it)->getInternalNode()->getRenderEngine();
        _imp->wasLaskUserSeekDuringPlayback |= engine->abortRendering(true);
    }
}

void
NodeGraph::onTimeChanged(SequenceTime time,
                         int reason)
{
    assert(QThread::currentThread() == qApp->thread());
    std::vector<ViewerInstance* > viewers;

    if (!_imp->_gui) {
        return;
    }
    boost::shared_ptr<Natron::Project> project = _imp->_gui->getApp()->getProject();

    ///Refresh all knobs at the current time
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>( (*it)->getNode()->getLiveInstance() );
        if (isViewer) {
            viewers.push_back(isViewer);
        }
        (*it)->refreshKnobsAfterTimeChange(time);
    }
    
    ViewerInstance* leadViewer = getGui()->getApp()->getLastViewerUsingTimeline();

    bool isUserEdited = reason == eTimelineChangeReasonUserSeek ||
    reason == eTimelineChangeReasonDopeSheetEditorSeek ||
    reason == eTimelineChangeReasonCurveEditorSeek;
    
    bool startPlayback = isUserEdited && _imp->wasLaskUserSeekDuringPlayback;
    
    ///Syncrhronize viewers
    for (U32 i = 0; i < viewers.size(); ++i) {
        if (!startPlayback) {
            if (viewers[i] != leadViewer || isUserEdited) {
                viewers[i]->renderCurrentFrame(reason != eTimelineChangeReasonPlaybackSeek);
            }
        } else {
            viewers[i]->renderFromCurrentFrameUsingCurrentDirection(getGui()->getApp()->isRenderStatsActionChecked());
        }
    }
}

void
NodeGraph::onGuiFrozenChanged(bool frozen)
{
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        (*it)->onGuiFrozenChanged(frozen);
    }
}

void
NodeGraph::refreshAllKnobsGui()
{
    for (std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        if ((*it)->isSettingsPanelVisible()) {
            const std::map<boost::weak_ptr<KnobI>,KnobGui*> & knobs = (*it)->getKnobs();
            
            for (std::map<boost::weak_ptr<KnobI>,KnobGui*>::const_iterator it2 = knobs.begin(); it2!=knobs.end(); ++it2) {
                boost::shared_ptr<KnobI> knob = it2->first.lock();
                if (!knob->getIsSecret()) {
                    for (int i = 0; i < knob->getDimension(); ++i) {
                        if (knob->isAnimated(i)) {
                            it2->second->onInternalValueChanged(i, Natron::eValueChangedReasonPluginEdited);
                            it2->second->onAnimationLevelChanged(i, Natron::eValueChangedReasonPluginEdited);
                        }
                    }
                }
            }
        }
    }
}

void
NodeGraph::focusInEvent(QFocusEvent* e)
{
    QGraphicsView::focusInEvent(e);
    if (_imp->_gui) {
        _imp->_gui->setLastSelectedGraph(this);
    }
}

void
NodeGraph::focusOutEvent(QFocusEvent* e)
{
    if (_imp->_bendPointsVisible) {
        _imp->setNodesBendPointsVisible(false);
    }
    QGraphicsView::focusOutEvent(e);
}

void
NodeGraph::toggleSelectedNodesEnabled()
{
    _imp->toggleSelectedNodesEnabled();
}


bool
NodeGraph::areKnobLinksVisible() const
{
    return _imp->_knobLinksVisible;
}

void
NodeGraph::popFindDialog(const QPoint& p)
{
    QPoint realPos = p;

    FindNodeDialog* dialog = new FindNodeDialog(this,this);
    
    if (realPos.x() == 0 && realPos.y() == 0) {
        QPoint global = QCursor::pos();
        QSize sizeH = dialog->sizeHint();
        global.rx() -= sizeH.width() / 2;
        global.ry() -= sizeH.height() / 2;
        realPos = global;
        
    }
    
    QObject::connect(dialog ,SIGNAL(rejected()), this, SLOT(onFindNodeDialogFinished()));
    QObject::connect(dialog ,SIGNAL(accepted()), this, SLOT(onFindNodeDialogFinished()));
    dialog->move( realPos.x(), realPos.y() );
    dialog->raise();
    dialog->show();
    
}

void
NodeGraph::popRenameDialog(const QPoint& pos)
{
    boost::shared_ptr<NodeGui> node;
    if (_imp->_selection.size() == 1 ) {
        node = _imp->_selection.front();
    } else {
        return;
    }
    
    assert(node);

    
    QPoint realPos = pos;
    
    EditNodeNameDialog* dialog = new EditNodeNameDialog(node,this);
    
    if (realPos.x() == 0 && realPos.y() == 0) {
        QPoint global = QCursor::pos();
        QSize sizeH = dialog->sizeHint();
        global.rx() -= sizeH.width() / 2;
        global.ry() -= sizeH.height() / 2;
        realPos = global;
        
    }
    
    QObject::connect(dialog ,SIGNAL(rejected()), this, SLOT(onNodeNameEditDialogFinished()));
    QObject::connect(dialog ,SIGNAL(accepted()), this, SLOT(onNodeNameEditDialogFinished()));
    dialog->move( realPos.x(), realPos.y() );
    dialog->raise();
    dialog->show();
  
}

void
NodeGraph::onFindNodeDialogFinished()
{
    FindNodeDialog* dialog = qobject_cast<FindNodeDialog*>( sender() );
    
    if (dialog) {
        dialog->deleteLater();
    }
}

struct FindNodeDialogPrivate
{
    NodeGraph* graph;
    
    QString currentFilter;
    std::list<boost::shared_ptr<NodeGui> > nodeResults;
    int currentFindIndex;
    
    QVBoxLayout* mainLayout;
    Natron::Label* label;
    

    QCheckBox* unixWildcards;
    QCheckBox* caseSensitivity;

    Natron::Label* resultLabel;
    LineEdit* filter;
    QDialogButtonBox* buttons;
    
    
    FindNodeDialogPrivate(NodeGraph* graph)
    : graph(graph)
    , currentFilter()
    , nodeResults()
    , currentFindIndex(-1)
    , mainLayout(0)
    , label(0)
    , unixWildcards(0)
    , caseSensitivity(0)
    , resultLabel(0)
    , filter(0)
    , buttons(0)
    {
        
    }
};

FindNodeDialog::FindNodeDialog(NodeGraph* graph,QWidget* parent)
: QDialog(parent)
, _imp(new FindNodeDialogPrivate(graph))
{
    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint);
    
    _imp->mainLayout = new QVBoxLayout(this);
    _imp->mainLayout->setContentsMargins(0, 0, 0, 0);
    
    _imp->label = new Natron::Label(tr("Select all nodes containing this text:"),this);
    //_imp->label->setFont(QFont(appFont,appFontSize));
    _imp->mainLayout->addWidget(_imp->label);

    _imp->filter = new LineEdit(this);
    QObject::connect(_imp->filter, SIGNAL(editingFinished()), this, SLOT(updateFindResultsWithCurrentFilter()));
    QObject::connect(_imp->filter, SIGNAL(textEdited(QString)), this, SLOT(updateFindResults(QString)));
    
    _imp->mainLayout->addWidget(_imp->filter);
    
    
    _imp->unixWildcards = new QCheckBox(tr("Use Unix wildcards (*, ?, etc..)"),this);
    _imp->unixWildcards->setChecked(false);
    QObject::connect(_imp->unixWildcards, SIGNAL(toggled(bool)), this, SLOT(forceUpdateFindResults()));
    _imp->mainLayout->addWidget(_imp->unixWildcards);
    
    _imp->caseSensitivity = new QCheckBox(tr("Case sensitive"),this);
    _imp->caseSensitivity->setChecked(false);
    QObject::connect(_imp->caseSensitivity, SIGNAL(toggled(bool)), this, SLOT(forceUpdateFindResults()));
    _imp->mainLayout->addWidget(_imp->caseSensitivity);
    
    
    _imp->resultLabel = new Natron::Label(this);
    _imp->mainLayout->addWidget(_imp->resultLabel);
    //_imp->resultLabel->setFont(QFont(appFont,appFontSize));
    
    _imp->buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,Qt::Horizontal,this);
    QObject::connect(_imp->buttons, SIGNAL(accepted()), this, SLOT(onOkClicked()));
    QObject::connect(_imp->buttons, SIGNAL(rejected()), this, SLOT(onCancelClicked()));
    
    _imp->mainLayout->addWidget(_imp->buttons);
    _imp->filter->setFocus();
}

FindNodeDialog::~FindNodeDialog()
{
    
}

void
FindNodeDialog::updateFindResults(const QString& filter)
{
    if (filter == _imp->currentFilter) {
        return;
    }

    _imp->currentFilter = filter;
    _imp->currentFindIndex = 0;
    _imp->nodeResults.clear();
    
    _imp->graph->deselect();
    
    if (_imp->currentFilter.isEmpty()) {
        _imp->resultLabel->setText("");
        return;
    }
    Qt::CaseSensitivity sensitivity = _imp->caseSensitivity->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive;
    
    const std::list<boost::shared_ptr<NodeGui> >& activeNodes = _imp->graph->getAllActiveNodes();
    
    if (_imp->unixWildcards->isChecked()) {
        QRegExp exp(filter,sensitivity,QRegExp::Wildcard);
        if (!exp.isValid()) {
            return;
        }
        
        
        
        for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = activeNodes.begin(); it != activeNodes.end(); ++it) {
            if ((*it)->isVisible() && exp.exactMatch((*it)->getNode()->getLabel().c_str())) {
                _imp->nodeResults.push_back(*it);
            }
        }
    } else {
        for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = activeNodes.begin(); it != activeNodes.end(); ++it) {
            if ((*it)->isVisible() && QString((*it)->getNode()->getLabel().c_str()).contains(filter,sensitivity)) {
                _imp->nodeResults.push_back(*it);
            }
        }

    }
    
    if ((_imp->nodeResults.size()) == 0) {
        _imp->resultLabel->setText("");
    }

    
    selectNextResult();
}

void
FindNodeDialog::selectNextResult()
{
    if (_imp->currentFindIndex >= (int)(_imp->nodeResults.size())) {
        _imp->currentFindIndex = 0;
    }
    
    if (_imp->nodeResults.empty()) {
        return;
    }
    
    std::list<boost::shared_ptr<NodeGui> >::iterator it = _imp->nodeResults.begin();
    std::advance(it,_imp->currentFindIndex);
    
    _imp->graph->selectNode(*it, false);
    _imp->graph->centerOnItem(it->get());
    
    
    QString text = QString("Selecting result %1 of %2").arg(_imp->currentFindIndex + 1).arg(_imp->nodeResults.size());
    _imp->resultLabel->setText(text);

    
    ++_imp->currentFindIndex;
    
}



void
FindNodeDialog::updateFindResultsWithCurrentFilter()
{
    updateFindResults(_imp->filter->text());
    
}

void
FindNodeDialog::forceUpdateFindResults()
{
    _imp->currentFilter.clear();
    updateFindResultsWithCurrentFilter();
}


void
FindNodeDialog::onOkClicked()
{
    QString filterText = _imp->filter->text();
    if (_imp->currentFilter != filterText) {
        updateFindResults(filterText);
    } else {
        selectNextResult();
    }
}

void
FindNodeDialog::onCancelClicked()
{
    reject();
}

void
FindNodeDialog::keyPressEvent(QKeyEvent* e)
{
    if ( (e->key() == Qt::Key_Return) || (e->key() == Qt::Key_Enter) ) {
        selectNextResult();
        _imp->filter->setFocus();
    } else if (e->key() == Qt::Key_Escape) {
        reject();
    } else {
        QDialog::keyPressEvent(e);
    }
}

void
FindNodeDialog::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::ActivationChange) {
        if ( !isActiveWindow() ) {
            reject();
            
            return;
        }
    }
    QDialog::changeEvent(e);
}


struct EditNodeNameDialogPrivate
{
    
    LineEdit* field;
    boost::shared_ptr<NodeGui> node;
    
    EditNodeNameDialogPrivate(const boost::shared_ptr<NodeGui>& node)
    : field(0)
    , node(node)
    {
        
    }
};

EditNodeNameDialog::EditNodeNameDialog(const boost::shared_ptr<NodeGui>& node,QWidget* parent)
: QDialog(parent)
, _imp(new EditNodeNameDialogPrivate(node))
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint);
    _imp->field = new LineEdit(this);
    _imp->field->setPlaceholderText(tr("Edit node name"));
    mainLayout->addWidget(_imp->field);
}

EditNodeNameDialog::~EditNodeNameDialog()
{
    
}


void
EditNodeNameDialog::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::ActivationChange) {
        if ( !isActiveWindow() ) {
            reject();
            
            return;
        }
    }
    QDialog::changeEvent(e);
}

void
EditNodeNameDialog::keyPressEvent(QKeyEvent* e)
{
    if ( (e->key() == Qt::Key_Return) || (e->key() == Qt::Key_Enter) ) {
        accept();
    } else if (e->key() == Qt::Key_Escape) {
        reject();
    } else {
        QDialog::keyPressEvent(e);
    }
}

QString
EditNodeNameDialog::getTypedName() const
{
    return _imp->field->text();
}

boost::shared_ptr<NodeGui>
EditNodeNameDialog::getNode() const
{
    return _imp->node;
}

void
NodeGraph::onNodeNameEditDialogFinished()
{
    EditNodeNameDialog* dialog = qobject_cast<EditNodeNameDialog*>(sender());
    if (dialog) {
        QDialog::DialogCode code =  (QDialog::DialogCode)dialog->result();
        if (code == QDialog::Accepted) {
            QString newName = dialog->getTypedName();
            QString oldName = QString(dialog->getNode()->getNode()->getLabel().c_str());
            pushUndoCommand(new RenameNodeUndoRedoCommand(dialog->getNode(),oldName,newName));
            
        }
        dialog->deleteLater();
    }
}

void
NodeGraph::extractSelectedNode()
{
    pushUndoCommand(new ExtractNodeUndoRedoCommand(this,_imp->_selection));
}

void
NodeGraph::createGroupFromSelection()
{
    pushUndoCommand(new GroupFromSelectionCommand(this,_imp->_selection));
}

void
NodeGraph::expandSelectedGroups()
{
    NodeGuiList nodes;
    for (NodeGuiList::iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
        NodeGroup* isGroup = dynamic_cast<NodeGroup*>((*it)->getNode()->getLiveInstance());
        if (isGroup) {
            nodes.push_back(*it);
        }
    }
    if (!nodes.empty()) {
        pushUndoCommand(new InlineGroupCommand(this,nodes));
    } else {
        Natron::warningDialog(tr("Expand group").toStdString(), tr("You must select a group to expand first").toStdString());
    }
}

void
NodeGraph::onGroupNameChanged(const QString& name)
{
    
    setLabel(name.toStdString());
    TabWidget* parent = dynamic_cast<TabWidget*>(parentWidget() );
    if (parent) {
        parent->setTabLabel(this, name);
    }
}

void
NodeGraph::onGroupScriptNameChanged(const QString& /*name*/)
{
    assert( qApp && qApp->thread() == QThread::currentThread() );
    
    boost::shared_ptr<NodeCollection> group = getGroup();
    if (!group) {
        return;
    }
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>(group.get());
    if (!isGrp) {
        return;
    }
    std::string newName = isGrp->getNode()->getFullyQualifiedName();
    for (std::size_t i = 0; i < newName.size(); ++i) {
        if (newName[i] == '.') {
            newName[i] = '_';
        }
    }
    std::string oldName = getScriptName();
    for (std::size_t i = 0; i < oldName.size(); ++i) {
        if (oldName[i] == '.') {
            oldName[i] = '_';
        }
    }
    getGui()->unregisterTab(this);
    setScriptName(newName);
    getGui()->registerTab(this,this);
    TabWidget* parent = dynamic_cast<TabWidget*>(parentWidget() );
    if (parent) {
        parent->onTabScriptNameChanged(this, oldName, newName);
    }

}

void
NodeGraph::copyNodesAndCreateInGroup(const std::list<boost::shared_ptr<NodeGui> >& nodes,
                                     const boost::shared_ptr<NodeCollection>& group)
{
    NodeClipBoard clipboard;
    _imp->copyNodesInternal(nodes,clipboard);
    
    std::list<std::pair<std::string,boost::shared_ptr<NodeGui> > > newNodes;
    std::list<boost::shared_ptr<NodeSerialization> >::const_iterator itOther = clipboard.nodes.begin();
    for (std::list<boost::shared_ptr<NodeGuiSerialization> >::const_iterator it = clipboard.nodesUI.begin();
         it != clipboard.nodesUI.end(); ++it, ++itOther) {
        boost::shared_ptr<NodeGui> node = _imp->pasteNode( **itOther,**it,QPointF(0,0),group,std::string(), false);
        newNodes.push_back(std::make_pair((*itOther)->getNodeScriptName(),node));
    }
    assert( clipboard.nodes.size() == newNodes.size() );
    
    ///Now that all nodes have been duplicated, try to restore nodes connections
    _imp->restoreConnections(clipboard.nodes, newNodes);

}

QPointF
NodeGraph::getRootPos() const
{
    return _imp->_root->pos();
}
