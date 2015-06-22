#ifndef ACTIONSHORTCUTS_H
#define ACTIONSHORTCUTS_H

//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/**
 * @brief In this file all Natron's actions that can have their shortcut edited should be listed.
 **/

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <map>
#include <list>

#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QKeyEvent>
#include <QMouseEvent>
#include <QString>
#include <QAction>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#define kShortcutGroupGlobal "Global"
#define kShortcutGroupNodegraph "NodeGraph"
#define kShortcutGroupCurveEditor "CurveEditor"
#define kShortcutGroupDopeSheetEditor "DopeSheetEditor"
#define kShortcutGroupViewer "Viewer"
#define kShortcutGroupRoto "Roto"
#define kShortcutGroupTracking "Tracking"
#define kShortcutGroupPlayer "Player"
#define kShortcutGroupNodes "Nodes"

/////////GLOBAL SHORTCUTS
#define kShortcutIDActionNewProject "newProject"
#define kShortcutDescActionNewProject "New Project"

#define kShortcutIDActionOpenProject "openProject"
#define kShortcutDescActionOpenProject "Open Project..."

#define kShortcutIDActionCloseProject "closeProject"
#define kShortcutDescActionCloseProject "Close Project"

#define kShortcutIDActionSaveProject "saveProject"
#define kShortcutDescActionSaveProject "Save Project"

#define kShortcutIDActionSaveAsProject "saveAsProject"
#define kShortcutDescActionSaveAsProject "Save Project As.."

#define kShortcutIDActionSaveAndIncrVersion "saveAndIncr"
#define kShortcutDescActionSaveAndIncrVersion "New Project Version"

#define kShortcutIDActionExportProject "exportAsGroup"
#define kShortcutDescActionExportProject "Export Project As Group"

#define kShortcutIDActionPreferences "preferences"
#define kShortcutDescActionPreferences "Preferences..."

#define kShortcutIDActionQuit "quit"
#define kShortcutDescActionQuit "Quit"

#define kShortcutIDActionProjectSettings "projectSettings"
#define kShortcutDescActionProjectSettings "Project Settings..."

#define kShortcutIDActionShowOFXLog "showOFXLog"
#define kShortcutDescActionShowOFXLog "Error Log..."

#define kShortcutIDActionShowShortcutEditor "showShortcutEditor"
#define kShortcutDescActionShowShortcutEditor "Shortcuts Editor..."

#define kShortcutIDActionNewViewer "newViewer"
#define kShortcutDescActionNewViewer "New Viewer"

#define kShortcutIDActionFullscreen "fullScreen"
#define kShortcutDescActionFullscreen "Enter Full Screen"

#define kShortcutIDActionClearDiskCache "clearDiskCache"
#define kShortcutDescActionClearDiskCache "Clear Disk Cache"

#define kShortcutIDActionClearPlaybackCache "clearPlaybackCache"
#define kShortcutDescActionClearPlaybackCache "Clear Playback Cache"

#define kShortcutIDActionClearNodeCache "clearNodeCache"
#define kShortcutDescActionClearNodeCache "Clear Per-Node Cache"

#define kShortcutIDActionClearPluginsLoadCache "clearPluginsCache"
#define kShortcutDescActionClearPluginsLoadCache "Clear Plug-ins Load Cache"

#define kShortcutIDActionClearAllCaches "clearAllCaches"
#define kShortcutDescActionClearAllCaches "Clear All Caches"

#define kShortcutIDActionShowAbout "showAbout"
#define kShortcutDescActionShowAbout "About..."

#define kShortcutIDActionRenderSelected "renderSelect"
#define kShortcutDescActionRenderSelected "Render Selected Writers"

#define kShortcutIDActionRenderAll "renderAll"
#define kShortcutDescActionRenderAll "Render All Writers"

#define kShortcutIDActionConnectViewerToInput1 "connectViewerInput1"
#define kShortcutDescActionConnectViewerToInput1 "Connect Viewer to Input 1"

