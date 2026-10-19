#include "whiteboard/DrawingController.h"

#include "core/Settings.h"
#include "core/Application.h"

#include "whiteboard/GraphicsScene.h"
#include "whiteboard/BoardController.h"

#include "ui/MainWindow.h"
#include "ui/ActionPalette.h"

DrawingController* DrawingController::sDrawingController = 0;

DrawingController* DrawingController::drawingController()
{
    if(!sDrawingController)
        sDrawingController = new DrawingController();

    return sDrawingController;
}

void DrawingController::destroy()
{
    if(sDrawingController)
        delete sDrawingController;
    sDrawingController = NULL;
}

DrawingController::DrawingController(QObject * parent)
    : QObject(parent)
    , mActiveRuler(NULL)
    , mStylusTool((StylusTool::Enum)-1)
    , mLatestDrawingTool((StylusTool::Enum)-1)
    , mIsDesktopMode(false)
{
    connect(Settings::settings(), SIGNAL(colorContextChanged()), this, SIGNAL(colorPaletteChanged()));

    connect(Application::mainWindow->ui().actionPen, SIGNAL(triggered(bool)), this, SLOT(penToolSelected(bool)));
    connect(Application::mainWindow->ui().actionEraser, SIGNAL(triggered(bool)), this, SLOT(eraserToolSelected(bool)));
    connect(Application::mainWindow->ui().actionMarker, SIGNAL(triggered(bool)), this, SLOT(markerToolSelected(bool)));
    connect(Application::mainWindow->ui().actionSelector, SIGNAL(triggered(bool)), this, SLOT(selectorToolSelected(bool)));
    connect(Application::mainWindow->ui().actionPlay, SIGNAL(triggered(bool)), this, SLOT(playToolSelected(bool)));
    connect(Application::mainWindow->ui().actionHand, SIGNAL(triggered(bool)), this, SLOT(handToolSelected(bool)));
    connect(Application::mainWindow->ui().actionZoomIn, SIGNAL(triggered(bool)), this, SLOT(zoomInToolSelected(bool)));
    connect(Application::mainWindow->ui().actionZoomOut, SIGNAL(triggered(bool)), this, SLOT(zoomOutToolSelected(bool)));
    connect(Application::mainWindow->ui().actionPointer, SIGNAL(triggered(bool)), this, SLOT(pointerToolSelected(bool)));
    connect(Application::mainWindow->ui().actionLine, SIGNAL(triggered(bool)), this, SLOT(lineToolSelected(bool)));
    connect(Application::mainWindow->ui().actionText, SIGNAL(triggered(bool)), this, SLOT(textToolSelected(bool)));
    connect(Application::mainWindow->ui().actionCapture, SIGNAL(triggered(bool)), this, SLOT(captureToolSelected(bool)));

}

DrawingController::~DrawingController()
{
    // NOOP
}

int DrawingController::stylusTool()
{
    return mStylusTool;
}

int DrawingController::latestDrawingTool()
{
    return mLatestDrawingTool;
}

void DrawingController::setStylusTool(int tool)
{
    if (tool != mStylusTool)
    {
        Application::boardController->activeScene()->deselectAllItems();
        if (mStylusTool == StylusTool::Pen || mStylusTool == StylusTool::Marker
                || mStylusTool == StylusTool::Line)
        {
            mLatestDrawingTool = mStylusTool;
        }

        if (tool == StylusTool::Pen || tool == StylusTool::Line)
        {
             emit lineWidthIndexChanged(Settings::settings()->penWidthIndex());
             emit colorIndexChanged(Settings::settings()->penColorIndex());
        }
        else if (tool == StylusTool::Marker)
        {
            emit lineWidthIndexChanged(Settings::settings()->markerWidthIndex());
            emit colorIndexChanged(Settings::settings()->markerColorIndex());
        }

        mStylusTool = (StylusTool::Enum)tool;

        if (mStylusTool == StylusTool::Pen)
            Application::mainWindow->ui().actionPen->setChecked(true);
        else if (mStylusTool == StylusTool::Eraser)
            Application::mainWindow->ui().actionEraser->setChecked(true);
        else if (mStylusTool == StylusTool::Marker)
            Application::mainWindow->ui().actionMarker->setChecked(true);
        else if (mStylusTool == StylusTool::Selector)
            Application::mainWindow->ui().actionSelector->setChecked(true);
        else if (mStylusTool == StylusTool::Play)
            Application::mainWindow->ui().actionPlay->setChecked(true);
        else if (mStylusTool == StylusTool::Hand)
            Application::mainWindow->ui().actionHand->setChecked(true);
        else if (mStylusTool == StylusTool::ZoomIn)
            Application::mainWindow->ui().actionZoomIn->setChecked(true);
        else if (mStylusTool == StylusTool::ZoomOut)
            Application::mainWindow->ui().actionZoomOut->setChecked(true);
        else if (mStylusTool == StylusTool::Pointer)
            Application::mainWindow->ui().actionPointer->setChecked(true);
        else if (mStylusTool == StylusTool::Line)
            Application::mainWindow->ui().actionLine->setChecked(true);
        else if (mStylusTool == StylusTool::Text)
            Application::mainWindow->ui().actionText->setChecked(true);
        else if (mStylusTool == StylusTool::Capture)
            Application::mainWindow->ui().actionCapture->setChecked(true);

        emit stylusToolChanged(tool);
        if (mStylusTool != StylusTool::Selector)
            emit colorPaletteChanged();
    }
}

bool DrawingController::isDrawingTool()
{
    return (stylusTool() == StylusTool::Pen)
            || (stylusTool() == StylusTool::Marker)
            || (stylusTool() == StylusTool::Line);
}

