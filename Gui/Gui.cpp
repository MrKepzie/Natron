//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Gui/Gui.h"

#include <cassert>
#include <fstream>

#include <QtCore/QTextStream>
#include <QWaitCondition>
#include <QMutex>
#include <QCoreApplication>
#include <QAction>
#include <QSettings>
#include <QDebug>
#include <QThread>
#include <QCheckBox>
#include <QTimer>
#include <QTextEdit>


#if QT_VERSION >= 0x050000
#include <QScreen>
#endif
#include <QUndoGroup>
CLANG_DIAG_OFF(unused-private-field)
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QCloseEvent>
CLANG_DIAG_ON(unused-private-field)
#include <QHBoxLayout>
#include <QGraphicsScene>
#include <QApplication>
#include <QMenuBar>
#include <QDesktopWidget>
#include <QToolBar>
#include <QKeySequence>
#include <QScrollArea>
#include <QScrollBar>
#include <QToolButton>
#include <QMessageBox>
#include <QImage>
#include <QProgressDialog>

#include <cairo/cairo.h>

#include <boost/version.hpp>
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
GCC_DIAG_ON(unused-parameter)
#include <boost/archive/xml_oarchive.hpp>

#include "Engine/ViewerInstance.h"
#include "Engine/Project.h"
#include "Engine/Plugin.h"
#include "Engine/Settings.h"
#include "Engine/KnobFile.h"
#include "SequenceParsing.h"
#include "Engine/ProcessHandler.h"
#include "Engine/Lut.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/KnobSerialization.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/NodeGroup.h"
#include "Engine/NoOp.h"

#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/NodeGraph.h"
#include "Gui/CurveEditor.h"
#include "Gui/CurveWidget.h"
#include "Gui/DopeSheet.h"
#include "Gui/PreferencesPanel.h"
#include "Gui/AboutWindow.h"
#include "Gui/ProjectGui.h"
#include "Gui/ToolButton.h"
#include "Gui/ViewerTab.h"
#include "Gui/ViewerGL.h"
#include "Gui/TabWidget.h"
#include "Gui/DockablePanel.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/FromQtEnums.h"
#include "Gui/RenderingProgressDialog.h"
#include "Gui/NodeGui.h"
#include "Gui/Histogram.h"
#include "Gui/Splitter.h"
#include "Gui/SpinBox.h"
#include "Gui/Button.h"
#include "Gui/RotoGui.h"
#include "Gui/ProjectGuiSerialization.h"
#include "Gui/ActionShortcuts.h"
#include "Gui/ShortCutEditor.h"
#include "Gui/MessageBox.h"
#include "Gui/MultiInstancePanel.h"
#include "Gui/ScriptEditor.h"
#include "Gui/PythonPanels.h"
#include "Gui/Menu.h"
#include "Gui/Utils.h"

#define kPropertiesBinName "properties"

#define NAMED_PLUGIN_GROUP_NO 15

static std::string namedGroupsOrdered[NAMED_PLUGIN_GROUP_NO] = {
    PLUGIN_GROUP_IMAGE,
    PLUGIN_GROUP_COLOR,
    PLUGIN_GROUP_CHANNEL,
    PLUGIN_GROUP_MERGE,
    PLUGIN_GROUP_FILTER,
    PLUGIN_GROUP_TRANSFORM,
    PLUGIN_GROUP_TIME,
    PLUGIN_GROUP_PAINT,
    PLUGIN_GROUP_KEYER,
    PLUGIN_GROUP_MULTIVIEW,
    PLUGIN_GROUP_DEEP,
    PLUGIN_GROUP_3D,
    PLUGIN_GROUP_TOOLSETS,
    PLUGIN_GROUP_OTHER,
    PLUGIN_GROUP_DEFAULT
};

#define PLUGIN_GROUP_DEFAULT_ICON_PATH NATRON_IMAGES_PATH "GroupingIcons/Set" NATRON_ICON_SET_NUMBER "/other_grouping_" NATRON_ICON_SET_NUMBER ".png"


using namespace Natron;

namespace {
static void
getPixmapForGrouping(QPixmap* pixmap,
                     const QString & grouping)
{
    if (grouping == PLUGIN_GROUP_COLOR) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_COLOR_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_FILTER) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_FILTER_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_IMAGE) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_IO_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_TRANSFORM) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_TRANSFORM_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_DEEP) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_DEEP_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_MULTIVIEW) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_MULTIVIEW_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_TIME) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_TIME_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_PAINT) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_PAINT_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_OTHER) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_MISC_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_KEYER) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_KEYER_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_TOOLSETS) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_TOOLSETS_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_3D) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_3D_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_CHANNEL) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_CHANNEL_GROUPING, pixmap);
    } else if (grouping == PLUGIN_GROUP_MERGE) {
        appPTR->getIcon(Natron::NATRON_PIXMAP_MERGE_GROUPING, pixmap);
    } else {
        appPTR->getIcon(Natron::NATRON_PIXMAP_OTHER_PLUGINS, pixmap);
    }
}
}


class AutoHideToolBar
    : public QToolBar
{
    Gui* _gui;

public:

    AutoHideToolBar(Gui* gui,
                    QWidget* parent) : QToolBar(parent), _gui(gui)
    {
    }

private:

    virtual void leaveEvent(QEvent* e) OVERRIDE FINAL
    {
        if ( _gui->isLeftToolBarDisplayedOnMouseHoverOnly() ) {
            _gui->setLeftToolBarVisible(false);
        }
        QToolBar::leaveEvent(e);
    }
};

struct GuiPrivate
{
    Gui* _gui; //< ptr to the public interface
    mutable QMutex _isUserScrubbingSliderMutex;
    bool _isUserScrubbingSlider; //< true if the user is actively moving the cursor on the timeline or a slider. False on mouse release.
    GuiAppInstance* _appInstance; //< ptr to the appInstance

    ///Dialogs handling members
    QWaitCondition _uiUsingMainThreadCond; //< used with _uiUsingMainThread
    bool _uiUsingMainThread; //< true when the Gui is showing a dialog in the main thread
    mutable QMutex _uiUsingMainThreadMutex; //< protects _uiUsingMainThread
    Natron::StandardButtonEnum _lastQuestionDialogAnswer; //< stores the last question answer
    bool _lastStopAskingAnswer;

    ///ptrs to the undo/redo actions from the active stack.
    QAction* _currentUndoAction;
    QAction* _currentRedoAction;

    ///all the undo stacks of Natron are gathered here
    QUndoGroup* _undoStacksGroup;
    std::map<QUndoStack*, std::pair<QAction*, QAction*> > _undoStacksActions;

    ///all the splitters used to separate the "panes" of the application
    mutable QMutex _splittersMutex;
    std::list<Splitter*> _splitters;
    mutable QMutex _pyPanelsMutex;
    std::map<PyPanel*, std::string> _userPanels;

    bool _isTripleSyncEnabled;


    ///all the menu actions
    ActionWithShortcut *actionNew_project;
    ActionWithShortcut *actionOpen_project;
    ActionWithShortcut *actionClose_project;
    ActionWithShortcut *actionSave_project;
    ActionWithShortcut *actionSaveAs_project;
    ActionWithShortcut *actionExportAsGroup;
    ActionWithShortcut *actionSaveAndIncrementVersion;
    ActionWithShortcut *actionPreferences;
    ActionWithShortcut *actionExit;
    ActionWithShortcut *actionProject_settings;
    ActionWithShortcut *actionShowOfxLog;
    ActionWithShortcut *actionShortcutEditor;
    ActionWithShortcut *actionNewViewer;
    ActionWithShortcut *actionFullScreen;
    ActionWithShortcut *actionClearDiskCache;
    ActionWithShortcut *actionClearPlayBackCache;
    ActionWithShortcut *actionClearNodeCache;
    ActionWithShortcut *actionClearPluginsLoadingCache;
    ActionWithShortcut *actionClearAllCaches;
    ActionWithShortcut *actionShowAboutWindow;
    QAction *actionsOpenRecentFile[NATRON_MAX_RECENT_FILES];
    ActionWithShortcut *renderAllWriters;
    ActionWithShortcut *renderSelectedNode;
    ActionWithShortcut* actionConnectInput1;
    ActionWithShortcut* actionConnectInput2;
    ActionWithShortcut* actionConnectInput3;
    ActionWithShortcut* actionConnectInput4;
    ActionWithShortcut* actionConnectInput5;
    ActionWithShortcut* actionConnectInput6;
    ActionWithShortcut* actionConnectInput7;
    ActionWithShortcut* actionConnectInput8;
    ActionWithShortcut* actionConnectInput9;
    ActionWithShortcut* actionConnectInput10;
    ActionWithShortcut* actionImportLayout;
    ActionWithShortcut* actionExportLayout;
    ActionWithShortcut* actionRestoreDefaultLayout;
    ActionWithShortcut* actionNextTab;
    ActionWithShortcut* actionCloseTab;

    ///the main "central" widget
    QWidget *_centralWidget;
    QHBoxLayout* _mainLayout; //< its layout

    ///strings that remember for project save/load and writers/reader dialog where
    ///the user was the last time.
    QString _lastLoadSequenceOpenedDir;
    QString _lastLoadProjectOpenedDir;
    QString _lastSaveSequenceOpenedDir;
    QString _lastSaveProjectOpenedDir;
    QString _lastPluginDir;

    // this one is a ptr to others TabWidget.
    //It tells where to put the viewer when making a new one
    // If null it places it on default tab widget
    TabWidget* _nextViewerTabPlace;

    ///the splitter separating the gui and the left toolbar
    Splitter* _leftRightSplitter;

    ///a list of ptrs to all the viewer tabs.
    mutable QMutex _viewerTabsMutex;
    std::list<ViewerTab*> _viewerTabs;

    ///a list of ptrs to all histograms
    mutable QMutex _histogramsMutex;
    std::list<Histogram*> _histograms;
    int _nextHistogramIndex; //< for giving a unique name to histogram tabs

    ///The node graph (i.e: the view of the scene)
    NodeGraph* _nodeGraphArea;
    NodeGraph* _lastFocusedGraph;
    std::list<NodeGraph*> _groups;

    ///The curve editor.
    CurveEditor *_curveEditor;

    // The dope sheet
    DopeSheetEditor *_dopeSheetEditor;

    ///the left toolbar
    QToolBar* _toolBox;

    ///a vector of all the toolbuttons
    std::vector<ToolButton* > _toolButtons;

    ///holds the properties dock
    PropertiesBinWrapper *_propertiesBin;
    QScrollArea* _propertiesScrollArea;
    QWidget* _propertiesContainer;

    ///the vertical layout for the properties dock container.
    QVBoxLayout *_layoutPropertiesBin;
    Button* _clearAllPanelsButton;
    Button* _minimizeAllPanelsButtons;
    SpinBox* _maxPanelsOpenedSpinBox;
    QMutex _isGUIFrozenMutex;
    bool _isGUIFrozen;

    ///The menu bar and all the menus
    QMenuBar *menubar;
    Menu *menuFile;
    Menu *menuRecentFiles;
    Menu *menuEdit;
    Menu *menuLayout;
    Menu *menuDisplay;
    Menu *menuOptions;
    Menu *menuRender;
    Menu *viewersMenu;
    Menu *viewerInputsMenu;
    Menu *viewersViewMenu;
    Menu *cacheMenu;


    ///all TabWidget's : used to know what to hide/show for fullscreen mode
    mutable QMutex _panesMutex;
    std::list<TabWidget*> _panes;
    mutable QMutex _floatingWindowMutex;
    std::list<FloatingWidget*> _floatingWindows;

    ///All the tabs used in the TabWidgets (used for d&d purpose)
    RegisteredTabs _registeredTabs;

    ///The user preferences window
    PreferencesPanel* _settingsGui;

    ///The project Gui (stored in the properties pane)
    ProjectGui* _projectGui;

    ///ptr to the currently dragged tab for d&d purpose.
    QWidget* _currentlyDraggedPanel;

    ///The "About" window.
    AboutWindow* _aboutWindow;
    std::map<KnobHolder*, QProgressDialog*> _progressBars;

    ///list of the currently opened property panels
    std::list<DockablePanel*> openedPanels;
    QString _openGLVersion;
    QString _glewVersion;
    QToolButton* _toolButtonMenuOpened;
    QMutex aboutToCloseMutex;
    bool _aboutToClose;
    ShortCutEditor* shortcutEditor;
    bool leftToolBarDisplayedOnHoverOnly;
    ScriptEditor* _scriptEditor;
    TabWidget* _lastEnteredTabWidget;

    ///Menu entries added by the user
    std::map<ActionWithShortcut*, std::string> pythonCommands;

    GuiPrivate(GuiAppInstance* app,
               Gui* gui)
        : _gui(gui)
        , _isUserScrubbingSliderMutex()
        , _isUserScrubbingSlider(false)
        , _appInstance(app)
        , _uiUsingMainThreadCond()
        , _uiUsingMainThread(false)
        , _uiUsingMainThreadMutex()
        , _lastQuestionDialogAnswer(Natron::eStandardButtonNo)
        , _lastStopAskingAnswer(false)
        , _currentUndoAction(0)
        , _currentRedoAction(0)
        , _undoStacksGroup(0)
        , _undoStacksActions()
        , _splittersMutex()
        , _splitters()
        , _pyPanelsMutex()
        , _userPanels()
        , _isTripleSyncEnabled(false)
        , actionNew_project(0)
        , actionOpen_project(0)
        , actionClose_project(0)
        , actionSave_project(0)
        , actionSaveAs_project(0)
        , actionExportAsGroup(0)
        , actionSaveAndIncrementVersion(0)
        , actionPreferences(0)
        , actionExit(0)
        , actionProject_settings(0)
        , actionShowOfxLog(0)
        , actionShortcutEditor(0)
        , actionNewViewer(0)
        , actionFullScreen(0)
        , actionClearDiskCache(0)
        , actionClearPlayBackCache(0)
        , actionClearNodeCache(0)
        , actionClearPluginsLoadingCache(0)
        , actionClearAllCaches(0)
        , actionShowAboutWindow(0)
        , actionsOpenRecentFile()
        , renderAllWriters(0)
        , renderSelectedNode(0)
        , actionConnectInput1(0)
        , actionConnectInput2(0)
        , actionConnectInput3(0)
        , actionConnectInput4(0)
        , actionConnectInput5(0)
        , actionConnectInput6(0)
        , actionConnectInput7(0)
        , actionConnectInput8(0)
        , actionConnectInput9(0)
        , actionConnectInput10(0)
        , actionImportLayout(0)
        , actionExportLayout(0)
        , actionRestoreDefaultLayout(0)
        , actionNextTab(0)
        , actionCloseTab(0)
        , _centralWidget(0)
        , _mainLayout(0)
        , _lastLoadSequenceOpenedDir()
        , _lastLoadProjectOpenedDir()
        , _lastSaveSequenceOpenedDir()
        , _lastSaveProjectOpenedDir()
        , _lastPluginDir()
        , _nextViewerTabPlace(0)
        , _leftRightSplitter(0)
        , _viewerTabsMutex()
        , _viewerTabs()
        , _histogramsMutex()
        , _histograms()
        , _nextHistogramIndex(1)
        , _nodeGraphArea(0)
        , _lastFocusedGraph(0)
        , _groups()
        , _curveEditor(0)
        , _toolBox(0)
        , _propertiesBin(0)
        , _propertiesScrollArea(0)
        , _propertiesContainer(0)
        , _layoutPropertiesBin(0)
        , _clearAllPanelsButton(0)
        , _minimizeAllPanelsButtons(0)
        , _maxPanelsOpenedSpinBox(0)
        , _isGUIFrozenMutex()
        , _isGUIFrozen(false)
        , menubar(0)
        , menuFile(0)
        , menuRecentFiles(0)
        , menuEdit(0)
        , menuLayout(0)
        , menuDisplay(0)
        , menuOptions(0)
        , menuRender(0)
        , viewersMenu(0)
        , viewerInputsMenu(0)
        , viewersViewMenu(0)
        , cacheMenu(0)
        , _panesMutex()
        , _panes()
        , _floatingWindowMutex()
        , _floatingWindows()
        , _settingsGui(0)
        , _projectGui(0)
        , _currentlyDraggedPanel(0)
        , _aboutWindow(0)
        , _progressBars()
        , openedPanels()
        , _openGLVersion()
        , _glewVersion()
        , _toolButtonMenuOpened(NULL)
        , aboutToCloseMutex()
        , _aboutToClose(false)
        , shortcutEditor(0)
        , leftToolBarDisplayedOnHoverOnly(false)
        , _scriptEditor(0)
        , _lastEnteredTabWidget(0)
        , pythonCommands()
    {
    }

    void restoreGuiGeometry();

    void saveGuiGeometry();

    void setUndoRedoActions(QAction* undoAction, QAction* redoAction);

    void addToolButton(ToolButton* tool);

    ///Creates the properties bin and appends it as a tab to the propertiesPane TabWidget
    void createPropertiesBinGui();

    void notifyGuiClosing();

    ///Must be called absolutely before createPropertiesBinGui
    void createNodeGraphGui();

    void createCurveEditorGui();

    void createDopeSheetGui();

    void createScriptEditorGui();

    ///If there's only 1 non-floating pane in the main window, return it, otherwise returns NULL
    TabWidget* getOnly1NonFloatingPane(int & count) const;

    void refreshLeftToolBarVisibility(const QPoint & p);

    QAction* findActionRecursive(int i, QWidget* widget, const QStringList & grouping);
    
    ///True= yes overwrite
    bool checkProjectLockAndWarn(const QString& projectPath,const QString& projectName);
};

// Helper function: Get the icon with the given name from the icon theme.
// If unavailable, fall back to the built-in icon. Icon names conform to this specification:
// http://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html
static QIcon
get_icon(const QString &name)
{
    return QIcon::fromTheme( name, QIcon(QString(":icons/") + name) );
}

Gui::Gui(GuiAppInstance* app,
         QWidget* parent)
    : QMainWindow(parent)
    , SerializableWindow()
    , _imp( new GuiPrivate(app, this) )

{
    QObject::connect( this, SIGNAL( doDialog(int, QString, QString, bool, Natron::StandardButtons, int) ), this,
                      SLOT( onDoDialog(int, QString, QString, bool, Natron::StandardButtons, int) ) );
    QObject::connect( this, SIGNAL( doDialogWithStopAskingCheckbox(int, QString, QString, bool, Natron::StandardButtons, int) ), this,
                      SLOT( onDoDialogWithStopAskingCheckbox(int, QString, QString, bool, Natron::StandardButtons, int) ) );
    QObject::connect( app, SIGNAL( pluginsPopulated() ), this, SLOT( addToolButttonsToToolBar() ) );
}

Gui::~Gui()
{
    _imp->_nodeGraphArea->invalidateAllNodesParenting();
    delete _imp->_projectGui;
    delete _imp->_undoStacksGroup;
    _imp->_viewerTabs.clear();
    for (U32 i = 0; i < _imp->_toolButtons.size(); ++i) {
        delete _imp->_toolButtons[i];
    }
}

bool
Gui::isLeftToolBarDisplayedOnMouseHoverOnly() const
{
    return _imp->leftToolBarDisplayedOnHoverOnly;
}

void
Gui::setLeftToolBarDisplayedOnMouseHoverOnly(bool b)
{
    _imp->leftToolBarDisplayedOnHoverOnly = b;
    QPoint p = QCursor::pos();

    if (b) {
        _imp->refreshLeftToolBarVisibility( mapFromGlobal(p) );
    } else {
        _imp->_toolBox->show();
    }
}

void
Gui::refreshLeftToolBarVisibility(const QPoint & globalPos)
{
    _imp->refreshLeftToolBarVisibility( mapFromGlobal(globalPos) );
}

void
Gui::setLeftToolBarVisible(bool visible)
{
    _imp->_toolBox->setVisible(visible);
}

void
GuiPrivate::refreshLeftToolBarVisibility(const QPoint & p)
{
    int toolbarW = _toolBox->sizeHint().width();

    if (p.x() <= toolbarW) {
        _toolBox->show();
    } else {
        _toolBox->hide();
    }
}