#define kShortcutIDActionConnectViewerToInput2 "connectViewerInput2"
#define kShortcutDescActionConnectViewerToInput2 "Connect Viewer to Input 2"

#define kShortcutIDActionConnectViewerToInput3 "connectViewerInput3"
#define kShortcutDescActionConnectViewerToInput3 "Connect Viewer to Input 3"

#define kShortcutIDActionConnectViewerToInput4 "connectViewerInput4"
#define kShortcutDescActionConnectViewerToInput4 "Connect Viewer to Input 4"

#define kShortcutIDActionConnectViewerToInput5 "connectViewerInput5"
#define kShortcutDescActionConnectViewerToInput5 "Connect Viewer to Input 5"

#define kShortcutIDActionConnectViewerToInput6 "connectViewerInput6"
#define kShortcutDescActionConnectViewerToInput6 "Connect Viewer to Input 6"

#define kShortcutIDActionConnectViewerToInput7 "connectViewerInput7"
#define kShortcutDescActionConnectViewerToInput7 "Connect Viewer to Input 7"

#define kShortcutIDActionConnectViewerToInput8 "connectViewerInput8"
#define kShortcutDescActionConnectViewerToInput8 "Connect Viewer to Input 8"

#define kShortcutIDActionConnectViewerToInput9 "connectViewerInput9"
#define kShortcutDescActionConnectViewerToInput9 "Connect Viewer to Input 9"

#define kShortcutIDActionConnectViewerToInput10 "connectViewerInput10"
#define kShortcutDescActionConnectViewerToInput10 "Connect Viewer to Input 10"

#define kShortcutIDActionShowPaneFullScreen "showPaneFullScreen"
#define kShortcutDescActionShowPaneFullScreen "Show Pane Full Screen"

#define kShortcutIDActionImportLayout "importLayout"
#define kShortcutDescActionImportLayout "Import Layout..."

#define kShortcutIDActionExportLayout "exportLayout"
#define kShortcutDescActionExportLayout "Export Layout..."

#define kShortcutIDActionDefaultLayout "restoreDefaultLayout"
#define kShortcutDescActionDefaultLayout "Restore Default Layout"

#define kShortcutIDActionNextTab "nextTab"
#define kShortcutDescActionNextTab "Next Tab"

#define kShortcutIDActionCloseTab "closeTab"
#define kShortcutDescActionCloseTab "Close Tab"

/////////VIEWER SHORTCUTS
#define kShortcutIDActionLuminance "luminance"
#define kShortcutDescActionLuminance "Luminance"

#define kShortcutIDActionR "channelR"
#define kShortcutDescActionR "Red"

#define kShortcutIDActionG "channelG"
#define kShortcutDescActionG "Green"

#define kShortcutIDActionB "channelB"
#define kShortcutDescActionB "Blue"

#define kShortcutIDActionA "channelA"
#define kShortcutDescActionA "Alpha"

#define kShortcutIDActionFitViewer "fitViewer"
#define kShortcutDescActionFitViewer "Fit Image to Viewer"

#define kShortcutIDActionClipEnabled "clipEnabled"
#define kShortcutDescActionClipEnabled "Enable Clipping to Project Window"

#define kShortcutIDActionRefresh "refresh"
#define kShortcutDescActionRefresh "Refresh Image"

#define kShortcutIDActionROIEnabled "userRoiEnabled"
#define kShortcutDescActionROIEnabled "Enable User RoI"

#define kShortcutIDActionProxyEnabled "proxyEnabled"
#define kShortcutDescActionProxyEnabled "Enable Proxy Rendering"

#define kShortcutIDActionProxyLevel2 "proxy2"
#define kShortcutDescActionProxyLevel2 "2"

#define kShortcutIDActionProxyLevel4 "proxy4"
#define kShortcutDescActionProxyLevel4 "4"

#define kShortcutIDActionProxyLevel8 "proxy8"
#define kShortcutDescActionProxyLevel8 "8"