int DrawingController::currentToolWidthIndex()
{
    if (stylusTool() == StylusTool::Pen || stylusTool() == StylusTool::Line)
        return Settings::settings()->penWidthIndex();
    else if (stylusTool() == StylusTool::Marker)
        return Settings::settings()->markerWidthIndex();
    else
        return -1;
}

qreal DrawingController::currentToolWidth()
{
    if (stylusTool() == StylusTool::Pen || stylusTool() == StylusTool::Line)
        return Settings::settings()->currentPenWidth();
    else if (stylusTool() == StylusTool::Marker)
        return Settings::settings()->currentMarkerWidth();
    else
        //failsafe
        return Settings::settings()->currentPenWidth();
}

void DrawingController::setLineWidthIndex(int index)
{
    if (stylusTool() == StylusTool::Marker)
    {
        Settings::settings()->setMarkerWidthIndex(index);
    }
    else
    {
        Settings::settings()->setPenWidthIndex(index);

        if(stylusTool() != StylusTool::Line
            && stylusTool() != StylusTool::Selector)
        {
            setStylusTool(StylusTool::Pen);
        }
    }

    emit lineWidthIndexChanged(index);
}


int DrawingController::currentToolColorIndex()
{
    if (stylusTool() == StylusTool::Pen || stylusTool() == StylusTool::Line)
    {
        return Settings::settings()->penColorIndex();
    }
    else if (stylusTool() == StylusTool::Marker)
    {
        return Settings::settings()->markerColorIndex();
    }
    else
    {
        return -1;
    }
}

QColor DrawingController::currentToolColor()
{
    return toolColor(Settings::settings()->isDarkBackground());
}

QColor DrawingController::toolColor(bool onDarkBackground)
{
    if (stylusTool() == StylusTool::Pen || stylusTool() == StylusTool::Line)
    {
        return Settings::settings()->penColor(onDarkBackground);
    }
    else if (stylusTool() == StylusTool::Marker)
    {
        return Settings::settings()->markerColor(onDarkBackground);
    }
    else
    {
        //failsafe
        if (onDarkBackground)
        {
            return Qt::white;
        }
        else
        {
            return Qt::black;
        }
    }
}

void DrawingController::setColorIndex(int index)
{
    Q_ASSERT(index >= 0 && index < Settings::settings()->colorPaletteSize);

    if (stylusTool() == StylusTool::Marker)
    {
        Settings::settings()->setMarkerColorIndex(index);
    }
    else
    {
        Settings::settings()->setPenColorIndex(index);
    }

    emit colorIndexChanged(index);
}

void DrawingController::setEraserWidthIndex(int index)
{
    setStylusTool(StylusTool::Eraser);
    Settings::settings()->setEraserWidthIndex(index);
}

void DrawingController::setPenColor(bool onDarkBackground, const QColor& color, int pIndex)
{
    if (onDarkBackground)
    {
        Settings::settings()->boardPenDarkBackgroundSelectedColors->setColor(pIndex, color);
    }
    else
    {
        Settings::settings()->boardPenLightBackgroundSelectedColors->setColor(pIndex, color);
    }

    emit colorPaletteChanged();
}

void DrawingController::setMarkerColor(bool onDarkBackground, const QColor& color, int pIndex)
{
    if (onDarkBackground)
    {
        Settings::settings()->boardMarkerDarkBackgroundSelectedColors->setColor(pIndex, color);
    }
    else
    {
        Settings::settings()->boardMarkerLightBackgroundSelectedColors->setColor(pIndex, color);
    }

    emit colorPaletteChanged();
}

void DrawingController::setMarkerAlpha(qreal alpha)
{
    Settings::settings()->boardMarkerLightBackgroundColors->setAlpha(alpha);
    Settings::settings()->boardMarkerLightBackgroundSelectedColors->setAlpha(alpha);

    Settings::settings()->boardMarkerDarkBackgroundColors->setAlpha(alpha);
    Settings::settings()->boardMarkerDarkBackgroundSelectedColors->setAlpha(alpha);

    Settings::settings()->boardMarkerAlpha->set(alpha);

    emit colorPaletteChanged();
}


void DrawingController::penToolSelected(bool checked)
{
    if (checked)
        setStylusTool(StylusTool::Pen);
}

void DrawingController::eraserToolSelected(bool checked)
{
    if (checked)
        setStylusTool(StylusTool::Eraser);
}

void DrawingController::markerToolSelected(bool checked)
{
    if (checked)
        setStylusTool(StylusTool::Marker);

}

void DrawingController::selectorToolSelected(bool checked)
{
    if (checked)
        setStylusTool(StylusTool::Selector);
}

void DrawingController::playToolSelected(bool checked)
{
    if (checked)
        setStylusTool(StylusTool::Play);
}

void DrawingController::handToolSelected(bool checked)
{
    if (checked)
        setStylusTool(StylusTool::Hand);
}


void DrawingController::zoomInToolSelected(bool checked)
{
    if (checked)
        setStylusTool(StylusTool::ZoomIn);
}

void DrawingController::zoomOutToolSelected(bool checked)
{
    if (checked)
        setStylusTool(StylusTool::ZoomOut);
}

void DrawingController::pointerToolSelected(bool checked)
{
    if (checked)
        setStylusTool(StylusTool::Pointer);
}


void DrawingController::lineToolSelected(bool checked)
{
    if (checked)
        setStylusTool(StylusTool::Line);
}

void DrawingController::textToolSelected(bool checked)
{
    if (checked)
        setStylusTool(StylusTool::Text);
}

void DrawingController::captureToolSelected(bool checked)
{
    if (checked)
        setStylusTool(StylusTool::Capture);
}