void
GuiPrivate::notifyGuiClosing()
{
    ///This is to workaround an issue that when destroying a widget it calls the focusOut() handler hence can
    ///cause bad pointer dereference to the Gui object since we're destroying it.
    {
        QMutexLocker k(&_viewerTabsMutex);
        for (std::list<ViewerTab*>::iterator it = _viewerTabs.begin(); it != _viewerTabs.end(); ++it) {
            (*it)->notifyAppClosing();
        }
    }

    const std::list<boost::shared_ptr<NodeGui> > allNodes = _nodeGraphArea->getAllActiveNodes();

    for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = allNodes.begin(); it != allNodes.end(); ++it) {
        DockablePanel* panel = (*it)->getSettingPanel();
        if (panel) {
            panel->onGuiClosing();
        }
    }
    _lastFocusedGraph = 0;
    _nodeGraphArea->discardGuiPointer();
    for (std::list<NodeGraph*>::iterator it = _groups.begin(); it != _groups.end(); ++it) {
        (*it)->discardGuiPointer();
    }

    {
        QMutexLocker k(&_panesMutex);
        for (std::list<TabWidget*>::iterator it = _panes.begin(); it != _panes.end(); ++it) {
            (*it)->discardGuiPointer();
        }
    }
}

bool
Gui::closeInstance()
{
    if ( getApp()->getProject()->hasNodes() ) {
        int ret = saveWarning();
        if (ret == 0) {
            if ( !saveProject() ) {
                return false;
            }
        } else if (ret == 2) {
            return false;
        }
    }
    _imp->saveGuiGeometry();
    abortProject(true);

    return true;
}

void
Gui::closeProject()
{
    if ( getApp()->getProject()->hasNodes() ) {
        int ret = saveWarning();
        if (ret == 0) {
            if ( !saveProject() ) {
                return;
            }
        } else if (ret == 2) {
            return;
        }
    }
    ///When closing a project we can remove the ViewerCache from memory and put it on disk
    ///since we're not sure it will be used right away
    appPTR->clearPlaybackCache();
    abortProject(false);

    _imp->_appInstance->getProject()->createViewer();
    _imp->_appInstance->execOnProjectCreatedCallback();
}

void
Gui::abortProject(bool quitApp)
{
    if (quitApp) {
        ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
        {
            QMutexLocker l(&_imp->aboutToCloseMutex);
            _imp->_aboutToClose = true;
        }

        assert(_imp->_appInstance);

        _imp->_appInstance->getProject()->closeProject(true);
        _imp->notifyGuiClosing();
        _imp->_appInstance->quit();
    } else {
        _imp->_appInstance->resetPreviewProvider();
        _imp->_appInstance->getProject()->closeProject(false);
        centerAllNodeGraphsWithTimer();
        restoreDefaultLayout();
    }

    ///Reset current undo/reso actions
    _imp->_currentUndoAction = 0;
    _imp->_currentRedoAction = 0;
}

void
Gui::toggleFullScreen()
{
    QWidget* activeWin = qApp->activeWindow();

    if (!activeWin) {
        qApp->setActiveWindow(this);
        activeWin = this;
    }

    if ( activeWin->isFullScreen() ) {
        activeWin->showNormal();
    } else {
        activeWin->showFullScreen();
    }
}

void
Gui::closeEvent(QCloseEvent* e)
{
    assert(e);
    if ( _imp->_appInstance->isClosing() ) {
        e->ignore();
    } else {
        if ( !closeInstance() ) {
            e->ignore();

            return;
        }
        e->accept();
    }
}

boost::shared_ptr<NodeGui>
Gui::createNodeGUI( boost::shared_ptr<Node> node,
                    bool requestedByLoad,
                    double xPosHint,
                    double yPosHint,
                    bool pushUndoRedoCommand,
                    bool autoConnect)
{
    assert(_imp->_nodeGraphArea);

    boost::shared_ptr<NodeCollection> group = node->getGroup();
    NodeGraph* graph;
    if (group) {
        NodeGraphI* graph_i = group->getNodeGraph();
        assert(graph_i);
        graph = dynamic_cast<NodeGraph*>(graph_i);
        assert(graph);
    } else {
        graph = _imp->_nodeGraphArea;
    }
    boost::shared_ptr<NodeGui> nodeGui = graph->createNodeGUI(node, requestedByLoad,
                                                              xPosHint, yPosHint, pushUndoRedoCommand, autoConnect);
    QObject::connect( node.get(), SIGNAL( labelChanged(QString) ), this, SLOT( onNodeNameChanged(QString) ) );
    assert(nodeGui);

    return nodeGui;
}

void
Gui::addNodeGuiToCurveEditor(const boost::shared_ptr<NodeGui> &node)
{
    _imp->_curveEditor->addNode(node);
}

void
Gui::removeNodeGuiFromCurveEditor(const boost::shared_ptr<NodeGui>& node)
{
    _imp->_curveEditor->removeNode(node.get());
}

void Gui::addNodeGuiToDopeSheetEditor(const boost::shared_ptr<NodeGui> &node)
{
    _imp->_dopeSheetEditor->addNode(node);
}

void Gui::removeNodeGuiFromDopeSheetEditor(const boost::shared_ptr<NodeGui> &node)
{
    _imp->_dopeSheetEditor->removeNode(node.get());
}

void
Gui::createViewerGui(boost::shared_ptr<Node> viewer)
{
    TabWidget* where = _imp->_nextViewerTabPlace;

    if (!where) {
        where = getAnchor();
    } else {
        _imp->_nextViewerTabPlace = NULL; // < reseting next viewer anchor to default
    }
    assert(where);

    ViewerInstance* v = dynamic_cast<ViewerInstance*>( viewer->getLiveInstance() );
    assert(v);

    ViewerTab* tab = addNewViewerTab(v, where);
    
    NodeGraph* graph = 0;
    boost::shared_ptr<NodeCollection> collection = viewer->getGroup();
    if (!collection) {
        return;
    }
    NodeGroup* isGrp = dynamic_cast<NodeGroup*>( collection.get() );
    if (isGrp) {
        NodeGraphI* graph_i = isGrp->getNodeGraph();
        assert(graph_i);
        graph = dynamic_cast<NodeGraph*>(graph_i);
    } else {
        graph = getNodeGraph();
    }
    assert(graph);
    graph->setLastSelectedViewer(tab);
}

const std::list<boost::shared_ptr<NodeGui> > &
Gui::getSelectedNodes() const
{
    assert(_imp->_nodeGraphArea);

    return _imp->_nodeGraphArea->getSelectedNodes();
}

void
Gui::createGui()
{
    setupUi();

    ///post a fake event so the qt handlers are called and the proper widget receives the focus
    QMouseEvent e(QEvent::MouseMove, QCursor::pos(), Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    qApp->sendEvent(this, &e);
}

bool
Gui::eventFilter(QObject *target,
                 QEvent* e)
{
    if (_imp->_aboutToClose) {
        return true;
    }
    assert(_imp->_appInstance);
    if ( dynamic_cast<QInputEvent*>(e) ) {
        /*Make top level instance this instance since it receives all
           user inputs.*/
        appPTR->setAsTopLevelInstance( _imp->_appInstance->getAppID() );
    }

    return QMainWindow::eventFilter(target, e);
}

void
Gui::createMenuActions()
{
    _imp->menubar = new QMenuBar(this);
    setMenuBar(_imp->menubar);

    _imp->menuFile = new Menu(QObject::tr("File"), _imp->menubar);
    _imp->menuRecentFiles = new Menu(QObject::tr("Open Recent"), _imp->menuFile);
    _imp->menuEdit = new Menu(QObject::tr("Edit"), _imp->menubar);
    _imp->menuLayout = new Menu(QObject::tr("Layout"), _imp->menubar);
    _imp->menuDisplay = new Menu(QObject::tr("Display"), _imp->menubar);
    _imp->menuOptions = new Menu(QObject::tr("Options"), _imp->menubar);
    _imp->menuRender = new Menu(QObject::tr("Render"), _imp->menubar);
    _imp->viewersMenu = new Menu(QObject::tr("Viewer(s)"), _imp->menuDisplay);
    _imp->viewerInputsMenu = new Menu(QObject::tr("Connect Current Viewer"), _imp->viewersMenu);
    _imp->viewersViewMenu = new Menu(QObject::tr("Display View Number"), _imp->viewersMenu);
    _imp->cacheMenu = new Menu(QObject::tr("Cache"), _imp->menubar);


    _imp->actionNew_project = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionNewProject, kShortcutDescActionNewProject, this);
    _imp->actionNew_project->setIcon( get_icon("document-new") );
    QObject::connect( _imp->actionNew_project, SIGNAL( triggered() ), this, SLOT( newProject() ) );

    _imp->actionOpen_project = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionOpenProject, kShortcutDescActionOpenProject, this);
    _imp->actionOpen_project->setIcon( get_icon("document-open") );
    QObject::connect( _imp->actionOpen_project, SIGNAL( triggered() ), this, SLOT( openProject() ) );

    _imp->actionClose_project = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionCloseProject, kShortcutDescActionCloseProject, this);
    _imp->actionClose_project->setIcon( get_icon("document-close") );
    QObject::connect( _imp->actionClose_project, SIGNAL( triggered() ), this, SLOT( closeProject() ) );

    _imp->actionSave_project = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionSaveProject, kShortcutDescActionSaveProject, this);
    _imp->actionSave_project->setIcon( get_icon("document-save") );
    QObject::connect( _imp->actionSave_project, SIGNAL( triggered() ), this, SLOT( saveProject() ) );

    _imp->actionSaveAs_project = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionSaveAsProject, kShortcutDescActionSaveAsProject, this);
    _imp->actionSaveAs_project->setIcon( get_icon("document-save-as") );
    QObject::connect( _imp->actionSaveAs_project, SIGNAL( triggered() ), this, SLOT( saveProjectAs() ) );

    _imp->actionExportAsGroup = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionExportProject, kShortcutDescActionExportProject, this);
    _imp->actionExportAsGroup->setIcon( get_icon("document-save-as") );
    QObject::connect( _imp->actionExportAsGroup, SIGNAL( triggered() ), this, SLOT( exportProjectAsGroup() ) );

    _imp->actionSaveAndIncrementVersion = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionSaveAndIncrVersion, kShortcutDescActionSaveAndIncrVersion, this);
    QObject::connect( _imp->actionSaveAndIncrementVersion, SIGNAL( triggered() ), this, SLOT( saveAndIncrVersion() ) );

    _imp->actionPreferences = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionPreferences, kShortcutDescActionPreferences, this);
    _imp->actionPreferences->setMenuRole(QAction::PreferencesRole);
    QObject::connect( _imp->actionPreferences, SIGNAL( triggered() ), this, SLOT( showSettings() ) );

    _imp->actionExit = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionQuit, kShortcutDescActionQuit, this);
    _imp->actionExit->setMenuRole(QAction::QuitRole);
    _imp->actionExit->setIcon( get_icon("application-exit") );
    QObject::connect( _imp->actionExit, SIGNAL( triggered() ), appPTR, SLOT( exitApp() ) );

    _imp->actionProject_settings = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionProjectSettings, kShortcutDescActionProjectSettings, this);
    _imp->actionProject_settings->setIcon( get_icon("document-properties") );
    QObject::connect( _imp->actionProject_settings, SIGNAL( triggered() ), this, SLOT( setVisibleProjectSettingsPanel() ) );

    _imp->actionShowOfxLog = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionShowOFXLog, kShortcutDescActionShowOFXLog, this);
    QObject::connect( _imp->actionShowOfxLog, SIGNAL( triggered() ), this, SLOT( showOfxLog() ) );

    _imp->actionShortcutEditor = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionShowShortcutEditor, kShortcutDescActionShowShortcutEditor, this);
    QObject::connect( _imp->actionShortcutEditor, SIGNAL( triggered() ), this, SLOT( showShortcutEditor() ) );

    _imp->actionNewViewer = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionNewViewer, kShortcutDescActionNewViewer, this);
    QObject::connect( _imp->actionNewViewer, SIGNAL( triggered() ), this, SLOT( createNewViewer() ) );

    _imp->actionFullScreen = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionFullscreen, kShortcutDescActionFullscreen, this);
    _imp->actionFullScreen->setIcon( get_icon("view-fullscreen") );
    _imp->actionFullScreen->setShortcutContext(Qt::ApplicationShortcut);
    QObject::connect( _imp->actionFullScreen, SIGNAL( triggered() ), this, SLOT( toggleFullScreen() ) );

    _imp->actionClearDiskCache = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionClearDiskCache, kShortcutDescActionClearDiskCache, this);
    QObject::connect( _imp->actionClearDiskCache, SIGNAL( triggered() ), appPTR, SLOT( clearDiskCache() ) );

    _imp->actionClearPlayBackCache = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionClearPlaybackCache, kShortcutDescActionClearPlaybackCache, this);
    QObject::connect( _imp->actionClearPlayBackCache, SIGNAL( triggered() ), appPTR, SLOT( clearPlaybackCache() ) );

    _imp->actionClearNodeCache = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionClearNodeCache, kShortcutDescActionClearNodeCache, this);
    QObject::connect( _imp->actionClearNodeCache, SIGNAL( triggered() ), appPTR, SLOT( clearNodeCache() ) );
    QObject::connect( _imp->actionClearNodeCache, SIGNAL( triggered() ), _imp->_appInstance, SLOT( clearOpenFXPluginsCaches() ) );

    _imp->actionClearPluginsLoadingCache = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionClearPluginsLoadCache, kShortcutDescActionClearPluginsLoadCache, this);
    QObject::connect( _imp->actionClearPluginsLoadingCache, SIGNAL( triggered() ), appPTR, SLOT( clearPluginsLoadedCache() ) );

    _imp->actionClearAllCaches = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionClearAllCaches, kShortcutDescActionClearAllCaches, this);
    QObject::connect( _imp->actionClearAllCaches, SIGNAL( triggered() ), appPTR, SLOT( clearAllCaches() ) );

    _imp->actionShowAboutWindow = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionShowAbout, kShortcutDescActionShowAbout, this);
    _imp->actionShowAboutWindow->setMenuRole(QAction::AboutRole);
    QObject::connect( _imp->actionShowAboutWindow, SIGNAL( triggered() ), this, SLOT( showAbout() ) );

    _imp->renderAllWriters = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionRenderAll, kShortcutDescActionRenderAll, this);
    QObject::connect( _imp->renderAllWriters, SIGNAL( triggered() ), this, SLOT( renderAllWriters() ) );

    _imp->renderSelectedNode = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionRenderSelected, kShortcutDescActionRenderSelected, this);
    QObject::connect( _imp->renderSelectedNode, SIGNAL( triggered() ), this, SLOT( renderSelectedNode() ) );


    for (int c = 0; c < NATRON_MAX_RECENT_FILES; ++c) {
        _imp->actionsOpenRecentFile[c] = new QAction(this);
        _imp->actionsOpenRecentFile[c]->setVisible(false);
        connect( _imp->actionsOpenRecentFile[c], SIGNAL( triggered() ), this, SLOT( openRecentFile() ) );
    }

    _imp->actionConnectInput1 = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput1, kShortcutDescActionConnectViewerToInput1, this);
    QObject::connect( _imp->actionConnectInput1, SIGNAL( triggered() ), this, SLOT( connectInput1() ) );

    _imp->actionConnectInput2 = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput2, kShortcutDescActionConnectViewerToInput2, this);
    QObject::connect( _imp->actionConnectInput2, SIGNAL( triggered() ), this, SLOT( connectInput2() ) );

    _imp->actionConnectInput3 = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput3, kShortcutDescActionConnectViewerToInput3, this);
    QObject::connect( _imp->actionConnectInput3, SIGNAL( triggered() ), this, SLOT( connectInput3() ) );

    _imp->actionConnectInput4 = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput4, kShortcutDescActionConnectViewerToInput4, this);
    QObject::connect( _imp->actionConnectInput4, SIGNAL( triggered() ), this, SLOT( connectInput4() ) );

    _imp->actionConnectInput5 = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput5, kShortcutDescActionConnectViewerToInput5, this);
    QObject::connect( _imp->actionConnectInput5, SIGNAL( triggered() ), this, SLOT( connectInput5() ) );


    _imp->actionConnectInput6 = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput6, kShortcutDescActionConnectViewerToInput6, this);
    QObject::connect( _imp->actionConnectInput6, SIGNAL( triggered() ), this, SLOT( connectInput6() ) );

    _imp->actionConnectInput7 = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput7, kShortcutDescActionConnectViewerToInput7, this);
    QObject::connect( _imp->actionConnectInput7, SIGNAL( triggered() ), this, SLOT( connectInput7() ) );

    _imp->actionConnectInput8 = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput8, kShortcutDescActionConnectViewerToInput8, this);
    QObject::connect( _imp->actionConnectInput8, SIGNAL( triggered() ), this, SLOT( connectInput8() ) );

    _imp->actionConnectInput9 = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput9, kShortcutDescActionConnectViewerToInput9, this);

    _imp->actionConnectInput10 = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput10, kShortcutDescActionConnectViewerToInput10, this);
    QObject::connect( _imp->actionConnectInput9, SIGNAL( triggered() ), this, SLOT( connectInput9() ) );
    QObject::connect( _imp->actionConnectInput10, SIGNAL( triggered() ), this, SLOT( connectInput10() ) );

    _imp->actionImportLayout = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionImportLayout, kShortcutDescActionImportLayout, this);
    QObject::connect( _imp->actionImportLayout, SIGNAL( triggered() ), this, SLOT( importLayout() ) );

    _imp->actionExportLayout = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionExportLayout, kShortcutDescActionExportLayout, this);
    QObject::connect( _imp->actionExportLayout, SIGNAL( triggered() ), this, SLOT( exportLayout() ) );

    _imp->actionRestoreDefaultLayout = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionDefaultLayout, kShortcutDescActionDefaultLayout, this);
    QObject::connect( _imp->actionRestoreDefaultLayout, SIGNAL( triggered() ), this, SLOT( restoreDefaultLayout() ) );

    _imp->actionNextTab = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionNextTab, kShortcutDescActionNextTab, this);
    QObject::connect( _imp->actionNextTab, SIGNAL( triggered() ), this, SLOT( onNextTabTriggered() ) );
    _imp->actionCloseTab = new ActionWithShortcut(kShortcutGroupGlobal, kShortcutIDActionCloseTab, kShortcutDescActionCloseTab, this);
    QObject::connect( _imp->actionCloseTab, SIGNAL( triggered() ), this, SLOT( onCloseTabTriggered() ) );

    _imp->menubar->addAction( _imp->menuFile->menuAction() );
    _imp->menubar->addAction( _imp->menuEdit->menuAction() );
    _imp->menubar->addAction( _imp->menuLayout->menuAction() );
    _imp->menubar->addAction( _imp->menuDisplay->menuAction() );
    _imp->menubar->addAction( _imp->menuOptions->menuAction() );
    _imp->menubar->addAction( _imp->menuRender->menuAction() );
    _imp->menubar->addAction( _imp->cacheMenu->menuAction() );
    _imp->menuFile->addAction(_imp->actionShowAboutWindow);
    _imp->menuFile->addAction(_imp->actionNew_project);
    _imp->menuFile->addAction(_imp->actionOpen_project);
    _imp->menuFile->addAction( _imp->menuRecentFiles->menuAction() );
    updateRecentFileActions();
    for (int c = 0; c < NATRON_MAX_RECENT_FILES; ++c) {
        _imp->menuRecentFiles->addAction(_imp->actionsOpenRecentFile[c]);
    }

    _imp->menuFile->addSeparator();
    _imp->menuFile->addAction(_imp->actionClose_project);
    _imp->menuFile->addAction(_imp->actionSave_project);
    _imp->menuFile->addAction(_imp->actionSaveAs_project);
    _imp->menuFile->addAction(_imp->actionSaveAndIncrementVersion);
    _imp->menuFile->addAction(_imp->actionExportAsGroup);
    _imp->menuFile->addSeparator();
    _imp->menuFile->addAction(_imp->actionExit);

    _imp->menuEdit->addAction(_imp->actionPreferences);

    _imp->menuLayout->addAction(_imp->actionImportLayout);
    _imp->menuLayout->addAction(_imp->actionExportLayout);
    _imp->menuLayout->addAction(_imp->actionRestoreDefaultLayout);
    _imp->menuLayout->addAction(_imp->actionNextTab);
    _imp->menuLayout->addAction(_imp->actionCloseTab);

    _imp->menuOptions->addAction(_imp->actionProject_settings);
    _imp->menuOptions->addAction(_imp->actionShowOfxLog);
    _imp->menuOptions->addAction(_imp->actionShortcutEditor);
    _imp->menuDisplay->addAction(_imp->actionNewViewer);
    _imp->menuDisplay->addAction( _imp->viewersMenu->menuAction() );
    _imp->viewersMenu->addAction( _imp->viewerInputsMenu->menuAction() );
    _imp->viewersMenu->addAction( _imp->viewersViewMenu->menuAction() );
    _imp->viewerInputsMenu->addAction(_imp->actionConnectInput1);
    _imp->viewerInputsMenu->addAction(_imp->actionConnectInput2);
    _imp->viewerInputsMenu->addAction(_imp->actionConnectInput3);
    _imp->viewerInputsMenu->addAction(_imp->actionConnectInput4);
    _imp->viewerInputsMenu->addAction(_imp->actionConnectInput5);
    _imp->viewerInputsMenu->addAction(_imp->actionConnectInput6);
    _imp->viewerInputsMenu->addAction(_imp->actionConnectInput7);
    _imp->viewerInputsMenu->addAction(_imp->actionConnectInput8);
    _imp->viewerInputsMenu->addAction(_imp->actionConnectInput9);
    _imp->viewerInputsMenu->addAction(_imp->actionConnectInput10);
    _imp->menuDisplay->addSeparator();
    _imp->menuDisplay->addAction(_imp->actionFullScreen);

    _imp->menuRender->addAction(_imp->renderAllWriters);
    _imp->menuRender->addAction(_imp->renderSelectedNode);

    _imp->cacheMenu->addAction(_imp->actionClearDiskCache);
    _imp->cacheMenu->addAction(_imp->actionClearPlayBackCache);
    _imp->cacheMenu->addAction(_imp->actionClearNodeCache);
    _imp->cacheMenu->addAction(_imp->actionClearAllCaches);
    _imp->cacheMenu->addSeparator();
    _imp->cacheMenu->addAction(_imp->actionClearPluginsLoadingCache);

    ///Create custom menu
    const std::list<PythonUserCommand> & commands = appPTR->getUserPythonCommands();
    for (std::list<PythonUserCommand>::const_iterator it = commands.begin(); it != commands.end(); ++it) {
        addMenuEntry(it->grouping, it->pythonFunction, it->key, it->modifiers);
    }
} // createMenuActions

