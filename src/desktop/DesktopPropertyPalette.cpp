#include "desktop/DesktopPropertyPalette.h"

#include "core/Application.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/DrawingController.h"
#include "ui/MainWindow.h"
#include "ui/ToolbarButtonGroup.h"
#include "ui/RightPalette.h"


DesktopPropertyPalette::DesktopPropertyPalette(QWidget *parent, RightPalette* _rightPalette)
    :PropertyPalette(Qt::Horizontal, parent)
    ,rightPalette(_rightPalette)
{}

int DesktopPropertyPalette::getParentRightOffset()
{
    return rightPalette->width();
}


DesktopPenPalette::DesktopPenPalette(QWidget *parent, RightPalette* rightPalette)
    : DesktopPropertyPalette(parent, rightPalette)
{
    // Setup color choice widget
    QList<QAction *> colorActions;
    colorActions.append(Application::mainWindow->ui().actionColor0);
    colorActions.append(Application::mainWindow->ui().actionColor1);
    colorActions.append(Application::mainWindow->ui().actionColor2);
    colorActions.append(Application::mainWindow->ui().actionColor3);
    colorActions.append(Application::mainWindow->ui().actionColor4);

    ToolbarButtonGroup *colorChoice =
            new ToolbarButtonGroup(Application::mainWindow->ui().boardToolBar, colorActions);

    colorChoice->displayText(false);

    //connect(colorChoice, SIGNAL(activated(int)), this, SLOT(Application::boardController->setColorIndex(int)));
    connect(DrawingController::drawingController(), &DrawingController::colorIndexChanged, colorChoice, &ToolbarButtonGroup::setCurrentIndex);
    connect(DrawingController::drawingController(), &DrawingController::colorIndexChanged, this, [this] { close(); });
    connect(DrawingController::drawingController(), &DrawingController::colorPaletteChanged, colorChoice, &ToolbarButtonGroup::colorPaletteChanged);
    connect(DrawingController::drawingController(), &DrawingController::colorPaletteChanged, this, [this] { close(); });

    layout()->addWidget(colorChoice);

    // Setup line width choice widget
    QList<QAction *> lineWidthActions;
    lineWidthActions.append(Application::mainWindow->ui().actionLineSmall);
    lineWidthActions.append(Application::mainWindow->ui().actionLineMedium);
    lineWidthActions.append(Application::mainWindow->ui().actionLineLarge);

    ToolbarButtonGroup *lineWidthChoice =
            new ToolbarButtonGroup(Application::mainWindow->ui().boardToolBar, lineWidthActions);
    lineWidthChoice->displayText(false);

    connect(lineWidthChoice, &ToolbarButtonGroup::activated, DrawingController::drawingController(), &DrawingController::setLineWidthIndex);
    connect(lineWidthChoice, &ToolbarButtonGroup::activated, this, [this] { close(); });
    connect(DrawingController::drawingController(), &DrawingController::lineWidthIndexChanged, lineWidthChoice, &ToolbarButtonGroup::setCurrentIndex);
    connect(DrawingController::drawingController(), &DrawingController::lineWidthIndexChanged, this, [this] { close(); });

    onParentMaximized();

    layout()->addWidget(lineWidthChoice);
}


void DesktopPenPalette::onButtonReleased()
{
    close();
}

/**
 * \brief Disconnect the released event of the buttons
 */
void DesktopPenPalette::onParentMinimized()
{
    for(int i = 0; i < buttons().size(); i++)
    {
        disconnect(buttons().at(i), &QAbstractButton::released, this, &DesktopPenPalette::onButtonReleased);
    }
}

/**
 * \brief Connect the released event of the buttons
 */
void DesktopPenPalette::onParentMaximized()
{
    for(int i = 0; i < buttons().size(); i++)
    {
        connect(buttons().at(i), &QAbstractButton::released, this, &DesktopPenPalette::onButtonReleased);
    }
}