#define kShortcutIDActionProxyLevel16 "proxy16"
#define kShortcutDescActionProxyLevel16 "16"

#define kShortcutIDActionProxyLevel32 "proxy32"
#define kShortcutDescActionProxyLevel32 "32"

#define kShortcutIDActionZoomLevel100 "100%"
#define kShortcutDescActionZoomLevel100 "100%"

#define kShortcutIDActionHideOverlays "hideOverlays"
#define kShortcutDescActionHideOverlays "Show/Hide Overlays"

#define kShortcutIDActionHidePlayer "hidePlayer"
#define kShortcutDescActionHidePlayer "Show/Hide Player"

#define kShortcutIDActionHideTimeline "hideTimeline"
#define kShortcutDescActionHideTimeline "Show/Hide Timeline"

#define kShortcutIDActionHideLeft "hideLeft"
#define kShortcutDescActionHideLeft "Show/Hide Left Toolbar"

#define kShortcutIDActionHideRight "hideRight"
#define kShortcutDescActionHideRight "Show/Hide Right Toolbar"

#define kShortcutIDActionHideTop "hideTop"
#define kShortcutDescActionHideTop "Show/Hide Top Toolbar"

#define kShortcutIDActionHideInfobar "hideInfo"
#define kShortcutDescActionHideInfobar "Show/Hide info bar"

#define kShortcutIDActionHideAll "hideAll"
#define kShortcutDescActionHideAll "Hide All"

#define kShortcutIDActionShowAll "showAll"
#define kShortcutDescActionShowAll "Show All"

#define kShortcutIDMousePickColor "pick"
#define kShortcutDescMousePickColor "Pick a Color"

#define kShortcutIDMouseRectanglePick "rectanglePick"
#define kShortcutDescMouseRectanglePick "Rectangle Color Picker"

#define kShortcutIDToggleWipe "toggleWipe"
#define kShortcutDescToggleWipe "Toggle Wipe"

///////////PLAYER SHORTCUTS

#define kShortcutIDActionPlayerPrevious "prev"
#define kShortcutDescActionPlayerPrevious "Previous Frame"

#define kShortcutIDActionPlayerNext "next"
#define kShortcutDescActionPlayerNext "Next Frame"

#define kShortcutIDActionPlayerBackward "backward"
#define kShortcutDescActionPlayerBackward "Play Backward"

#define kShortcutIDActionPlayerForward "forward"
#define kShortcutDescActionPlayerForward "Play Forward"

#define kShortcutIDActionPlayerStop "stop"
#define kShortcutDescActionPlayerStop "Stop"

#define kShortcutIDActionPlayerPrevIncr "prevIncr"
#define kShortcutDescActionPlayerPrevIncr "Go to Current Frame Minus Increment"

#define kShortcutIDActionPlayerNextIncr "nextIncr"
#define kShortcutDescActionPlayerNextIncr "Go to Current Frame Plus Increment"

#define kShortcutIDActionPlayerPrevKF "prevKF"
#define kShortcutDescActionPlayerPrevKF "Go to Previous Keyframe"

#define kShortcutIDActionPlayerNextKF "nextKF"
#define kShortcutDescActionPlayerNextKF "Go to Next Keyframe"

#define kShortcutIDActionPlayerFirst "first"
#define kShortcutDescActionPlayerFirst "Go to First Frame"

#define kShortcutIDActionPlayerLast "last"
#define kShortcutDescActionPlayerLast "Go to Last Frame"


///////////ROTO SHORTCUTS
#define kShortcutIDActionRotoDelete "delete"
#define kShortcutDescActionRotoDelete "Delete Element"

#define kShortcutIDActionRotoCloseBezier "closeBezier"
#define kShortcutDescActionRotoCloseBezier "Close Bezier"

#define kShortcutIDActionRotoSelectAll "selectAll"
#define kShortcutDescActionRotoSelectAll "Select All"

#define kShortcutIDActionRotoSelectionTool "selectionTool"
#define kShortcutDescActionRotoSelectionTool "Switch to Selection Mode"