void
Gui::setupUi()
{
    setWindowTitle( QCoreApplication::applicationName() );
    setMouseTracking(true);
    installEventFilter(this);
    assert( !isFullScreen() );

    loadStyleSheet();

    ///Restores position, size of the main window as well as whether it was fullscreen or not.
    _imp->restoreGuiGeometry();


    _imp->_undoStacksGroup = new QUndoGroup;
    QObject::connect( _imp->_undoStacksGroup, SIGNAL( activeStackChanged(QUndoStack*) ), this, SLOT( onCurrentUndoStackChanged(QUndoStack*) ) );

    createMenuActions();

    /*CENTRAL AREA*/
    //======================
    _imp->_centralWidget = new QWidget(this);
    setCentralWidget(_imp->_centralWidget);
    _imp->_mainLayout = new QHBoxLayout(_imp->_centralWidget);
    _imp->_mainLayout->setContentsMargins(0, 0, 0, 0);
    _imp->_centralWidget->setLayout(_imp->_mainLayout);

    _imp->_leftRightSplitter = new Splitter(_imp->_centralWidget);
    _imp->_leftRightSplitter->setChildrenCollapsible(false);
    _imp->_leftRightSplitter->setObjectName(kMainSplitterObjectName);
    _imp->_splitters.push_back(_imp->_leftRightSplitter);
    _imp->_leftRightSplitter->setOrientation(Qt::Horizontal);
    _imp->_leftRightSplitter->setContentsMargins(0, 0, 0, 0);


    _imp->_toolBox = new AutoHideToolBar(this, _imp->_leftRightSplitter);
    _imp->_toolBox->setOrientation(Qt::Vertical);
    _imp->_toolBox->setMaximumWidth(40);

    if (_imp->leftToolBarDisplayedOnHoverOnly) {
        _imp->refreshLeftToolBarVisibility( mapFromGlobal( QCursor::pos() ) );
    }

    _imp->_leftRightSplitter->addWidget(_imp->_toolBox);

    _imp->_mainLayout->addWidget(_imp->_leftRightSplitter);

    _imp->createNodeGraphGui();
    _imp->createCurveEditorGui();
    _imp->createDopeSheetGui();
    _imp->createScriptEditorGui();
    ///Must be absolutely called once _nodeGraphArea has been initialized.
    _imp->createPropertiesBinGui();

    createDefaultLayoutInternal(false);

    _imp->_projectGui = new ProjectGui(this);
    _imp->_projectGui->create(_imp->_appInstance->getProject(),
                              _imp->_layoutPropertiesBin,
                              this);

    initProjectGuiKnobs();


    setVisibleProjectSettingsPanel();

    _imp->_aboutWindow = new AboutWindow(this, this);
    _imp->_aboutWindow->hide();

    _imp->shortcutEditor = new ShortCutEditor(this);
    _imp->shortcutEditor->hide();


    //the same action also clears the ofx plugins caches, they are not the same cache but are used to the same end
    QObject::connect( _imp->_appInstance->getProject().get(), SIGNAL( projectNameChanged(QString) ), this, SLOT( onProjectNameChanged(QString) ) );


    /*Searches recursively for all child objects of the given object,
       and connects matching signals from them to slots of object that follow the following form:

        void on_<object name>_<signal name>(<signal parameters>);

       Let's assume our object has a child object of type QPushButton with the object name button1.
       The slot to catch the button's clicked() signal would be:

       void on_button1_clicked();

       If object itself has a properly set object name, its own signals are also connected to its respective slots.
     */
    QMetaObject::connectSlotsByName(this);
} // setupUi

void
GuiPrivate::createPropertiesBinGui()
{
    _propertiesBin = new PropertiesBinWrapper(_gui);
    _propertiesBin->setScriptName(kPropertiesBinName);
    _propertiesBin->setLabel( QObject::tr("Properties").toStdString() );

    QVBoxLayout* mainPropertiesLayout = new QVBoxLayout(_propertiesBin);
    mainPropertiesLayout->setContentsMargins(0, 0, 0, 0);
    mainPropertiesLayout->setSpacing(0);

    _propertiesScrollArea = new QScrollArea(_propertiesBin);
    QObject::connect( _propertiesScrollArea->verticalScrollBar(), SIGNAL( valueChanged(int) ), _gui, SLOT( onPropertiesScrolled() ) );
    _propertiesScrollArea->setObjectName("Properties");
    assert(_nodeGraphArea);

    _propertiesContainer = new QWidget(_propertiesScrollArea);
    _propertiesContainer->setObjectName("_propertiesContainer");
    _layoutPropertiesBin = new QVBoxLayout(_propertiesContainer);
    _layoutPropertiesBin->setSpacing(0);
    _layoutPropertiesBin->setContentsMargins(0, 0, 0, 0);
    _propertiesContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    _propertiesScrollArea->setWidget(_propertiesContainer);
    _propertiesScrollArea->setWidgetResizable(true);

    QWidget* propertiesAreaButtonsContainer = new QWidget(_propertiesBin);
    QHBoxLayout* propertiesAreaButtonsLayout = new QHBoxLayout(propertiesAreaButtonsContainer);
    propertiesAreaButtonsLayout->setContentsMargins(0, 0, 0, 0);
    propertiesAreaButtonsLayout->setSpacing(5);
    QPixmap closePanelPix;
    appPTR->getIcon(NATRON_PIXMAP_CLOSE_PANEL, &closePanelPix);
    _clearAllPanelsButton = new Button(QIcon(closePanelPix), "", propertiesAreaButtonsContainer);
    _clearAllPanelsButton->setFixedSize(NATRON_SMALL_BUTTON_SIZE, NATRON_SMALL_BUTTON_SIZE);
    _clearAllPanelsButton->setToolTip( Natron::convertFromPlainText(_gui->tr("Clears all the panels in the properties bin pane."),
                                                                Qt::WhiteSpaceNormal) );
    _clearAllPanelsButton->setFocusPolicy(Qt::NoFocus);
    QObject::connect( _clearAllPanelsButton, SIGNAL( clicked(bool) ), _gui, SLOT( clearAllVisiblePanels() ) );
    QPixmap minimizePix, maximizePix;
    appPTR->getIcon(NATRON_PIXMAP_MINIMIZE_WIDGET, &minimizePix);
    appPTR->getIcon(NATRON_PIXMAP_MAXIMIZE_WIDGET, &maximizePix);
    QIcon mIc;
    mIc.addPixmap(minimizePix, QIcon::Normal, QIcon::On);
    mIc.addPixmap(maximizePix, QIcon::Normal, QIcon::Off);
    _minimizeAllPanelsButtons = new Button(mIc, "", propertiesAreaButtonsContainer);
    _minimizeAllPanelsButtons->setCheckable(true);
    _minimizeAllPanelsButtons->setChecked(false);
    _minimizeAllPanelsButtons->setFixedSize(NATRON_SMALL_BUTTON_SIZE, NATRON_SMALL_BUTTON_SIZE);
    _minimizeAllPanelsButtons->setToolTip( Natron::convertFromPlainText(_gui->tr("Minimize / Maximize all panels."), Qt::WhiteSpaceNormal) );
    _minimizeAllPanelsButtons->setFocusPolicy(Qt::NoFocus);
    QObject::connect( _minimizeAllPanelsButtons, SIGNAL( clicked(bool) ), _gui, SLOT( minimizeMaximizeAllPanels(bool) ) );

    _maxPanelsOpenedSpinBox = new SpinBox(propertiesAreaButtonsContainer);
    _maxPanelsOpenedSpinBox->setMaximumSize(NATRON_SMALL_BUTTON_SIZE, NATRON_SMALL_BUTTON_SIZE);
    _maxPanelsOpenedSpinBox->setMinimum(1);
    _maxPanelsOpenedSpinBox->setMaximum(100);
    _maxPanelsOpenedSpinBox->setToolTip( Natron::convertFromPlainText(_gui->tr("Set the maximum of panels that can be opened at the same time "
                                                                           "in the properties bin pane. The special value of 0 indicates "
                                                                           "that an unlimited number of panels can be opened."),
                                                                  Qt::WhiteSpaceNormal) );
    _maxPanelsOpenedSpinBox->setValue( appPTR->getCurrentSettings()->getMaxPanelsOpened() );
    QObject::connect( _maxPanelsOpenedSpinBox, SIGNAL( valueChanged(double) ), _gui, SLOT( onMaxPanelsSpinBoxValueChanged(double) ) );

    propertiesAreaButtonsLayout->addWidget(_maxPanelsOpenedSpinBox);
    propertiesAreaButtonsLayout->addWidget(_clearAllPanelsButton);
    propertiesAreaButtonsLayout->addWidget(_minimizeAllPanelsButtons);
    propertiesAreaButtonsLayout->addStretch();

    mainPropertiesLayout->addWidget(propertiesAreaButtonsContainer);
    mainPropertiesLayout->addWidget(_propertiesScrollArea);

    _gui->registerTab(_propertiesBin, _propertiesBin);
} // createPropertiesBinGui

void
Gui::onPropertiesScrolled()
{
#ifdef __NATRON_WIN32__
    //On Windows Qt 4.8.6 has a bug where the viewport of the scrollarea gets scrolled outside the bounding rect of the QScrollArea and overlaps all widgets inheriting QGLWidget.
    //The only thing I could think of was to repaint all GL widgets manually...

    {
        QMutexLocker k(&_imp->_viewerTabsMutex);
        for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
            (*it)->redrawGLWidgets();
        }
    }
    _imp->_curveEditor->getCurveWidget()->updateGL();

    {
        QMutexLocker k (&_imp->_histogramsMutex);
        for (std::list<Histogram*>::iterator it = _imp->_histograms.begin(); it != _imp->_histograms.end(); ++it) {
            (*it)->updateGL();
        }
    }
#endif
}

void
Gui::createGroupGui(const boost::shared_ptr<Natron::Node> & group,
                    bool requestedByLoad)
{
    boost::shared_ptr<NodeGroup> isGrp = boost::dynamic_pointer_cast<NodeGroup>( group->getLiveInstance()->shared_from_this() );

    assert(isGrp);
    boost::shared_ptr<NodeCollection> collection = boost::dynamic_pointer_cast<NodeCollection>(isGrp);
    assert(collection);

    TabWidget* where = 0;
    if (_imp->_lastFocusedGraph) {
        TabWidget* isTab = dynamic_cast<TabWidget*>( _imp->_lastFocusedGraph->parentWidget() );
        if (isTab) {
            where = isTab;
        } else {
            QMutexLocker k(&_imp->_panesMutex);
            assert( !_imp->_panes.empty() );
            where = _imp->_panes.front();
        }
    }

    QGraphicsScene* scene = new QGraphicsScene(this);
    scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    NodeGraph* nodeGraph = new NodeGraph(this, collection, scene, this);
    nodeGraph->setObjectName( group->getLabel().c_str() );
    _imp->_groups.push_back(nodeGraph);
    if ( where && !requestedByLoad && !getApp()->isCreatingPythonGroup() ) {
        where->appendTab(nodeGraph, nodeGraph);
        QTimer::singleShot( 25, nodeGraph, SLOT( centerOnAllNodes() ) );
    } else {
        nodeGraph->setVisible(false);
    }
}

void
Gui::addGroupGui(NodeGraph* tab,
                 TabWidget* where)
{
    assert(tab);
    assert(where);
    {
        std::list<NodeGraph*>::iterator it = std::find(_imp->_groups.begin(), _imp->_groups.end(), tab);
        if ( it == _imp->_groups.end() ) {
            _imp->_groups.push_back(tab);
        }
    }
    where->appendTab(tab, tab);
}

void
Gui::removeGroupGui(NodeGraph* tab,
                    bool deleteData)
{
    tab->hide();

    if (_imp->_lastFocusedGraph == tab) {
        _imp->_lastFocusedGraph = 0;
    }
    TabWidget* container = dynamic_cast<TabWidget*>( tab->parentWidget() );
    if (container) {
        container->removeTab(tab, true);
    }

    if (deleteData) {
        std::list<NodeGraph*>::iterator it = std::find(_imp->_groups.begin(), _imp->_groups.end(), tab);
        if ( it != _imp->_groups.end() ) {
            _imp->_groups.erase(it);
        }
        tab->deleteLater();
    }
}

void
Gui::setLastSelectedGraph(NodeGraph* graph)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->_lastFocusedGraph = graph;
}

NodeGraph*
Gui::getLastSelectedGraph() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->_lastFocusedGraph;
}

boost::shared_ptr<NodeCollection>
Gui::getLastSelectedNodeCollection() const
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    boost::shared_ptr<NodeCollection> group = graph->getGroup();
    assert(group);

    return group;
}

void
GuiPrivate::createNodeGraphGui()
{
    QGraphicsScene* scene = new QGraphicsScene(_gui);

    scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    _nodeGraphArea = new NodeGraph(_gui, _appInstance->getProject(), scene, _gui);
    _nodeGraphArea->setScriptName(kNodeGraphObjectName);
    _nodeGraphArea->setLabel( QObject::tr("Node Graph").toStdString() );
    _gui->registerTab(_nodeGraphArea, _nodeGraphArea);
}

void
GuiPrivate::createCurveEditorGui()
{
    _curveEditor = new CurveEditor(_gui, _appInstance->getTimeLine(), _gui);
    _curveEditor->setScriptName(kCurveEditorObjectName);
    _curveEditor->setLabel( QObject::tr("Curve Editor").toStdString() );
    _gui->registerTab(_curveEditor, _curveEditor);
}

void
GuiPrivate::createDopeSheetGui()
{
    _dopeSheetEditor = new DopeSheetEditor(_gui,_appInstance->getTimeLine(), _gui);
    _dopeSheetEditor->setScriptName(kDopeSheetEditorObjectName);
    _dopeSheetEditor->setLabel(QObject::tr("Dope Sheet").toStdString());
    _gui->registerTab(_dopeSheetEditor, _dopeSheetEditor);
}

void
GuiPrivate::createScriptEditorGui()
{
    _scriptEditor = new ScriptEditor(_gui);
    _scriptEditor->setScriptName("scriptEditor");
    _scriptEditor->setLabel( QObject::tr("Script Editor").toStdString() );
    _scriptEditor->hide();
    _gui->registerTab(_scriptEditor, _scriptEditor);
}

void
Gui::wipeLayout()
{
    std::list<TabWidget*> panesCpy;
    {
        QMutexLocker l(&_imp->_panesMutex);
        panesCpy = _imp->_panes;
        _imp->_panes.clear();
    }

    for (std::list<TabWidget*>::iterator it = panesCpy.begin(); it != panesCpy.end(); ++it) {
        ///Conserve tabs by removing them from the tab widgets. This way they will not be deleted.
        while ( (*it)->count() > 0 ) {
            (*it)->removeTab(0, false);
        }
        (*it)->setParent(NULL);
        delete *it;
    }

    std::list<Splitter*> splittersCpy;
    {
        QMutexLocker l(&_imp->_splittersMutex);
        splittersCpy = _imp->_splitters;
        _imp->_splitters.clear();
    }
    for (std::list<Splitter*>::iterator it = splittersCpy.begin(); it != splittersCpy.end(); ++it) {
        if (_imp->_leftRightSplitter != *it) {
            while ( (*it)->count() > 0 ) {
                (*it)->widget(0)->setParent(NULL);
            }
            (*it)->setParent(NULL);
            delete *it;
        }
    }

    Splitter *newSplitter = new Splitter(_imp->_centralWidget);
    newSplitter->addWidget(_imp->_toolBox);
    newSplitter->setObjectName_mt_safe( _imp->_leftRightSplitter->objectName_mt_safe() );
    _imp->_mainLayout->removeWidget(_imp->_leftRightSplitter);
    unregisterSplitter(_imp->_leftRightSplitter);
    _imp->_leftRightSplitter->deleteLater();
    _imp->_leftRightSplitter = newSplitter;
    _imp->_leftRightSplitter->setChildrenCollapsible(false);
    _imp->_mainLayout->addWidget(newSplitter);

    {
        QMutexLocker l(&_imp->_splittersMutex);
        _imp->_splitters.push_back(newSplitter);
    }
}

void
Gui::createDefaultLayout1()
{
    ///First tab widget must be created this way
    TabWidget* mainPane = new TabWidget(this, _imp->_leftRightSplitter);
    {
        QMutexLocker l(&_imp->_panesMutex);
        _imp->_panes.push_back(mainPane);
    }

    mainPane->setObjectName_mt_safe("pane1");
    mainPane->setAsAnchor(true);

    _imp->_leftRightSplitter->addWidget(mainPane);

    QList<int> sizes;
    sizes << _imp->_toolBox->sizeHint().width() << width();
    _imp->_leftRightSplitter->setSizes_mt_safe(sizes);


    TabWidget* propertiesPane = mainPane->splitHorizontally(false);
    TabWidget* workshopPane = mainPane->splitVertically(false);
    Splitter* propertiesSplitter = dynamic_cast<Splitter*>( propertiesPane->parentWidget() );
    assert(propertiesSplitter);
    sizes.clear();
    sizes << width() * 0.65 << width() * 0.35;
    propertiesSplitter->setSizes_mt_safe(sizes);

    TabWidget::moveTab(_imp->_nodeGraphArea, _imp->_nodeGraphArea, workshopPane);
    TabWidget::moveTab(_imp->_curveEditor, _imp->_curveEditor, workshopPane);
    TabWidget::moveTab(_imp->_dopeSheetEditor, _imp->_dopeSheetEditor, workshopPane);
    TabWidget::moveTab(_imp->_propertiesBin, _imp->_propertiesBin, propertiesPane);

    {
        QMutexLocker l(&_imp->_viewerTabsMutex);
        for (std::list<ViewerTab*>::iterator it2 = _imp->_viewerTabs.begin(); it2 != _imp->_viewerTabs.end(); ++it2) {
            TabWidget::moveTab(*it2, *it2, mainPane);
        }
    }
    {
        QMutexLocker l(&_imp->_histogramsMutex);
        for (std::list<Histogram*>::iterator it2 = _imp->_histograms.begin(); it2 != _imp->_histograms.end(); ++it2) {
            TabWidget::moveTab(*it2, *it2, mainPane);
        }
    }


    ///Default to NodeGraph displayed
    workshopPane->makeCurrentTab(0);
}

