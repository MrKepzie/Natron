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

#ifndef NATRON_GUI_NODEGUI_H
#define NATRON_GUI_NODEGUI_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <map>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QRectF>
#include <QtCore/QMutex>
#include <QGraphicsItem>
#include <QDialog>
#include <QMutex>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"
#include "Engine/NodeGuiI.h"

class DefaultOverlay;
class Edge;
class QPainterPath;
class QScrollArea;
class NodeSettingsPanel;
class QVBoxLayout;
class QLinearGradient;
class AppInstance;
class NodeGraph;
class QAction;
class KnobI;
class NodeGuiSerialization;
class NodeSerialization;
class KnobGui;
class Gui;
class QUndoStack;
class LinkArrow;
class MultiInstancePanel;
class QMenu;
class NodeGroup;
class QUndoStack;
class NodeCollection;
namespace Natron {
class ChannelSet;
class Node;
}

struct NodeGuiIndicatorPrivate;
class NodeGuiIndicator
{
public:

    NodeGuiIndicator(int depth,
                     const QString & text,
                     const QPointF & topLeft,
                     int width,
                     int height,
                     const QGradientStops & gradient,
                     const QColor & textColor,
                     QGraphicsItem* parent);

    ~NodeGuiIndicator();

    void setToolTip(const QString & tooltip);

    void setActive(bool active);

    bool isActive() const;

    void refreshPosition(const QPointF & topLeft);

private:

    boost::scoped_ptr<NodeGuiIndicatorPrivate> _imp;
};

///A richer text item which allows text alignmenent
class TextItem
    : public QGraphicsTextItem
{
    Q_OBJECT

public:

    enum
    {
        Type = UserType + 1
    };

    TextItem(QGraphicsItem* parent = 0);
    TextItem(const QString & text,
             QGraphicsItem* parent = 0);

    void setAlignment(Qt::Alignment alignment);
    virtual int type() const;

public Q_SLOTS:

    void updateGeometry(int, int, int);
    void updateGeometry();

private:
    void init();

    Qt::Alignment _alignement;
};

class NodeGui
: public QObject,public QGraphicsItem, public NodeGuiI, public boost::enable_shared_from_this<NodeGui>
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    Q_INTERFACES(QGraphicsItem)
    
