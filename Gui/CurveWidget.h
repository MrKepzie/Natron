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

#ifndef CURVE_WIDGET_H
#define CURVE_WIDGET_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <set>

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtOpenGL/QGLWidget>
#include <QMetaType>
#include <QDialog>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif
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

class CurveGui
    : public QObject
{
    Q_OBJECT

public:

  
    CurveGui(const CurveWidget *curveWidget,
             boost::shared_ptr<Curve>  curve,
             const QString & name,
             const QColor & color,
             int thickness = 1);

    virtual ~CurveGui() OVERRIDE;

    const QString & getName() const
    {
        return _name;
    }

    void setName(const QString & name)
    {
        _name = name;
    }

    const QColor & getColor() const
    {
        return _color;
    }

    void setColor(const QColor & color)
    {
        _color = color;
    }

    int getThickness() const
    {
        return _thickness;
    }

    void setThickness(int t)
    {
        _thickness = t;
    }

    bool isVisible() const
    {
        return _visible;
    }

    void setVisibleAndRefresh(bool visible);

    /**
     * @brief same as setVisibleAndRefresh() but doesn't Q_EMIT any signal for a repaint
     **/
    void setVisible(bool visible);

    bool isSelected() const
    {
        return _selected;
    }

    void setSelected(bool s)
    {
        _selected = s;
    }


    /**
     * @brief Evaluates the curve and returns the y position corresponding to the given x.
     * The coordinates are those of the curve, not of the widget.
     **/
    virtual double evaluate(bool useExpr, double x) const = 0;
    
    virtual boost::shared_ptr<Curve>  getInternalCurve() const;

    void drawCurve(int curveIndex,int curvesCount);

    virtual std::pair<double,double> getCurveYRange() const;

    virtual bool areKeyFramesTimeClampedToIntegers() const;
    virtual bool areKeyFramesValuesClampedToBooleans() const;
    virtual bool areKeyFramesValuesClampedToIntegers() const;
    virtual bool isYComponentMovable() const;
    virtual KeyFrameSet getKeyFrames() const;
    virtual int getKeyFrameIndex(double time) const = 0;
    virtual void setKeyFrameInterpolation(Natron::KeyframeTypeEnum interp,int index) = 0;
    
Q_SIGNALS:

    void curveChanged();
    
    

private:

    std::pair<KeyFrame,bool> nextPointForSegment(double x1,double* x2,const KeyFrameSet & keyframes);
    
protected:
    
    boost::shared_ptr<Curve> _internalCurve; ///ptr to the internal curve
    
private:
    
    QString _name; /// the name of the curve
    QColor _color; /// the color that must be used to draw the curve
    int _thickness; /// its thickness
    bool _visible; /// should we draw this curve ?
    bool _selected; /// is this curve selected
    const CurveWidget* _curveWidget;
   
};

class KnobCurveGui : public CurveGui
{
    
public:
    
    
    KnobCurveGui(const CurveWidget *curveWidget,
             boost::shared_ptr<Curve>  curve,
             KnobGui* knob,
             int dimension,
             const QString & name,
             const QColor & color,
             int thickness = 1);
    
    
    KnobCurveGui(const CurveWidget *curveWidget,
                 boost::shared_ptr<Curve>  curve,
                 const boost::shared_ptr<KnobI>& knob,
                 const boost::shared_ptr<RotoContext>& roto,
                 int dimension,
                 const QString & name,
                 const QColor & color,
                 int thickness = 1);
    
    virtual ~KnobCurveGui();
    
    virtual boost::shared_ptr<Curve>  getInternalCurve() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    KnobGui* getKnobGui() const
    {
        return _knob;
    }
    
    virtual double evaluate(bool useExpr,double x) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    boost::shared_ptr<RotoContext> getRotoContext() const { return _roto; }
    
    boost::shared_ptr<KnobI> getInternalKnob() const;
    
    int getDimension() const
    {
        return _dimension;
    }
    
    virtual int getKeyFrameIndex(double time) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setKeyFrameInterpolation(Natron::KeyframeTypeEnum interp,int index) OVERRIDE FINAL;

    
private:
    
    boost::shared_ptr<RotoContext> _roto;
    boost::shared_ptr<KnobI> _internalKnob;
    KnobGui* _knob; //< ptr to the knob holding this curve
    int _dimension; //< which dimension is this curve representing
};

class BezierCPCurveGui : public CurveGui
{
public:
    
    BezierCPCurveGui(const CurveWidget *curveWidget,
                 const boost::shared_ptr<Bezier>& bezier,
                 const boost::shared_ptr<RotoContext>& roto,
                 const QString & name,
                 const QColor & color,
                 int thickness = 1);
    