static void
restoreTabWidget(TabWidget* pane,
                 const PaneLayout & serialization)
{
    ///Find out if the name is already used
    QString availableName = pane->getGui()->getAvailablePaneName( serialization.name.c_str() );

    pane->setObjectName_mt_safe(availableName);
    pane->setAsAnchor(serialization.isAnchor);
    const RegisteredTabs & tabs = pane->getGui()->getRegisteredTabs();
    for (std::list<std::string>::const_iterator it = serialization.tabs.begin(); it != serialization.tabs.end(); ++it) {
        RegisteredTabs::const_iterator found = tabs.find(*it);

        ///If the tab exists in the current project, move it
        if ( found != tabs.end() ) {
            TabWidget::moveTab(found->second.first, found->second.second, pane);
        }
    }
    pane->makeCurrentTab(serialization.currentIndex);
}

static void
restoreSplitterRecursive(Gui* gui,
                         Splitter* splitter,
                         const SplitterSerialization & serialization)
{
    Qt::Orientation qO;
    Natron::OrientationEnum nO = (Natron::OrientationEnum)serialization.orientation;

    switch (nO) {
    case Natron::eOrientationHorizontal:
        qO = Qt::Horizontal;
        break;
    case Natron::eOrientationVertical:
        qO = Qt::Vertical;
        break;
    default:
        throw std::runtime_error("Unrecognized splitter orientation");
        break;
    }
    splitter->setOrientation(qO);

    if (serialization.children.size() != 2) {
        throw std::runtime_error("Splitter has a child count that is not 2");
    }

    for (std::vector<SplitterSerialization::Child*>::const_iterator it = serialization.children.begin();
         it != serialization.children.end(); ++it) {
        if ( (*it)->child_asSplitter ) {
            Splitter* child = new Splitter(splitter);
            splitter->addWidget_mt_safe(child);
            restoreSplitterRecursive( gui, child, *( (*it)->child_asSplitter ) );
        } else {
            assert( (*it)->child_asPane );
            TabWidget* pane = new TabWidget(gui, splitter);
            gui->registerPane(pane);
            splitter->addWidget_mt_safe(pane);
            restoreTabWidget( pane, *( (*it)->child_asPane ) );
        }
    }

    splitter->restoreNatron( serialization.sizes.c_str() );
}

void
Gui::restoreLayout(bool wipePrevious,
                   bool enableOldProjectCompatibility,
                   const GuiLayoutSerialization & layoutSerialization)
{
    ///Wipe the current layout
    if (wipePrevious) {
        wipeLayout();
    }

    ///For older projects prior to the layout change, just set default layout.
    if (enableOldProjectCompatibility) {
        createDefaultLayout1();
    } else {
        std::list<ApplicationWindowSerialization*> floatingDockablePanels;

        ///now restore the gui layout
        for (std::list<ApplicationWindowSerialization*>::const_iterator it = layoutSerialization._windows.begin();
             it != layoutSerialization._windows.end(); ++it) {
            QWidget* mainWidget = 0;

            ///The window contains only a pane (for the main window it also contains the toolbar)
            if ( (*it)->child_asPane ) {
                TabWidget* centralWidget = new TabWidget(this);
                registerPane(centralWidget);
                restoreTabWidget(centralWidget, *(*it)->child_asPane);
                mainWidget = centralWidget;
            }
            ///The window contains a splitter as central widget
            else if ( (*it)->child_asSplitter ) {
                Splitter* centralWidget = new Splitter(this);
                restoreSplitterRecursive(this, centralWidget, *(*it)->child_asSplitter);
                mainWidget = centralWidget;
            }
            ///The child is a dockable panel, restore it later
            else if ( !(*it)->child_asDockablePanel.empty() ) {
                assert(!(*it)->isMainWindow);
                floatingDockablePanels.push_back(*it);
                continue;
            }

            assert(mainWidget);
            QWidget* window;
            if ( (*it)->isMainWindow ) {
                // mainWidget->setParent(_imp->_leftRightSplitter);
                _imp->_leftRightSplitter->addWidget_mt_safe(mainWidget);
                window = this;
            } else {
                FloatingWidget* floatingWindow = new FloatingWidget(this, this);
                floatingWindow->setWidget(mainWidget);
                registerFloatingWindow(floatingWindow);
                window = floatingWindow;
            }

            ///Restore geometry
            window->resize( (*it)->w, (*it)->h );
            window->move( QPoint( (*it)->x, (*it)->y ) );
        }

        for (std::list<ApplicationWindowSerialization*>::iterator it = floatingDockablePanels.begin();
             it != floatingDockablePanels.end(); ++it) {
            ///Find the node associated to the floating panel if any and float it
            assert( !(*it)->child_asDockablePanel.empty() );
            if ( (*it)->child_asDockablePanel == kNatronProjectSettingsPanelSerializationName ) {
                _imp->_projectGui->getPanel()->floatPanel();
            } else {
                ///Find a node with the dockable panel name
                const std::list<boost::shared_ptr<NodeGui> > & nodes = getNodeGraph()->getAllActiveNodes();
                DockablePanel* panel = 0;
                for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it2 = nodes.begin(); it2 != nodes.end(); ++it2) {
                    if ( (*it2)->getNode()->getScriptName() == (*it)->child_asDockablePanel ) {
                        ( (*it2)->getSettingPanel()->floatPanel() );
                        panel = (*it2)->getSettingPanel();
                        break;
                    }
                }
                if (panel) {
                    FloatingWidget* fWindow = dynamic_cast<FloatingWidget*>( panel->parentWidget() );
                    assert(fWindow);
                    fWindow->move( QPoint( (*it)->x, (*it)->y ) );
                    fWindow->resize( (*it)->w, (*it)->h );
                }
            }
        }
    }
} // restoreLayout

void
Gui::exportLayout()
{
    std::vector<std::string> filters;

    filters.push_back(NATRON_LAYOUT_FILE_EXT);
    SequenceFileDialog dialog( this, filters, false, SequenceFileDialog::eFileDialogModeSave, _imp->_lastSaveProjectOpenedDir.toStdString(), this, false );
    if ( dialog.exec() ) {
        std::string filename = dialog.filesToSave();
        QString filenameCpy( filename.c_str() );
        QString ext = Natron::removeFileExtension(filenameCpy);
        if (ext != NATRON_LAYOUT_FILE_EXT) {
            filename.append("." NATRON_LAYOUT_FILE_EXT);
        }

        std::ofstream ofile;
        try {
            ofile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            ofile.open(filename.c_str(), std::ofstream::out);
        } catch (const std::ofstream::failure & e) {
            Natron::errorDialog( tr("Error").toStdString()
                                 , tr("Exception occured when opening file").toStdString(), false );

            return;
        }

        if ( !ofile.good() ) {
            Natron::errorDialog( tr("Error").toStdString()
                                 , tr("Failure to open the file").toStdString(), false );

            return;
        }

        try {
            boost::archive::xml_oarchive oArchive(ofile);
            GuiLayoutSerialization s;
            s.initialize(this);
            oArchive << boost::serialization::make_nvp("Layout", s);
        }catch (...) {
            Natron::errorDialog( tr("Error").toStdString()
                                 , tr("Failure when saving the layout").toStdString(), false );
            ofile.close();

            return;
        }

        ofile.close();
    }
}

const QString &
Gui::getLastLoadProjectDirectory() const
{
    return _imp->_lastLoadProjectOpenedDir;
}

const QString &
Gui::getLastSaveProjectDirectory() const
{
    return _imp->_lastSaveProjectOpenedDir;
}

const QString &
Gui::getLastPluginDirectory() const
{
    return _imp->_lastPluginDir;
}

void
Gui::updateLastPluginDirectory(const QString & str)
{
    _imp->_lastPluginDir = str;
}

void
Gui::importLayout()
{
    std::vector<std::string> filters;

    filters.push_back(NATRON_LAYOUT_FILE_EXT);
    SequenceFileDialog dialog( this, filters, false, SequenceFileDialog::eFileDialogModeOpen, _imp->_lastLoadProjectOpenedDir.toStdString(), this, false );
    if ( dialog.exec() ) {
        std::string filename = dialog.selectedFiles();
        std::ifstream ifile;
        try {
            ifile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            ifile.open(filename.c_str(), std::ifstream::in);
        } catch (const std::ifstream::failure & e) {
            QString err = QString("Exception occured when opening file %1: %2").arg( filename.c_str() ).arg( e.what() );
            Natron::errorDialog( tr("Error").toStdString(), tr( err.toStdString().c_str() ).toStdString(), false );

            return;
        }

        try {
            boost::archive::xml_iarchive iArchive(ifile);
            GuiLayoutSerialization s;
            iArchive >> boost::serialization::make_nvp("Layout", s);
            restoreLayout(true, false, s);
        } catch (const boost::archive::archive_exception & e) {
            ifile.close();
            QString err = QString("Exception occured when opening file %1: %2").arg( filename.c_str() ).arg( e.what() );
            Natron::errorDialog( tr("Error").toStdString(), tr( err.toStdString().c_str() ).toStdString(), false );

            return;
        } catch (const std::exception & e) {
            ifile.close();
            QString err = QString("Exception occured when opening file %1: %2").arg( filename.c_str() ).arg( e.what() );
            Natron::errorDialog( tr("Error").toStdString(), tr( err.toStdString().c_str() ).toStdString(), false );

            return;
        }

        ifile.close();
    }
}

void
Gui::createDefaultLayoutInternal(bool wipePrevious)
{
    if (wipePrevious) {
        wipeLayout();
    }

    std::string fileLayout = appPTR->getCurrentSettings()->getDefaultLayoutFile();
    if ( !fileLayout.empty() ) {
        std::ifstream ifile;
        ifile.open( fileLayout.c_str() );
        if ( !ifile.is_open() ) {
            createDefaultLayout1();
        } else {
            try {
                boost::archive::xml_iarchive iArchive(ifile);
                GuiLayoutSerialization s;
                iArchive >> boost::serialization::make_nvp("Layout", s);
                restoreLayout(false, false, s);
            } catch (const boost::archive::archive_exception & e) {
                ifile.close();
                QString err = QString("Exception occured when opening file %1: %2").arg( fileLayout.c_str() ).arg( e.what() );
                Natron::errorDialog( tr("Error").toStdString(), tr( err.toStdString().c_str() ).toStdString(), false );

                return;
            } catch (const std::exception & e) {
                ifile.close();
                QString err = QString("Exception occured when opening file %1: %2").arg( fileLayout.c_str() ).arg( e.what() );
                Natron::errorDialog( tr("Error").toStdString(), tr( err.toStdString().c_str() ).toStdString(), false );

                return;
            }

            ifile.close();
        }
    } else {
        createDefaultLayout1();
    }
}

void
Gui::restoreDefaultLayout()
{
    createDefaultLayoutInternal(true);
}

void
Gui::initProjectGuiKnobs()
{
    assert(_imp->_projectGui);
    _imp->_projectGui->initializeKnobsGui();
}

QKeySequence
Gui::keySequenceForView(int v)
{
    switch (v) {
    case 0:

        return QKeySequence(Qt::CTRL + Qt::ALT +  Qt::Key_1);
        break;
    case 1:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_2);
        break;
    case 2:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_3);
        break;
    case 3:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_4);
        break;
    case 4:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_5);
        break;
    case 5:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_6);
        break;
    case 6:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_7);
        break;
    case 7:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_8);
        break;
    case 8:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_9);
        break;
    case 9:

        return QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_0);
        break;
    default:

        return QKeySequence();
    }
}

static const char*
slotForView(int view)
{
    switch (view) {
    case 0:

        return SLOT( showView0() );
        break;
    case 1:

        return SLOT( showView1() );
        break;
    case 2:

        return SLOT( showView2() );
        break;
    case 3:

        return SLOT( showView3() );
        break;
    case 4:

        return SLOT( showView4() );
        break;
    case 5:

        return SLOT( showView5() );
        break;
    case 6:

        return SLOT( showView6() );
        break;
    case 7:

        return SLOT( showView7() );
        break;
    case 8:

        return SLOT( showView7() );
        break;
    case 9:

        return SLOT( showView8() );
        break;
    default:

        return NULL;
    }
}

void
Gui::updateViewsActions(int viewsCount)
{
    _imp->viewersViewMenu->clear();
    //if viewsCount == 1 we don't add a menu entry
    _imp->viewersMenu->removeAction( _imp->viewersViewMenu->menuAction() );
    if (viewsCount == 2) {
        QAction* left = new QAction(this);
        left->setCheckable(false);
        left->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_1) );
        _imp->viewersViewMenu->addAction(left);
        left->setText( tr("Display Left View") );
        QObject::connect( left, SIGNAL( triggered() ), this, SLOT( showView0() ) );
        QAction* right = new QAction(this);
        right->setCheckable(false);
        right->setShortcut( QKeySequence(Qt::CTRL + Qt::Key_2) );
        _imp->viewersViewMenu->addAction(right);
        right->setText( tr("Display Right View") );
        QObject::connect( right, SIGNAL( triggered() ), this, SLOT( showView1() ) );

        _imp->viewersMenu->addAction( _imp->viewersViewMenu->menuAction() );
    } else if (viewsCount > 2) {
        for (int i = 0; i < viewsCount; ++i) {
            if (i > 9) {
                break;
            }
            QAction* viewI = new QAction(this);
            viewI->setCheckable(false);
            QKeySequence seq = keySequenceForView(i);
            if ( !seq.isEmpty() ) {
                viewI->setShortcut(seq);
            }
            _imp->viewersViewMenu->addAction(viewI);
            const char* slot = slotForView(i);
            viewI->setText( QString( tr("Display View ") ) + QString::number(i + 1) );
            if (slot) {
                QObject::connect(viewI, SIGNAL( triggered() ), this, slot);
            }
        }

        _imp->viewersMenu->addAction( _imp->viewersViewMenu->menuAction() );
    }
}

void
Gui::putSettingsPanelFirst(DockablePanel* panel)
{
    _imp->_layoutPropertiesBin->removeWidget(panel);
    _imp->_layoutPropertiesBin->insertWidget(0, panel);
    _imp->_propertiesScrollArea->verticalScrollBar()->setValue(0);
    buildTabFocusOrderPropertiesBin();
}

void
Gui::buildTabFocusOrderPropertiesBin()
{
    int next = 1;

    for (int i = 0; i < _imp->_layoutPropertiesBin->count(); ++i, ++next) {
        QLayoutItem* item = _imp->_layoutPropertiesBin->itemAt(i);
        QWidget* w = item->widget();
        QWidget* nextWidget = _imp->_layoutPropertiesBin->itemAt(next < _imp->_layoutPropertiesBin->count() ? next : 0)->widget();

        if (w && nextWidget) {
            setTabOrder(w, nextWidget);
        }
    }
}

void
Gui::setVisibleProjectSettingsPanel()
{
    addVisibleDockablePanel( _imp->_projectGui->getPanel() );
    if ( !_imp->_projectGui->isVisible() ) {
        _imp->_projectGui->setVisible(true);
    }
}

void
Gui::reloadStylesheet()
{
    loadStyleSheet();
}

void
Gui::loadStyleSheet()
{
    boost::shared_ptr<Settings> settings = appPTR->getCurrentSettings();

    QString selStr, sunkStr, baseStr, raisedStr, txtStr, intStr, kfStr, eStr, altStr;

    //settings->
    {
        double r, g, b;
        settings->getSelectionColor(&r, &g, &b);
        selStr = QString("rgb(%1,%2,%3)").arg(Color::floatToInt<256>(r)).arg(Color::floatToInt<256>(g)).arg(Color::floatToInt<256>(b));
    }
    {
        double r, g, b;
        settings->getBaseColor(&r, &g, &b);
        baseStr = QString("rgb(%1,%2,%3)").arg(Color::floatToInt<256>(r)).arg(Color::floatToInt<256>(g)).arg(Color::floatToInt<256>(b));
    }
    {
        double r, g, b;
        settings->getRaisedColor(&r, &g, &b);
        raisedStr = QString("rgb(%1,%2,%3)").arg(Color::floatToInt<256>(r)).arg(Color::floatToInt<256>(g)).arg(Color::floatToInt<256>(b));
    }
    {
        double r, g, b;
        settings->getSunkenColor(&r, &g, &b);
        sunkStr = QString("rgb(%1,%2,%3)").arg(Color::floatToInt<256>(r)).arg(Color::floatToInt<256>(g)).arg(Color::floatToInt<256>(b));
    }
    {
        double r, g, b;
        settings->getTextColor(&r, &g, &b);
        txtStr = QString("rgb(%1,%2,%3)").arg(Color::floatToInt<256>(r)).arg(Color::floatToInt<256>(g)).arg(Color::floatToInt<256>(b));
    }
    {
        double r, g, b;
        settings->getInterpolatedColor(&r, &g, &b);
        intStr = QString("rgb(%1,%2,%3)").arg(Color::floatToInt<256>(r)).arg(Color::floatToInt<256>(g)).arg(Color::floatToInt<256>(b));
    }
    {
        double r, g, b;
        settings->getKeyframeColor(&r, &g, &b);
        kfStr = QString("rgb(%1,%2,%3)").arg(Color::floatToInt<256>(r)).arg(Color::floatToInt<256>(g)).arg(Color::floatToInt<256>(b));
    }
    {
        double r, g, b;
        settings->getExprColor(&r, &g, &b);
        eStr = QString("rgb(%1,%2,%3)").arg(Color::floatToInt<256>(r)).arg(Color::floatToInt<256>(g)).arg(Color::floatToInt<256>(b));
    }
    {
        double r, g, b;
        settings->getAltTextColor(&r, &g, &b);
        altStr = QString("rgb(%1,%2,%3)").arg(Color::floatToInt<256>(r)).arg(Color::floatToInt<256>(g)).arg(Color::floatToInt<256>(b));
    }

    QFile qss(":/Resources/Stylesheets/mainstyle.qss");

    if ( qss.open(QIODevice::ReadOnly
                  | QIODevice::Text) ) {
        QTextStream in(&qss);
        QString content( in.readAll() );
        setStyleSheet( content
                       .arg(selStr) // %1: selection-color
                       .arg(baseStr) // %2: medium background
                       .arg(raisedStr) // %3: soft background
                       .arg(sunkStr) // %4: strong background
                       .arg(txtStr) // %5: text colour
                       .arg(intStr) // %6: interpolated value color
                       .arg(kfStr) // %7: keyframe value color
                       .arg("rgb(0,0,0)") // %8: disabled editable text
                       .arg(eStr) // %9: expression background color
                       .arg(altStr) ); // %10 = altered text color
    }
} // Gui::loadStyleSheet

void
Gui::maximize(TabWidget* what)
{
    assert(what);
    if ( what->isFloatingWindowChild() ) {
        return;
    }

    QMutexLocker l(&_imp->_panesMutex);
    for (std::list<TabWidget*>::iterator it = _imp->_panes.begin(); it != _imp->_panes.end(); ++it) {
        //if the widget is not what we want to maximize and it is not floating , hide it
        if ( (*it != what) && !(*it)->isFloatingWindowChild() ) {
            // also if we want to maximize the workshop pane, don't hide the properties pane

            bool hasProperties = false;
            for (int i = 0; i < (*it)->count(); ++i) {
                QString tabName = (*it)->tabAt(i)->objectName();
                if (tabName == kPropertiesBinName) {
                    hasProperties = true;
                    break;
                }
            }

            bool hasNodeGraphOrCurveEditor = false;
            for (int i = 0; i < what->count(); ++i) {
                QWidget* tab = what->tabAt(i);
                assert(tab);
                NodeGraph* isGraph = dynamic_cast<NodeGraph*>(tab);
                CurveEditor* isEditor = dynamic_cast<CurveEditor*>(tab);
                if (isGraph || isEditor) {
                    hasNodeGraphOrCurveEditor = true;
                    break;
                }
            }

            if (hasProperties && hasNodeGraphOrCurveEditor) {
                continue;
            }
            (*it)->hide();
        }
    }
}

void
Gui::minimize()
{
    QMutexLocker l(&_imp->_panesMutex);

    for (std::list<TabWidget*>::iterator it = _imp->_panes.begin(); it != _imp->_panes.end(); ++it) {
        (*it)->show();
    }
}