public:
    
    
    typedef std::vector<Edge*> InputEdges;

    NodeGui(QGraphicsItem *parent = 0);

    void initialize(NodeGraph* dag,
                    const boost::shared_ptr<Natron::Node> & internalNode);

    //Creates panel if needed, might be expensive
    void ensurePanelCreated();
    
    /**
     * @brief Called by NodeGraph::clearExceedingUndoRedoEvents when we want to delete a node.
     **/
    void deleteReferences();

    ~NodeGui() OVERRIDE;
    
    virtual void restoreStateAfterCreation() OVERRIDE FINAL;

    /**
     * @brief Fills the serializationObject with the current state of the NodeGui.
     **/
    void serialize(NodeGuiSerialization* serializationObject) const;


    void copyFrom(const NodeGuiSerialization & obj);


    boost::shared_ptr<Natron::Node> getNode() const
    {
        return _internalNode.lock();
    }

    /*Returns a pointer to the dag gui*/
    NodeGraph* getDagGui()
    {
        return _graph;
    }
    
    virtual bool isSettingsPanelOpened() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual bool shouldDrawOverlay() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual void setPosition(double x,double y) OVERRIDE FINAL;
    
    virtual void getPosition(double *x, double* y) const OVERRIDE FINAL;
 
    virtual void getSize(double* w, double* h) const OVERRIDE FINAL;
    
    virtual void setSize(double w, double h) OVERRIDE FINAL;
    
    virtual void getColor(double* r,double *g, double* b) const OVERRIDE FINAL;
    
    virtual void setColor(double r, double g, double b) OVERRIDE FINAL;


    /*Returns true if the NodeGUI contains the point (in items coordinates)*/
    virtual bool contains(const QPointF &point) const OVERRIDE FINAL;

    /*Returns true if the bounding box of the node intersects the given rectangle in scene coordinates.*/
    bool intersects(const QRectF & rect) const;

    /*returns a QPainterPath indicating the global shape of the node.
       This must be provided so the QGraphicsView framework recognises the
       item correctly.*/
    virtual QPainterPath shape() const OVERRIDE;

    /*Returns the bouding box of the nodeGUI, must be derived if you
       plan on changing the shape of the node.*/
    virtual QRectF boundingRect() const OVERRIDE;

    QRectF boundingRectWithEdges() const;

    /*this function does the painting, using QPainter, you can overload it to change the aspect of
       the node.*/
    virtual void paint(QPainter* painter,const QStyleOptionGraphicsItem* options,QWidget* parent) OVERRIDE;


    /*Returns a ref to the vector of all the input arrows. This can be used
       to query the src and dst of a specific arrow.*/
    const std::vector<Edge*> & getInputsArrows() const
    {
        return _inputEdges;
    }

    Edge* getOutputArrow() const;
    Edge* getInputArrow(int inputNb) const;

    /*Returns true if the point is included in the rectangle +10px on all edges.*/
    bool isNearby(QPointF &point);

    /*Returns a pointer to the settings panel of this node.*/
    NodeSettingsPanel* getSettingPanel() const
    {
        return _settingsPanel;
    }

    /*Returns a pointer to the layout containing settings panels.*/
    QVBoxLayout* getDockContainer() const;


    /* @brief toggles selected on/off. MT-Safe*/
    void setUserSelected(bool b);

    /* @brief Is the node selected ? MT-Safe */
    bool getIsSelected() const;
    
    virtual bool isUserSelected() const OVERRIDE FINAL
    {
        return getIsSelected();
    }

    /*Returns a pointer to the first available input. Otherwise returns NULL*/
    Edge* firstAvailableEdge();

    /*find the edge connecting this as dst and the parent as src.
       Return a valid ptr to the edge if it found it, otherwise returns NULL.*/
    Edge* findConnectedEdge(NodeGui* parent);


    void markInputNull(Edge* e);

    const std::map<boost::weak_ptr<KnobI>,KnobGui*> & getKnobs() const;
    static const int DEFAULT_OFFSET_BETWEEN_NODES = 30;


    QSize getSize() const;

    /*Returns an edge if the node has an edge close to the
       point pt. pt is in scene coord.*/
    Edge* hasEdgeNearbyPoint(const QPointF & pt);
    Edge* hasEdgeNearbyRect(const QRectF & rect);
    Edge* hasBendPointNearbyPoint(const QPointF & pt);

    void refreshPosition( double x,double y,bool skipMagnet = false,const QPointF & mouseScenePos = QPointF() );

    void changePosition(double dx,double dy);

    bool isSettingsPanelVisible() const;
    
    bool isSettingsPanelMinimized() const;

    void removeSettingsPanel();

    boost::shared_ptr<QUndoStack> getUndoStack() const;

    void removeUndoStack();

    void discardGraphPointer();
    /**
     * @brief Given the rectangle r, move the node down so it doesn't belong
     * to this rectangle and call the same function with the new bounding box of this node
     * recursively on its outputs.
     **/
    void moveBelowPositionRecursively(const QRectF & r);

    /**
     * @brief Given the rectangle r, move the node up so it doesn't belong
     * to this rectangle and call the same function with the new bounding box of this node
     * recursively on its inputs.
     **/
    void moveAbovePositionRecursively(const QRectF & r);

    QPointF getPos_mt_safe() const;

    void setPos_mt_safe(const QPointF & pos);

    ///same as setScale() but also scales the arrows
    void setScale_natron(double scale);

    void removeHighlightOnAllEdges();

    QColor getCurrentColor() const;

    void setCurrentColor(const QColor & c);
    
    void setOverlayColor(const QColor& c);

    void refreshKnobsAfterTimeChange(SequenceTime time);
    
    void onGuiFrozenChanged(bool frozen);

    boost::shared_ptr<MultiInstancePanel> getMultiInstancePanel() const;

    void setParentMultiInstance(const boost::shared_ptr<NodeGui> & parent);

    boost::shared_ptr<NodeGui> getParentMultiInstance() const
    {
        return _parentMultiInstance.lock();
    }
    
    
    void setKnobLinksVisible(bool visible);
    
    /**
     * @brief Serialize this node. If this is a multi-instance node, every instance will
     * be serialized, hence the list.
     **/
    void serializeInternal(std::list<boost::shared_ptr<NodeSerialization> >& internalSerialization) const;
    void restoreInternal(const boost::shared_ptr<NodeGui>& thisShared,
                         const std::list<boost::shared_ptr<NodeSerialization> >& internalSerialization) ;
        
    void setMergeHintActive(bool active);
    
    /**
     * @brief Called after the node-graph view reaches a certain detail level (zoom) to show/hide elements that would otherwise 
     * not be visible and would just clutter and slow down the interface
     **/
    void setVisibleDetails(bool visible);
    
    void setOverlayLocked(bool locked);
    
    virtual void refreshStateIndicator();
    
    virtual void exportGroupAsPythonScript() OVERRIDE FINAL;
        
    bool isNearbyNameFrame(const QPointF& pos) const;
    
    bool isNearbyResizeHandle(const QPointF& pos) const;
    
    int getFrameNameHeight() const;
    
    void checkOptionalEdgesVisibility();
    
    virtual bool getOverlayColor(double* r, double* g, double* b) const OVERRIDE FINAL;
    
    virtual void addDefaultPositionInteract(const boost::shared_ptr<KnobDouble>& point) OVERRIDE FINAL;
    
    boost::shared_ptr<DefaultOverlay> getDefaultOverlay() const WARN_UNUSED_RETURN;
    
    virtual void drawDefaultOverlay(double time,double scaleX, double scaleY)  OVERRIDE FINAL;
    
    virtual bool onOverlayPenDownDefault(double scaleX, double scaleY, const QPointF & viewportPos, const QPointF & pos, double pressure)  OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual bool onOverlayPenMotionDefault(double scaleX, double scaleY, const QPointF & viewportPos, const QPointF & pos, double pressure)  OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual bool onOverlayPenUpDefault(double scaleX, double scaleY, const QPointF & viewportPos, const QPointF & pos, double pressure)  OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual bool onOverlayKeyDownDefault(double scaleX, double scaleY, Natron::Key key, Natron::KeyboardModifiers modifiers) OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual bool onOverlayKeyUpDefault(double scaleX, double scaleY, Natron::Key key, Natron::KeyboardModifiers modifiers)  OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual bool onOverlayKeyRepeatDefault(double scaleX, double scaleY, Natron::Key key, Natron::KeyboardModifiers modifiers) OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual bool onOverlayFocusGainedDefault(double scaleX, double scaleY) OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual bool onOverlayFocusLostDefault(double scaleX, double scaleY) OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual bool hasDefaultOverlay() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual void setCurrentViewportForDefaultOverlays(OverlaySupport* viewPort) OVERRIDE FINAL;

    virtual bool hasDefaultOverlayForParam(const KnobI* param) OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual void removeDefaultOverlay(KnobI* knob) OVERRIDE FINAL;
    
    virtual void setPluginIconFilePath(const std::string& filePath) OVERRIDE FINAL;
    
    virtual void setPluginDescription(const std::string& description) OVERRIDE FINAL;
    
    virtual void setPluginIDAndVersion(const std::string& pluginLabel,const std::string& pluginID,unsigned int version) OVERRIDE FINAL;
    