#define kShortcutIDActionRotoAddTool "addTool"
#define kShortcutDescActionRotoAddTool "Switch to Add Mode"

#define kShortcutIDActionRotoEditTool "editTool"
#define kShortcutDescActionRotoEditTool "Switch to Edition Mode"

#define kShortcutIDActionRotoBrushTool "brushTool"
#define kShortcutDescActionRotoBrushTool "Switch to Brush Mode"

#define kShortcutIDActionRotoCloneTool "cloneTool"
#define kShortcutDescActionRotoCloneTool "Switch to Clone Mode"

#define kShortcutIDActionRotoEffectTool "EffectTool"
#define kShortcutDescActionRotoEffectTool "Switch to Effect Mode"

#define kShortcutIDActionRotoColorTool "colorTool"
#define kShortcutDescActionRotoColorTool "Switch to Color Mode"

#define kShortcutIDActionRotoNudgeLeft "nudgeLeft"
#define kShortcutDescActionRotoNudgeLeft "Move Bezier to the Left"

#define kShortcutIDActionRotoNudgeRight "nudgeRight"
#define kShortcutDescActionRotoNudgeRight "Move Bezier to the Right"

#define kShortcutIDActionRotoNudgeBottom "nudgeBottom"
#define kShortcutDescActionRotoNudgeBottom "Move Bezier to the Bottom"

#define kShortcutIDActionRotoNudgeTop "nudgeTop"
#define kShortcutDescActionRotoNudgeTop "Move Bezier to the Top"

#define kShortcutIDActionRotoSmooth "smooth"
#define kShortcutDescActionRotoSmooth "Smooth Bezier"

#define kShortcutIDActionRotoCuspBezier "cusp"
#define kShortcutDescActionRotoCuspBezier "Cusp Bezier"

#define kShortcutIDActionRotoRemoveFeather "rmvFeather"
#define kShortcutDescActionRotoRemoveFeather "Remove Feather"

#define kShortcutIDActionRotoLinkToTrack "linkToTrack"
#define kShortcutDescActionRotoLinkToTrack "Link to Track"

#define kShortcutIDActionRotoUnlinkToTrack "unlinkFromTrack"
#define kShortcutDescActionRotoUnlinkToTrack "Unlink from Track"

#define kShortcutIDActionRotoLockCurve "lock"
#define kShortcutDescActionRotoLockCurve "Lock Shape"

///////////TRACKING SHORTCUTS
#define kShortcutIDActionTrackingSelectAll "selectAll"
#define kShortcutDescActionTrackingSelectAll "Select All Tracks"

#define kShortcutIDActionTrackingDelete "delete"
#define kShortcutDescActionTrackingDelete "Remove Tracks"

#define kShortcutIDActionTrackingBackward "backward"
#define kShortcutDescActionTrackingBackward "Track Backward"

#define kShortcutIDActionTrackingForward "forward"
#define kShortcutDescActionTrackingForward "Track Forward"

#define kShortcutIDActionTrackingPrevious "prev"
#define kShortcutDescActionTrackingPrevious "Track to Previous Frame"

#define kShortcutIDActionTrackingNext "next"
#define kShortcutDescActionTrackingNext "Track to Next Frame"

#define kShortcutIDActionTrackingStop "stop"
#define kShortcutDescActionTrackingStop "Stop Tracking"

///////////NODEGRAPH SHORTCUTS
#define kShortcutIDActionGraphCreateReader "createReader"
#define kShortcutDescActionGraphCreateReader "Create Teader"

#define kShortcutIDActionGraphCreateWriter "createWriter"
#define kShortcutDescActionGraphCreateWriter "Create Writer"

#define kShortcutIDActionGraphRearrangeNodes "rearrange"
#define kShortcutDescActionGraphRearrangeNodes "Rearrange Nodes"

#define kShortcutIDActionGraphRemoveNodes "remove"
#define kShortcutDescActionGraphRemoveNodes "Remove Nodes"