ViewerTab*
Gui::addNewViewerTab(ViewerInstance* viewer,
                     TabWidget* where)
{
    std::map<NodeGui*, RotoGui*> rotoNodes;
    std::list<NodeGui*> rotoNodesList;
    std::pair<NodeGui*, RotoGui*> currentRoto;
    std::map<NodeGui*, TrackerGui*> trackerNodes;
    std::list<NodeGui*> trackerNodesList;
    std::pair<NodeGui*, TrackerGui*> currentTracker;
    
    if (!viewer) {
        return 0;
    }
    
    //Don't create tracker & roto interface for file dialog preview viewer
    if (viewer->getNode()->getScriptName() != NATRON_FILE_DIALOG_PREVIEW_VIEWER_NAME) {
        if ( !_imp->_viewerTabs.empty() ) {
            ( *_imp->_viewerTabs.begin() )->getRotoContext(&rotoNodes, &currentRoto);
            ( *_imp->_viewerTabs.begin() )->getTrackerContext(&trackerNodes, &currentTracker);
        } else {
            const std::list<boost::shared_ptr<NodeGui> > & allNodes = _imp->_nodeGraphArea->getAllActiveNodes();
            for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = allNodes.begin(); it != allNodes.end(); ++it) {
                if ( (*it)->getNode()->getRotoContext() ) {
                    rotoNodesList.push_back( it->get() );
                    if (!currentRoto.first) {
                        currentRoto.first = it->get();
                    }
                } else if ( (*it)->getNode()->isPointTrackerNode() ) {
                    trackerNodesList.push_back( it->get() );
                    if (!currentTracker.first) {
                        currentTracker.first = it->get();
                    }
                }
            }
        }
    }
    for (std::map<NodeGui*, RotoGui*>::iterator it = rotoNodes.begin(); it != rotoNodes.end(); ++it) {
        rotoNodesList.push_back(it->first);
    }

    for (std::map<NodeGui*, TrackerGui*>::iterator it = trackerNodes.begin(); it != trackerNodes.end(); ++it) {
        trackerNodesList.push_back(it->first);
    }

    ViewerTab* tab = new ViewerTab(rotoNodesList, currentRoto.first, trackerNodesList, currentTracker.first, this, viewer, where);
    QObject::connect( tab->getViewer(), SIGNAL( imageChanged(int, bool) ), this, SLOT( onViewerImageChanged(int, bool) ) );
    {
        QMutexLocker l(&_imp->_viewerTabsMutex);
        _imp->_viewerTabs.push_back(tab);
    }
    where->appendTab(tab, tab);
    Q_EMIT viewersChanged();

    return tab;
}

void
Gui::onViewerImageChanged(int texIndex,
                          bool hasImageBackend)
{
    ///notify all histograms a viewer image changed
    ViewerGL* viewer = qobject_cast<ViewerGL*>( sender() );

    if (viewer) {
        QMutexLocker l(&_imp->_histogramsMutex);
        for (std::list<Histogram*>::iterator it = _imp->_histograms.begin(); it != _imp->_histograms.end(); ++it) {
            (*it)->onViewerImageChanged(viewer, texIndex, hasImageBackend);
        }
    }
}

void
Gui::addViewerTab(ViewerTab* tab,
                  TabWidget* where)
{
    assert(tab);
    assert(where);
    {
        QMutexLocker l(&_imp->_viewerTabsMutex);
        std::list<ViewerTab*>::iterator it = std::find(_imp->_viewerTabs.begin(), _imp->_viewerTabs.end(), tab);
        if ( it == _imp->_viewerTabs.end() ) {
            _imp->_viewerTabs.push_back(tab);
        }
    }
    where->appendTab(tab, tab);
    Q_EMIT viewersChanged();
}

void
Gui::registerTab(QWidget* tab,
                 ScriptObject* obj)
{
    std::string name = obj->getScriptName();
    RegisteredTabs::iterator registeredTab = _imp->_registeredTabs.find(name);

    if ( registeredTab == _imp->_registeredTabs.end() ) {
        _imp->_registeredTabs.insert( std::make_pair( name, std::make_pair(tab, obj) ) );
    }
}

void
Gui::unregisterTab(QWidget* tab)
{
    for (RegisteredTabs::iterator it = _imp->_registeredTabs.begin(); it != _imp->_registeredTabs.end(); ++it) {
        if (it->second.first == tab) {
            _imp->_registeredTabs.erase(it);
            break;
        }
    }
}

void
Gui::registerFloatingWindow(FloatingWidget* window)
{
    QMutexLocker k(&_imp->_floatingWindowMutex);
    std::list<FloatingWidget*>::iterator found = std::find(_imp->_floatingWindows.begin(), _imp->_floatingWindows.end(), window);

    if ( found == _imp->_floatingWindows.end() ) {
        _imp->_floatingWindows.push_back(window);
    }
}

void
Gui::unregisterFloatingWindow(FloatingWidget* window)
{
    QMutexLocker k(&_imp->_floatingWindowMutex);
    std::list<FloatingWidget*>::iterator found = std::find(_imp->_floatingWindows.begin(), _imp->_floatingWindows.end(), window);

    if ( found != _imp->_floatingWindows.end() ) {
        _imp->_floatingWindows.erase(found);
    }
}

std::list<FloatingWidget*>
Gui::getFloatingWindows() const
{
    QMutexLocker l(&_imp->_floatingWindowMutex);

    return _imp->_floatingWindows;
}

void
Gui::removeViewerTab(ViewerTab* tab,
                     bool initiatedFromNode,
                     bool deleteData)
{
    assert(tab);
    unregisterTab(tab);

    NodeGraph* graph = 0;
    NodeGroup* isGrp = 0;
    boost::shared_ptr<NodeCollection> collection;
    if ( tab->getInternalNode() && tab->getInternalNode()->getNode() ) {
        boost::shared_ptr<NodeCollection> collection = tab->getInternalNode()->getNode()->getGroup();
        isGrp = dynamic_cast<NodeGroup*>( collection.get() );
    }


    if (isGrp) {
        NodeGraphI* graph_i = isGrp->getNodeGraph();
        assert(graph_i);
        graph = dynamic_cast<NodeGraph*>(graph_i);
    } else {
        graph = getNodeGraph();
    }
    assert(graph);

    ViewerTab* lastSelectedViewer = graph->getLastSelectedViewer();

    if (lastSelectedViewer == tab) {
        bool foundOne = false;
        NodeList nodes;
        if (collection) {
            nodes = collection->getNodes();
        }
        for (NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>( (*it)->getLiveInstance() );
            if ( !isViewer || ( isViewer == tab->getInternalNode() ) || !(*it)->isActivated() ) {
                continue;
            }
            OpenGLViewerI* viewerI = isViewer->getUiContext();
            assert(viewerI);
            ViewerGL* glViewer = dynamic_cast<ViewerGL*>(viewerI);
            assert(glViewer);
            graph->setLastSelectedViewer( glViewer->getViewerTab() );
            foundOne = true;
            break;
        }
        if (!foundOne) {
            graph->setLastSelectedViewer(0);
        }
    }

    ViewerInstance* internalViewer = tab->getInternalNode();
    if (internalViewer) {
        if (getApp()->getLastViewerUsingTimeline() == internalViewer) {
            getApp()->discardLastViewerUsingTimeline();
        }
    }

    if (!initiatedFromNode) {
        assert(_imp->_nodeGraphArea);
        ///call the deleteNode which will call this function again when the node will be deactivated.
        NodePtr internalNode = tab->getInternalNode()->getNode();
        boost::shared_ptr<NodeGuiI> guiI = internalNode->getNodeGui();
        boost::shared_ptr<NodeGui> gui = boost::dynamic_pointer_cast<NodeGui>(guiI);
        assert(gui);
        NodeGraphI* graph_i = internalNode->getGroup()->getNodeGraph();
        assert(graph_i);
        NodeGraph* graph = dynamic_cast<NodeGraph*>(graph_i);
        assert(graph);
        if (graph) {
            graph->removeNode(gui);
        }
    } else {
        tab->hide();


        TabWidget* container = dynamic_cast<TabWidget*>( tab->parentWidget() );
        if (container) {
            container->removeTab(tab, false);
        }

        if (deleteData) {
            QMutexLocker l(&_imp->_viewerTabsMutex);
            std::list<ViewerTab*>::iterator it = std::find(_imp->_viewerTabs.begin(), _imp->_viewerTabs.end(), tab);
            if ( it != _imp->_viewerTabs.end() ) {
                _imp->_viewerTabs.erase(it);
            }
            tab->notifyAppClosing();
            tab->deleteLater();
        }
    }
    Q_EMIT viewersChanged();
} // Gui::removeViewerTab

Histogram*
Gui::addNewHistogram()
{
    Histogram* h = new Histogram(this);
    QMutexLocker l(&_imp->_histogramsMutex);
    std::stringstream ss;

    ss << _imp->_nextHistogramIndex;

    h->setScriptName( "histogram" + ss.str() );
    h->setLabel( "Histogram" + ss.str() );
    ++_imp->_nextHistogramIndex;
    _imp->_histograms.push_back(h);

    return h;
}

void
Gui::removeHistogram(Histogram* h)
{
    unregisterTab(h);
    QMutexLocker l(&_imp->_histogramsMutex);
    std::list<Histogram*>::iterator it = std::find(_imp->_histograms.begin(), _imp->_histograms.end(), h);

    assert( it != _imp->_histograms.end() );
    delete *it;
    _imp->_histograms.erase(it);
}

const std::list<Histogram*> &
Gui::getHistograms() const
{
    QMutexLocker l(&_imp->_histogramsMutex);

    return _imp->_histograms;
}

std::list<Histogram*>
Gui::getHistograms_mt_safe() const
{
    QMutexLocker l(&_imp->_histogramsMutex);

    return _imp->_histograms;
}

TabWidget*
GuiPrivate::getOnly1NonFloatingPane(int & count) const
{
    assert( !_panesMutex.tryLock() );
    count = 0;
    if ( _panes.empty() ) {
        return NULL;
    }
    TabWidget* firstNonFloating = 0;
    for (std::list<TabWidget*>::const_iterator it = _panes.begin(); it != _panes.end(); ++it) {
        if ( !(*it)->isFloatingWindowChild() ) {
            if (!firstNonFloating) {
                firstNonFloating = *it;
            }
            ++count;
        }
    }
    ///there should always be at least 1 non floating window
    assert(firstNonFloating);

    return firstNonFloating;
}

void
Gui::unregisterPane(TabWidget* pane)
{
    {
        QMutexLocker l(&_imp->_panesMutex);
        std::list<TabWidget*>::iterator found = std::find(_imp->_panes.begin(), _imp->_panes.end(), pane);

        if ( found != _imp->_panes.end() ) {
            if (_imp->_lastEnteredTabWidget == pane) {
                _imp->_lastEnteredTabWidget = 0;
            }
            _imp->_panes.erase(found);
        }

        if ( ( pane->isAnchor() ) && !_imp->_panes.empty() ) {
            _imp->_panes.front()->setAsAnchor(true);
        }
    }
    checkNumberOfNonFloatingPanes();
}

void
Gui::checkNumberOfNonFloatingPanes()
{
    QMutexLocker l(&_imp->_panesMutex);
    ///If dropping to 1 non floating pane, make it non closable:floatable
    int nbNonFloatingPanes;
    TabWidget* nonFloatingPane = _imp->getOnly1NonFloatingPane(nbNonFloatingPanes);

    ///When there's only 1 tab left make it closable/floatable again
    if (nbNonFloatingPanes == 1) {
        assert(nonFloatingPane);
        nonFloatingPane->setClosable(false);
    } else {
        for (std::list<TabWidget*>::iterator it = _imp->_panes.begin(); it != _imp->_panes.end(); ++it) {
            (*it)->setClosable(true);
        }
    }
}

void
Gui::registerPane(TabWidget* pane)
{
    {
        QMutexLocker l(&_imp->_panesMutex);
        bool hasAnchor = false;

        for (std::list<TabWidget*>::iterator it = _imp->_panes.begin(); it != _imp->_panes.end(); ++it) {
            if ( (*it)->isAnchor() ) {
                hasAnchor = true;
                break;
            }
        }
        std::list<TabWidget*>::iterator found = std::find(_imp->_panes.begin(), _imp->_panes.end(), pane);

        if ( found == _imp->_panes.end() ) {
            if ( _imp->_panes.empty() ) {
                _imp->_leftRightSplitter->addWidget(pane);
                pane->setClosable(false);
            }
            _imp->_panes.push_back(pane);

            if (!hasAnchor) {
                pane->setAsAnchor(true);
            }
        }
    }
    checkNumberOfNonFloatingPanes();
}

void
Gui::registerSplitter(Splitter* s)
{
    QMutexLocker l(&_imp->_splittersMutex);
    std::list<Splitter*>::iterator found = std::find(_imp->_splitters.begin(), _imp->_splitters.end(), s);

    if ( found == _imp->_splitters.end() ) {
        _imp->_splitters.push_back(s);
    }
}

void
Gui::unregisterSplitter(Splitter* s)
{
    QMutexLocker l(&_imp->_splittersMutex);
    std::list<Splitter*>::iterator found = std::find(_imp->_splitters.begin(), _imp->_splitters.end(), s);

    if ( found != _imp->_splitters.end() ) {
        _imp->_splitters.erase(found);
    }
}

void
Gui::registerPyPanel(PyPanel* panel,
                     const std::string & pythonFunction)
{
    QMutexLocker l(&_imp->_pyPanelsMutex);
    std::map<PyPanel*, std::string>::iterator found = _imp->_userPanels.find(panel);

    if ( found == _imp->_userPanels.end() ) {
        _imp->_userPanels.insert( std::make_pair(panel, pythonFunction) );
    }
}

void
Gui::unregisterPyPanel(PyPanel* panel)
{
    QMutexLocker l(&_imp->_pyPanelsMutex);
    std::map<PyPanel*, std::string>::iterator found = _imp->_userPanels.find(panel);

    if ( found != _imp->_userPanels.end() ) {
        _imp->_userPanels.erase(found);
    }
}

std::map<PyPanel*, std::string>
Gui::getPythonPanels() const
{
    QMutexLocker l(&_imp->_pyPanelsMutex);

    return _imp->_userPanels;
}

QWidget*
Gui::findExistingTab(const std::string & name) const
{
    RegisteredTabs::const_iterator it = _imp->_registeredTabs.find(name);

    if ( it != _imp->_registeredTabs.end() ) {
        return it->second.first;
    } else {
        return NULL;
    }
}

void
Gui::findExistingTab(const std::string & name,
                     QWidget** w,
                     ScriptObject** o) const
{
    RegisteredTabs::const_iterator it = _imp->_registeredTabs.find(name);

    if ( it != _imp->_registeredTabs.end() ) {
        *w = it->second.first;
        *o = it->second.second;
    } else {
        *w = 0;
        *o = 0;
    }
}

ToolButton*
Gui::findExistingToolButton(const QString & label) const
{
    for (U32 i = 0; i < _imp->_toolButtons.size(); ++i) {
        if (_imp->_toolButtons[i]->getLabel() == label) {
            return _imp->_toolButtons[i];
        }
    }

    return NULL;
}

ToolButton*
Gui::findOrCreateToolButton(const boost::shared_ptr<PluginGroupNode> & plugin)
{
    if ( !Natron::isPluginCreatable( plugin->getID().toStdString() ) ) {
        return 0;
    }

    for (U32 i = 0; i < _imp->_toolButtons.size(); ++i) {
        if (_imp->_toolButtons[i]->getPluginToolButton() == plugin) {
            return _imp->_toolButtons[i];
        }
    }

    //first-off create the tool-button's parent, if any
    ToolButton* parentToolButton = NULL;
    if ( plugin->hasParent() ) {
        assert(plugin->getParent() != plugin);
        if (plugin->getParent() != plugin) {
            parentToolButton = findOrCreateToolButton( plugin->getParent() );
        }
    }

    QIcon icon;
    if ( !plugin->getIconPath().isEmpty() && QFile::exists( plugin->getIconPath() ) ) {
        icon.addFile( plugin->getIconPath() );
    } else {
        //add the default group icon only if it has no parent
        if ( !plugin->hasParent() ) {
            QPixmap pix;
            getPixmapForGrouping( &pix, plugin->getLabel() );
            icon.addPixmap(pix);
        }
    }
    //if the tool-button has no children, this is a leaf, we must create an action
    bool isLeaf = false;
    if ( plugin->getChildren().empty() ) {
        isLeaf = true;
        //if the plugin has no children and no parent, put it in the "others" group
        if ( !plugin->hasParent() ) {
            ToolButton* othersGroup = findExistingToolButton(PLUGIN_GROUP_DEFAULT);
            QStringList grouping(PLUGIN_GROUP_DEFAULT);
            boost::shared_ptr<PluginGroupNode> othersToolButton =
                appPTR->findPluginToolButtonOrCreate(grouping,
                                                     PLUGIN_GROUP_DEFAULT,
                                                     PLUGIN_GROUP_DEFAULT_ICON_PATH,
                                                     PLUGIN_GROUP_DEFAULT_ICON_PATH,
                                                     1,
                                                     0);
            othersToolButton->tryAddChild(plugin);

            //if the othersGroup doesn't exist, create it
            if (!othersGroup) {
                othersGroup = findOrCreateToolButton(othersToolButton);
            }
            parentToolButton = othersGroup;
        }
    }
    ToolButton* pluginsToolButton = new ToolButton(_imp->_appInstance, plugin, plugin->getID(), plugin->getMajorVersion(),
                                                   plugin->getMinorVersion(),
                                                   plugin->getLabel(), icon);

    if (isLeaf) {
        QString label = plugin->getNotHighestMajorVersion() ? plugin->getLabelVersionMajorEncoded() : plugin->getLabel();
        assert(parentToolButton);
        QAction* action = new QAction(this);
        action->setText(label);
        action->setIcon( pluginsToolButton->getIcon() );
        QObject::connect( action, SIGNAL( triggered() ), pluginsToolButton, SLOT( onTriggered() ) );
        pluginsToolButton->setAction(action);
    } else {
        Menu* menu = new Menu(this);
        //menu->setFont( QFont(appFont,appFontSize) );
        menu->setTitle( pluginsToolButton->getLabel() );
        pluginsToolButton->setMenu(menu);
        pluginsToolButton->setAction( menu->menuAction() );
    }

    if (pluginsToolButton->getLabel() == PLUGIN_GROUP_IMAGE) {
        ///create 2 special actions to create a reader and a writer so the user doesn't have to guess what
        ///plugin to choose for reading/writing images, let Natron deal with it. THe user can still change
        ///the behavior of Natron via the Preferences Readers/Writers tabs.
        QMenu* imageMenu = pluginsToolButton->getMenu();
        assert(imageMenu);
        QAction* createReaderAction = new QAction(imageMenu);
        QObject::connect( createReaderAction, SIGNAL( triggered() ), this, SLOT( createReader() ) );
        createReaderAction->setText( tr("Read") );
        QPixmap readImagePix;
        appPTR->getIcon(Natron::NATRON_PIXMAP_READ_IMAGE, &readImagePix);
        createReaderAction->setIcon( QIcon(readImagePix) );
        createReaderAction->setShortcutContext(Qt::WidgetShortcut);
        createReaderAction->setShortcut( QKeySequence(Qt::Key_R) );
        imageMenu->addAction(createReaderAction);

        QAction* createWriterAction = new QAction(imageMenu);
        QObject::connect( createWriterAction, SIGNAL( triggered() ), this, SLOT( createWriter() ) );
        createWriterAction->setText( tr("Write") );
        QPixmap writeImagePix;
        appPTR->getIcon(Natron::NATRON_PIXMAP_WRITE_IMAGE, &writeImagePix);
        createWriterAction->setIcon( QIcon(writeImagePix) );
        createWriterAction->setShortcutContext(Qt::WidgetShortcut);
        createWriterAction->setShortcut( QKeySequence(Qt::Key_W) );
        imageMenu->addAction(createWriterAction);
    }


    //if it has a parent, add the new tool button as a child
    if (parentToolButton) {
        parentToolButton->tryAddChild(pluginsToolButton);
    }
    _imp->_toolButtons.push_back(pluginsToolButton);

    return pluginsToolButton;
} // findOrCreateToolButton