protected:
    
    virtual int getBaseDepth() const { return 20; }
    
    virtual bool canResize() { return true; }
    
    virtual bool mustFrameName() const { return false; }
    
    virtual bool mustAddResizeHandle() const { return false; }
    
    virtual void getInitialSize(int *w, int *h) const;
    
    void getSizeWithPreview(int *w, int *h) const;
    
    virtual void adjustSizeToContent(int *w,int *h,bool adjustToTextSize);
    
    virtual void resizeExtraContent(int /*w*/,int /*h*/,bool /*forceResize*/) {}
    
public Q_SLOTS:

    void onOutputLayerChanged();

    void onSettingsPanelClosed(bool closed);
    
    void onSettingsPanelColorChanged(const QColor & color);

    void togglePreview();

    void onPreviewKnobToggled();

    void onDisabledKnobToggled(bool disabled);

    /**
     * @brief Updates the position of the items contained by the node to fit into
     * the new width and height.
     **/
    void resize(int width,int height, bool forceSize = false, bool adjustToTextSize = false);
    
    void refreshSize();

    /*Updates the preview image, only if the project is in auto-preview mode*/
    void updatePreviewImage(int time);

    /*Updates the preview image no matter what*/
    void forceComputePreview(int time);

    void setName(const QString & _nameItem);

    void onInternalNameChanged(const QString &);

    void onPersistentMessageChanged();

    void refreshEdges();

    void refreshDashedStateOfEdges();
    
    void refreshKnobLinks();

    /*initialises the input edges*/
    void initializeInputs();

    void initializeKnobs();

    void activate(bool triggerRender);

    void deactivate(bool triggerRender);

    void hideGui();

    void showGui();

    /*Use NULL for src to disconnect.*/
    bool connectEdge(int edgeNumber);

    void setVisibleSettingsPanel(bool b);

    void onRenderingStarted();

    void onRenderingFinished();

    void onInputNRenderingStarted(int input);

    void onInputNRenderingFinished(int input);

    void beginEditKnobs();

    bool wasBeginEditCalled()
    {
        return _wasBeginEditCalled;
    }

    void centerGraphOnIt();

    void onAllKnobsSlaved(bool b);

    void onKnobsLinksChanged();

    void refreshOutputEdgeVisibility();

    void toggleBitDepthIndicator(bool on,const QString & tooltip);

    void onNodeExtraLabelChanged(const QString & label);

    void onSwitchInputActionTriggered();

    void onSettingsPanelClosedChanged(bool closed);

    void onParentMultiInstancePositionChanged(int x,int y);
    
    void setOptionalInputsVisible(bool visible);
    