#define kShortcutIDActionGraphShowExpressions "displayExp"
#define kShortcutDescActionGraphShowExpressions "Show Expressions Links"

#define kShortcutIDActionGraphNavigateUpstream "navigateUp"
#define kShortcutDescActionGraphNavigateUpstream "Navigate Tree Upward"

#define kShortcutIDActionGraphNavigateDownstream "navigateDown"
#define kShortcutDescActionGraphNavigateDownstram "Navigate Tree Downward"

#define kShortcutIDActionGraphSelectUp "selUp"
#define kShortcutDescActionGraphSelectUp "Select Tree Upward"

#define kShortcutIDActionGraphSelectDown "selDown"
#define kShortcutDescActionGraphSelectDown "Select Tree Downward"

#define kShortcutIDActionGraphSelectAll "selectAll"
#define kShortcutDescActionGraphSelectAll "Select All Nodes"

#define kShortcutIDActionGraphSelectAllVisible "selectAllVisible"
#define kShortcutDescActionGraphSelectAllVisible "Select All Visible Nodes"

#define kShortcutIDActionGraphEnableHints "hints"
#define kShortcutDescActionGraphEnableHints "Enable Connection Hints"

#define kShortcutIDActionGraphAutoHideInputs "autoHideInputs"
#define kShortcutDescActionGraphAutoHideInputs "Auto-Hide Masks Inputs"

#define kShortcutIDActionGraphSwitchInputs "switchInputs"
#define kShortcutDescActionGraphSwitchInputs "Switch Inputs 1 and 2"

#define kShortcutIDActionGraphCopy "copy"
#define kShortcutDescActionGraphCopy "Copy Nodes"

#define kShortcutIDActionGraphPaste "paste"
#define kShortcutDescActionGraphPaste "Paste Nodes"

#define kShortcutIDActionGraphClone "clone"
#define kShortcutDescActionGraphClone "Clone Nodes"

#define kShortcutIDActionGraphDeclone "declone"
#define kShortcutDescActionGraphDeclone "De-clone Nodes"

#define kShortcutIDActionGraphCut "cut"
#define kShortcutDescActionGraphCut "Cut Nodes"

#define kShortcutIDActionGraphDuplicate "duplicate"
#define kShortcutDescActionGraphDuplicate "Duplicate Nodes"

#define kShortcutIDActionGraphDisableNodes "disable"
#define kShortcutDescActionGraphDisableNodes "Disable Nodes"

#define kShortcutIDActionGraphToggleAutoPreview "toggleAutoPreview"
#define kShortcutDescActionGraphToggleAutoPreview "Toggle Auto Previews"

#define kShortcutIDActionGraphToggleAutoTurbo "toggleAutoTurbo"
#define kShortcutDescActionGraphToggleAutoTurbo "Toggle Auto Turbo"

#define kShortcutIDActionGraphTogglePreview "togglePreview"
#define kShortcutDescActionGraphTogglePreview "Toggle Preview Images"

#define kShortcutIDActionGraphForcePreview "preview"
#define kShortcutDescActionGraphForcePreview "Refresh Preview Images"

#define kShortcutIDActionGraphShowCacheSize "cacheSize"
#define kShortcutDescActionGraphShowCacheSize "Diplay Cache Memory Consumption"


#define kShortcutIDActionGraphFrameNodes "frameNodes"
#define kShortcutDescActionGraphFrameNodes "Center on All Nodes"

#define kShortcutIDActionGraphFindNode "findNode"
#define kShortcutDescActionGraphFindNode "Find"

#define kShortcutIDActionGraphRenameNode "renameNode"
#define kShortcutDescActionGraphRenameNode "Rename Node"

#define kShortcutIDActionGraphExtractNode "extractNode"
#define kShortcutDescActionGraphExtractNode "Extract Node"

#define kShortcutIDActionGraphMakeGroup "makeGroup"
#define kShortcutDescActionGraphMakeGroup "Group From Selection"

#define kShortcutIDActionGraphExpandGroup "expandGroup"
#define kShortcutDescActionGraphExpandGroup "Expand group"