std::list<ToolButton*>
Gui::getToolButtonsOrdered() const
{
    ///First-off find the tool buttons that should be ordered
    ///and put in another list the rest
    std::list<ToolButton*> namedToolButtons;
    std::list<ToolButton*> otherToolButtons;

    for (int n = 0; n < NAMED_PLUGIN_GROUP_NO; ++n) {
        for (U32 i = 0; i < _imp->_toolButtons.size(); ++i) {
            if ( _imp->_toolButtons[i]->hasChildren() && !_imp->_toolButtons[i]->getPluginToolButton()->hasParent() ) {
                std::string toolButtonName = _imp->_toolButtons[i]->getLabel().toStdString();

                if (n == 0) {
                    ///he first time register unnamed buttons
                    bool isNamedToolButton = false;
                    for (int j = 0; j < NAMED_PLUGIN_GROUP_NO; ++j) {
                        if (toolButtonName == namedGroupsOrdered[j]) {
                            isNamedToolButton = true;
                            break;
                        }
                    }
                    if (!isNamedToolButton) {
                        otherToolButtons.push_back(_imp->_toolButtons[i]);
                    }
                }
                if (toolButtonName == namedGroupsOrdered[n]) {
                    namedToolButtons.push_back(_imp->_toolButtons[i]);
                }
            }
        }
    }
    namedToolButtons.insert( namedToolButtons.end(), otherToolButtons.begin(), otherToolButtons.end() );

    return namedToolButtons;
}

void
Gui::addToolButttonsToToolBar()
{
    std::list<ToolButton*> orederedToolButtons = getToolButtonsOrdered();

    for (std::list<ToolButton*>::iterator it = orederedToolButtons.begin(); it != orederedToolButtons.end(); ++it) {
        _imp->addToolButton(*it);
    }
}

namespace {
class AutoRaiseToolButton
    : public QToolButton
{
    Gui* _gui;
    bool _menuOpened;

public:

    AutoRaiseToolButton(Gui* gui,
                        QWidget* parent)
        : QToolButton(parent)
        , _gui(gui)
        , _menuOpened(false)
    {
        setMouseTracking(true);
        setFocusPolicy(Qt::StrongFocus);
    }

private:

    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL
    {
        _menuOpened = !_menuOpened;
        if (_menuOpened) {
            setFocus();
            _gui->setToolButtonMenuOpened(this);
        } else {
            _gui->setToolButtonMenuOpened(NULL);
        }
        QToolButton::mousePressEvent(e);
    }

    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL
    {
        _gui->setToolButtonMenuOpened(NULL);
        QToolButton::mouseReleaseEvent(e);
    }

    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL
    {
        if (e->key() == Qt::Key_Right) {
            QMenu* m = menu();
            if (m) {
                QList<QAction*> actions = m->actions();
                if ( !actions.isEmpty() ) {
                    m->setActiveAction(actions[0]);
                }
            }
            showMenu();
        } else if (e->key() == Qt::Key_Left) {
            //This code won't work because the menu is active and modal
            //But at least it deactivate the focus tabbing when pressing the left key
            QMenu* m = menu();
            if ( m && m->isVisible() ) {
                m->hide();
            }
        } else {
            QToolButton::keyPressEvent(e);
        }
    }

    virtual void enterEvent(QEvent* e) OVERRIDE FINAL
    {
        AutoRaiseToolButton* btn = dynamic_cast<AutoRaiseToolButton*>( _gui->getToolButtonMenuOpened() );

        if ( btn && (btn != this) && btn->menu()->isActiveWindow() ) {
            btn->menu()->close();
            btn->_menuOpened = false;
            setFocus();
            _gui->setToolButtonMenuOpened(this);
            _menuOpened = true;
            showMenu();
        }
        QToolButton::enterEvent(e);
    }
};
} // anonymous namespace

void
Gui::setToolButtonMenuOpened(QToolButton* button)
{
    _imp->_toolButtonMenuOpened = button;
}

QToolButton*
Gui::getToolButtonMenuOpened() const
{
    return _imp->_toolButtonMenuOpened;
}

void
GuiPrivate::addToolButton(ToolButton* tool)
{
    QToolButton* button = new AutoRaiseToolButton(_gui, _toolBox);

    button->setIcon( tool->getIcon() );
    button->setMenu( tool->getMenu() );
    button->setPopupMode(QToolButton::InstantPopup);
    button->setToolTip( Natron::convertFromPlainText(tool->getLabel().trimmed(), Qt::WhiteSpaceNormal) );
    _toolBox->addWidget(button);
}

void
GuiPrivate::setUndoRedoActions(QAction* undoAction,
                               QAction* redoAction)
{
    if (_currentUndoAction) {
        menuEdit->removeAction(_currentUndoAction);
    }
    if (_currentRedoAction) {
        menuEdit->removeAction(_currentRedoAction);
    }
    _currentUndoAction = undoAction;
    _currentRedoAction = redoAction;
    menuEdit->addAction(undoAction);
    menuEdit->addAction(redoAction);
}

void
Gui::newProject()
{
    CLArgs cl;
    AppInstance* app = appPTR->newAppInstance(cl);

    app->execOnProjectCreatedCallback();
}

void
Gui::openProject()
{
    std::vector<std::string> filters;

    filters.push_back(NATRON_PROJECT_FILE_EXT);
    std::string selectedFile =  popOpenFileDialog( false, filters, _imp->_lastLoadProjectOpenedDir.toStdString(), false );

    if ( !selectedFile.empty() ) {
        
        std::string patternCpy = selectedFile;
        std::string path = SequenceParsing::removePath(patternCpy);
        _imp->_lastLoadProjectOpenedDir = path.c_str();
        
        openProjectInternal(selectedFile);
    }
}

void
Gui::openProject(const std::string & filename)
{
    openProjectInternal(filename);
}

void
Gui::openProjectInternal(const std::string & absoluteFileName)
{
    QFileInfo file(absoluteFileName.c_str());
    if (!file.exists()) {
        return;
    }
    QString fileUnPathed = file.fileName();
    QString path = file.path() + "/";
    int openedProject = appPTR->isProjectAlreadyOpened(absoluteFileName);

    if (openedProject != -1) {
        AppInstance* instance = appPTR->getAppInstance(openedProject);
        if (instance) {
            GuiAppInstance* guiApp = dynamic_cast<GuiAppInstance*>(instance);
            assert(guiApp);
            if (guiApp) {
                guiApp->getGui()->activateWindow();

                return;
            }
        }
    }

    ///if the current graph has no value, just load the project in the same window
    if ( _imp->_appInstance->getProject()->isGraphWorthLess() ) {
        _imp->_appInstance->getProject()->loadProject( path, fileUnPathed);
    } else {
        CLArgs cl;
        AppInstance* newApp = appPTR->newAppInstance(cl);
        newApp->getProject()->loadProject( path, fileUnPathed);
    }

    QSettings settings;
    QStringList recentFiles = settings.value("recentFileList").toStringList();
    recentFiles.removeAll( absoluteFileName.c_str() );
    recentFiles.prepend( absoluteFileName.c_str() );
    while (recentFiles.size() > NATRON_MAX_RECENT_FILES) {
        recentFiles.removeLast();
    }

    settings.setValue("recentFileList", recentFiles);
    appPTR->updateAllRecentFileMenus();
}

static void
updateRecentFiles(const QString & filename)
{
    QSettings settings;
    QStringList recentFiles = settings.value("recentFileList").toStringList();

    recentFiles.removeAll(filename);
    recentFiles.prepend(filename);
    while (recentFiles.size() > NATRON_MAX_RECENT_FILES) {
        recentFiles.removeLast();
    }

    settings.setValue("recentFileList", recentFiles);
    appPTR->updateAllRecentFileMenus();
}

bool
GuiPrivate::checkProjectLockAndWarn(const QString& projectPath,const QString& projectName)
{
    boost::shared_ptr<Natron::Project> project= _appInstance->getProject();
    QString author,lockCreationDate;
    qint64 lockPID;
    if (project->getLockFileInfos(projectPath,projectName,&author, &lockCreationDate, &lockPID)) {
        if (lockPID != QCoreApplication::applicationPid()) {
            Natron::StandardButtonEnum rep = Natron::questionDialog(QObject::tr("Project").toStdString(),
                                                                    QObject::tr("This project is already opened in another instance of Natron by ").toStdString() +
                                                                    author.toStdString() + QObject::tr(" and was opened on ").toStdString() + lockCreationDate.toStdString()
                                                                    + QObject::tr(" by a Natron process ID of ").toStdString() + QString::number(lockPID).toStdString() + QObject::tr(".\nContinue anyway?").toStdString(), false, Natron::StandardButtons(Natron::eStandardButtonYes | Natron::eStandardButtonNo));
            if (rep == Natron::eStandardButtonYes) {
                return true;
            } else {
                return false;
            }
        }
    }
    return true;
    
}

bool
Gui::saveProject()
{
    boost::shared_ptr<Natron::Project> project= _imp->_appInstance->getProject();
    if (project->hasProjectBeenSavedByUser()) {
        
        
        QString projectName = project->getProjectName();
        QString projectPath = project->getProjectPath();
        
        if (!_imp->checkProjectLockAndWarn(projectPath,projectName)) {
            return false;
        }

        project->saveProject(projectPath, projectName, false);

        ///update the open recents
        if (!projectPath.endsWith('/')) {
            projectPath.append('/');
        }
        QString file = projectPath + projectName;
        updateRecentFiles(file);

        return true;
    } else {
        return saveProjectAs();
    }
}

bool
Gui::saveProjectAs()
{
    std::vector<std::string> filter;

    filter.push_back(NATRON_PROJECT_FILE_EXT);
    std::string outFile = popSaveFileDialog( false, filter, _imp->_lastSaveProjectOpenedDir.toStdString(), false );
    if (outFile.size() > 0) {
        if (outFile.find("." NATRON_PROJECT_FILE_EXT) == std::string::npos) {
            outFile.append("." NATRON_PROJECT_FILE_EXT);
        }
        std::string path = SequenceParsing::removePath(outFile);
        
        if (!_imp->checkProjectLockAndWarn(path.c_str(),outFile.c_str())) {
            return false;
        }
        _imp->_lastSaveProjectOpenedDir = path.c_str();
        
        _imp->_appInstance->getProject()->saveProject(path.c_str(), outFile.c_str(), false);

        QString filePath = QString( path.c_str() ) + QString( outFile.c_str() );
        updateRecentFiles(filePath);

        return true;
    }

    return false;
}

void
Gui::saveAndIncrVersion()
{
    QString path = _imp->_appInstance->getProject()->getProjectPath();
    QString name = _imp->_appInstance->getProject()->getProjectName();
    int currentVersion = 0;
    int positionToInsertVersion;
    bool mustAppendFileExtension = false;

    // extension is everything after the last '.'
    int lastDotPos = name.lastIndexOf('.');

    if (lastDotPos == -1) {
        positionToInsertVersion = name.size();
        mustAppendFileExtension = true;
    } else {
        //Extract the current version number if any
        QString versionStr;
        int i = lastDotPos - 1;
        while ( i >= 0 && name.at(i).isDigit() ) {
            versionStr.prepend( name.at(i) );
            --i;
        }

        ++i; //move back the head to the first digit

        if ( !versionStr.isEmpty() ) {
            name.remove( i, versionStr.size() );
            --i; //move 1 char backward, if the char is a '_' remove it
            if ( (i >= 0) && ( name.at(i) == QChar('_') ) ) {
                name.remove(i, 1);
            }
            currentVersion = versionStr.toInt();
        }

        positionToInsertVersion = i;
    }

    //Incr version
    ++currentVersion;

    QString newVersionStr = QString::number(currentVersion);

    //Add enough 0s in the beginning of the version number to have at least 3 digits
    int nb0s = 3 - newVersionStr.size();
    nb0s = std::max(0, nb0s);

    QString toInsert("_");
    for (int c = 0; c < nb0s; ++c) {
        toInsert.append('0');
    }
    toInsert.append(newVersionStr);
    if (mustAppendFileExtension) {
        toInsert.append("." NATRON_PROJECT_FILE_EXT);
    }

    if ( positionToInsertVersion >= name.size() ) {
        name.append(toInsert);
    } else {
        name.insert(positionToInsertVersion, toInsert);
    }

    _imp->_appInstance->getProject()->saveProject(path, name, false);

    QString filename = path = name;
    updateRecentFiles(filename);
} // Gui::saveAndIncrVersion

void
Gui::createNewViewer()
{
    NodeGraph* graph = _imp->_lastFocusedGraph ? _imp->_lastFocusedGraph : _imp->_nodeGraphArea;

    assert(graph);
    ignore_result( _imp->_appInstance->createNode( CreateNodeArgs( PLUGINID_NATRON_VIEWER,
                                                                   "",
                                                                   -1, -1,
                                                                   true,
                                                                   INT_MIN, INT_MIN,
                                                                   true,
                                                                   true,
                                                                   true,
                                                                   QString(),
                                                                   CreateNodeArgs::DefaultValuesList(),
                                                                   graph->getGroup() ) ) );
}

boost::shared_ptr<Natron::Node>
Gui::createReader()
{
    boost::shared_ptr<Natron::Node> ret;
    std::map<std::string, std::string> readersForFormat;

    appPTR->getCurrentSettings()->getFileFormatsForReadingAndReader(&readersForFormat);
    std::vector<std::string> filters;
    for (std::map<std::string, std::string>::const_iterator it = readersForFormat.begin(); it != readersForFormat.end(); ++it) {
        filters.push_back(it->first);
    }
    std::string pattern = popOpenFileDialog( true, filters, _imp->_lastLoadSequenceOpenedDir.toStdString(), true );
    if ( !pattern.empty() ) {
        
        QString qpattern( pattern.c_str() );
        
        std::string patternCpy = pattern;
        std::string path = SequenceParsing::removePath(patternCpy);
        _imp->_lastLoadSequenceOpenedDir = path.c_str();
        
        std::string ext = Natron::removeFileExtension(qpattern).toLower().toStdString();
        std::map<std::string, std::string>::iterator found = readersForFormat.find(ext);
        if ( found == readersForFormat.end() ) {
            errorDialog( tr("Reader").toStdString(), tr("No plugin capable of decoding ").toStdString() + ext + tr(" was found.").toStdString(), false);
        } else {
            NodeGraph* graph = 0;
            if (_imp->_lastFocusedGraph) {
                graph = _imp->_lastFocusedGraph;
            } else {
                graph = _imp->_nodeGraphArea;
            }
            boost::shared_ptr<NodeCollection> group = graph->getGroup();
            assert(group);

            CreateNodeArgs::DefaultValuesList defaultValues;
            defaultValues.push_back( createDefaultValueForParam<std::string>(kOfxImageEffectFileParamName, pattern) );
            CreateNodeArgs args(found->second.c_str(),
                                "",
                                -1, -1,
                                true,
                                INT_MIN, INT_MIN,
                                true,
                                true,
                                true,
                                QString(),
                                defaultValues,
                                group);
            ret = _imp->_appInstance->createNode(args);

            if (!ret) {
                return ret;
            }
        }
    }

    return ret;
}

boost::shared_ptr<Natron::Node>
Gui::createWriter()
{
    boost::shared_ptr<Natron::Node> ret;
    std::map<std::string, std::string> writersForFormat;

    appPTR->getCurrentSettings()->getFileFormatsForWritingAndWriter(&writersForFormat);
    std::vector<std::string> filters;
    for (std::map<std::string, std::string>::const_iterator it = writersForFormat.begin(); it != writersForFormat.end(); ++it) {
        filters.push_back(it->first);
    }
    std::string file = popSaveFileDialog( true, filters, _imp->_lastSaveSequenceOpenedDir.toStdString(), true );
    if ( !file.empty() ) {
        
        std::string patternCpy = file;
        std::string path = SequenceParsing::removePath(patternCpy);
        _imp->_lastSaveSequenceOpenedDir = path.c_str();
        
        NodeGraph* graph = 0;
        if (_imp->_lastFocusedGraph) {
            graph = _imp->_lastFocusedGraph;
        } else {
            graph = _imp->_nodeGraphArea;
        }
        boost::shared_ptr<NodeCollection> group = graph->getGroup();
        assert(group);

        ret =  getApp()->createWriter(file, group, true);
    }

    return ret;
}

std::string
Gui::popOpenFileDialog(bool sequenceDialog,
                       const std::vector<std::string> & initialfilters,
                       const std::string & initialDir,
                       bool allowRelativePaths)
{
    SequenceFileDialog dialog(this, initialfilters, sequenceDialog, SequenceFileDialog::eFileDialogModeOpen, initialDir, this, allowRelativePaths);

    if ( dialog.exec() ) {
        return dialog.selectedFiles();
    } else {
        return std::string();
    }
}

std::string
Gui::openImageSequenceDialog()
{
    std::map<std::string, std::string> readersForFormat;

    appPTR->getCurrentSettings()->getFileFormatsForReadingAndReader(&readersForFormat);
    std::vector<std::string> filters;
    for (std::map<std::string, std::string>::const_iterator it = readersForFormat.begin(); it != readersForFormat.end(); ++it) {
        filters.push_back(it->first);
    }

    return popOpenFileDialog(true, filters, _imp->_lastLoadSequenceOpenedDir.toStdString(), true);
}

std::string
Gui::saveImageSequenceDialog()
{
    std::map<std::string, std::string> writersForFormat;

    appPTR->getCurrentSettings()->getFileFormatsForWritingAndWriter(&writersForFormat);
    std::vector<std::string> filters;
    for (std::map<std::string, std::string>::const_iterator it = writersForFormat.begin(); it != writersForFormat.end(); ++it) {
        filters.push_back(it->first);
    }

    return popSaveFileDialog(true, filters, _imp->_lastSaveSequenceOpenedDir.toStdString(), true);
}

std::string
Gui::popSaveFileDialog(bool sequenceDialog,
                       const std::vector<std::string> & initialfilters,
                       const std::string & initialDir,
                       bool allowRelativePaths)
{
    SequenceFileDialog dialog(this, initialfilters, sequenceDialog, SequenceFileDialog::eFileDialogModeSave, initialDir, this, allowRelativePaths);

    if ( dialog.exec() ) {
        return dialog.filesToSave();
    } else {
        return "";
    }
}

void
Gui::autoSave()
{
    _imp->_appInstance->getProject()->autoSave();
}

int
Gui::saveWarning()
{
    if ( !_imp->_appInstance->getProject()->isSaveUpToDate() ) {
        Natron::StandardButtonEnum ret =  Natron::questionDialog(NATRON_APPLICATION_NAME, tr("Save changes to ").toStdString() +
                                                                 _imp->_appInstance->getProject()->getProjectName().toStdString() + " ?",
                                                                 false,
                                                                 Natron::StandardButtons(Natron::eStandardButtonSave | Natron::eStandardButtonDiscard | Natron::eStandardButtonCancel), Natron::eStandardButtonSave);
        if ( (ret == Natron::eStandardButtonEscape) || (ret == Natron::eStandardButtonCancel) ) {
            return 2;
        } else if (ret == Natron::eStandardButtonDiscard) {
            return 1;
        } else {
            return 0;
        }
    }

    return -1;
}

void
Gui::loadProjectGui(boost::archive::xml_iarchive & obj) const
{
    assert(_imp->_projectGui);
    _imp->_projectGui->load(obj);
}

void
Gui::saveProjectGui(boost::archive::xml_oarchive & archive)
{
    assert(_imp->_projectGui);
    _imp->_projectGui->save(archive);
}

void
Gui::errorDialog(const std::string & title,
                 const std::string & text,
                 bool useHtml)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return;
        }
    }


    Natron::StandardButtons buttons(Natron::eStandardButtonYes | Natron::eStandardButtonNo);

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        Q_EMIT doDialog(0, QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonYes);
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        Q_EMIT doDialog(0, QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonYes);
    }
}

void
Gui::errorDialog(const std::string & title,
                 const std::string & text,
                 bool* stopAsking,
                 bool useHtml)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return;
        }
    }

    Natron::StandardButtons buttons(Natron::eStandardButtonOk);

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        Q_EMIT doDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeError, QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonOk );
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        Q_EMIT doDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeError,
                                               QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonOk );
    }
    *stopAsking = _imp->_lastStopAskingAnswer;
}

void
Gui::warningDialog(const std::string & title,
                   const std::string & text,
                   bool useHtml)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return;
        }
    }

    Natron::StandardButtons buttons(Natron::eStandardButtonYes | Natron::eStandardButtonNo);

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        Q_EMIT doDialog(1, QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonYes);
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        Q_EMIT doDialog(1, QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonYes);
    }
}

