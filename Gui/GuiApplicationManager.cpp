//  Natron
//
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

#include "GuiApplicationManager.h"

///gui
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QPixmapCache>
#include <QBitmap>
#include <QImage>
#include <QApplication>
#include <QFontDatabase>
#include <QIcon>
#include <QFileOpenEvent>

CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

//core
#include <QSettings>
#include <QDebug>
#include <QMetaType>

//engine
#include "Global/Enums.h"
#include "Engine/KnobSerialization.h"
#include "Engine/Plugin.h"
#include "Engine/LibraryBinary.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include <SequenceParsing.h>

//gui
#include "Gui/Gui.h"
#include "Gui/SplashScreen.h"
#include "Gui/KnobGuiFactory.h"
#include "Gui/QtDecoder.h"
#include "Gui/QtEncoder.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/CurveWidget.h"
#include "Gui/ActionShortcuts.h"
CLANG_DIAG_OFF(mismatched-tags)
GCC_DIAG_OFF(unused-parameter)
#include "NatronGui/natrongui_python.h"
CLANG_DIAG_ON(mismatched-tags)
GCC_DIAG_ON(unused-parameter)

/**
 * @macro Registers a keybind to the application.
 * @param group The name of the group under which the shortcut should be (e.g: Global, Viewer,NodeGraph...)
 * @param id The ID of the shortcut within the group so that we can find it later. The shorter the better, it isn't visible by the user.
 * @param description The label of the shortcut and the action visible in the GUI. This is what describes the action and what is visible in
 * the application's menus.
 * @param modifiers The modifiers of the keybind. This is of type Qt::KeyboardModifiers. Use Qt::NoModifier to indicate no modifier should
 * be held.
 * @param symbol The key symbol. This is of type Qt::Key. Use (Qt::Key)0 to indicate there's no keybind for this action.
 *
 * Note: Even actions that don't have a default shortcut should be registered this way so that the user can add its own shortcut
 * on them later. If you don't register an action with that macro, its shortcut won't be editable.
 * To register an action without a keybind use Qt::NoModifier and (Qt::Key)0
 **/
#define registerKeybind(group,id,description, modifiers,symbol) ( _imp->addKeybind(group,id,description, modifiers,symbol) )

/**
 * @brief Performs the same that the registerKeybind macro, except that it takes a QKeySequence::StandardKey in parameter.
 * This way the keybind will be standard and will adapt bery well across all platforms supporting the standard.
 **/
#define registerStandardKeybind(group,id,description,key) ( _imp->addStandardKeybind(group,id,description,key) )

/**
 * @brief Performs the same that the registerKeybind macro, except that it works for shortcuts using mouse buttons instead of key symbols.
 * Generally you don't want this kind of interaction to be editable because it might break compatibility with non 3-button mouses.
 * Also the shortcut editor of Natron doesn't support editing mouse buttons.
 * In the end a mouse shortcut will be NON editable. The reason why you should call this is just to show the shortcut in the shortcut view
 * so that the user is aware it exists, but he/she will not be able to modify it.
 **/
#define registerMouseShortcut(group,id,description, modifiers,button) ( _imp->addMouseShortcut(group,id,description, modifiers,button) )

//Increment this when making change to default shortcuts or changes that would break expected default shortcuts
//in a way. This way the user will get prompted to restore default shortcuts on next launch
#define NATRON_SHORTCUTS_DEFAULT_VERSION 4

using namespace Natron;

struct KnobsClipBoard
{
    std::list<Variant> values; //< values
    std::list< boost::shared_ptr<Curve> > curves; //< animation
    std::list< boost::shared_ptr<Curve> > parametricCurves; //< for parametric knobs
    std::map<int,std::string> stringAnimation; //< for animating string knobs
    bool isEmpty; //< is the clipboard empty
    bool copyAnimation; //< should we copy all the animation or not
    
    std::string appID;
    std::string nodeFullyQualifiedName;
    std::string paramName;
};



struct GuiApplicationManagerPrivate
{
    GuiApplicationManager* _publicInterface;
    std::list<boost::shared_ptr<PluginGroupNode> > _topLevelToolButtons;
    boost::scoped_ptr<KnobsClipBoard> _knobsClipBoard;
    boost::scoped_ptr<KnobGuiFactory> _knobGuiFactory;
    QCursor* _colorPickerCursor;
    SplashScreen* _splashScreen;

    ///We store here the file open request that was made on startup but that
    ///we couldn't handle at that time
    QString _openFileRequest;
    AppShortcuts _actionShortcuts;

    bool _shortcutsChangedVersion;
    
    QString _fontFamily;
    int _fontSize;
    
    NodeClipBoard _nodeCB;
    
    std::list<PythonUserCommand> pythonCommands;
    
    GuiApplicationManagerPrivate(GuiApplicationManager* publicInterface)
        :   _publicInterface(publicInterface)
    , _topLevelToolButtons()
    , _knobsClipBoard(new KnobsClipBoard)
    , _knobGuiFactory( new KnobGuiFactory() )
    , _colorPickerCursor(NULL)
    , _splashScreen(NULL)
    , _openFileRequest()
    , _actionShortcuts()
    , _shortcutsChangedVersion(false)
    , _fontFamily()
    , _fontSize(0)
    , _nodeCB()
    , pythonCommands()
    {
    }

    void createColorPickerCursor();
    
    void removePluginToolButtonInternal(const boost::shared_ptr<PluginGroupNode>& n,const QStringList& grouping);
    void removePluginToolButton(const QStringList& grouping);

    void addStandardKeybind(const QString & grouping,const QString & id,
                            const QString & description,QKeySequence::StandardKey key);

    void addKeybind(const QString & grouping,const QString & id,
                    const QString & description,
                    const Qt::KeyboardModifiers & modifiers,Qt::Key symbol);
    
    void removeKeybind(const QString& grouping,const QString& id);

    void addMouseShortcut(const QString & grouping,const QString & id,
                          const QString & description,
                          const Qt::KeyboardModifiers & modifiers,Qt::MouseButton button);
    
    boost::shared_ptr<PluginGroupNode>  findPluginToolButtonInternal(const boost::shared_ptr<PluginGroupNode>& parent,
                                                                     const QStringList & grouping,
                                                                     const QString & name,
                                                                     const QString & iconPath,
                                                                     int major,
                                                                     int minor);
};

GuiApplicationManager::GuiApplicationManager()
    : AppManager()
      , _imp(new GuiApplicationManagerPrivate(this))
{
}

GuiApplicationManager::~GuiApplicationManager()
{

    delete _imp->_colorPickerCursor;
    for (AppShortcuts::iterator it = _imp->_actionShortcuts.begin(); it != _imp->_actionShortcuts.end(); ++it) {
        for (GroupShortcuts::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            delete it2->second;
        }
    }
}