///////////CURVEEDITOR SHORTCUTS
#define kShortcutIDActionCurveEditorRemoveKeys "remove"
#define kShortcutDescActionCurveEditorRemoveKeys "Delete Keyframes"

#define kShortcutIDActionCurveEditorConstant "constant"
#define kShortcutDescActionCurveEditorConstant "Constant Interpolation"

#define kShortcutIDActionCurveEditorLinear "linear"
#define kShortcutDescActionCurveEditorLinear "Linear Interpolation"

#define kShortcutIDActionCurveEditorSmooth "smooth"
#define kShortcutDescActionCurveEditorSmooth "Smooth Interpolation"

#define kShortcutIDActionCurveEditorCatmullrom "catmullrom"
#define kShortcutDescActionCurveEditorCatmullrom "Catmull-Rom Interpolation"

#define kShortcutIDActionCurveEditorCubic "cubic"
#define kShortcutDescActionCurveEditorCubic "Cubic Interpolation"

#define kShortcutIDActionCurveEditorHorizontal "horiz"
#define kShortcutDescActionCurveEditorHorizontal "Horizontal Interpolation"

#define kShortcutIDActionCurveEditorBreak "break"
#define kShortcutDescActionCurveEditorBreak "Break Curvature"

#define kShortcutIDActionCurveEditorSelectAll "selectAll"
#define kShortcutDescActionCurveEditorSelectAll "Select All Keyframes"

#define kShortcutIDActionCurveEditorCenter "center"
#define kShortcutDescActionCurveEditorCenter "Center on Curve"

#define kShortcutIDActionCurveEditorCopy "copy"
#define kShortcutDescActionCurveEditorCopy "Copy Keyframes"

#define kShortcutIDActionCurveEditorPaste "paste"
#define kShortcutDescActionCurveEditorPaste "Paste Keyframes"

// Dope Sheet Editor shortcuts
#define kShortcutIDActionDopeSheetEditorDeleteKeys "deleteKeys"
#define kShortcutDescActionDopeSheetEditorDeleteKeys "Delete selected keyframes"

#define kShortcutIDActionDopeSheetEditorFrameSelection "frameonselection"
#define kShortcutDescActionDopeSheetEditorFrameSelection "Frame on selection"

#define kShortcutIDActionDopeSheetEditorSelectAllKeyframes "selectallkeyframes"
#define kShortcutDescActionDopeSheetEditorSelectAllKeyframes "Select all keyframes"

#define kShortcutIDActionDopeSheetEditorRenameNode "renamenode"
#define kShortcutDescActionDopeSheetEditorRenameNode "Rename node label"

#define kShortcutIDActionDopeSheetEditorCopySelectedKeyframes "copyselectedkeyframes"
#define kShortcutDescActionDopeSheetEditorCopySelectedKeyframes "Copy selected keyframes"

#define kShortcutIDActionDopeSheetEditorPasteKeyframes "pastekeyframes"
#define kShortcutDescActionDopeSheetEditorPasteKeyframes "Paste keyframes"


inline
QKeySequence
makeKeySequence(const Qt::KeyboardModifiers & modifiers,
                Qt::Key key)
{
    int keys = 0;

    if ( modifiers.testFlag(Qt::ControlModifier) ) {
        keys |= Qt::CTRL;
    }
    if ( modifiers.testFlag(Qt::ShiftModifier) ) {
        keys |= Qt::SHIFT;
    }
    if ( modifiers.testFlag(Qt::AltModifier) ) {
        keys |= Qt::ALT;
    }
    if ( modifiers.testFlag(Qt::MetaModifier) ) {
        keys |= Qt::META;
    }
    if (key != (Qt::Key)0) {
        keys |= key;
    }

    return QKeySequence(keys);
}