void
Gui::warningDialog(const std::string & title,
                   const std::string & text,
                   bool* stopAsking,
                   bool useHtml)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return;
        }
    }

    Natron::StandardButtons buttons(Natron::eStandardButtonOk);

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        Q_EMIT doDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeWarning, QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonOk );
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        Q_EMIT doDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeWarning,
                                               QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonOk );
    }
    *stopAsking = _imp->_lastStopAskingAnswer;
}

void
Gui::informationDialog(const std::string & title,
                       const std::string & text,
                       bool useHtml)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return;
        }
    }

    Natron::StandardButtons buttons(Natron::eStandardButtonYes | Natron::eStandardButtonNo);

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        Q_EMIT doDialog(2, QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonYes);
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        Q_EMIT doDialog(2, QString( title.c_str() ), QString( text.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonYes);
    }
}

void
Gui::informationDialog(const std::string & title,
                       const std::string & message,
                       bool* stopAsking,
                       bool useHtml)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return;
        }
    }

    Natron::StandardButtons buttons(Natron::eStandardButtonOk);

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        Q_EMIT doDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeInformation, QString( title.c_str() ), QString( message.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonOk );
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        Q_EMIT doDialogWithStopAskingCheckbox( (int)MessageBox::eMessageBoxTypeInformation, QString( title.c_str() ), QString( message.c_str() ), useHtml, buttons, (int)Natron::eStandardButtonOk );
    }
    *stopAsking = _imp->_lastStopAskingAnswer;
}

namespace {
    // resizable message box,
    // see http://www.qtcentre.org/threads/24888-Resizing-a-QMessageBox#post135851
    // and http://stackoverflow.com/questions/2655354/how-to-allow-resizing-of-qmessagebox-in-pyqt4
    class MyMessageBox : public QMessageBox {
    public:
        explicit MyMessageBox(QWidget *parent = 0)
        : QMessageBox(parent)
        {
            setSizeGripEnabled(true);
        }

        MyMessageBox(Icon icon, const QString &title, const QString &text,
                    StandardButtons buttons = NoButton, QWidget *parent = 0,
                    Qt::WindowFlags flags = Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint)
        : QMessageBox(icon, title, text, buttons, parent, flags)
        {
            setSizeGripEnabled(true);
        }
    private:
        bool event(QEvent *e) OVERRIDE FINAL
        {
            bool result = QMessageBox::event(e);
            
            //QMessageBox::event in this case will call setFixedSize on the dialog frame, making it not resizable by the user
            if (e->type() == QEvent::LayoutRequest || e->type() == QEvent::Resize) {
                setMinimumHeight(0);
                setMaximumHeight(QWIDGETSIZE_MAX);
                setMinimumWidth(0);
                setMaximumWidth(QWIDGETSIZE_MAX);
                setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                
                // make the detailed text expanding
                QTextEdit *textEdit = findChild<QTextEdit *>();
                
                if (textEdit) {
                    textEdit->setMinimumHeight(0);
                    textEdit->setMaximumHeight(QWIDGETSIZE_MAX);
                    textEdit->setMinimumWidth(0);
                    textEdit->setMaximumWidth(QWIDGETSIZE_MAX);
                    textEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                }
                
               
            }
            return result;
        }
    };
}

void
Gui::onDoDialog(int type,
                const QString & title,
                const QString & content,
                bool useHtml,
                Natron::StandardButtons buttons,
                int defaultB)
{
    QString msg = useHtml ? content : Natron::convertFromPlainText(content.trimmed(), Qt::WhiteSpaceNormal);


    if (type == 0) { // error dialog
        QMessageBox critical(QMessageBox::Critical, title, msg, QMessageBox::NoButton, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
        critical.setWindowFlags(critical.windowFlags() | Qt::WindowStaysOnTopHint);
        critical.setTextFormat(Qt::RichText);   //this is what makes the links clickable
        ignore_result( critical.exec() );
    } else if (type == 1) { // warning dialog
        QMessageBox warning(QMessageBox::Warning, title, msg, QMessageBox::NoButton, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
        warning.setTextFormat(Qt::RichText);
        warning.setWindowFlags(warning.windowFlags() | Qt::WindowStaysOnTopHint);
        ignore_result( warning.exec() );
    } else if (type == 2) { // information dialog
        if (msg.count() < 1000) {
            QMessageBox info(QMessageBox::Information, title, msg, QMessageBox::NoButton, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint| Qt::WindowStaysOnTopHint);
            info.setTextFormat(Qt::RichText);
            info.setWindowFlags(info.windowFlags() | Qt::WindowStaysOnTopHint);
            ignore_result( info.exec() );
        } else {
            // text may be very long: use resizable QMessageBox
            MyMessageBox info(QMessageBox::Information, title, msg.left(1000), QMessageBox::NoButton, this, Qt::Dialog | Qt::WindowStaysOnTopHint);
            info.setTextFormat(Qt::RichText);
            info.setWindowFlags(info.windowFlags() | Qt::WindowStaysOnTopHint);
            QGridLayout *layout = qobject_cast<QGridLayout *>( info.layout() );
            if (layout) {
                QTextEdit *edit = new QTextEdit();
                edit->setReadOnly(true);
                edit->setAcceptRichText(true);
                edit->setHtml(msg);
                layout->setRowStretch(1, 0);
                layout->addWidget(edit, 0, 1);
            }
            ignore_result( info.exec() );
        }
    } else { // question dialog
        assert(type == 3);
        QMessageBox ques(QMessageBox::Question, title, msg, QtEnumConvert::toQtStandarButtons(buttons),
                         this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
        ques.setDefaultButton( QtEnumConvert::toQtStandardButton( (Natron::StandardButtonEnum)defaultB ) );
        ques.setWindowFlags(ques.windowFlags() | Qt::WindowStaysOnTopHint);
        if ( ques.exec() ) {
            _imp->_lastQuestionDialogAnswer = QtEnumConvert::fromQtStandardButton( ques.standardButton( ques.clickedButton() ) );
        }
    }

    QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
    _imp->_uiUsingMainThread = false;
    _imp->_uiUsingMainThreadCond.wakeOne();
}

Natron::StandardButtonEnum
Gui::questionDialog(const std::string & title,
                    const std::string & message,
                    bool useHtml,
                    Natron::StandardButtons buttons,
                    Natron::StandardButtonEnum defaultButton)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return Natron::eStandardButtonNo;
        }
    }

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        Q_EMIT doDialog(3, QString( title.c_str() ), QString( message.c_str() ), useHtml, buttons, (int)defaultButton);
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        Q_EMIT doDialog(3, QString( title.c_str() ), QString( message.c_str() ), useHtml, buttons, (int)defaultButton);
    }

    return _imp->_lastQuestionDialogAnswer;
}

Natron::StandardButtonEnum
Gui::questionDialog(const std::string & title,
                    const std::string & message,
                    bool useHtml,
                    Natron::StandardButtons buttons,
                    Natron::StandardButtonEnum defaultButton,
                    bool* stopAsking)
{
    ///don't show dialogs when about to close, otherwise we could enter in a deadlock situation
    {
        QMutexLocker l(&_imp->aboutToCloseMutex);
        if (_imp->_aboutToClose) {
            return Natron::eStandardButtonNo;
        }
    }

    if ( QThread::currentThread() != QCoreApplication::instance()->thread() ) {
        QMutexLocker locker(&_imp->_uiUsingMainThreadMutex);
        _imp->_uiUsingMainThread = true;
        locker.unlock();
        Q_EMIT onDoDialogWithStopAskingCheckbox( (int)Natron::MessageBox::eMessageBoxTypeQuestion,
                                                 QString( title.c_str() ), QString( message.c_str() ), useHtml, buttons, (int)defaultButton );
        locker.relock();
        while (_imp->_uiUsingMainThread) {
            _imp->_uiUsingMainThreadCond.wait(&_imp->_uiUsingMainThreadMutex);
        }
    } else {
        Q_EMIT onDoDialogWithStopAskingCheckbox( (int)Natron::MessageBox::eMessageBoxTypeQuestion,
                                                 QString( title.c_str() ), QString( message.c_str() ), useHtml, buttons, (int)defaultButton );
    }

    *stopAsking = _imp->_lastStopAskingAnswer;

    return _imp->_lastQuestionDialogAnswer;
}

void
Gui::onDoDialogWithStopAskingCheckbox(int type,
                                      const QString & title,
                                      const QString & content,
                                      bool useHtml,
                                      Natron::StandardButtons buttons,
                                      int defaultB)
{
    QString message = useHtml ? content : Natron::convertFromPlainText(content.trimmed(), Qt::WhiteSpaceNormal);
    Natron::MessageBox dialog(title, content, (Natron::MessageBox::MessageBoxTypeEnum)type, buttons, (Natron::StandardButtonEnum)defaultB, this);
    QCheckBox* stopAskingCheckbox = new QCheckBox(tr("Do Not Show This Again"), &dialog);

    dialog.setCheckBox(stopAskingCheckbox);
    dialog.setWindowFlags(dialog.windowFlags() | Qt::WindowStaysOnTopHint);
    if ( dialog.exec() ) {
        _imp->_lastQuestionDialogAnswer = dialog.getReply();
        _imp->_lastStopAskingAnswer = stopAskingCheckbox->isChecked();
    }
}

void
Gui::selectNode(boost::shared_ptr<NodeGui> node)
{
    if (!node) {
        return;
    }
    _imp->_nodeGraphArea->selectNode(node, false); //< wipe current selection
}

void
Gui::connectInput1()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(0);
}

void
Gui::connectInput2()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(1);
}

void
Gui::connectInput3()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(2);
}

void
Gui::connectInput4()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(3);
}

void
Gui::connectInput5()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(4);
}

void
Gui::connectInput6()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(5);
}

void
Gui::connectInput7()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(6);
}

void
Gui::connectInput8()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(7);
}

void
Gui::connectInput9()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(8);
}

void
Gui::connectInput10()
{
    NodeGraph* graph = 0;

    if (_imp->_lastFocusedGraph) {
        graph = _imp->_lastFocusedGraph;
    } else {
        graph = _imp->_nodeGraphArea;
    }
    graph->connectCurrentViewerToSelection(9);
}

void
GuiPrivate::restoreGuiGeometry()
{
    QSettings settings(NATRON_ORGANIZATION_NAME, NATRON_APPLICATION_NAME);

    settings.beginGroup("MainWindow");

    if ( settings.contains("pos") ) {
        QPoint pos = settings.value("pos").toPoint();
        _gui->move(pos);
    }
    if ( settings.contains("size") ) {
        QSize size = settings.value("size").toSize();
        _gui->resize(size);
    } else {
        ///No window size serialized, give some appriopriate default value according to the screen size
        QDesktopWidget* desktop = QApplication::desktop();
        QRect screen = desktop->screenGeometry();
        _gui->resize( (int)( 0.93 * screen.width() ), (int)( 0.93 * screen.height() ) ); // leave some space
    }
    if ( settings.contains("fullScreen") ) {
        bool fs = settings.value("fullScreen").toBool();
        if (fs) {
            _gui->toggleFullScreen();
        }
    }

    if ( settings.contains("ToolbarHidden") ) {
        leftToolBarDisplayedOnHoverOnly = settings.value("ToolbarHidden").toBool();
    }

    settings.endGroup();

    if ( settings.contains("LastOpenProjectDialogPath") ) {
        _lastLoadProjectOpenedDir = settings.value("LastOpenProjectDialogPath").toString();
        QDir d(_lastLoadProjectOpenedDir);
        if ( !d.exists() ) {
            _lastLoadProjectOpenedDir.clear();
        }
    }
    if ( settings.contains("LastSaveProjectDialogPath") ) {
        _lastSaveProjectOpenedDir = settings.value("LastSaveProjectDialogPath").toString();
        QDir d(_lastSaveProjectOpenedDir);
        if ( !d.exists() ) {
            _lastSaveProjectOpenedDir.clear();
        }
    }
    if ( settings.contains("LastLoadSequenceDialogPath") ) {
        _lastLoadSequenceOpenedDir = settings.value("LastLoadSequenceDialogPath").toString();
        QDir d(_lastLoadSequenceOpenedDir);
        if ( !d.exists() ) {
            _lastLoadSequenceOpenedDir.clear();
        }
    }
    if ( settings.contains("LastSaveSequenceDialogPath") ) {
        _lastSaveSequenceOpenedDir = settings.value("LastSaveSequenceDialogPath").toString();
        QDir d(_lastSaveSequenceOpenedDir);
        if ( !d.exists() ) {
            _lastSaveSequenceOpenedDir.clear();
        }
    }
    if ( settings.contains("LastPluginDir") ) {
        _lastPluginDir = settings.value("LastPluginDir").toString();
    }
} // GuiPrivate::restoreGuiGeometry

void
GuiPrivate::saveGuiGeometry()
{
    QSettings settings(NATRON_ORGANIZATION_NAME, NATRON_APPLICATION_NAME);

    settings.beginGroup("MainWindow");
    settings.setValue( "pos", _gui->pos() );
    settings.setValue( "size", _gui->size() );
    settings.setValue( "fullScreen", _gui->isFullScreen() );
    settings.setValue( "ToolbarHidden", leftToolBarDisplayedOnHoverOnly);
    settings.endGroup();

    settings.setValue("LastOpenProjectDialogPath", _lastLoadProjectOpenedDir);
    settings.setValue("LastSaveProjectDialogPath", _lastSaveProjectOpenedDir);
    settings.setValue("LastLoadSequenceDialogPath", _lastLoadSequenceOpenedDir);
    settings.setValue("LastSaveSequenceDialogPath", _lastSaveSequenceOpenedDir);
    settings.setValue("LastPluginDir", _lastPluginDir);
}

void
Gui::showView0()
{
    _imp->_appInstance->setViewersCurrentView(0);
}

void
Gui::showView1()
{
    _imp->_appInstance->setViewersCurrentView(1);
}

void
Gui::showView2()
{
    _imp->_appInstance->setViewersCurrentView(2);
}

void
Gui::showView3()
{
    _imp->_appInstance->setViewersCurrentView(3);
}

void
Gui::showView4()
{
    _imp->_appInstance->setViewersCurrentView(4);
}

void
Gui::showView5()
{
    _imp->_appInstance->setViewersCurrentView(5);
}

void
Gui::showView6()
{
    _imp->_appInstance->setViewersCurrentView(6);
}

void
Gui::showView7()
{
    _imp->_appInstance->setViewersCurrentView(7);
}

void
Gui::showView8()
{
    _imp->_appInstance->setViewersCurrentView(8);
}

void
Gui::showView9()
{
    _imp->_appInstance->setViewersCurrentView(9);
}

void
Gui::setCurveEditorOnTop()
{
    QMutexLocker l(&_imp->_panesMutex);

    for (std::list<TabWidget*>::iterator it = _imp->_panes.begin(); it != _imp->_panes.end(); ++it) {
        TabWidget* cur = (*it);
        assert(cur);
        for (int i = 0; i < cur->count(); ++i) {
            if (cur->tabAt(i) == _imp->_curveEditor) {
                cur->makeCurrentTab(i);
                break;
            }
        }
    }
}

void
Gui::showSettings()
{
    if (!_imp->_settingsGui) {
        _imp->_settingsGui = new PreferencesPanel(appPTR->getCurrentSettings(), this);
    }
    _imp->_settingsGui->show();
    _imp->_settingsGui->raise();
    _imp->_settingsGui->activateWindow();
}

void
Gui::registerNewUndoStack(QUndoStack* stack)
{
    _imp->_undoStacksGroup->addStack(stack);
    QAction* undo = stack->createUndoAction(stack);
    undo->setShortcut(QKeySequence::Undo);
    QAction* redo = stack->createRedoAction(stack);
    redo->setShortcut(QKeySequence::Redo);
    _imp->_undoStacksActions.insert( std::make_pair( stack, std::make_pair(undo, redo) ) );
}

void
Gui::removeUndoStack(QUndoStack* stack)
{
    std::map<QUndoStack*, std::pair<QAction*, QAction*> >::iterator it = _imp->_undoStacksActions.find(stack);
	if (it == _imp->_undoStacksActions.end()) {
		return;
	}
    if (_imp->_currentUndoAction == it->second.first) {
        _imp->menuEdit->removeAction(_imp->_currentUndoAction);
    }
    if (_imp->_currentRedoAction == it->second.second) {
        _imp->menuEdit->removeAction(_imp->_currentRedoAction);
    }
    if ( it != _imp->_undoStacksActions.end() ) {
        _imp->_undoStacksActions.erase(it);
    }
}

void
Gui::onCurrentUndoStackChanged(QUndoStack* stack)
{
    std::map<QUndoStack*, std::pair<QAction*, QAction*> >::iterator it = _imp->_undoStacksActions.find(stack);

    //the stack must have been registered first with registerNewUndoStack()
    if ( it != _imp->_undoStacksActions.end() ) {
        _imp->setUndoRedoActions(it->second.first, it->second.second);
    }
}

void
Gui::refreshAllPreviews()
{
    _imp->_appInstance->getProject()->refreshPreviews();
}

void
Gui::forceRefreshAllPreviews()
{
    _imp->_appInstance->getProject()->forceRefreshPreviews();
}

void
Gui::startDragPanel(QWidget* panel)
{
    assert(!_imp->_currentlyDraggedPanel);
    _imp->_currentlyDraggedPanel = panel;
}

QWidget*
Gui::stopDragPanel()
{
    assert(_imp->_currentlyDraggedPanel);
    QWidget* ret = _imp->_currentlyDraggedPanel;
    _imp->_currentlyDraggedPanel = 0;

    return ret;
}

void
Gui::showAbout()
{
    _imp->_aboutWindow->show();
    _imp->_aboutWindow->raise();
    _imp->_aboutWindow->activateWindow();
    ignore_result( _imp->_aboutWindow->exec() );
}

void
Gui::showShortcutEditor()
{
    _imp->shortcutEditor->show();
    _imp->shortcutEditor->raise();
    _imp->shortcutEditor->activateWindow();
}

void
Gui::openRecentFile()
{
    QAction *action = qobject_cast<QAction *>( sender() );

    if (action) {
        QFileInfo f( action->data().toString() );
        QString path = f.path() + '/';
        QString filename = path + f.fileName();
        int openedProject = appPTR->isProjectAlreadyOpened( filename.toStdString() );
        if (openedProject != -1) {
            AppInstance* instance = appPTR->getAppInstance(openedProject);
            if (instance) {
                GuiAppInstance* guiApp = dynamic_cast<GuiAppInstance*>(instance);
                assert(guiApp);
                if (guiApp) {
                    guiApp->getGui()->activateWindow();

                    return;
                }
            }
        }

        ///if the current graph has no value, just load the project in the same window
        if ( _imp->_appInstance->getProject()->isGraphWorthLess() ) {
            _imp->_appInstance->getProject()->loadProject( path, f.fileName() );
        } else {
            CLArgs cl;
            AppInstance* newApp = appPTR->newAppInstance(cl);
            newApp->getProject()->loadProject( path, f.fileName() );
        }
    }
}

void
Gui::updateRecentFileActions()
{
    QSettings settings;
    QStringList files = settings.value("recentFileList").toStringList();
    int numRecentFiles = std::min(files.size(), (int)NATRON_MAX_RECENT_FILES);

    for (int i = 0; i < numRecentFiles; ++i) {
        QString text = tr("&%1 %2").arg(i + 1).arg( QFileInfo(files[i]).fileName() );
        _imp->actionsOpenRecentFile[i]->setText(text);
        _imp->actionsOpenRecentFile[i]->setData(files[i]);
        _imp->actionsOpenRecentFile[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < NATRON_MAX_RECENT_FILES; ++j) {
        _imp->actionsOpenRecentFile[j]->setVisible(false);
    }
}

QPixmap
Gui::screenShot(QWidget* w)
{
#if QT_VERSION < 0x050000
    if (w->objectName() == "CurveEditor") {
        return QPixmap::grabWidget(w);
    }

    return QPixmap::grabWindow( w->winId() );
#else

    return QApplication::primaryScreen()->grabWindow( w->winId() );
#endif
}

void
Gui::onProjectNameChanged(const QString & name)
{
    QString text(QCoreApplication::applicationName() + " - ");

    text.append(name);
    setWindowTitle(text);
}

void
Gui::setColorPickersColor(double r,double g, double b,double a)
{
    assert(_imp->_projectGui);
    _imp->_projectGui->setPickersColor(r,g,b,a);
}

void
Gui::registerNewColorPicker(boost::shared_ptr<Color_Knob> knob)
{
    assert(_imp->_projectGui);
    _imp->_projectGui->registerNewColorPicker(knob);
}

void
Gui::removeColorPicker(boost::shared_ptr<Color_Knob> knob)
{
    assert(_imp->_projectGui);
    _imp->_projectGui->removeColorPicker(knob);
}

bool
Gui::hasPickers() const
{
    assert(_imp->_projectGui);

    return _imp->_projectGui->hasPickers();
}

void
Gui::updateViewersViewsMenu(int viewsCount)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->updateViewsMenu(viewsCount);
    }
}