void
GuiApplicationManager::getIcon(Natron::PixmapEnum e,
                               QPixmap* pix) const
{
    int iconSet = appPTR->getCurrentSettings()->getIconsBlackAndWhite() ? 2 : 3;
    QString iconSetStr = QString::number(iconSet);
    
    if ( !QPixmapCache::find(QString::number(e),pix) ) {
        QImage img;
        switch (e) {
            case NATRON_PIXMAP_PLAYER_PREVIOUS:
                img.load(NATRON_IMAGES_PATH "back1.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_FIRST_FRAME:
                img.load(NATRON_IMAGES_PATH "firstFrame.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_NEXT:
                img.load(NATRON_IMAGES_PATH "forward1.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_LAST_FRAME:
                img.load(NATRON_IMAGES_PATH "lastFrame.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_NEXT_INCR:
                img.load(NATRON_IMAGES_PATH "nextIncr.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_NEXT_KEY:
                img.load(NATRON_IMAGES_PATH "nextKF.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_PREVIOUS_INCR:
                img.load(NATRON_IMAGES_PATH "previousIncr.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_PREVIOUS_KEY:
                img.load(NATRON_IMAGES_PATH "prevKF.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ADD_KEYFRAME:
                img.load(NATRON_IMAGES_PATH "addKF.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_REMOVE_KEYFRAME:
                img.load(NATRON_IMAGES_PATH "removeKF.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_REWIND_ENABLED:
                img.load(NATRON_IMAGES_PATH "rewind_enabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_REWIND_DISABLED:
                img.load(NATRON_IMAGES_PATH "rewind.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_PLAY_ENABLED:
                img.load(NATRON_IMAGES_PATH "play_enabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_PLAY_DISABLED:
                img.load(NATRON_IMAGES_PATH "play.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_STOP:
                img.load(NATRON_IMAGES_PATH "stop.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_LOOP_MODE:
                img.load(NATRON_IMAGES_PATH "loopmode.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_BOUNCE:
                img.load(NATRON_IMAGES_PATH "bounce.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PLAYER_PLAY_ONCE:
                img.load(NATRON_IMAGES_PATH "playOnce.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MAXIMIZE_WIDGET:
                img.load(NATRON_IMAGES_PATH "maximize.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MINIMIZE_WIDGET:
                img.load(NATRON_IMAGES_PATH "minimize.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_CLOSE_WIDGET:
                img.load(NATRON_IMAGES_PATH "close.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_CLOSE_PANEL:
                img.load(NATRON_IMAGES_PATH "closePanel.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_HIDE_UNMODIFIED:
                img.load(NATRON_IMAGES_PATH "hide_unmodified.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_UNHIDE_UNMODIFIED:
                img.load(NATRON_IMAGES_PATH "unhide_unmodified.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_HELP_WIDGET:
                img.load(NATRON_IMAGES_PATH "help.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_GROUPBOX_FOLDED:
                img.load(NATRON_IMAGES_PATH "groupbox_folded.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_GROUPBOX_UNFOLDED:
                img.load(NATRON_IMAGES_PATH "groupbox_unfolded.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_UNDO:
                img.load(NATRON_IMAGES_PATH "undo.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_REDO:
                img.load(NATRON_IMAGES_PATH "redo.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_UNDO_GRAYSCALE:
                img.load(NATRON_IMAGES_PATH "undo_grayscale.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_REDO_GRAYSCALE:
                img.load(NATRON_IMAGES_PATH "redo_grayscale.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_RESTORE_DEFAULTS_ENABLED:
                img.load(NATRON_IMAGES_PATH "restoreDefaultEnabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_RESTORE_DEFAULTS_DISABLED:
                img.load(NATRON_IMAGES_PATH "restoreDefaultDisabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON:
                img.load(NATRON_IMAGES_PATH "layout.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_TAB_WIDGET_LAYOUT_BUTTON_ANCHOR:
                img.load(NATRON_IMAGES_PATH "layout_anchor.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_TAB_WIDGET_SPLIT_HORIZONTALLY:
                img.load(NATRON_IMAGES_PATH "splitHorizontally.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_TAB_WIDGET_SPLIT_VERTICALLY:
                img.load(NATRON_IMAGES_PATH "splitVertically.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_CENTER:
                img.load(NATRON_IMAGES_PATH "centerViewer.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_ENABLED:
                img.load(NATRON_IMAGES_PATH "cliptoprojectEnabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_CLIP_TO_PROJECT_DISABLED:
                img.load(NATRON_IMAGES_PATH "cliptoprojectDisabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_REFRESH:
                img.load(NATRON_IMAGES_PATH "refresh.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_REFRESH_ACTIVE:
                img.load(NATRON_IMAGES_PATH "refreshActive.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_ROI_ENABLED:
                img.load(NATRON_IMAGES_PATH "viewer_roiEnabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_ROI_DISABLED:
                img.load(NATRON_IMAGES_PATH "viewer_roiDisabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_RENDER_SCALE:
                img.load(NATRON_IMAGES_PATH "renderScale.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_RENDER_SCALE_CHECKED:
                img.load(NATRON_IMAGES_PATH "renderScale_checked.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_OPEN_FILE:
                img.load(NATRON_IMAGES_PATH "open-file.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_COLORWHEEL:
                img.load(NATRON_IMAGES_PATH "colorwheel.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_OVERLAY:
                img.load(NATRON_IMAGES_PATH "colorwheel_overlay.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ROTO_MERGE:
                img.load(NATRON_IMAGES_PATH "roto_merge.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_COLOR_PICKER:
                img.load(NATRON_IMAGES_PATH "color_picker.png");
                *pix = QPixmap::fromImage(img);
                break;


            case NATRON_PIXMAP_IO_GROUPING:
                img.load(NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/image_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_3D_GROUPING:
                img.load(NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/3D_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_CHANNEL_GROUPING:
                img.load(NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/channel_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_GROUPING:
                img.load(NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/merge_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_COLOR_GROUPING:
                img.load(NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/color_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_TRANSFORM_GROUPING:
                img.load(NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/transform_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_DEEP_GROUPING:
                img.load(NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/deep_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_FILTER_GROUPING:
                img.load(NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/filter_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MULTIVIEW_GROUPING:
                img.load(NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/multiview_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MISC_GROUPING:
                img.load(NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/misc_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_OTHER_PLUGINS:
                img.load(NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/other_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_TOOLSETS_GROUPING:
                img.load(NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/toolsets_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_KEYER_GROUPING:
                img.load(NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/keyer_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_TIME_GROUPING:
                img.load(NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/time_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_PAINT_GROUPING:
                img.load(NATRON_IMAGES_PATH "GroupingIcons/Set" + iconSetStr + "/paint_grouping_" + iconSetStr + ".png");
                *pix = QPixmap::fromImage(img);
                break;


            case NATRON_PIXMAP_OPEN_EFFECTS_GROUPING:
                img.load(NATRON_IMAGES_PATH "openeffects.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_COMBOBOX:
                img.load(NATRON_IMAGES_PATH "combobox.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_COMBOBOX_PRESSED:
                img.load(NATRON_IMAGES_PATH "pressed_combobox.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_READ_IMAGE:
                img.load(NATRON_IMAGES_PATH "readImage.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_WRITE_IMAGE:
                img.load(NATRON_IMAGES_PATH "writeImage.png");
                *pix = QPixmap::fromImage(img);
                break;

            case NATRON_PIXMAP_APP_ICON:
                img.load(NATRON_APPLICATION_ICON_PATH);
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_INVERTED:
                img.load(NATRON_IMAGES_PATH "inverted.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_UNINVERTED:
                img.load(NATRON_IMAGES_PATH "uninverted.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VISIBLE:
                img.load(NATRON_IMAGES_PATH "visible.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_UNVISIBLE:
                img.load(NATRON_IMAGES_PATH "unvisible.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_LOCKED:
                img.load(NATRON_IMAGES_PATH "locked.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_UNLOCKED:
                img.load(NATRON_IMAGES_PATH "unlocked.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_LAYER:
                img.load(NATRON_IMAGES_PATH "layer.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_BEZIER:
                img.load(NATRON_IMAGES_PATH "bezier.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_CURVE:
                img.load(NATRON_IMAGES_PATH "curve.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_BEZIER_32:
                img.load(NATRON_IMAGES_PATH "bezier32.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ELLIPSE:
                img.load(NATRON_IMAGES_PATH "ellipse.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_RECTANGLE:
                img.load(NATRON_IMAGES_PATH "rectangle.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ADD_POINTS:
                img.load(NATRON_IMAGES_PATH "addPoints.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_REMOVE_POINTS:
                img.load(NATRON_IMAGES_PATH "removePoints.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_REMOVE_FEATHER:
                img.load(NATRON_IMAGES_PATH "removeFeather.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_CUSP_POINTS:
                img.load(NATRON_IMAGES_PATH "cuspPoints.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_SMOOTH_POINTS:
                img.load(NATRON_IMAGES_PATH "smoothPoints.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_OPEN_CLOSE_CURVE:
                img.load(NATRON_IMAGES_PATH "openCloseCurve.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_SELECT_ALL:
                img.load(NATRON_IMAGES_PATH "cursor.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_SELECT_POINTS:
                img.load(NATRON_IMAGES_PATH "selectPoints.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_SELECT_FEATHER:
                img.load(NATRON_IMAGES_PATH "selectFeather.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_SELECT_CURVES:
                img.load(NATRON_IMAGES_PATH "selectCurves.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_AUTO_KEYING_ENABLED:
                img.load(NATRON_IMAGES_PATH "autoKeyingEnabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_AUTO_KEYING_DISABLED:
                img.load(NATRON_IMAGES_PATH "autoKeyingDisabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_STICKY_SELECTION_ENABLED:
                img.load(NATRON_IMAGES_PATH "stickySelectionEnabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_STICKY_SELECTION_DISABLED:
                img.load(NATRON_IMAGES_PATH "stickySelectionDisabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_FEATHER_LINK_ENABLED:
                img.load(NATRON_IMAGES_PATH "featherLinkEnabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_FEATHER_LINK_DISABLED:
                img.load(NATRON_IMAGES_PATH "featherLinkDisabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_FEATHER_VISIBLE:
                img.load(NATRON_IMAGES_PATH "featherEnabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_FEATHER_UNVISIBLE:
                img.load(NATRON_IMAGES_PATH "featherDisabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_RIPPLE_EDIT_ENABLED:
                img.load(NATRON_IMAGES_PATH "rippleEditEnabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_RIPPLE_EDIT_DISABLED:
                img.load(NATRON_IMAGES_PATH "rippleEditDisabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_BOLD_CHECKED:
                img.load(NATRON_IMAGES_PATH "bold_checked.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_BOLD_UNCHECKED:
                img.load(NATRON_IMAGES_PATH "bold_unchecked.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ITALIC_UNCHECKED:
                img.load(NATRON_IMAGES_PATH "italic_unchecked.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ITALIC_CHECKED:
                img.load(NATRON_IMAGES_PATH "italic_checked.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_CLEAR_ALL_ANIMATION:
                img.load(NATRON_IMAGES_PATH "clearAnimation.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_CLEAR_BACKWARD_ANIMATION:
                img.load(NATRON_IMAGES_PATH "clearAnimationBw.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_CLEAR_FORWARD_ANIMATION:
                img.load(NATRON_IMAGES_PATH "clearAnimationFw.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_UPDATE_VIEWER_ENABLED:
                img.load(NATRON_IMAGES_PATH "updateViewerEnabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_UPDATE_VIEWER_DISABLED:
                img.load(NATRON_IMAGES_PATH "updateViewerDisabled.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_SETTINGS:
                img.load(NATRON_IMAGES_PATH "settings.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_FREEZE_ENABLED:
                img.load(NATRON_IMAGES_PATH "turbo_on.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_FREEZE_DISABLED:
                img.load(NATRON_IMAGES_PATH "turbo_off.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_ICON:
                img.load(NATRON_IMAGES_PATH "viewer_icon.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_CHECKERBOARD_ENABLED:
                img.load(NATRON_IMAGES_PATH "checkerboard_on.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_CHECKERBOARD_DISABLED:
                img.load(NATRON_IMAGES_PATH "checkerboard_off.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_ZEBRA_ENABLED:
                img.load(NATRON_IMAGES_PATH "zebra_on.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_ZEBRA_DISABLED:
                img.load(NATRON_IMAGES_PATH "zebra_off.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_GAMMA_ENABLED:
                img.load(NATRON_IMAGES_PATH "gammaON.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_GAMMA_DISABLED:
                img.load(NATRON_IMAGES_PATH "gammaOFF.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_GAIN_ENABLED:
                img.load(NATRON_IMAGES_PATH "expoON.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_VIEWER_GAIN_DISABLED:
                img.load(NATRON_IMAGES_PATH "expoOFF.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ADD_TRACK:
                img.load(NATRON_IMAGES_PATH "addTrack.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_SCRIPT_CLEAR_OUTPUT:
                img.load(NATRON_IMAGES_PATH "clearOutput.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_SCRIPT_EXEC_SCRIPT:
                img.load(NATRON_IMAGES_PATH "execScript.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_SCRIPT_LOAD_EXEC_SCRIPT:
                img.load(NATRON_IMAGES_PATH "loadAndExecScript.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_SCRIPT_LOAD_SCRIPT:
                img.load(NATRON_IMAGES_PATH "loadScript.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_SCRIPT_NEXT_SCRIPT:
                img.load(NATRON_IMAGES_PATH "nextScript.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_SCRIPT_OUTPUT_PANE_ACTIVATED:
                img.load(NATRON_IMAGES_PATH "outputPanelActivated.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_SCRIPT_OUTPUT_PANE_DEACTIVATED:
                img.load(NATRON_IMAGES_PATH "outputPanelDeactivated.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_SCRIPT_PREVIOUS_SCRIPT:
                img.load(NATRON_IMAGES_PATH "previousScript.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_SCRIPT_SAVE_SCRIPT:
                img.load(NATRON_IMAGES_PATH "saveScript.png");
                *pix = QPixmap::fromImage(img);
                break;

            case NATRON_PIXMAP_MERGE_ATOP:
                img.load(NATRON_IMAGES_PATH "merge_atop.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_AVERAGE:
                img.load(NATRON_IMAGES_PATH "merge_average.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_COLOR_BURN:
                img.load(NATRON_IMAGES_PATH "merge_color_burn.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_COLOR_DODGE:
                img.load(NATRON_IMAGES_PATH "merge_color_dodge.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_CONJOINT_OVER:
                img.load(NATRON_IMAGES_PATH "merge_conjoint_over.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_COPY:
                img.load(NATRON_IMAGES_PATH "merge_copy.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_DIFFERENCE:
                img.load(NATRON_IMAGES_PATH "merge_difference.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_DISJOINT_OVER:
                img.load(NATRON_IMAGES_PATH "merge_disjoint_over.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_DIVIDE:
                img.load(NATRON_IMAGES_PATH "merge_divide.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_EXCLUSION:
                img.load(NATRON_IMAGES_PATH "merge_exclusion.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_FREEZE:
                img.load(NATRON_IMAGES_PATH "merge_freeze.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_FROM:
                img.load(NATRON_IMAGES_PATH "merge_from.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_GEOMETRIC:
                img.load(NATRON_IMAGES_PATH "merge_geometric.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_HARD_LIGHT:
                img.load(NATRON_IMAGES_PATH "merge_hard_light.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_HYPOT:
                img.load(NATRON_IMAGES_PATH "merge_hypot.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_IN:
                img.load(NATRON_IMAGES_PATH "merge_in.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_INTERPOLATED:
                img.load(NATRON_IMAGES_PATH "merge_interpolated.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_MASK:
                img.load(NATRON_IMAGES_PATH "merge_mask.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_MATTE:
                img.load(NATRON_IMAGES_PATH "merge_matte.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_MAX:
                img.load(NATRON_IMAGES_PATH "merge_max.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_MIN:
                img.load(NATRON_IMAGES_PATH "merge_min.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_MINUS:
                img.load(NATRON_IMAGES_PATH "merge_minus.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_MULTIPLY:
                img.load(NATRON_IMAGES_PATH "merge_multiply.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_OUT:
                img.load(NATRON_IMAGES_PATH "merge_out.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_OVER:
                img.load(NATRON_IMAGES_PATH "merge_over.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_OVERLAY:
                img.load(NATRON_IMAGES_PATH "merge_overlay.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_PINLIGHT:
                img.load(NATRON_IMAGES_PATH "merge_pinlight.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_PLUS:
                img.load(NATRON_IMAGES_PATH "merge_plus.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_REFLECT:
                img.load(NATRON_IMAGES_PATH "merge_reflect.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_SCREEN:
                img.load(NATRON_IMAGES_PATH "merge_screen.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_SOFT_LIGHT:
                img.load(NATRON_IMAGES_PATH "merge_soft_light.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_STENCIL:
                img.load(NATRON_IMAGES_PATH "merge_stencil.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_UNDER:
                img.load(NATRON_IMAGES_PATH "merge_under.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_MERGE_XOR:
                img.load(NATRON_IMAGES_PATH "merge_xor.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_LINK_CURSOR:
                img.load(NATRON_IMAGES_PATH "linkCursor.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ENTER_GROUP:
                img.load(NATRON_IMAGES_PATH "enter_group.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ROTOPAINT_BUILDUP_ENABLED:
                img.load(NATRON_IMAGES_PATH "rotopaint_buildup_on.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ROTOPAINT_BUILDUP_DISABLED:
                img.load(NATRON_IMAGES_PATH "rotopaint_buildup_off.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ROTOPAINT_BLUR:
                img.load(NATRON_IMAGES_PATH "rotopaint_blur.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ROTOPAINT_BURN:
                img.load(NATRON_IMAGES_PATH "rotopaint_burn.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ROTOPAINT_CLONE:
                img.load(NATRON_IMAGES_PATH "rotopaint_clone.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ROTOPAINT_DODGE:
                img.load(NATRON_IMAGES_PATH "rotopaint_dodge.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ROTOPAINT_ERASER:
                img.load(NATRON_IMAGES_PATH "rotopaint_eraser.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ROTOPAINT_PRESSURE_ENABLED:
                img.load(NATRON_IMAGES_PATH "rotopaint_pressure_on.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ROTOPAINT_PRESSURE_DISABLED:
                img.load(NATRON_IMAGES_PATH "rotopaint_pressure_off.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ROTOPAINT_REVEAL:
                img.load(NATRON_IMAGES_PATH "rotopaint_reveal.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ROTOPAINT_SHARPEN:
                img.load(NATRON_IMAGES_PATH "rotopaint_sharpen.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ROTOPAINT_SMEAR:
                img.load(NATRON_IMAGES_PATH "rotopaint_smear.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_ROTOPAINT_SOLID:
                img.load(NATRON_IMAGES_PATH "rotopaint_solid.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_INTERP_LINEAR:
                img.load(NATRON_IMAGES_PATH "interp_linear.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_INTERP_CURVE:
                img.load(NATRON_IMAGES_PATH "interp_curve.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_INTERP_CONSTANT:
                img.load(NATRON_IMAGES_PATH "interp_constant.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_INTERP_BREAK:
                img.load(NATRON_IMAGES_PATH "interp_break.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_INTERP_CURVE_C:
                img.load(NATRON_IMAGES_PATH "interp_curve_c.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_INTERP_CURVE_H:
                img.load(NATRON_IMAGES_PATH "interp_curve_h.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_INTERP_CURVE_R:
                img.load(NATRON_IMAGES_PATH "interp_curve_r.png");
                *pix = QPixmap::fromImage(img);
                break;
            case NATRON_PIXMAP_INTERP_CURVE_Z:
                img.load(NATRON_IMAGES_PATH "interp_curve_z.png");
                *pix = QPixmap::fromImage(img);
                break;

            default:
                assert(!"Missing image.");
        } // switch
        QPixmapCache::insert(QString::number(e),*pix);
    }
} // getIcon

const std::list<boost::shared_ptr<PluginGroupNode> >&
GuiApplicationManager::getTopLevelPluginsToolButtons() const
{
    return _imp->_topLevelToolButtons;
}

const QCursor &
GuiApplicationManager::getColorPickerCursor() const
{
    return *(_imp->_colorPickerCursor);
}

void
GuiApplicationManager::initGui()
{
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);

    //load custom fonts
    QString fontResource = QString(":/Resources/Fonts/%1.ttf");
    QStringList fontFilenames;
    fontFilenames << fontResource.arg("DroidSans");
    fontFilenames << fontResource.arg("DroidSans-Bold");
    Q_FOREACH (QString fontFilename, fontFilenames) {
        int fontID = QFontDatabase::addApplicationFont(fontFilename);
        qDebug() << "fontID=" << fontID << "families=" << QFontDatabase::applicationFontFamilies(fontID);
    }
    QString fontFamily(NATRON_FONT);
    int fontSize = NATRON_FONT_SIZE_11;
    
    
    ///Do not load old font stored in the setting "systemFont" on Natron < 2 because it might contain a crappy font.
    if (settings.contains(kQSettingsSoftwareMajorVersionSettingName) && settings.value(kQSettingsSoftwareMajorVersionSettingName).toInt() >= 2) {
        if (settings.contains("systemFont")) {
            fontFamily = settings.value("systemFont").toString();
        }
    }
    
    
    if (settings.contains("fontSize")) {
        fontSize = settings.value("fontSize").toInt();
    }
    //fontFamily = "Courier"; fontSize = 24; // for debugging purposes
    qDebug() << "Setting application font to " << fontFamily << fontSize;
    {
        QFontDatabase database;
        Q_FOREACH (const QString &family, database.families()) {
            if (family == fontFamily) {
                qDebug() << "... found" << fontFamily << "available styles:";
                Q_FOREACH (const QString &style, database.styles(family)) {
                    qDebug() << family << style;
                }
            }
        }
        QFont font(fontFamily, fontSize);
        if (!font.exactMatch()) {
            QFontInfo fi(font);
            qDebug() << "Not an exact match, got: " << fi.family() << fi.pointSize();
        }
        QApplication::setFont(font);
#ifdef Q_OS_MAC
        // https://bugreports.qt.io/browse/QTBUG-32789
        QFont::insertSubstitution(".Lucida Grande UI", fontFamily/*"Lucida Grande"*/);
        // https://bugreports.qt.io/browse/QTBUG-40833
        QFont::insertSubstitution(".Helvetica Neue DeskInterface", fontFamily/*"Helvetica Neue"*/);
        // there are lots of remaining bugs on Yosemite in 4.8.6, to be fixed in 4.8.7&
#endif
    }
    _imp->_fontFamily = fontFamily;
    _imp->_fontSize = fontSize;

    /*Display a splashscreen while we wait for the engine to load*/
    QString filename(NATRON_IMAGES_PATH "splashscreen.png");

    _imp->_splashScreen = new SplashScreen(filename);
    QCoreApplication::processEvents();
    QPixmap appIcPixmap;
    appPTR->getIcon(Natron::NATRON_PIXMAP_APP_ICON, &appIcPixmap);
    QIcon appIc(appIcPixmap);
    qApp->setWindowIcon(appIc);
    
    QFontDatabase db;
    QStringList families = db.families(QFontDatabase::Latin); // We need a Latin font for the UI
    std::vector<std::string> systemFonts(families.size());
    for (int i = 0; i < families.size(); ++i) {
        systemFonts[i] = families[i].toStdString();
    }
    getCurrentSettings()->populateSystemFonts(settings,systemFonts);


    _imp->createColorPickerCursor();
    _imp->_knobsClipBoard->isEmpty = true;
}

void
GuiApplicationManager::onPluginLoaded(Natron::Plugin* plugin)
{
    QString shortcutGrouping(kShortcutGroupNodes);
    const QStringList & groups = plugin->getGrouping();
    const QString & pluginID = plugin->getPluginID();
    const QString  pluginLabel = plugin->getLabelWithoutSuffix();
    const QString & pluginIconPath = plugin->getIconFilePath();
    const QString & groupIconPath = plugin->getGroupIconFilePath();

    QStringList groupingWithID = groups;
    groupingWithID.push_back(pluginID);
    boost::shared_ptr<PluginGroupNode> child = findPluginToolButtonOrCreate(groupingWithID,
                                                                            pluginLabel,
                                                                            groupIconPath,
                                                                            pluginIconPath,
                                                                            plugin->getMajorVersion(),
                                                                            plugin->getMinorVersion());
    for (int i = 0; i < groups.size(); ++i) {

        shortcutGrouping.push_back('/');
        shortcutGrouping.push_back(groups[i]);
    }
    
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
    Qt::Key symbol = (Qt::Key)0;
    bool hasShortcut = true;

    /*These are the plug-ins which have a default shortcut. Other plug-ins can have a user-assigned shortcut.*/
    if (pluginID == PLUGINID_OFX_TRANSFORM) {
        symbol = Qt::Key_T;
    } else if (pluginID == PLUGINID_OFX_ROTO) {
        symbol = Qt::Key_O;
    } else if (pluginID == PLUGINID_NATRON_ROTOPAINT) {
        symbol = Qt::Key_P;
    } else if (pluginID == PLUGINID_OFX_MERGE) {
        symbol = Qt::Key_M;
    } else if (pluginID == PLUGINID_OFX_GRADE) {
        symbol = Qt::Key_G;
    } else if (pluginID == PLUGINID_OFX_COLORCORRECT) {
        symbol = Qt::Key_C;
    } else if (pluginID == PLUGINID_OFX_BLURCIMG) {
        symbol = Qt::Key_B;
    } else if (pluginID == PLUGINID_NATRON_DOT) {
        symbol = Qt::Key_Period;
        modifiers |= Qt::ShiftModifier;
    } else {
        hasShortcut = false;
    }
    plugin->setHasShortcut(hasShortcut);
    
    if (Natron::isPluginCreatable(plugin->getPluginID().toStdString())) {
        _imp->addKeybind(shortcutGrouping, pluginID, pluginLabel, modifiers, symbol);
    }
    
}

void
GuiApplicationManager::ignorePlugin(Natron::Plugin* plugin)
{
    _imp->removePluginToolButton(plugin->getGrouping());
    _imp->removeKeybind(kShortcutGroupNodes, plugin->getPluginID());
}


void
GuiApplicationManagerPrivate::removePluginToolButtonInternal(const boost::shared_ptr<PluginGroupNode>& n,const QStringList& grouping)
{
    assert(grouping.size() > 0);
    
    const std::list<boost::shared_ptr<PluginGroupNode> >& children = n->getChildren();
    for (std::list<boost::shared_ptr<PluginGroupNode> >::const_iterator it = children.begin();
         it != children.end(); ++it) {
        if ((*it)->getID() == grouping[0]) {
            
            if (grouping.size() > 1) {
                QStringList newGrouping;
                for (int i = 1; i < grouping.size(); ++i) {
                    newGrouping.push_back(grouping[i]);
                }
                removePluginToolButtonInternal(*it, newGrouping);
                if ((*it)->getChildren().empty()) {
                    n->tryRemoveChild(it->get());
                }
            } else {
                n->tryRemoveChild(it->get());
            }
            break;
        }
    }
}
void
GuiApplicationManagerPrivate::removePluginToolButton(const QStringList& grouping)
{
    assert(grouping.size() > 0);
    
    for (std::list<boost::shared_ptr<PluginGroupNode> >::iterator it = _topLevelToolButtons.begin();
         it != _topLevelToolButtons.end(); ++it) {
        if ((*it)->getID() == grouping[0]) {
            
            if (grouping.size() > 1) {
                QStringList newGrouping;
                for (int i = 1; i < grouping.size(); ++i) {
                    newGrouping.push_back(grouping[i]);
                }
                removePluginToolButtonInternal(*it, newGrouping);
                if ((*it)->getChildren().empty()) {
                    _topLevelToolButtons.erase(it);
                }
            } else {
                _topLevelToolButtons.erase(it);
            }
            break;
        }
    }
}

boost::shared_ptr<PluginGroupNode>
GuiApplicationManagerPrivate::findPluginToolButtonInternal(const boost::shared_ptr<PluginGroupNode>& parent,
                                                           const QStringList & grouping,
                                                           const QString & name,
                                                           const QString & iconPath,
                                                           int major,
                                                           int minor)
{
    assert(grouping.size() > 0);
    
    const std::list<boost::shared_ptr<PluginGroupNode> >& children = parent->getChildren();
    
    for (std::list<boost::shared_ptr<PluginGroupNode> >::const_iterator it = children.begin(); it != children.end(); ++it) {
        if ((*it)->getID() == grouping[0]) {
            
            if (grouping.size() > 1) {
                QStringList newGrouping;
                for (int i = 1; i < grouping.size(); ++i) {
                    newGrouping.push_back(grouping[i]);
                }
                return findPluginToolButtonInternal(*it, newGrouping, name, iconPath, major,minor);
            }
            if (major == (*it)->getMajorVersion()) {
                return *it;
            } else {
                (*it)->setNotHighestMajorVersion(true);
            }
        }
    }
    boost::shared_ptr<PluginGroupNode> ret(new PluginGroupNode(grouping[0],grouping.size() == 1 ? name : grouping[0],iconPath,major,minor));
    parent->tryAddChild(ret);
    ret->setParent(parent);
    
    if (grouping.size() > 1) {
        QStringList newGrouping;
        for (int i = 1; i < grouping.size(); ++i) {
            newGrouping.push_back(grouping[i]);
        }
        return findPluginToolButtonInternal(ret, newGrouping, name, iconPath, major,minor);
    }
    return ret;
}

boost::shared_ptr<PluginGroupNode>
GuiApplicationManager::findPluginToolButtonOrCreate(const QStringList & grouping,
                                                    const QString & name,
                                                    const QString& groupIconPath,
                                                    const QString & iconPath,
                                                    int major,
                                                    int minor)
{
    assert(grouping.size() > 0);
    
    for (std::list<boost::shared_ptr<PluginGroupNode> >::iterator it = _imp->_topLevelToolButtons.begin(); it != _imp->_topLevelToolButtons.end(); ++it) {
        if ((*it)->getID() == grouping[0]) {
            
            if (grouping.size() > 1) {
                QStringList newGrouping;
                for (int i = 1; i < grouping.size(); ++i) {
                    newGrouping.push_back(grouping[i]);
                }
                return _imp->findPluginToolButtonInternal(*it, newGrouping, name, iconPath, major , minor);
            }
            if (major == (*it)->getMajorVersion()) {
                return *it;
            } else {
                (*it)->setNotHighestMajorVersion(true);
            }
        }
    }
    
    boost::shared_ptr<PluginGroupNode> ret(new PluginGroupNode(grouping[0],grouping.size() == 1 ? name : grouping[0],iconPath,major,minor));
    _imp->_topLevelToolButtons.push_back(ret);
    if (grouping.size() > 1) {
        ret->setIconPath(groupIconPath);
        QStringList newGrouping;
        for (int i = 1; i < grouping.size(); ++i) {
            newGrouping.push_back(grouping[i]);
        }
        return _imp->findPluginToolButtonInternal(ret, newGrouping, name, iconPath, major,minor);
    }
    return ret;
}

void
GuiApplicationManagerPrivate::createColorPickerCursor()
{
    QImage originalImage;
    originalImage.load(NATRON_IMAGES_PATH "color_picker.png");
    originalImage = originalImage.scaled(16, 16);
    QImage dstImage(32,32,QImage::Format_ARGB32);
    dstImage.fill(QColor(0,0,0,0));
    
    int oW = originalImage.width();
    int oH = originalImage.height();
    for (int y = 0; y < oH; ++y) {
        for (int x = 0; x < oW; ++x) {
            dstImage.setPixel(x + oW, y, originalImage.pixel(x, y));
        }
    }
    QPixmap pix = QPixmap::fromImage(dstImage);
    _colorPickerCursor = new QCursor(pix);
}

bool
GuiApplicationManager::isSplashcreenVisible() const
{
    return _imp->_splashScreen ? true : false;
}

void
GuiApplicationManager::hideSplashScreen()
{
    if (_imp->_splashScreen) {
        _imp->_splashScreen->hide();
        delete _imp->_splashScreen;
        _imp->_splashScreen = 0;
    }
}

bool
GuiApplicationManager::isClipBoardEmpty() const
{
    return _imp->_knobsClipBoard->isEmpty;
}

void
GuiApplicationManager::setKnobClipBoard(bool copyAnimation,
                                        const std::list<Variant> & values,
                                        const std::list<boost::shared_ptr<Curve> > & animation,
                                        const std::map<int,std::string> & stringAnimation,
                                        const std::list<boost::shared_ptr<Curve> > & parametricCurves,
                                        const std::string& appID,
                                        const std::string& nodeFullyQualifiedName,
                                        const std::string& paramName)
{
    _imp->_knobsClipBoard->copyAnimation = copyAnimation;
    _imp->_knobsClipBoard->isEmpty = false;
    _imp->_knobsClipBoard->values = values;
    _imp->_knobsClipBoard->curves = animation;
    _imp->_knobsClipBoard->stringAnimation = stringAnimation;
    _imp->_knobsClipBoard->parametricCurves = parametricCurves;
    _imp->_knobsClipBoard->appID = appID;
    _imp->_knobsClipBoard->nodeFullyQualifiedName = nodeFullyQualifiedName;
    _imp->_knobsClipBoard->paramName = paramName;
}

void
GuiApplicationManager::getKnobClipBoard(bool* copyAnimation,
                                        std::list<Variant>* values,
                                        std::list<boost::shared_ptr<Curve> >* animation,
                                        std::map<int,std::string>* stringAnimation,
                                        std::list<boost::shared_ptr<Curve> >* parametricCurves,
                                        std::string* appID,
                                        std::string* nodeFullyQualifiedName,
                                        std::string* paramName) const
{
    *copyAnimation = _imp->_knobsClipBoard->copyAnimation;
    *values = _imp->_knobsClipBoard->values;
    *animation = _imp->_knobsClipBoard->curves;
    *stringAnimation = _imp->_knobsClipBoard->stringAnimation;
    *parametricCurves = _imp->_knobsClipBoard->parametricCurves;
    *appID = _imp->_knobsClipBoard->appID;
    *nodeFullyQualifiedName = _imp->_knobsClipBoard->nodeFullyQualifiedName;
    *paramName = _imp->_knobsClipBoard->paramName;
}

void
GuiApplicationManager::updateAllRecentFileMenus()
{
    const std::map<int,AppInstanceRef> & instances = getAppInstances();

    for (std::map<int,AppInstanceRef>::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        GuiAppInstance* appInstance = dynamic_cast<GuiAppInstance*>(it->second.app);
        assert(appInstance);
        Gui* gui = appInstance->getGui();
        assert(gui);
        gui->updateRecentFileActions();
    }
}

void
GuiApplicationManager::setLoadingStatus(const QString & str)
{
    if ( isLoaded() ) {
        return;
    }
    if (_imp->_splashScreen) {
        _imp->_splashScreen->updateText(str);
    }
}

void
GuiApplicationManager::loadBuiltinNodePlugins(std::map<std::string,std::vector< std::pair<std::string,double> > >* readersMap,
                                              std::map<std::string,std::vector< std::pair<std::string,double> > >* writersMap)
{
    ////Use ReadQt and WriteQt only for debug versions of Natron.
    // these  are built-in nodes
    QStringList grouping;

    grouping.push_back(PLUGIN_GROUP_IMAGE); // Readers, Writers, and Generators are in the "Image" group in Nuke

# ifdef DEBUG
    {
        std::auto_ptr<QtReader> reader( dynamic_cast<QtReader*>( QtReader::BuildEffect( boost::shared_ptr<Natron::Node>() ) ) );
        assert(reader.get());
        std::map<std::string,void*> readerFunctions;
        readerFunctions.insert( std::make_pair("BuildEffect", (void*)&QtReader::BuildEffect) );
        LibraryBinary *readerPlugin = new LibraryBinary(readerFunctions);
        assert(readerPlugin);
        
        registerPlugin(grouping, reader->getPluginID().c_str(), reader->getPluginLabel().c_str(), "", "", false, false, readerPlugin, false, reader->getMajorVersion(), reader->getMinorVersion());

        std::vector<std::string> extensions = reader->supportedFileFormats();
        for (U32 k = 0; k < extensions.size(); ++k) {
            std::map<std::string,std::vector< std::pair<std::string,double> > >::iterator it;
            it = readersMap->find(extensions[k]);

            if ( it != readersMap->end() ) {
                it->second.push_back( std::make_pair(reader->getPluginID(), -1) );
            } else {
                std::vector<std::pair<std::string,double> > newVec(1);
                newVec[0] = std::make_pair(reader->getPluginID(),-1);
                readersMap->insert( std::make_pair(extensions[k], newVec) );
            }
        }
    }

    {
        std::auto_ptr<QtWriter> writer( dynamic_cast<QtWriter*>( QtWriter::BuildEffect( boost::shared_ptr<Natron::Node>() ) ) );
        assert(writer.get());
        std::map<std::string,void*> writerFunctions;
        writerFunctions.insert( std::make_pair("BuildEffect", (void*)&QtWriter::BuildEffect) );
        LibraryBinary *writerPlugin = new LibraryBinary(writerFunctions);
        assert(writerPlugin);
        
        registerPlugin(grouping, writer->getPluginID().c_str(), writer->getPluginLabel().c_str(),"", "", false, false, writerPlugin, false, writer->getMajorVersion(), writer->getMinorVersion());
        


        std::vector<std::string> extensions = writer->supportedFileFormats();
        for (U32 k = 0; k < extensions.size(); ++k) {
            std::map<std::string,std::vector< std::pair<std::string,double> > >::iterator it;
            it = writersMap->find(extensions[k]);

            if ( it != writersMap->end() ) {
                it->second.push_back( std::make_pair(writer->getPluginID(), -1) );
            } else {
                std::vector<std::pair<std::string,double> > newVec(1);
                newVec[0] = std::make_pair(writer->getPluginID(),-1);
                writersMap->insert( std::make_pair(extensions[k], newVec) );
            }
        }
    }
# else // !DEBUG
    (void)readersMap;
    (void)writersMap;
# endif // DEBUG

    {
        boost::shared_ptr<EffectInstance> viewer( ViewerInstance::BuildEffect( boost::shared_ptr<Natron::Node>() ) );
        assert(viewer);
        std::map<std::string,void*> viewerFunctions;
        viewerFunctions.insert( std::make_pair("BuildEffect", (void*)&ViewerInstance::BuildEffect) );
        LibraryBinary *viewerPlugin = new LibraryBinary(viewerFunctions);
        assert(viewerPlugin);
        
        registerPlugin(grouping, viewer->getPluginID().c_str(), viewer->getPluginLabel().c_str(),NATRON_IMAGES_PATH "viewer_icon.png", "", false, false, viewerPlugin, false, viewer->getMajorVersion(), viewer->getMinorVersion());

    }

    ///Also load the plug-ins of the AppManager
    AppManager::loadBuiltinNodePlugins(readersMap, writersMap);
} // loadBuiltinNodePlugins

AppInstance*
GuiApplicationManager::makeNewInstance(int appID) const
{
    return new GuiAppInstance(appID);
}

KnobGui*
GuiApplicationManager::createGuiForKnob(boost::shared_ptr<KnobI> knob,
                                        DockablePanel *container) const
{
    return _imp->_knobGuiFactory->createGuiForKnob(knob,container);
}

void
GuiApplicationManager::registerGuiMetaTypes() const
{
    qRegisterMetaType<CurveWidget*>();
}

class Application
    : public QApplication
{
    GuiApplicationManager* _app;

public:

    Application(GuiApplicationManager* app,
                int &argc,
                char** argv)                                      ///< qapplication needs to be subclasses with reference otherwise lots of crashes will happen.
        : QApplication(argc,argv)
          , _app(app)
    {
    }

protected:

    bool event(QEvent* e);
};

bool
Application::event(QEvent* e)
{
    switch ( e->type() ) {
    case QEvent::FileOpen: {
        assert(_app);
        QFileOpenEvent* foe = dynamic_cast<QFileOpenEvent*>(e);
        assert(foe);
        if (foe) {
            QString file =  foe->file();
#ifdef Q_OS_UNIX
            if ( !file.isEmpty() ) {
                file = AppManager::qt_tildeExpansion(file);
            }
#endif
            _app->setFileToOpen(file);
        }
    }

        return true;
    default:

        return QApplication::event(e);
    }
}

void
GuiApplicationManager::initializeQApp(int &argc,
                                      char** argv)
{
    QApplication* app = new Application(this,argc, argv);

    app->setQuitOnLastWindowClosed(true);
    Q_INIT_RESOURCE(GuiResources);
    
    ///Register all the shortcuts.
    populateShortcuts();
}

QString
GuiApplicationManager::getAppFont() const
{
    return _imp->_fontFamily;
}

int
GuiApplicationManager::getAppFontSize() const
{
    return _imp->_fontSize;
}


void
GuiApplicationManager::onAllPluginsLoaded()
{
    ///Restore user shortcuts only when all plug-ins are populated.
    loadShortcuts();
    AppManager::onAllPluginsLoaded();
}

void
GuiApplicationManager::setUndoRedoStackLimit(int limit)
{
    const std::map<int,AppInstanceRef> & apps = getAppInstances();

    for (std::map<int,AppInstanceRef>::const_iterator it = apps.begin(); it != apps.end(); ++it) {
        GuiAppInstance* guiApp = dynamic_cast<GuiAppInstance*>(it->second.app);
        if (guiApp) {
            guiApp->setUndoRedoStackLimit(limit);
        }
    }
}

void
GuiApplicationManager::debugImage(const Natron::Image* image,
                                  const QString & filename) const
{
    Gui::debugImage(image,filename);
}

void
GuiApplicationManager::setFileToOpen(const QString & str)
{
    _imp->_openFileRequest = str;
    if ( isLoaded() && !_imp->_openFileRequest.isEmpty() ) {
        handleOpenFileRequest();
    }
}

void
GuiApplicationManager::handleOpenFileRequest()
{


    AppInstance* mainApp = getAppInstance(0);
    GuiAppInstance* guiApp = dynamic_cast<GuiAppInstance*>(mainApp);
    assert(guiApp);
    if (guiApp) {
        guiApp->getGui()->openProject(_imp->_openFileRequest.toStdString());
        _imp->_openFileRequest.clear();
    }
}

void
GuiApplicationManager::onLoadCompleted()
{
    if ( !_imp->_openFileRequest.isEmpty() ) {
        handleOpenFileRequest();
    }
}

void
GuiApplicationManager::exitApp()
{
    ///make a copy of the map because it will be modified when closing projects
    std::map<int,AppInstanceRef> instances = getAppInstances();

    for (std::map<int,AppInstanceRef>::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        GuiAppInstance* app = dynamic_cast<GuiAppInstance*>(it->second.app);
        if ( app && !app->getGui()->closeInstance() ) {
            return;
        }
    }
}

static bool
matchesModifers(const Qt::KeyboardModifiers & lhs,
                const Qt::KeyboardModifiers & rhs,
                Qt::Key key)
{
    if (lhs == rhs) {
        return true;
    }

    bool isDigit =
        key == Qt::Key_0 ||
        key == Qt::Key_1 ||
        key == Qt::Key_2 ||
        key == Qt::Key_3 ||
        key == Qt::Key_4 ||
        key == Qt::Key_5 ||
        key == Qt::Key_6 ||
        key == Qt::Key_7 ||
        key == Qt::Key_8 ||
        key == Qt::Key_9;
    // On some keyboards (e.g. French AZERTY), the number keys are shifted
    if ( ( lhs == (Qt::ShiftModifier | Qt::AltModifier) ) && (rhs == Qt::AltModifier) && isDigit ) {
        return true;
    }

    return false;
}

static bool
matchesKey(Qt::Key lhs,
           Qt::Key rhs)
{
    if (lhs == rhs) {
        return true;
    }
    ///special case for the backspace and delete keys that mean the same thing generally
    else if ( ( (lhs == Qt::Key_Backspace) && (rhs == Qt::Key_Delete) ) ||
              ( ( lhs == Qt::Key_Delete) && ( rhs == Qt::Key_Backspace) ) ) {
        return true;
    }
    ///special case for the return and enter keys that mean the same thing generally
    else if ( ( (lhs == Qt::Key_Return) && (rhs == Qt::Key_Enter) ) ||
              ( ( lhs == Qt::Key_Enter) && ( rhs == Qt::Key_Return) ) ) {
        return true;
    }

    return false;
}

bool
GuiApplicationManager::matchesKeybind(const QString & group,
                                      const QString & actionID,
                                      const Qt::KeyboardModifiers & modifiers,
                                      int symbol) const
{
    AppShortcuts::const_iterator it = _imp->_actionShortcuts.find(group);

    if ( it == _imp->_actionShortcuts.end() ) {
        // we didn't find the group
        return false;
    }

    GroupShortcuts::const_iterator it2 = it->second.find(actionID);
    if ( it2 == it->second.end() ) {
        // we didn't find the action
        return false;
    }

    const KeyBoundAction* keybind = dynamic_cast<const KeyBoundAction*>(it2->second);
    if (!keybind) {
        return false;
    }

    // the following macro only tests the Control, Alt, and Shift modifiers, and discards the others
    Qt::KeyboardModifiers onlyCAS = modifiers & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier);

    if ( matchesModifers(onlyCAS,keybind->modifiers,(Qt::Key)symbol) ) {
        // modifiers are equal, now test symbol
        return matchesKey( (Qt::Key)symbol, keybind->currentShortcut );
    }

    return false;
}

bool
GuiApplicationManager::matchesMouseShortcut(const QString & group,
                                            const QString & actionID,
                                            const Qt::KeyboardModifiers & modifiers,
                                            int button) const
{
    AppShortcuts::const_iterator it = _imp->_actionShortcuts.find(group);

    if ( it == _imp->_actionShortcuts.end() ) {
        // we didn't find the group
        return false;
    }

    GroupShortcuts::const_iterator it2 = it->second.find(actionID);
    if ( it2 == it->second.end() ) {
        // we didn't find the action
        return false;
    }

    const MouseAction* mAction = dynamic_cast<const MouseAction*>(it2->second);
    if (!mAction) {
        return false;
    }

    // the following macro only tests the Control, Alt, and Shift (and cmd an mac) modifiers, and discards the others
    Qt::KeyboardModifiers onlyMCAS = modifiers & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier | Qt::MetaModifier);

    /*Note that the default configuration of Apple's X11 (see X11 menu:Preferences:Input tab) is to "emulate three button mouse". This sets a single button mouse or laptop trackpad to behave as follows:
       X11 left mouse button click
       X11 middle mouse button Option-click
       X11 right mouse button Ctrl-click
       (Command=clover=apple key)*/

    if ( (onlyMCAS == Qt::AltModifier) && ( (Qt::MouseButton)button == Qt::LeftButton ) ) {
        onlyMCAS = Qt::NoModifier;
        button = Qt::MiddleButton;
    } else if ( (onlyMCAS == Qt::MetaModifier) && ( (Qt::MouseButton)button == Qt::LeftButton ) ) {
        onlyMCAS = Qt::NoModifier;
        button = Qt::RightButton;
    }

    if (onlyMCAS == mAction->modifiers) {
        // modifiers are equal, now test symbol
        if ( (Qt::MouseButton)button == mAction->button ) {
            return true;
        }
    }

    return false;
}

void
GuiApplicationManager::saveShortcuts() const
{
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    
    for (AppShortcuts::const_iterator it = _imp->_actionShortcuts.begin(); it != _imp->_actionShortcuts.end(); ++it) {
        settings.beginGroup(it->first);
        for (GroupShortcuts::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            const MouseAction* mAction = dynamic_cast<const MouseAction*>(it2->second);
            const KeyBoundAction* kAction = dynamic_cast<const KeyBoundAction*>(it2->second);
            settings.setValue(it2->first + "_Modifiers", (int)it2->second->modifiers);
            if (mAction) {
                settings.setValue(it2->first + "_Button", (int)mAction->button);
            } else if (kAction) {
                settings.setValue(it2->first + "_Symbol", (int)kAction->currentShortcut);
            }
        }
        settings.endGroup();
    }
}

void
GuiApplicationManager::loadShortcuts()
{
    
    bool settingsExistd = getCurrentSettings()->didSettingsExistOnStartup();
    
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);

    int settingsVersion = -1;
    if (settings.contains("NATRON_SHORTCUTS_DEFAULT_VERSION")) {
        settingsVersion = settings.value("NATRON_SHORTCUTS_DEFAULT_VERSION").toInt();
    }
    
    if (settingsExistd && settingsVersion < NATRON_SHORTCUTS_DEFAULT_VERSION) {
        _imp->_shortcutsChangedVersion = true;
    }
    
    settings.setValue("NATRON_SHORTCUTS_DEFAULT_VERSION", NATRON_SHORTCUTS_DEFAULT_VERSION);

    
    for (AppShortcuts::iterator it = _imp->_actionShortcuts.begin(); it != _imp->_actionShortcuts.end(); ++it) {
        settings.beginGroup(it->first);
        for (GroupShortcuts::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            MouseAction* mAction = dynamic_cast<MouseAction*>(it2->second);
            KeyBoundAction* kAction = dynamic_cast<KeyBoundAction*>(it2->second);

            if ( settings.contains(it2->first + "_Modifiers") ) {
                it2->second->modifiers = Qt::KeyboardModifiers( settings.value(it2->first + "_Modifiers").toInt() );
            }
            if ( mAction && settings.contains(it2->first + "_Button") ) {
                mAction->button = (Qt::MouseButton)settings.value(it2->first + "_Button").toInt();
            }
            if ( kAction && settings.contains(it2->first + "_Symbol") ) {
                
                Qt::Key newShortcut = (Qt::Key)settings.value(it2->first + "_Symbol").toInt();
                kAction->currentShortcut = newShortcut;
                

                //If this is a node shortcut, notify the Plugin object that it has a shortcut.
                if ( (kAction->currentShortcut != (Qt::Key)0) &&
                     it->first.startsWith(kShortcutGroupNodes) ) {
                    const PluginsMap & allPlugins = getPluginsList();
                    PluginsMap::const_iterator found = allPlugins.find(it2->first.toStdString());
                    if (found != allPlugins.end()) {
                        assert(found->second.size() > 0);
                        (*found->second.rbegin())->setHasShortcut(true);
                    }
                }
            }
        }
        settings.endGroup();
    }
}

bool
GuiApplicationManager::isShorcutVersionUpToDate() const
{
    return !_imp->_shortcutsChangedVersion;
}

void
GuiApplicationManager::restoreDefaultShortcuts()
{
    for (AppShortcuts::iterator it = _imp->_actionShortcuts.begin(); it != _imp->_actionShortcuts.end(); ++it) {
        for (GroupShortcuts::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            KeyBoundAction* kAction = dynamic_cast<KeyBoundAction*>(it2->second);
            it2->second->modifiers = it2->second->defaultModifiers;
            if (kAction) {
                kAction->currentShortcut = kAction->defaultShortcut;
                notifyShortcutChanged(kAction);
            }
            ///mouse actions cannot have their button changed
        }
    }
}

void
GuiApplicationManager::populateShortcuts()
{
    ///General
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionNewProject, kShortcutDescActionNewProject,QKeySequence::New);
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionOpenProject, kShortcutDescActionOpenProject,QKeySequence::Open);
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionSaveProject, kShortcutDescActionSaveProject,QKeySequence::Save);
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionSaveAsProject, kShortcutDescActionSaveAsProject,QKeySequence::SaveAs);
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionCloseProject, kShortcutDescActionCloseProject,QKeySequence::Close);
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionPreferences, kShortcutDescActionPreferences,QKeySequence::Preferences);
    registerStandardKeybind(kShortcutGroupGlobal, kShortcutIDActionQuit, kShortcutDescActionQuit,QKeySequence::Quit);

    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionSaveAndIncrVersion, kShortcutDescActionSaveAndIncrVersion, Qt::ControlModifier | Qt::ShiftModifier |
                    Qt::AltModifier, Qt::Key_S);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionExportProject, kShortcutDescActionExportProject, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionShowAbout, kShortcutDescActionShowAbout, Qt::NoModifier, (Qt::Key)0);

    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionImportLayout, kShortcutDescActionImportLayout, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionExportLayout, kShortcutDescActionExportLayout, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionDefaultLayout, kShortcutDescActionDefaultLayout, Qt::NoModifier, (Qt::Key)0);

    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionProjectSettings, kShortcutDescActionProjectSettings, Qt::NoModifier, Qt::Key_S);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionShowOFXLog, kShortcutDescActionShowOFXLog, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionShowShortcutEditor, kShortcutDescActionShowShortcutEditor, Qt::NoModifier, (Qt::Key)0);

    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionNewViewer, kShortcutDescActionNewViewer, Qt::ControlModifier, Qt::Key_I);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionFullscreen, kShortcutDescActionFullscreen, Qt::AltModifier, Qt::Key_S); // as in Nuke
    //registerKeybind(kShortcutGroupGlobal, kShortcutIDActionFullscreen, kShortcutDescActionFullscreen, Qt::ControlModifier | Qt::AltModifier, Qt::Key_F);

    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionClearDiskCache, kShortcutDescActionClearDiskCache, Qt::NoModifier,(Qt::Key)0);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionClearPlaybackCache, kShortcutDescActionClearPlaybackCache, Qt::NoModifier,(Qt::Key)0);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionClearNodeCache, kShortcutDescActionClearNodeCache, Qt::NoModifier,(Qt::Key)0);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionClearPluginsLoadCache, kShortcutDescActionClearPluginsLoadCache, Qt::NoModifier,(Qt::Key)0);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionClearAllCaches, kShortcutDescActionClearAllCaches, Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_K);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionRenderSelected, kShortcutDescActionRenderSelected, Qt::NoModifier, Qt::Key_F7);

    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionRenderAll, kShortcutDescActionRenderAll, Qt::NoModifier, Qt::Key_F5);


    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput1, kShortcutDescActionConnectViewerToInput1, Qt::NoModifier, Qt::Key_1);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput2, kShortcutDescActionConnectViewerToInput2, Qt::NoModifier, Qt::Key_2);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput3, kShortcutDescActionConnectViewerToInput3, Qt::NoModifier, Qt::Key_3);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput4, kShortcutDescActionConnectViewerToInput4, Qt::NoModifier, Qt::Key_4);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput5, kShortcutDescActionConnectViewerToInput5, Qt::NoModifier, Qt::Key_5);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput6, kShortcutDescActionConnectViewerToInput6, Qt::NoModifier, Qt::Key_6);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput7, kShortcutDescActionConnectViewerToInput7, Qt::NoModifier, Qt::Key_7);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput8, kShortcutDescActionConnectViewerToInput8, Qt::NoModifier, Qt::Key_8);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput9, kShortcutDescActionConnectViewerToInput9, Qt::NoModifier, Qt::Key_9);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionConnectViewerToInput10, kShortcutDescActionConnectViewerToInput10, Qt::NoModifier, Qt::Key_0);

    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionShowPaneFullScreen, kShortcutDescActionShowPaneFullScreen, Qt::NoModifier, Qt::Key_Space);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionNextTab, kShortcutDescActionNextTab, Qt::ControlModifier, Qt::Key_T);
    registerKeybind(kShortcutGroupGlobal, kShortcutIDActionCloseTab, kShortcutDescActionCloseTab, Qt::ShiftModifier, Qt::Key_Escape);

    ///Viewer
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionLuminance, kShortcutDescActionLuminance, Qt::NoModifier, Qt::Key_Y);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionR, kShortcutDescActionR, Qt::NoModifier, Qt::Key_R);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionG, kShortcutDescActionG, Qt::NoModifier, Qt::Key_G);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionB, kShortcutDescActionB, Qt::NoModifier, Qt::Key_B);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionA, kShortcutDescActionA, Qt::NoModifier, Qt::Key_A);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionFitViewer, kShortcutDescActionFitViewer, Qt::NoModifier, Qt::Key_F);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionClipEnabled, kShortcutDescActionClipEnabled, Qt::ShiftModifier, Qt::Key_C);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionRefresh, kShortcutDescActionRefresh, Qt::NoModifier, Qt::Key_U);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionROIEnabled, kShortcutDescActionROIEnabled, Qt::ShiftModifier, Qt::Key_W);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionProxyEnabled, kShortcutDescActionProxyEnabled, Qt::ControlModifier, Qt::Key_P);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel2, kShortcutDescActionProxyLevel2, Qt::AltModifier, Qt::Key_1);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel4, kShortcutDescActionProxyLevel4, Qt::AltModifier, Qt::Key_2);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel8, kShortcutDescActionProxyLevel8, Qt::AltModifier, Qt::Key_3);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel16, kShortcutDescActionProxyLevel16, Qt::AltModifier, Qt::Key_4);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionProxyLevel32, kShortcutDescActionProxyLevel32, Qt::AltModifier, Qt::Key_5);

    registerKeybind(kShortcutGroupViewer, kShortcutIDActionHideOverlays, kShortcutDescActionHideOverlays, Qt::NoModifier, Qt::Key_O);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionHidePlayer, kShortcutDescActionHidePlayer, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionHideTimeline, kShortcutDescActionHideTimeline, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionHideLeft, kShortcutDescActionHideLeft, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionHideRight, kShortcutDescActionHideRight, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionHideTop, kShortcutDescActionHideTop, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionHideInfobar, kShortcutDescActionHideInfobar, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionHideAll, kShortcutDescActionHideAll, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionShowAll, kShortcutDescActionShowAll, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupViewer, kShortcutIDActionZoomLevel100, kShortcutDescActionZoomLevel100, Qt::ControlModifier, Qt::Key_1);
    registerKeybind(kShortcutGroupViewer, kShortcutIDToggleWipe, kShortcutDescToggleWipe, Qt::NoModifier, Qt::Key_W);
    
    registerMouseShortcut(kShortcutGroupViewer, kShortcutIDMousePickColor, kShortcutDescMousePickColor, Qt::ControlModifier, Qt::LeftButton);
    registerMouseShortcut(kShortcutGroupViewer, kShortcutIDMouseRectanglePick, kShortcutDescMouseRectanglePick, Qt::ShiftModifier | Qt::ControlModifier, Qt::LeftButton);
    
    

    ///Player
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevious, kShortcutDescActionPlayerPrevious, Qt::NoModifier, Qt::Key_Left);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNext, kShortcutDescActionPlayerNext, Qt::NoModifier, Qt::Key_Right);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerBackward, kShortcutDescActionPlayerBackward, Qt::NoModifier, Qt::Key_J);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerForward, kShortcutDescActionPlayerForward, Qt::NoModifier, Qt::Key_L);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerStop, kShortcutDescActionPlayerStop, Qt::NoModifier, Qt::Key_K);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevIncr, kShortcutDescActionPlayerPrevIncr, Qt::ShiftModifier, Qt::Key_Left);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextIncr, kShortcutDescActionPlayerNextIncr, Qt::ShiftModifier, Qt::Key_Right);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerPrevKF, kShortcutDescActionPlayerPrevKF, Qt::ShiftModifier | Qt::ControlModifier, Qt::Key_Left);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerNextKF, kShortcutDescActionPlayerPrevKF, Qt::ShiftModifier | Qt::ControlModifier, Qt::Key_Right);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerFirst, kShortcutDescActionPlayerFirst, Qt::ControlModifier, Qt::Key_Left);
    registerKeybind(kShortcutGroupPlayer, kShortcutIDActionPlayerLast, kShortcutDescActionPlayerLast, Qt::ControlModifier, Qt::Key_Right);

    ///Roto
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoDelete, kShortcutDescActionRotoDelete, Qt::NoModifier, Qt::Key_Backspace);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoCloseBezier, kShortcutDescActionRotoCloseBezier, Qt::NoModifier, Qt::Key_Enter);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoSelectAll, kShortcutDescActionRotoSelectAll, Qt::ControlModifier, Qt::Key_A);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoSelectionTool, kShortcutDescActionRotoSelectionTool, Qt::NoModifier, Qt::Key_Q);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoAddTool, kShortcutDescActionRotoAddTool, Qt::NoModifier, Qt::Key_D);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoEditTool, kShortcutDescActionRotoEditTool, Qt::NoModifier, Qt::Key_V);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoBrushTool, kShortcutDescActionRotoBrushTool, Qt::NoModifier, Qt::Key_N);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoCloneTool, kShortcutDescActionRotoCloneTool, Qt::NoModifier, Qt::Key_C);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoEffectTool, kShortcutDescActionRotoEffectTool, Qt::NoModifier, Qt::Key_X);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoColorTool, kShortcutDescActionRotoColorTool, Qt::NoModifier, Qt::Key_E);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoNudgeLeft, kShortcutDescActionRotoNudgeLeft, Qt::AltModifier, Qt::Key_Left);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoNudgeBottom, kShortcutDescActionRotoNudgeBottom, Qt::AltModifier, Qt::Key_Down);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoNudgeRight, kShortcutDescActionRotoNudgeRight, Qt::AltModifier, Qt::Key_Right);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoNudgeTop, kShortcutDescActionRotoNudgeTop, Qt::AltModifier, Qt::Key_Up);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoSmooth, kShortcutDescActionRotoSmooth, Qt::NoModifier, Qt::Key_Z);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoCuspBezier, kShortcutDescActionRotoCuspBezier, Qt::ShiftModifier, Qt::Key_Z);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoRemoveFeather, kShortcutDescActionRotoRemoveFeather, Qt::ShiftModifier, Qt::Key_E);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoLinkToTrack, kShortcutDescActionRotoLinkToTrack, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoUnlinkToTrack, kShortcutDescActionRotoUnlinkToTrack,
                    Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupRoto, kShortcutIDActionRotoLockCurve, kShortcutDescActionRotoLockCurve,
                    Qt::ShiftModifier, Qt::Key_L);


    ///Tracking
    registerKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingSelectAll, kShortcutDescActionTrackingSelectAll, Qt::ControlModifier, Qt::Key_A);
    registerKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingDelete, kShortcutDescActionTrackingDelete, Qt::NoModifier, Qt::Key_Backspace);
    registerKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingBackward, kShortcutDescActionTrackingBackward, Qt::NoModifier, Qt::Key_Z);
    registerKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingPrevious, kShortcutDescActionTrackingPrevious, Qt::NoModifier, Qt::Key_X);
    registerKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingNext, kShortcutDescActionTrackingNext, Qt::NoModifier, Qt::Key_C);
    registerKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingForward, kShortcutDescActionTrackingForward, Qt::NoModifier, Qt::Key_V);
    registerKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingStop, kShortcutDescActionTrackingStop, Qt::NoModifier, Qt::Key_Escape);

    ///Nodegraph
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCreateReader, kShortcutDescActionGraphCreateReader, Qt::NoModifier, Qt::Key_R);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCreateWriter, kShortcutDescActionGraphCreateWriter, Qt::NoModifier, Qt::Key_W);

    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphRearrangeNodes, kShortcutDescActionGraphRearrangeNodes, Qt::NoModifier, Qt::Key_L);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphDisableNodes, kShortcutDescActionGraphDisableNodes, Qt::NoModifier, Qt::Key_D);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphRemoveNodes, kShortcutDescActionGraphRemoveNodes, Qt::NoModifier, Qt::Key_Backspace);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphShowExpressions, kShortcutDescActionGraphShowExpressions, Qt::ShiftModifier, Qt::Key_E);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphNavigateDownstream, kShortcutDescActionGraphNavigateDownstram, Qt::NoModifier, Qt::Key_Down);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphNavigateUpstream, kShortcutDescActionGraphNavigateUpstream, Qt::NoModifier, Qt::Key_Up);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSelectUp, kShortcutDescActionGraphSelectUp, Qt::ShiftModifier, Qt::Key_Up);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSelectDown, kShortcutDescActionGraphSelectDown, Qt::ShiftModifier, Qt::Key_Down);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSelectAll, kShortcutDescActionGraphSelectAll, Qt::ControlModifier, Qt::Key_A);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSelectAllVisible, kShortcutDescActionGraphSelectAllVisible, Qt::ShiftModifier | Qt::ControlModifier, Qt::Key_A);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphEnableHints, kShortcutDescActionGraphEnableHints, Qt::NoModifier, Qt::Key_H);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphAutoHideInputs, kShortcutDescActionGraphAutoHideInputs, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphSwitchInputs, kShortcutDescActionGraphSwitchInputs, Qt::ShiftModifier, Qt::Key_X);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCopy, kShortcutDescActionGraphCopy, Qt::ControlModifier, Qt::Key_C);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphPaste, kShortcutDescActionGraphPaste, Qt::ControlModifier, Qt::Key_V);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphCut, kShortcutDescActionGraphCut, Qt::ControlModifier, Qt::Key_X);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphClone, kShortcutDescActionGraphClone, Qt::AltModifier, Qt::Key_K);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphDeclone, kShortcutDescActionGraphDeclone, Qt::AltModifier | Qt::ShiftModifier, Qt::Key_K);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphDuplicate, kShortcutDescActionGraphDuplicate, Qt::AltModifier, Qt::Key_C);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphForcePreview, kShortcutDescActionGraphForcePreview, Qt::ShiftModifier, Qt::Key_P);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphTogglePreview, kShortcutDescActionGraphToggleAutoPreview, Qt::AltModifier, Qt::Key_P);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphToggleAutoPreview, kShortcutDescActionGraphToggleAutoPreview, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphToggleAutoTurbo, kShortcutDescActionGraphToggleAutoTurbo, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphFrameNodes, kShortcutDescActionGraphFrameNodes, Qt::NoModifier, Qt::Key_F);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphShowCacheSize, kShortcutDescActionGraphShowCacheSize, Qt::NoModifier, (Qt::Key)0);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphFindNode, kShortcutDescActionGraphFindNode, Qt::ControlModifier, Qt::Key_F);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphRenameNode, kShortcutDescActionGraphRenameNode, Qt::NoModifier, Qt::Key_N);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphExtractNode, kShortcutDescActionGraphExtractNode, Qt::ControlModifier | Qt::ShiftModifier,
                    Qt::Key_X);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphMakeGroup, kShortcutDescActionGraphMakeGroup, Qt::ControlModifier | Qt::ShiftModifier,
                    Qt::Key_G);
    registerKeybind(kShortcutGroupNodegraph, kShortcutIDActionGraphExpandGroup, kShortcutDescActionGraphExpandGroup, Qt::ControlModifier | Qt::ShiftModifier,
                    Qt::Key_E);
    
    ///CurveEditor
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorRemoveKeys, kShortcutDescActionCurveEditorRemoveKeys, Qt::NoModifier,Qt::Key_Backspace);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorConstant, kShortcutDescActionCurveEditorConstant, Qt::NoModifier, Qt::Key_K);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorSmooth, kShortcutDescActionCurveEditorSmooth, Qt::NoModifier, Qt::Key_Z);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorLinear, kShortcutDescActionCurveEditorLinear, Qt::NoModifier, Qt::Key_L);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCatmullrom, kShortcutDescActionCurveEditorCatmullrom, Qt::NoModifier, Qt::Key_R);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCubic, kShortcutDescActionCurveEditorCubic, Qt::NoModifier, Qt::Key_C);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorHorizontal, kShortcutDescActionCurveEditorHorizontal, Qt::NoModifier, Qt::Key_H);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorBreak, kShortcutDescActionCurveEditorBreak, Qt::NoModifier, Qt::Key_X);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorSelectAll, kShortcutDescActionCurveEditorSelectAll, Qt::ControlModifier, Qt::Key_A);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCenter, kShortcutDescActionCurveEditorCenter
                    , Qt::NoModifier, Qt::Key_F);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorCopy, kShortcutDescActionCurveEditorCopy, Qt::ControlModifier, Qt::Key_C);
    registerKeybind(kShortcutGroupCurveEditor, kShortcutIDActionCurveEditorPaste, kShortcutDescActionCurveEditorPaste, Qt::ControlModifier, Qt::Key_V);

    // Dope Sheet Editor
    registerKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorDeleteKeys, kShortcutDescActionDopeSheetEditorDeleteKeys, Qt::NoModifier, Qt::Key_Backspace);
    registerKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorFrameSelection, kShortcutDescActionDopeSheetEditorFrameSelection, Qt::NoModifier, Qt::Key_F);
    registerKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorSelectAllKeyframes, kShortcutDescActionDopeSheetEditorSelectAllKeyframes, Qt::ControlModifier, Qt::Key_A);

    registerKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorConstant, kShortcutDescActionCurveEditorConstant, Qt::NoModifier, Qt::Key_K);
    registerKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorSmooth, kShortcutDescActionCurveEditorSmooth, Qt::NoModifier, Qt::Key_Z);
    registerKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorLinear, kShortcutDescActionCurveEditorLinear, Qt::NoModifier, Qt::Key_L);
    registerKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorCatmullrom, kShortcutDescActionCurveEditorCatmullrom, Qt::NoModifier, Qt::Key_R);
    registerKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorCubic, kShortcutDescActionCurveEditorCubic, Qt::NoModifier, Qt::Key_C);
    registerKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorHorizontal, kShortcutDescActionCurveEditorHorizontal, Qt::NoModifier, Qt::Key_H);
    registerKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionCurveEditorBreak, kShortcutDescActionCurveEditorBreak, Qt::NoModifier, Qt::Key_X);

    registerKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorCopySelectedKeyframes, kShortcutDescActionDopeSheetEditorCopySelectedKeyframes, Qt::ControlModifier, Qt::Key_C);
    registerKeybind(kShortcutGroupDopeSheetEditor, kShortcutIDActionDopeSheetEditorPasteKeyframes, kShortcutDescActionDopeSheetEditorPasteKeyframes, Qt::ControlModifier, Qt::Key_V);
} // populateShortcuts

void
GuiApplicationManagerPrivate::addKeybind(const QString & grouping,
                                         const QString & id,
                                         const QString & description,
                                         const Qt::KeyboardModifiers & modifiers,
                                         Qt::Key symbol)
{
    
    AppShortcuts::iterator foundGroup = _actionShortcuts.find(grouping);
    if ( foundGroup != _actionShortcuts.end() ) {
        GroupShortcuts::iterator foundAction = foundGroup->second.find(id);
        if ( foundAction != foundGroup->second.end() ) {
            return;
        }
    }
    KeyBoundAction* kA = new KeyBoundAction;

    kA->grouping = grouping;
    kA->description = description;
    kA->defaultModifiers = modifiers;
    kA->modifiers = modifiers;
    kA->defaultShortcut = symbol;
    kA->currentShortcut = symbol;
    if ( foundGroup != _actionShortcuts.end() ) {
        foundGroup->second.insert( std::make_pair(id, kA) );
    } else {
        GroupShortcuts group;
        group.insert( std::make_pair(id, kA) );
        _actionShortcuts.insert( std::make_pair(grouping, group) );
    }
    
    GuiAppInstance* app = dynamic_cast<GuiAppInstance*>(_publicInterface->getTopLevelInstance());
    if ( app && app->getGui()->hasShortcutEditorAlreadyBeenBuilt() ) {
        app->getGui()->addShortcut(kA);
    }
}

void
GuiApplicationManagerPrivate::removeKeybind(const QString& grouping,const QString& id)
{
    AppShortcuts::iterator foundGroup = _actionShortcuts.find(grouping);
    if ( foundGroup != _actionShortcuts.end() ) {
        GroupShortcuts::iterator foundAction = foundGroup->second.find(id);
        if ( foundAction != foundGroup->second.end() ) {
            foundGroup->second.erase(foundAction);
        }
        if (foundGroup->second.empty()) {
            _actionShortcuts.erase(foundGroup);
        }
    }
}

void
GuiApplicationManagerPrivate::addMouseShortcut(const QString & grouping,
                                               const QString & id,
                                               const QString & description,
                                               const Qt::KeyboardModifiers & modifiers,
                                               Qt::MouseButton button)
{
    
    AppShortcuts::iterator foundGroup = _actionShortcuts.find(grouping);
    if ( foundGroup != _actionShortcuts.end() ) {
        GroupShortcuts::iterator foundAction = foundGroup->second.find(id);
        if ( foundAction != foundGroup->second.end() ) {
            return;
        }
    }
    MouseAction* mA = new MouseAction;

    mA->grouping = grouping;
    mA->description = description;
    mA->defaultModifiers = modifiers;
    if ( modifiers & (Qt::AltModifier | Qt::MetaModifier) ) {
        qDebug() << "Warning: mouse shortcut " << grouping << '/' << description << '(' << id << ')' << " uses the Alt or Meta modifier, which is reserved for three-button mouse emulation. Fix this ASAP.";
    }
    mA->modifiers = modifiers;
    mA->button = button;

    ///Mouse shortcuts are not editable.
    mA->editable = false;

    if ( foundGroup != _actionShortcuts.end() ) {
        foundGroup->second.insert( std::make_pair(id, mA) );
    } else {
        GroupShortcuts group;
        group.insert( std::make_pair(id, mA) );
        _actionShortcuts.insert( std::make_pair(grouping, group) );
    }
    
    GuiAppInstance* app = dynamic_cast<GuiAppInstance*>(_publicInterface->getTopLevelInstance());
    if ( app && app->getGui()->hasShortcutEditorAlreadyBeenBuilt() ) {
        app->getGui()->addShortcut(mA);
    }
}

void
GuiApplicationManagerPrivate::addStandardKeybind(const QString & grouping,
                                                 const QString & id,
                                                 const QString & description,
                                                 QKeySequence::StandardKey key)
{
    AppShortcuts::iterator foundGroup = _actionShortcuts.find(grouping);
    if ( foundGroup != _actionShortcuts.end() ) {
        GroupShortcuts::iterator foundAction = foundGroup->second.find(id);
        if ( foundAction != foundGroup->second.end() ) {
            return;
        }
    }
    
    Qt::KeyboardModifiers modifiers;
    Qt::Key symbol;

    extractKeySequence(QKeySequence(key), modifiers, symbol);
    KeyBoundAction* kA = new KeyBoundAction;
    kA->grouping = grouping;
    kA->description = description;
    kA->defaultModifiers = modifiers;
    kA->modifiers = modifiers;
    kA->defaultShortcut = symbol;
    kA->currentShortcut = symbol;
    if ( foundGroup != _actionShortcuts.end() ) {
        foundGroup->second.insert( std::make_pair(id, kA) );
    } else {
        GroupShortcuts group;
        group.insert( std::make_pair(id, kA) );
        _actionShortcuts.insert( std::make_pair(grouping, group) );
    }
    
    GuiAppInstance* app = dynamic_cast<GuiAppInstance*>(_publicInterface->getTopLevelInstance());
    if ( app && app->getGui()->hasShortcutEditorAlreadyBeenBuilt() ) {
        app->getGui()->addShortcut(kA);
    }
}

QKeySequence
GuiApplicationManager::getKeySequenceForAction(const QString & group,
                                               const QString & actionID) const
{
    AppShortcuts::const_iterator foundGroup = _imp->_actionShortcuts.find(group);

    if ( foundGroup != _imp->_actionShortcuts.end() ) {
        GroupShortcuts::const_iterator found = foundGroup->second.find(actionID);
        if ( found != foundGroup->second.end() ) {
            const KeyBoundAction* ka = dynamic_cast<const KeyBoundAction*>(found->second);
            if (ka) {
                return makeKeySequence(found->second->modifiers, ka->currentShortcut);
            }
        }
    }

    return QKeySequence();
}

bool
GuiApplicationManager::getModifiersAndKeyForAction(const QString & group,
                                                   const QString & actionID,
                                                   Qt::KeyboardModifiers & modifiers,
                                                   int & symbol) const
{
    AppShortcuts::const_iterator foundGroup = _imp->_actionShortcuts.find(group);

    if ( foundGroup != _imp->_actionShortcuts.end() ) {
        GroupShortcuts::const_iterator found = foundGroup->second.find(actionID);
        if ( found != foundGroup->second.end() ) {
            const KeyBoundAction* ka = dynamic_cast<const KeyBoundAction*>(found->second);
            if (ka) {
                modifiers = found->second->modifiers;
                symbol = ka->currentShortcut;

                return true;
            }
        }
    }

    return false;
}

const std::map<QString,std::map<QString,BoundAction*> > &
GuiApplicationManager::getAllShortcuts() const
{
    return _imp->_actionShortcuts;
}

void
GuiApplicationManager::addShortcutAction(const QString & group,
                                         const QString & actionID,
                                         QAction* action)
{
    AppShortcuts::iterator foundGroup = _imp->_actionShortcuts.find(group);

    if ( foundGroup != _imp->_actionShortcuts.end() ) {
        GroupShortcuts::iterator found = foundGroup->second.find(actionID);
        if ( found != foundGroup->second.end() ) {
            KeyBoundAction* ka = dynamic_cast<KeyBoundAction*>(found->second);
            if (ka) {
                ka->actions.push_back(action);

                return;
            }
        }
    }
}

void
GuiApplicationManager::removeShortcutAction(const QString & group,
                                            const QString & actionID,
                                            QAction* action)
{
    AppShortcuts::iterator foundGroup = _imp->_actionShortcuts.find(group);

    if ( foundGroup != _imp->_actionShortcuts.end() ) {
        GroupShortcuts::iterator found = foundGroup->second.find(actionID);
        if ( found != foundGroup->second.end() ) {
            KeyBoundAction* ka = dynamic_cast<KeyBoundAction*>(found->second);
            if (ka) {
                std::list<QAction*>::iterator foundAction = std::find(ka->actions.begin(),ka->actions.end(),action);
                if ( foundAction != ka->actions.end() ) {
                    ka->actions.erase(foundAction);

                    return;
                }
            }
        }
    }
}

void
GuiApplicationManager::notifyShortcutChanged(KeyBoundAction* action)
{
    action->updateActionsShortcut();
    for (AppShortcuts::iterator it = _imp->_actionShortcuts.begin(); it != _imp->_actionShortcuts.end(); ++it) {
        if ( it->first.startsWith(kShortcutGroupNodes) ) {
            for (GroupShortcuts::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
                if (it2->second == action) {
                    const PluginsMap & allPlugins = getPluginsList();
                    PluginsMap::const_iterator found = allPlugins.find(it2->first.toStdString());
                    if (found != allPlugins.end()) {
                        assert(found->second.size() > 0);
                        (*found->second.rbegin())->setHasShortcut(action->currentShortcut != (Qt::Key)0);
                    }
                    break;
                }
            }
        }
    }
}

void
GuiApplicationManager::showOfxLog()
{
    GuiAppInstance* app = dynamic_cast<GuiAppInstance*>(getTopLevelInstance());
    if (app) {
        app->getGui()->showOfxLog();
    }
    
}

void
GuiApplicationManager::clearLastRenderedTextures()
{
    const std::map<int,AppInstanceRef>& instances = getAppInstances();
    for (std::map<int,AppInstanceRef>::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        GuiAppInstance* guiApp = dynamic_cast<GuiAppInstance*>(it->second.app);
        if (guiApp) {
            guiApp->clearAllLastRenderedImages();
        }
    }
}

bool
GuiApplicationManager::isNodeClipBoardEmpty() const
{
    return _imp->_nodeCB.isEmpty();
}

NodeClipBoard&
GuiApplicationManager::getNodeClipBoard()
{
    return _imp->_nodeCB;
}

void
GuiApplicationManager::clearNodeClipBoard()
{
    _imp->_nodeCB.nodes.clear();
    _imp->_nodeCB.nodesUI.clear();
}


///The symbol has been generated by Shiboken in  Engine/NatronEngine/natronengine_module_wrapper.cpp
extern "C"
{
    PyObject* PyInit_NatronGui();
}


void
GuiApplicationManager::initBuiltinPythonModules()
{
    AppManager::initBuiltinPythonModules();
    
    int ret = PyImport_AppendInittab(NATRON_GUI_PYTHON_MODULE_NAME,&PyInit_NatronGui);
    if (ret == -1) {
        throw std::runtime_error("Failed to initialize built-in Python module.");
    }
    
}

void
GuiApplicationManager::addCommand(const QString& grouping,const std::string& pythonFunction, Qt::Key key,const Qt::KeyboardModifiers& modifiers)
{
    
    QStringList split = grouping.split('/');
    if (grouping.isEmpty() || split.isEmpty()) {
        return;
    }
    PythonUserCommand c;
    c.grouping = grouping;
    c.pythonFunction = pythonFunction;
    c.key = key;
    c.modifiers = modifiers;
    _imp->pythonCommands.push_back(c);
    
    
    registerKeybind(kShortcutGroupGlobal, split[split.size() -1], split[split.size() - 1], modifiers, key);
}


const std::list<PythonUserCommand>&
GuiApplicationManager::getUserPythonCommands() const
{
    return _imp->pythonCommands;
}
void
GuiApplicationManager::reloadStylesheets()
{
    const std::map<int,AppInstanceRef>& instances = getAppInstances();
    for (std::map<int,AppInstanceRef>::const_iterator it = instances.begin(); it != instances.end(); ++it) {
        GuiAppInstance* guiApp = dynamic_cast<GuiAppInstance*>(it->second.app);
        if (guiApp) {
            guiApp->reloadStylesheet();
        }
    }
}