    virtual ~BezierCPCurveGui();
    
    boost::shared_ptr<RotoContext> getRotoContext() const { return _rotoContext; }
    
    boost::shared_ptr<Bezier> getBezier() const ;
    
    virtual double evaluate(bool useExpr,double x) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual std::pair<double,double> getCurveYRange() const;

    virtual bool areKeyFramesTimeClampedToIntegers() const { return true; }
    virtual bool areKeyFramesValuesClampedToBooleans() const { return false; }
    virtual bool areKeyFramesValuesClampedToIntegers() const { return true; }
    virtual bool isYComponentMovable() const { return false; }
    virtual KeyFrameSet getKeyFrames() const;
    
    virtual int getKeyFrameIndex(double time) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void setKeyFrameInterpolation(Natron::KeyframeTypeEnum interp,int index) OVERRIDE FINAL;

private:
    
    
    boost::shared_ptr<Bezier> _bezier;
    boost::shared_ptr<RotoContext> _rotoContext;
};


class CurveWidgetPrivate;

class CurveWidget
    : public QGLWidget, public OverlaySupport
{
    friend class CurveGui;
    friend class CurveWidgetPrivate;

    Q_OBJECT

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
    virtual void enterEvent(QEvent* e) OVERRIDE FINAL;
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

    bool isTabVisible() const;

    boost::scoped_ptr<CurveWidgetPrivate> _imp;
};

Q_DECLARE_METATYPE(CurveWidget*)


class ImportExportCurveDialog
    : public QDialog
{
    Q_OBJECT

public:

    ImportExportCurveDialog(bool isExportDialog,
                            const std::vector<boost::shared_ptr<CurveGui> > & curves,
                            Gui* gui,
                            QWidget* parent = 0);

    virtual ~ImportExportCurveDialog()
    {
    }

    QString getFilePath();

    double getXStart() const;

    double getXIncrement() const;

    double getXEnd() const;

    void getCurveColumns(std::map<int,boost::shared_ptr<CurveGui> >* columns) const;

public Q_SLOTS:

    void open_file();

private:
    Gui* _gui;
    bool _isExportDialog;
    QVBoxLayout* _mainLayout;

    //////File
    QWidget* _fileContainer;
    QHBoxLayout* _fileLayout;
    Natron::Label* _fileLabel;
    LineEdit* _fileLineEdit;
    Button* _fileBrowseButton;

    //////x start value
    QWidget* _startContainer;
    QHBoxLayout* _startLayout;
    Natron::Label* _startLabel;
    SpinBox* _startSpinBox;

    //////x increment
    QWidget* _incrContainer;
    QHBoxLayout* _incrLayout;
    Natron::Label* _incrLabel;
    SpinBox* _incrSpinBox;

    //////x end value
    QWidget* _endContainer;
    QHBoxLayout* _endLayout;
    Natron::Label* _endLabel;
    SpinBox* _endSpinBox;


    /////Columns
    struct CurveColumn
    {
        boost::shared_ptr<CurveGui> _curve;
        QWidget* _curveContainer;
        QHBoxLayout* _curveLayout;
        Natron::Label* _curveLabel;
        SpinBox* _curveSpinBox;
    };

    std::vector< CurveColumn > _curveColumns;

    ////buttons
    QWidget* _buttonsContainer;
    QHBoxLayout* _buttonsLayout;
    Button* _okButton;
    Button* _cancelButton;
};


struct EditKeyFrameDialogPrivate;
class EditKeyFrameDialog : public QDialog
{
    
    Q_OBJECT
    
public:
    
    enum EditModeEnum {
        eEditModeKeyframePosition,
        eEditModeLeftDerivative,
        eEditModeRightDerivative
    };
    
    EditKeyFrameDialog(EditModeEnum mode,CurveWidget* curveWidget, const KeyPtr& key,QWidget* parent);
    
    virtual ~EditKeyFrameDialog();
    
Q_SIGNALS:
    
    void valueChanged(int dimension,double value);
    
    
public Q_SLOTS:
    
    void onXSpinBoxValueChanged(double d);
    void onYSpinBoxValueChanged(double d);
    
private:
    
    void moveKeyTo(double newX,double newY);
    void moveDerivativeTo(double d);
    
    virtual void keyPressEvent(QKeyEvent* e) OVERRIDE FINAL;
    virtual void changeEvent(QEvent* e) OVERRIDE FINAL;
    
    boost::scoped_ptr<EditKeyFrameDialogPrivate> _imp;
};

#endif // CURVE_WIDGET_H