DesktopEraserPalette::DesktopEraserPalette(QWidget *parent, RightPalette* rightPalette)
    : DesktopPropertyPalette(parent, rightPalette)
{
    // Setup eraser width choice widget
    QList<QAction *> eraserWidthActions;
    eraserWidthActions.append(Application::mainWindow->ui().actionEraserSmall);
    eraserWidthActions.append(Application::mainWindow->ui().actionEraserMedium);
    eraserWidthActions.append(Application::mainWindow->ui().actionEraserLarge);

    ToolbarButtonGroup *eraserWidthChoice = new ToolbarButtonGroup(Application::mainWindow->ui().boardToolBar, eraserWidthActions);

    connect(eraserWidthChoice, &ToolbarButtonGroup::activated, DrawingController::drawingController(), &DrawingController::setEraserWidthIndex);
    connect(eraserWidthChoice, &ToolbarButtonGroup::activated, this, [this] { close(); });
    connect(Application::mainWindow->ui().actionEraseDesktopAnnotations, &QAction::triggered, this, [this] { close(); });

    eraserWidthChoice->displayText(false);
    eraserWidthChoice->setCurrentIndex(Settings::settings()->eraserWidthIndex());

    layout()->addWidget(eraserWidthChoice);

    addAction(Application::mainWindow->ui().actionEraseDesktopAnnotations);
}


DesktopMarkerPalette::DesktopMarkerPalette(QWidget *parent, RightPalette* rightPalette)
    : DesktopPropertyPalette(parent, rightPalette)
{
    // Setup color choice widget
    QList<QAction *> colorActions;
    colorActions.append(Application::mainWindow->ui().actionColor0);
    colorActions.append(Application::mainWindow->ui().actionColor1);
    colorActions.append(Application::mainWindow->ui().actionColor2);
    colorActions.append(Application::mainWindow->ui().actionColor3);
    colorActions.append(Application::mainWindow->ui().actionColor4);

    ToolbarButtonGroup *colorChoice = new ToolbarButtonGroup(Application::mainWindow->ui().boardToolBar, colorActions);
    colorChoice->displayText(false);

    //connect(colorChoice, SIGNAL(activated(int)), this, SLOT(Application::boardController->setColorIndex(int)));
    connect(DrawingController::drawingController(), &DrawingController::colorIndexChanged, colorChoice, &ToolbarButtonGroup::setCurrentIndex);
    connect(DrawingController::drawingController(), &DrawingController::colorIndexChanged, this, [this] { close(); });
    connect(DrawingController::drawingController(), &DrawingController::colorPaletteChanged, colorChoice, &ToolbarButtonGroup::colorPaletteChanged);
    connect(DrawingController::drawingController(), &DrawingController::colorPaletteChanged, this, [this] { close(); });

    layout()->addWidget(colorChoice);

    // Setup line width choice widget
    QList<QAction *> lineWidthActions;
    lineWidthActions.append(Application::mainWindow->ui().actionLineSmall);
    lineWidthActions.append(Application::mainWindow->ui().actionLineMedium);
    lineWidthActions.append(Application::mainWindow->ui().actionLineLarge);

    ToolbarButtonGroup *lineWidthChoice = new ToolbarButtonGroup(Application::mainWindow->ui().boardToolBar, lineWidthActions);
    lineWidthChoice->displayText(false);

    connect(lineWidthChoice, &ToolbarButtonGroup::activated, DrawingController::drawingController(), &DrawingController::setLineWidthIndex);
    connect(lineWidthChoice, &ToolbarButtonGroup::activated, this, [this] { close(); });
    connect(DrawingController::drawingController(), &DrawingController::lineWidthIndexChanged, lineWidthChoice, &ToolbarButtonGroup::setCurrentIndex);
    connect(DrawingController::drawingController(), &DrawingController::lineWidthIndexChanged, this, [this] { close(); });

    layout()->addWidget(lineWidthChoice);
}

