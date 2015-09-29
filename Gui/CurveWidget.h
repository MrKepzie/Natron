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

#ifndef CURVE_WIDGET_H
#define CURVE_WIDGET_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <set>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtOpenGL/QGLWidget>
#include <QMetaType>
#include <QDialog>
#include <QByteArray>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"
#include "Gui/CurveEditorUndoRedo.h"
#include "Engine/OverlaySupport.h"
#include "Engine/Curve.h"

class CurveSelection;
class QFont;
class Variant;
class TimeLine;
class KnobGui;
class CurveWidget;
class Button;
class LineEdit;
class SpinBox;
class Gui;
class Bezier;
class RotoContext;
class QVBoxLayout;
class QHBoxLayout;
namespace Natron {
    class Label;
}


class CurveWidgetPrivate;

class CurveWidget
    : public QGLWidget, public OverlaySupport
{
    friend class CurveGui;
    friend class CurveWidgetPrivate;

GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    /*Pass a null timeline ptr if you don't want interaction with the global timeline. */
    CurveWidget(Gui* gui,
                CurveSelection* selection,
                boost::shared_ptr<TimeLine> timeline = boost::shared_ptr<TimeLine>(),
                QWidget* parent = NULL,
                const QGLWidget* shareWidget = NULL);

    virtual ~CurveWidget() OVERRIDE;

    const QFont & getTextFont() const;

    void centerOn(double xmin, double xmax);
    void centerOn(double xmin,double xmax,double ymin,double ymax);

    void addCurveAndSetColor(const boost::shared_ptr<CurveGui>& curve);

    void removeCurve(CurveGui* curve);

    void centerOn(const std::vector<boost::shared_ptr<CurveGui> > & curves);

    void showCurvesAndHideOthers(const std::vector<boost::shared_ptr<CurveGui> > & curves);

    void getVisibleCurves(std::vector<boost::shared_ptr<CurveGui> >* curves) const;

    void setSelectedKeys(const SelectedKeys & keys);
    
    bool isSelectedKey(const boost::shared_ptr<CurveGui>& curve, double time) const;

    void refreshSelectedKeys();

    double getZoomFactor() const;

    void getProjection(double *zoomLeft, double *zoomBottom, double *zoomFactor, double *zoomAspectRatio) const;

    void setProjection(double zoomLeft, double zoomBottom, double zoomFactor, double zoomAspectRatio);

    /**
     * @brief Swap the OpenGL buffers.
     **/
    virtual void swapOpenGLBuffers() OVERRIDE FINAL;

    /**
     * @brief Repaint
     **/
    virtual void redraw() OVERRIDE FINAL;

    /**
     * @brief Returns the width and height of the viewport in window coordinates.
     **/
    virtual void getViewportSize(double &width, double &height) const OVERRIDE FINAL;

    /**
     * @brief Returns the pixel scale of the viewport.
     **/
    virtual void getPixelScale(double & xScale, double & yScale) const OVERRIDE FINAL;

    /**
     * @brief Returns the colour of the background (i.e: clear color) of the viewport.
     **/
    virtual void getBackgroundColour(double &r, double &g, double &b) const OVERRIDE FINAL;

    virtual unsigned int getCurrentRenderScale() const OVERRIDE FINAL { return 0; }
    
    virtual void saveOpenGLContext() OVERRIDE FINAL;
    virtual void restoreOpenGLContext() OVERRIDE FINAL;
    
    void pushUndoCommand(QUndoCommand* cmd);

public Q_SLOTS:

    void refreshDisplayedTangents();

    void exportCurveToAscii();

    void importCurveFromAscii();

    void deleteSelectedKeyFrames();

    void copySelectedKeyFrames();

    void pasteKeyFramesFromClipBoardToSelectedCurve();

    void selectAllKeyFrames();

    void constantInterpForSelectedKeyFrames();

    void linearInterpForSelectedKeyFrames();

    void smoothForSelectedKeyFrames();

    void catmullromInterpForSelectedKeyFrames();

    void cubicInterpForSelectedKeyFrames();

    void horizontalInterpForSelectedKeyFrames();

    void breakDerivativesForSelectedKeyFrames();

    void frameSelectedCurve();
    
    void loopSelectedCurve();
    
    void negateSelectedCurve();
    
    void reverseSelectedCurve();

    void refreshSelectedKeysBbox();

    void onTimeLineFrameChanged(SequenceTime time, int reason);

    void onTimeLineBoundariesChanged(int,int);

    void onCurveChanged();

    void onUpdateOnPenUpActionTriggered();

    void onEditKeyFrameDialogFinished();
private:

    virtual void initializeGL() OVERRIDE FINAL;
    virtual void resizeGL(int width,int height) OVERRIDE FINAL;
    virtual void paintGL() OVERRIDE FINAL;
    virtual QSize sizeHint() const OVERRIDE FINAL;
    virtual void mousePressEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseDoubleClickEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseReleaseEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void mouseMoveEvent(QMouseEvent* e) OVERRIDE FINAL;
    virtual void wheelEvent(QWheelEvent* e) OVERRIDE FINAL;
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void focusInEvent(QFocusEvent* e) OVERRIDE FINAL;

    void renderText(double x,double y,const QString & text,const QColor & color,const QFont & font) const;

    /**
     *@brief See toZoomCoordinates in ViewerGL.h
     **/
    QPointF toZoomCoordinates(double x, double y) const;

    /**
     *@brief See toWidgetCoordinates in ViewerGL.h
     **/
    QPointF toWidgetCoordinates(double x, double y) const;

    const QColor & getSelectedCurveColor() const;
    const QFont & getFont() const;
    const SelectedKeys & getSelectedKeyFrames() const;

    bool isSupportingOpenGLVAO() const;

private:


    boost::scoped_ptr<CurveWidgetPrivate> _imp;
};

Q_DECLARE_METATYPE(CurveWidget*)



#endif // CURVE_WIDGET_H