///This is tricky to do, what we do is we try to find the native strings of the modifiers
///in the sequence native's string. If we find them, we remove them. The last character
///is then the key symbol, we just have to call seq[0] to retrieve it.
inline void
extractKeySequence(const QKeySequence & seq,
                   Qt::KeyboardModifiers & modifiers,
                   Qt::Key & symbol)
{
    const QString nativeMETAStr = QKeySequence(Qt::META).toString(QKeySequence::NativeText);
    const QString nativeCTRLStr = QKeySequence(Qt::CTRL).toString(QKeySequence::NativeText);
    const QString nativeSHIFTStr = QKeySequence(Qt::SHIFT).toString(QKeySequence::NativeText);
    const QString nativeALTStr = QKeySequence(Qt::ALT).toString(QKeySequence::NativeText);
    QString nativeSeqStr = seq.toString(QKeySequence::NativeText);

    if (nativeSeqStr.indexOf(nativeMETAStr) != -1) {
        modifiers |= Qt::MetaModifier;
        nativeSeqStr = nativeSeqStr.remove(nativeMETAStr);
    }
    if (nativeSeqStr.indexOf(nativeCTRLStr) != -1) {
        modifiers |= Qt::ControlModifier;
        nativeSeqStr = nativeSeqStr.remove(nativeCTRLStr);
    }
    if (nativeSeqStr.indexOf(nativeSHIFTStr) != -1) {
        modifiers |= Qt::ShiftModifier;
        nativeSeqStr = nativeSeqStr.remove(nativeSHIFTStr);
    }
    if (nativeSeqStr.indexOf(nativeALTStr) != -1) {
        modifiers |= Qt::AltModifier;
        nativeSeqStr = nativeSeqStr.remove(nativeALTStr);
    }

    ///The nativeSeqStr now contains only the symbol
    QKeySequence newSeq(nativeSeqStr,QKeySequence::NativeText);
    if (newSeq.count() > 0) {
        symbol = (Qt::Key)newSeq[0];
    } else {
        symbol = (Qt::Key)0;
    }
}

class BoundAction
{
public:

    bool editable;
    QString grouping; //< the grouping of the action, such as CurveEditor/
    QString description; //< the description that will be in the shortcut editor
    Qt::KeyboardModifiers modifiers; //< the keyboard modifiers that must be held down during the action
    Qt::KeyboardModifiers defaultModifiers; //< the default keyboard modifiers

    BoundAction()
        : editable(true)
    {
    }

    virtual ~BoundAction()
    {
    }
};

class KeyBoundAction
    : public BoundAction
{
public:

    Qt::Key currentShortcut; //< the actual shortcut for the keybind
    Qt::Key defaultShortcut; //< the default shortcut proposed by the dev team
    std::list<QAction*> actions; //< list of actions using this shortcut

    KeyBoundAction()
        : BoundAction()
        , currentShortcut(Qt::Key_unknown)
        , defaultShortcut(Qt::Key_unknown)
    {
    }

    virtual ~KeyBoundAction()
    {
    }

    void updateActionsShortcut()
    {
        for (std::list<QAction*>::iterator it = actions.begin(); it != actions.end(); ++it) {
            (*it)->setShortcut( makeKeySequence(modifiers, currentShortcut) );
        }
    }
};

class MouseAction
    : public BoundAction
{
public:

    Qt::MouseButton button; //< the button that must be held down for the action. This cannot be edited!

    MouseAction()
        : BoundAction()
        , button(Qt::NoButton)
    {
    }

    virtual ~MouseAction()
    {
    }
};


///All the shortcuts of a group matched against their
///internal id to find and match the action in the event handlers
typedef std::map<QString,BoundAction*> GroupShortcuts;

///All groups shortcuts mapped against the name of the group
typedef std::map<QString,GroupShortcuts> AppShortcuts;

class ActionWithShortcut
: public QAction
{
    QString _group;
    QString _actionID;
    
public:
    
    ActionWithShortcut(const QString & group,
                       const QString & actionID,
                       const QString & actionDescription,
                       QObject* parent);
    
    virtual ~ActionWithShortcut();};

#endif // ACTIONSHORTCUTS_H