void
Gui::setViewersCurrentView(int view)
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        (*it)->setCurrentView(view);
    }
}

const std::list<ViewerTab*> &
Gui::getViewersList() const
{
    return _imp->_viewerTabs;
}

std::list<ViewerTab*>
Gui::getViewersList_mt_safe() const
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    return _imp->_viewerTabs;
}

void
Gui::activateViewerTab(ViewerInstance* viewer)
{
    OpenGLViewerI* viewport = viewer->getUiContext();

    {
        QMutexLocker l(&_imp->_viewerTabsMutex);
        for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
            if ( (*it)->getViewer() == viewport ) {
                TabWidget* viewerAnchor = getAnchor();
                assert(viewerAnchor);
                viewerAnchor->appendTab(*it, *it);
                (*it)->show();
            }
        }
    }
    Q_EMIT viewersChanged();
}

void
Gui::deactivateViewerTab(ViewerInstance* viewer)
{
    OpenGLViewerI* viewport = viewer->getUiContext();
    ViewerTab* v = 0;
    {
        QMutexLocker l(&_imp->_viewerTabsMutex);
        for (std::list<ViewerTab*>::iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
            if ( (*it)->getViewer() == viewport ) {
                v = *it;
                break;
            }
        }
    }

    if (v) {
        removeViewerTab(v, true, false);
    }
}

ViewerTab*
Gui::getViewerTabForInstance(ViewerInstance* node) const
{
    QMutexLocker l(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::const_iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        if ( (*it)->getInternalNode() == node ) {
            return *it;
        }
    }

    return NULL;
}

const std::list<boost::shared_ptr<NodeGui> > &
Gui::getVisibleNodes() const
{
    return _imp->_nodeGraphArea->getAllActiveNodes();
}

std::list<boost::shared_ptr<NodeGui> >
Gui::getVisibleNodes_mt_safe() const
{
    return _imp->_nodeGraphArea->getAllActiveNodes_mt_safe();
}

void
Gui::deselectAllNodes() const
{
    _imp->_nodeGraphArea->deselect();
}

void
Gui::onProcessHandlerStarted(const QString & sequenceName,
                             int firstFrame,
                             int lastFrame,
                             const boost::shared_ptr<ProcessHandler> & process)
{
    ///make the dialog which will show the progress
    RenderingProgressDialog *dialog = new RenderingProgressDialog(this, sequenceName, firstFrame, lastFrame, process, this);
    QObject::connect(dialog,SIGNAL(accepted()),this,SLOT(onRenderProgressDialogFinished()));
    QObject::connect(dialog,SIGNAL(rejected()),this,SLOT(onRenderProgressDialogFinished()));
    dialog->show();
}

void
Gui::onRenderProgressDialogFinished()
{
    RenderingProgressDialog* dialog = qobject_cast<RenderingProgressDialog*>(sender());
    if (!dialog) {
        return;
    }
    dialog->close();
    dialog->deleteLater();
}

void
Gui::setNextViewerAnchor(TabWidget* where)
{
    _imp->_nextViewerTabPlace = where;
}

const std::vector<ToolButton*> &
Gui::getToolButtons() const
{
    return _imp->_toolButtons;
}

GuiAppInstance*
Gui::getApp() const
{
    return _imp->_appInstance;
}

const std::list<TabWidget*> &
Gui::getPanes() const
{
    return _imp->_panes;
}

std::list<TabWidget*>
Gui::getPanes_mt_safe() const
{
    QMutexLocker l(&_imp->_panesMutex);

    return _imp->_panes;
}

int
Gui::getPanesCount() const
{
    QMutexLocker l(&_imp->_panesMutex);

    return (int)_imp->_panes.size();
}

QString
Gui::getAvailablePaneName(const QString & baseName) const
{
    QString name = baseName;
    QMutexLocker l(&_imp->_panesMutex);
    int baseNumber = _imp->_panes.size();

    if ( name.isEmpty() ) {
        name.append("pane");
        name.append( QString::number(baseNumber) );
    }

    for (;; ) {
        bool foundName = false;
        for (std::list<TabWidget*>::const_iterator it = _imp->_panes.begin(); it != _imp->_panes.end(); ++it) {
            if ( (*it)->objectName_mt_safe() == name ) {
                foundName = true;
                break;
            }
        }
        if (foundName) {
            ++baseNumber;
            name = QString("pane%1").arg(baseNumber);
        } else {
            break;
        }
    }

    return name;
}

void
Gui::setUserScrubbingSlider(bool b)
{
    QMutexLocker k(&_imp->_isUserScrubbingSliderMutex);
    _imp->_isUserScrubbingSlider = b;
}

bool
Gui::isUserScrubbingSlider() const
{
    QMutexLocker k(&_imp->_isUserScrubbingSliderMutex);
    return _imp->_isUserScrubbingSlider;
}

bool
Gui::isDraggingPanel() const
{
    return _imp->_currentlyDraggedPanel != NULL;
}

NodeGraph*
Gui::getNodeGraph() const
{
    return _imp->_nodeGraphArea;
}

CurveEditor*
Gui::getCurveEditor() const
{
    return _imp->_curveEditor;
}

DopeSheetEditor *Gui::getDopeSheetEditor() const
{
    return _imp->_dopeSheetEditor;
}

ScriptEditor*
Gui::getScriptEditor() const
{
    return _imp->_scriptEditor;
}

PropertiesBinWrapper*
Gui::getPropertiesBin() const
{
    return _imp->_propertiesBin;
}

QVBoxLayout*
Gui::getPropertiesLayout() const
{
    return _imp->_layoutPropertiesBin;
}

void
Gui::appendTabToDefaultViewerPane(QWidget* tab,
                                  ScriptObject* obj)
{
    TabWidget* viewerAnchor = getAnchor();

    assert(viewerAnchor);
    viewerAnchor->appendTab(tab, obj);
}

QWidget*
Gui::getCentralWidget() const
{
    std::list<QWidget*> children;

    _imp->_leftRightSplitter->getChildren_mt_safe(children);
    if (children.size() != 2) {
        ///something is wrong
        return NULL;
    }
    for (std::list<QWidget*>::iterator it = children.begin(); it != children.end(); ++it) {
        if (*it == _imp->_toolBox) {
            continue;
        }

        return *it;
    }

    return NULL;
}

const RegisteredTabs &
Gui::getRegisteredTabs() const
{
    return _imp->_registeredTabs;
}

void
Gui::debugImage(const Natron::Image* image,
                const QString & filename )
{
    if (image->getBitDepth() != Natron::eImageBitDepthFloat) {
        qDebug() << "Debug image only works on float images.";

        return;
    }
    RectI rod = image->getBounds();
    QImage output(rod.width(), rod.height(), QImage::Format_ARGB32);
    const Natron::Color::Lut* lut = Natron::Color::LutManager::sRGBLut();
    Natron::Image::ReadAccess acc = image->getReadRights();
    const float* from = (const float*)acc.pixelAt( rod.left(), rod.bottom() );

    ///offset the pointer to 0,0
    from -= ( ( rod.bottom() * image->getRowElements() ) + rod.left() * image->getComponentsCount() );
    lut->to_byte_packed(output.bits(), from, rod, rod, rod,
                        Natron::Color::ePixelPackingRGBA, Natron::Color::ePixelPackingBGRA, true, false);
    U64 hashKey = image->getHashKey();
    QString hashKeyStr = QString::number(hashKey);
    QString realFileName = filename.isEmpty() ? QString(hashKeyStr + ".png") : filename;
    std::cout << "DEBUG: writing image: " << realFileName.toStdString() << std::endl;
    output.save(realFileName);
}

void
Gui::updateLastSequenceOpenedPath(const QString & path)
{
    _imp->_lastLoadSequenceOpenedDir = path;
}

void
Gui::updateLastSequenceSavedPath(const QString & path)
{
    _imp->_lastSaveSequenceOpenedDir = path;
}

void
Gui::updateLastSavedProjectPath(const QString & project)
{
    _imp->_lastSaveProjectOpenedDir = project;
}

void
Gui::updateLastOpenedProjectPath(const QString & project)
{
    _imp->_lastLoadProjectOpenedDir = project;
}

void
Gui::onWriterRenderStarted(const QString & sequenceName,
                           int firstFrame,
                           int lastFrame,
                           Natron::OutputEffectInstance* writer)
{
    assert( QThread::currentThread() == qApp->thread() );

    RenderingProgressDialog *dialog = new RenderingProgressDialog(this, sequenceName, firstFrame, lastFrame,
                                                                  boost::shared_ptr<ProcessHandler>(), this);
    RenderEngine* engine = writer->getRenderEngine();
    QObject::connect( dialog, SIGNAL( canceled() ), engine, SLOT( abortRendering_Blocking() ) );
    QObject::connect( engine, SIGNAL( frameRendered(int) ), dialog, SLOT( onFrameRendered(int) ) );
    QObject::connect( engine, SIGNAL( renderFinished(int) ), dialog, SLOT( onVideoEngineStopped(int) ) );
    QObject::connect(dialog,SIGNAL(accepted()),this,SLOT(onRenderProgressDialogFinished()));
    QObject::connect(dialog,SIGNAL(rejected()),this,SLOT(onRenderProgressDialogFinished()));
    dialog->show();
}

void
Gui::setGlewVersion(const QString & version)
{
    _imp->_glewVersion = version;
    _imp->_aboutWindow->updateLibrariesVersions();
}

void
Gui::setOpenGLVersion(const QString & version)
{
    _imp->_openGLVersion = version;
    _imp->_aboutWindow->updateLibrariesVersions();
}

QString
Gui::getGlewVersion() const
{
    return _imp->_glewVersion;
}

QString
Gui::getOpenGLVersion() const
{
    return _imp->_openGLVersion;
}

QString
Gui::getBoostVersion() const
{
    return QString(BOOST_LIB_VERSION);
}

QString
Gui::getQtVersion() const
{
    return QString(QT_VERSION_STR) + " / " + qVersion();
}

QString
Gui::getCairoVersion() const
{
    return QString(CAIRO_VERSION_STRING) + " / " + QString( cairo_version_string() );
}

void
Gui::onNodeNameChanged(const QString & /*name*/)
{
    Natron::Node* node = qobject_cast<Natron::Node*>( sender() );

    if (!node) {
        return;
    }
    ViewerInstance* isViewer = dynamic_cast<ViewerInstance*>( node->getLiveInstance() );
    if (isViewer) {
        Q_EMIT viewersChanged();
    }
}

void
Gui::renderAllWriters()
{
    _imp->_appInstance->startWritersRendering( std::list<AppInstance::RenderRequest>() );
}

void
Gui::renderSelectedNode()
{
    NodeGraph* graph = getLastSelectedGraph();
    if (!graph) {
        return;
    }
    
    const std::list<boost::shared_ptr<NodeGui> > & selectedNodes = graph->getSelectedNodes();

    if ( selectedNodes.empty() ) {
        Natron::warningDialog( tr("Render").toStdString(), tr("You must select a node to render first!").toStdString() );
    } else {
        std::list<AppInstance::RenderWork> workList;

        for (std::list<boost::shared_ptr<NodeGui> >::const_iterator it = selectedNodes.begin();
             it != selectedNodes.end(); ++it) {
            Natron::EffectInstance* effect = (*it)->getNode()->getLiveInstance();
            assert(effect);
            if (effect->isWriter()) {
                if (!effect->areKnobsFrozen()) {
                    //if ((*it)->getNode()->is)
                    ///if the node is a writer, just use it to render!
                    AppInstance::RenderWork w;
                    w.writer = dynamic_cast<Natron::OutputEffectInstance*>(effect);
                    assert(w.writer);
                    w.firstFrame = INT_MIN;
                    w.lastFrame = INT_MAX;
                    workList.push_back(w);
                }
            } else {
                if (selectedNodes.size() == 1) {
                    ///create a node and connect it to the node and use it to render
                    boost::shared_ptr<Natron::Node> writer = createWriter();
                    if (writer) {
                        AppInstance::RenderWork w;
                        w.writer = dynamic_cast<Natron::OutputEffectInstance*>( writer->getLiveInstance() );
                        assert(w.writer);
                        w.firstFrame = INT_MIN;
                        w.lastFrame = INT_MAX;
                        workList.push_back(w);
                    }
                }
            }
        }
        _imp->_appInstance->startWritersRendering(workList);
    }
}

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
Gui::startProgress(KnobHolder* effect,
                   const std::string & message,
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
Gui::endProgress(KnobHolder* effect)
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
            qDebug() << isNamed->getScriptName_mt_safe().c_str() <<  " called progressUpdate but didn't called startProgress first.";
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

void
Gui::keyPressEvent(QKeyEvent* e)
{
    QWidget* w = qApp->widgetAt( QCursor::pos() );

    if ( w && ( w->objectName() == QString("SettingsPanel") ) && (e->key() == Qt::Key_Escape) ) {
        RightClickableWidget* panel = dynamic_cast<RightClickableWidget*>(w);
        assert(panel);
        panel->getPanel()->closePanel();
    }

    Qt::Key key = (Qt::Key)e->key();
    Qt::KeyboardModifiers modifiers = e->modifiers();

    if ( isKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevious, modifiers, key) ) {
        if ( getNodeGraph()->getLastSelectedViewer() ) {
            getNodeGraph()->getLastSelectedViewer()->previousFrame();
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
    } else {
        QMainWindow::keyPressEvent(e);
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

FloatingWidget::FloatingWidget(Gui* gui,
                               QWidget* parent)
    : QWidget(parent, Qt::Tool) // use Qt::Tool instead of Qt::Window to get a minimal titlebar
    , SerializableWindow()
    , _embeddedWidget(0)
    , _scrollArea(0)
    , _layout(0)
    , _gui(gui)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    _layout = new QVBoxLayout(this);
    _layout->setContentsMargins(0, 0, 0, 0);
    _scrollArea = new QScrollArea(this);
    _layout->addWidget(_scrollArea);
    _scrollArea->setWidgetResizable(true);
}

static void
closeWidgetRecursively(QWidget* w)
{
    Splitter* isSplitter = dynamic_cast<Splitter*>(w);
    TabWidget* isTab = dynamic_cast<TabWidget*>(w);

    if (!isSplitter && !isTab) {
        return;
    }

    if (isTab) {
        isTab->closePane();
    } else {
        assert(isSplitter);
        for (int i = 0; i < isSplitter->count(); ++i) {
            closeWidgetRecursively( isSplitter->widget(i) );
        }
    }
}

FloatingWidget::~FloatingWidget()
{
    if (_embeddedWidget) {
        closeWidgetRecursively(_embeddedWidget);
    }
}

void
FloatingWidget::setWidget(QWidget* w)
{
    QSize widgetSize = w->size();

    assert(w);
    if (_embeddedWidget) {
        return;
    }
    _embeddedWidget = w;
    _scrollArea->setWidget(w);
    w->setVisible(true);
    w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QDesktopWidget* dw = qApp->desktop();
    assert(dw);
    QRect geom = dw->screenGeometry();
    widgetSize.setWidth( std::min( widgetSize.width(), geom.width() ) );
    widgetSize.setHeight( std::min( widgetSize.height(), geom.height() ) );
    resize(widgetSize);
    show();
}

void
FloatingWidget::removeEmbeddedWidget()
{
    if (!_embeddedWidget) {
        return;
    }
    //_scrollArea->setViewport(0);
    _embeddedWidget->setParent(NULL);
    _embeddedWidget = 0;
    // _embeddedWidget->setVisible(false);
    hide();
}

void
FloatingWidget::moveEvent(QMoveEvent* e)
{
    QWidget::moveEvent(e);
    QPoint p = pos();

    setMtSafePosition( p.x(), p.y() );
}

void
FloatingWidget::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);

    setMtSafeWindowSize( width(), height() );
}

void
FloatingWidget::closeEvent(QCloseEvent* e)
{
    Q_EMIT closed();

    closeWidgetRecursively(_embeddedWidget);
    removeEmbeddedWidget();
    _gui->unregisterFloatingWindow(this);
    QWidget::closeEvent(e);
}

void
Gui::getNodesEntitledForOverlays(std::list<boost::shared_ptr<Natron::Node> > & nodes) const
{
    int layoutItemsCount = _imp->_layoutPropertiesBin->count();

    for (int i = 0; i < layoutItemsCount; ++i) {
        QLayoutItem* item = _imp->_layoutPropertiesBin->itemAt(i);
        NodeSettingsPanel* panel = dynamic_cast<NodeSettingsPanel*>( item->widget() );
        if (panel) {
            boost::shared_ptr<NodeGui> node = panel->getNode();
            boost::shared_ptr<Natron::Node> internalNode = node->getNode();
            if (node && internalNode) {
                boost::shared_ptr<MultiInstancePanel> multiInstance = node->getMultiInstancePanel();
                if (multiInstance) {
//                    const std::list< std::pair<boost::weak_ptr<Natron::Node>,bool > >& instances = multiInstance->getInstances();
//                    for (std::list< std::pair<boost::weak_ptr<Natron::Node>,bool > >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
//                        NodePtr instance = it->first.lock();
//                        if (node->isSettingsPanelVisible() &&
//                            !node->isSettingsPanelMinimized() &&
//                            instance->isActivated() &&
//                            instance->hasOverlay() &&
//                            it->second &&
//                            !instance->isNodeDisabled()) {
//                            nodes.push_back(instance);
//                        }
//                    }
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
    QMutexLocker k(&_imp->_viewerTabsMutex);

    for (std::list<ViewerTab*>::const_iterator it = _imp->_viewerTabs.begin(); it != _imp->_viewerTabs.end(); ++it) {
        if ( (*it)->isVisible() ) {
            (*it)->getInternalNode()->renderCurrentFrame(false);
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
Gui::onNextTabTriggered()
{
    TabWidget* t = getLastEnteredTabWidget();

    if (t) {
        t->moveToNextTab();
    }
}

void
Gui::onCloseTabTriggered()
{
    TabWidget* t = getLastEnteredTabWidget();

    if (t) {
        t->closeCurrentWidget();
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

QAction*
GuiPrivate::findActionRecursive(int i,
                                QWidget* widget,
                                const QStringList & grouping)
{
    assert( i < grouping.size() );
    QList<QAction*> actions = widget->actions();
    for (QList<QAction*>::iterator it = actions.begin(); it != actions.end(); ++it) {
        if ( (*it)->text() == grouping[i] ) {
            if (i == grouping.size() - 1) {
                return *it;
            } else {
                QMenu* menu = (*it)->menu();
                if (menu) {
                    return findActionRecursive(i + 1, menu, grouping);
                } else {
                    ///Error: specified the name of an already existing action
                    return 0;
                }
            }
        }
    }
    ///Create the entry
    if (i < grouping.size() - 1) {
        Menu* menu = new Menu(widget);
        menu->setTitle(grouping[i]);
        QMenu* isMenu = dynamic_cast<QMenu*>(widget);
        QMenuBar* isMenuBar = dynamic_cast<QMenuBar*>(widget);
        if (isMenu) {
            isMenu->addAction( menu->menuAction() );
        } else if (isMenuBar) {
            isMenuBar->addAction( menu->menuAction() );
        }

        return findActionRecursive(i + 1, menu, grouping);
    } else {
        ActionWithShortcut* action = new ActionWithShortcut(kShortcutGroupGlobal, grouping[i], grouping[i], widget);
        QObject::connect( action, SIGNAL( triggered() ), _gui, SLOT( onUserCommandTriggered() ) );
        QMenu* isMenu = dynamic_cast<QMenu*>(widget);
        if (isMenu) {
            isMenu->addAction(action);
        }

        return action;
    }

    return 0;
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
