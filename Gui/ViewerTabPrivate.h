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

#ifndef Gui_ViewerTabPrivate_h
#define Gui_ViewerTabPrivate_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <map>
#include <set>

#include "Global/Macros.h"

#include <QtCore/QMutex>

#include "Global/Enums.h"

#include "Gui/ComboBox.h"

class QWidget;
class QHBoxLayout;
class QVBoxLayout;
class QCheckBox;

namespace Natron {
    class Node;
    class Label;
    class ClickableLabel;
    class EffectInstance;
    class ImageComponents;
}
class ViewerGL;
class GuiAppInstance;
class ComboBox;
class Button;
class SpinBox;
class ChannelsComboBox;
class ScaleSliderQWidget;
class InfoViewerWidget;
class LineEdit;
class TimeLineGui;
namespace Transform {
    struct Matrix3x3;
}
class Gui;
class ViewerInstance;
class NodeGui;
class RotoGui;
class TrackerGui;

#define NATRON_TRANSFORM_AFFECTS_OVERLAYS


using namespace Natron;


struct ViewerTabPrivate
{
    struct InputName
    {
        QString name;
        boost::weak_ptr<Natron::Node> input;
    };

    typedef std::map<int,InputName> InputNamesMap;

    /*OpenGL viewer*/
    ViewerGL* viewer;
    GuiAppInstance* app;
    QWidget* viewerContainer;
    QHBoxLayout* viewerLayout;
    QWidget* viewerSubContainer;
    QVBoxLayout* viewerSubContainerLayout;
    QVBoxLayout* mainLayout;

    /*Viewer Settings*/
    QWidget* firstSettingsRow, *secondSettingsRow;
    QHBoxLayout* firstRowLayout, *secondRowLayout;

    /*1st row*/
    //ComboBox* viewerLayers;
    ComboBox* layerChoice;
    ComboBox* alphaChannelChoice;
    ChannelsComboBox* viewerChannels;
    ComboBox* zoomCombobox;
    Button* syncViewerButton;
    Button* centerViewerButton;
    Button* clipToProjectFormatButton;
    Button* enableViewerRoI;
    Button* refreshButton;
    QIcon iconRefreshOff, iconRefreshOn;
    int ongoingRenderCount;
    
    Button* activateRenderScale;
    bool renderScaleActive;
    ComboBox* renderScaleCombo;
    Natron::Label* firstInputLabel;
    ComboBox* firstInputImage;
    Natron::Label* compositingOperatorLabel;
    ComboBox* compositingOperator;
    Natron::Label* secondInputLabel;
    ComboBox* secondInputImage;

    /*2nd row*/
    Button* toggleGainButton;
    SpinBox* gainBox;
    ScaleSliderQWidget* gainSlider;
    double lastFstopValue;
    ClickableLabel* autoConstrastLabel;
    QCheckBox* autoContrast;
    SpinBox* gammaBox;
    double lastGammaValue;
    Button* toggleGammaButton;
    ScaleSliderQWidget* gammaSlider;
    ComboBox* viewerColorSpace;
    Button* checkerboardButton;
    Button* pickerButton;
    ComboBox* viewsComboBox;
    int currentViewIndex;
    QMutex currentViewMutex;
    /*Info*/
    InfoViewerWidget* infoWidget[2];


    /*TimeLine buttons*/
    QWidget* playerButtonsContainer;
    QHBoxLayout* playerLayout;
    SpinBox* currentFrameBox;
    Button* firstFrame_Button;
    Button* previousKeyFrame_Button;
    Button* play_Backward_Button;
    Button* previousFrame_Button;
    Button* stop_Button;
    Button* nextFrame_Button;
    Button* play_Forward_Button;
    Button* nextKeyFrame_Button;
    Button* lastFrame_Button;
    Button* previousIncrement_Button;
    SpinBox* incrementSpinBox;
    Button* nextIncrement_Button;
    Button* playbackMode_Button;
    
    mutable QMutex playbackModeMutex;
    Natron::PlaybackModeEnum playbackMode;
    
    LineEdit* frameRangeEdit;

    ClickableLabel* canEditFrameRangeLabel;
    Button* tripleSyncButton;
    
    QCheckBox* canEditFpsBox;
    ClickableLabel* canEditFpsLabel;
    mutable QMutex fpsLockedMutex;
    bool fpsLocked;
    SpinBox* fpsBox;
    Button* turboButton;

    /*frame seeker*/
    TimeLineGui* timeLineGui;
    std::map<NodeGui*,RotoGui*> rotoNodes;
    std::pair<NodeGui*,RotoGui*> currentRoto;
    std::map<NodeGui*,TrackerGui*> trackerNodes;
    std::pair<NodeGui*,TrackerGui*> currentTracker;
    InputNamesMap inputNamesMap;
    mutable QMutex compOperatorMutex;
    ViewerCompositingOperatorEnum compOperator;
    Gui* gui;
    ViewerInstance* viewerNode; // < pointer to the internal node
    
    mutable QMutex visibleToolbarsMutex; //< protects the 4 bool below
    bool infobarVisible;
    bool playerVisible;
    bool timelineVisible;
    bool leftToolbarVisible;
    bool rightToolbarVisible;
    bool topToolbarVisible;
    
    bool isFileDialogViewer;
    
    mutable QMutex checkerboardMutex;
    bool checkerboardEnabled;

    mutable QMutex fpsMutex;
    double fps;
    
    //The last node that took the penDown/motion/keyDown/keyRelease etc...
    boost::weak_ptr<Natron::Node> lastOverlayNode;
    
    ViewerTabPrivate(Gui* gui,
                     ViewerInstance* node);

#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
    // return the tronsform to apply to the overlay as a 3x3 homography in canonical coordinates
    bool getOverlayTransform(double time,
                             int view,
                             const boost::shared_ptr<Natron::Node>& target,
                             Natron::EffectInstance* currentNode,
                             Transform::Matrix3x3* transform) const;

    bool getTimeTransform(double time,
                          int view,
                          const boost::shared_ptr<Natron::Node>& target,
                          Natron::EffectInstance* currentNode,
                          double *newTime) const;

#endif

    void getComponentsAvailabel(std::set<ImageComponents>* comps) const;

};
#endif // Gui_ViewerTabPrivate_h