Q_SIGNALS:

    void positionChanged(int x,int y);

    void settingsPanelClosed(bool b);

protected:

    virtual void createGui();
    virtual NodeSettingsPanel* createPanel(QVBoxLayout* container,const boost::shared_ptr<NodeGui> & thisAsShared);
    virtual bool canMakePreview()
    {
        return true;
    }

    virtual void applyBrush(const QBrush & brush);

private:
    
    void setOptionalInputsVisibleInternal(bool visible);
    
    void refreshPositionEnd(double x,double y);

    void setNameItemHtml(const QString & name,const QString & label);

    void togglePreview_internal(bool refreshPreview = true);

    void ensurePreviewCreated();
    
    void setAboveItem(QGraphicsItem* item);

    void computePreviewImage(int time);

    void populateMenu();

    void refreshCurrentBrush();
    
    void initializeInputsForInspector();


    /*pointer to the dag*/
    NodeGraph* _graph;

    /*pointer to the internal node*/
    boost::weak_ptr<Natron::Node> _internalNode;

    /*true if the node is selected by the user*/
    bool _selected;
    mutable QMutex _selectedMutex;

    /*A pointer to the graphical text displaying the name.*/
    bool _settingNameFromGui;
    
    bool _panelOpenedBeforeDeactivate;
    
    QGraphicsPixmapItem* _pluginIcon;
    QGraphicsRectItem* _pluginIconFrame;
    
    QGraphicsPixmapItem* _mergeIcon;
    
    QGraphicsTextItem *_nameItem;
    QGraphicsRectItem *_nameFrame;
    
    QGraphicsPolygonItem* _resizeHandle;

    /*A pointer to the rectangle of the node.*/
    QGraphicsRectItem* _boundingBox;

    /*A pointer to the channels pixmap displayed*/
    QGraphicsPixmapItem* _channelsPixmap;

    /*A pointer to the preview pixmap displayed for readers/*/
    QGraphicsPixmapItem* _previewPixmap;
    QGraphicsTextItem* _persistentMessage;
    QGraphicsRectItem* _stateIndicator;    
    
    bool _mergeHintActive;
    NodeGuiIndicator* _bitDepthWarning;
    QGraphicsLineItem* _disabledTopLeftBtmRight;
    QGraphicsLineItem* _disabledBtmLeftTopRight;
    /*the graphical input arrows*/
    std::vector<Edge*> _inputEdges;
    Edge* _outputEdge;
    NodeSettingsPanel* _settingsPanel;

    ///This is valid only if the node is a multi-instance and this is the main instance.
    ///The "real" panel showed on the gui will be the _settingsPanel, but we still need to create
    ///another panel for the main-instance (hidden) knobs to function properly
    NodeSettingsPanel* _mainInstancePanel;
    
    //True when the settings panel has been  created
    bool _panelCreated;
    
    mutable QMutex _currentColorMutex; //< protects _currentColor
    QColor _currentColor; //< accessed by the serialization thread
    
    QColor _clonedColor;
    bool _wasBeginEditCalled;
    mutable QMutex positionMutex;

    ///This is the garphical red line displayed when the node is a clone
    LinkArrow* _slaveMasterLink;
    boost::weak_ptr<NodeGui> _masterNodeGui;

    ///For each knob that has a link to another parameter, display an arrow
    struct LinkedDim
    {
        std::list<std::pair<KnobI*,KnobI*> > knobs;
        LinkArrow* arrow;
    };

    typedef std::map<boost::shared_ptr<Natron::Node>,LinkedDim> KnobGuiLinks;
    KnobGuiLinks _knobsLinks;
    NodeGuiIndicator* _expressionIndicator;
    QPoint _magnecEnabled; //<enabled in X or/and Y
    QPointF _magnecDistance; //for x and for  y
    QPoint _updateDistanceSinceLastMagnec; //for x and for y
    QPointF _distanceSinceLastMagnec; //for x and for y
    QPointF _magnecStartingPos; //for x and for y
    QString _nodeLabel;
    boost::weak_ptr<NodeGui> _parentMultiInstance;
    
    int _renderingStartedCount;
    std::map<int,int> _inputNRenderingStartedCount;
    
    bool _optionalInputsVisible;
    
    ///For the serialization thread
    mutable QMutex _mtSafeSizeMutex;
    int _mtSafeWidth,_mtSafeHeight;
    
    boost::shared_ptr<DefaultOverlay> _defaultOverlay;
    boost::shared_ptr<QUndoStack> _undoStack; /*!< undo/redo stack*/

    bool _overlayLocked;
};


class DotGui
    : public NodeGui
{
public:

    DotGui(QGraphicsItem *parent = 0);

private:


    virtual void createGui() OVERRIDE FINAL;
    virtual NodeSettingsPanel* createPanel(QVBoxLayout* container, const boost::shared_ptr<NodeGui> & thisAsShared) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool canMakePreview() OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }
    
    virtual void refreshStateIndicator() OVERRIDE FINAL;
    
    virtual void applyBrush(const QBrush & brush) OVERRIDE FINAL;
    
    virtual bool canResize() OVERRIDE FINAL WARN_UNUSED_RETURN { return false; }
    
    virtual QRectF boundingRect() const OVERRIDE FINAL;
    virtual QPainterPath shape() const OVERRIDE FINAL;
    QGraphicsEllipseItem* diskShape;
    QGraphicsEllipseItem* ellipseIndicator;
};

struct ExportGroupTemplateDialogPrivate;
class ExportGroupTemplateDialog : public QDialog
{
    Q_OBJECT
    
public:
    
    ExportGroupTemplateDialog(NodeCollection* group,Gui* gui,QWidget* parent);
    
    virtual ~ExportGroupTemplateDialog();
    
public Q_SLOTS:

    void onButtonClicked();
    
    void onOkClicked();
    
    void onLabelEditingFinished();
    
private:
    
    boost::scoped_ptr<ExportGroupTemplateDialogPrivate> _imp;
};

#endif // NATRON_GUI_NODEGUI_H
