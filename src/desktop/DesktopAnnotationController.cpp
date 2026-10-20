#include "core/VirtualDesktop.h"

#include "desktop/DesktopAnnotationController.h"

#include "platform/PlatformUtils.h"

#include "core/Application.h"
#include "core/ApplicationController.h"
#include "core/DisplayManager.h"
#include "settings/Settings.h"

#include "web/WebController.h"

#include "ui/MainWindow.h"

#include "whiteboard/BoardView.h"
#include "whiteboard/DrawingController.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/BoardPaletteManager.h"

#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsPolygonItem.h"

#include "platform/CustomCaptureWindow.h"
#include "platform/WindowCapture.h"
#include "desktop/DesktopPalette.h"
#include "desktop/DesktopPropertyPalette.h"

#include "platform/KeyboardPalette.h"
#include "ui/Resources.h"


DesktopAnnotationController::DesktopAnnotationController(QObject *parent, RightPalette *rightPalette)
    : QObject(parent),
      mTransparentDrawingView(0),
      mTransparentDrawingScene(0),
      mDesktopPalette(NULL),
      mDesktopPenPalette(NULL),
      mDesktopMarkerPalette(NULL),
      mDesktopEraserPalette(NULL),
      mRightPalette(rightPalette),
      mWindowPositionInitialized(false),
      mIsFullyTransparent(false),
      mDesktopToolsPalettePositioned(false),
      mPendingPenButtonPressed(false),
      mPendingMarkerButtonPressed(false),
      mPendingEraserButtonPressed(false),
      mbArrowClicked(false),
      mBoardStylusTool(DrawingController::drawingController()->stylusTool()),
      mDesktopStylusTool(DrawingController::drawingController()->stylusTool()) {
    mTransparentDrawingView = new BoardView(Application::boardController, static_cast<QWidget *>(0), false,
                                            true); // deleted in DesktopAnnotationController::destructor
    mTransparentDrawingView->setAttribute(Qt::WA_TranslucentBackground, true);
#ifdef Q_OS_OSX
    mTransparentDrawingView->setAttribute(Qt::WA_MacNoShadow, true);
#endif
    mTransparentDrawingView->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Window |
                                            Qt::NoDropShadowWindowHint);
    mTransparentDrawingView->setCacheMode(QGraphicsView::CacheNone);
    mTransparentDrawingView->resize(VirtualDesktop::instance()->width(),
                                    VirtualDesktop::instance()->height());

    mTransparentDrawingView->setMouseTracking(true);

    mTransparentDrawingView->setAcceptDrops(true);

    QString backgroundStyle = "QWidget {background-color: rgba(127, 127, 127, 0)}";
    mTransparentDrawingView->setStyleSheet(backgroundStyle);

    mTransparentDrawingScene = new GraphicsScene(0, false);
    updateColors();

    mTransparentDrawingView->setScene(mTransparentDrawingScene);
    mTransparentDrawingScene->setDrawingMode(true);

    mDesktopPalette = new DesktopPalette(mTransparentDrawingView, rightPalette);
    // This was not fix, parent reverted
    // FIX #633: The palette must be 'floating' in order to stay on top of the library palette

    if (PlatformUtils::hasVirtualKeyboard()) {
        connect(Application::boardController->paletteManager()->mKeyboardPalette,
                &KeyboardPalette::keyboardActivated, mTransparentDrawingView,
                &BoardView::virtualKeyboardActivated);

#ifdef Q_OS_LINUX
        connect(Application::boardController->paletteManager()->mKeyboardPalette, &KeyboardPalette::moved,
                this, [this] { refreshMask(); });
        connect(Application::mainWindow->ui().actionVirtualKeyboard, &QAction::triggered, this,
                [this] { refreshMask(); });
        connect(mDesktopPalette, &DesktopPalette::refreshMask, this,
                &DesktopAnnotationController::refreshMask);
#endif
    }

    connect(mDesktopPalette, &DesktopPalette::boardClick, this, &DesktopAnnotationController::goToBoard);
    connect(mDesktopPalette, &DesktopPalette::customClick, this, &DesktopAnnotationController::customCapture);
    connect(mDesktopPalette, &DesktopPalette::windowClick, this, &DesktopAnnotationController::windowCapture);
    connect(mDesktopPalette, &DesktopPalette::screenClick, this, &DesktopAnnotationController::screenCapture);
    connect(Application::mainWindow->ui().actionPointer, &QAction::triggered, this,
            [this] { onToolClicked(); });
    connect(Application::mainWindow->ui().actionSelector, &QAction::triggered, this,
            [this] { onToolClicked(); });
    connect(mDesktopPalette, &FloatingPalette::maximized, this,
            &DesktopAnnotationController::onDesktopPaletteMaximized);
    connect(mDesktopPalette, &FloatingPalette::minimizeStart, this, [this] { onDesktopPaletteMinimize(); });
    connect(mDesktopPalette, &FloatingPalette::mouseEntered, mTransparentDrawingScene,
            &GraphicsScene::hideTool);
    connect(mRightPalette, &DockPalette::mouseEntered, mTransparentDrawingScene, &GraphicsScene::hideTool);

    connect(mTransparentDrawingView, &BoardView::resized, this, [this] { onTransparentWidgetResized(); });


    connect(DrawingController::drawingController(), &DrawingController::stylusToolChanged, this,
            &DesktopAnnotationController::stylusToolChanged);

    // Add the desktop associated palettes
    mDesktopPenPalette = new DesktopPenPalette(mTransparentDrawingView, rightPalette);

    connect(mDesktopPalette, &FloatingPalette::maximized, mDesktopPenPalette,
            &DesktopPenPalette::onParentMaximized);
    connect(mDesktopPalette, &FloatingPalette::minimizeStart, mDesktopPenPalette,
            &DesktopPenPalette::onParentMinimized);

    mDesktopMarkerPalette = new DesktopMarkerPalette(mTransparentDrawingView, rightPalette);
    mDesktopEraserPalette = new DesktopEraserPalette(mTransparentDrawingView, rightPalette);

    mDesktopPalette->setBackgroundBrush(Settings::settings()->opaquePaletteColor);
    mDesktopPenPalette->setBackgroundBrush(Settings::settings()->opaquePaletteColor);
    mDesktopMarkerPalette->setBackgroundBrush(Settings::settings()->opaquePaletteColor);
    mDesktopEraserPalette->setBackgroundBrush(Settings::settings()->opaquePaletteColor);


    // Hack : the size of the property palettes is computed the first time the palette is visible
    //        In order to prevent palette overlap on if the desktop palette is on the right of the
    //        screen, a setVisible(true) followed by a setVisible(false) is done.
    mDesktopPenPalette->setVisible(true);
    mDesktopMarkerPalette->setVisible(true);
    mDesktopEraserPalette->setVisible(true);
    mDesktopPenPalette->setVisible(false);
    mDesktopMarkerPalette->setVisible(false);
    mDesktopEraserPalette->setVisible(false);

    connect(Application::mainWindow->ui().actionEraseDesktopAnnotations, &QAction::triggered, this,
            [this] { eraseDesktopAnnotations(); });
    connect(Application::boardController, &BoardController::backgroundChanged, this,
            &DesktopAnnotationController::updateColors);
    connect(Application::boardController, &BoardController::activeSceneChanged, this,
            &DesktopAnnotationController::updateColors);
    connect(&mHoldTimerPen, &QTimer::timeout, this, &DesktopAnnotationController::penActionReleased);
    connect(&mHoldTimerMarker, &QTimer::timeout, this, &DesktopAnnotationController::markerActionReleased);
    connect(&mHoldTimerEraser, &QTimer::timeout, this, &DesktopAnnotationController::eraserActionReleased);

#ifdef Q_OS_LINUX
    connect(mDesktopPalette, &FloatingPalette::moving, this, &DesktopAnnotationController::refreshMask);
    connect(Application::boardController->paletteManager()->rightPalette(), &RightPalette::resized, this,
            &DesktopAnnotationController::refreshMask);
    connect(Application::boardController->paletteManager()->addItemPalette(), &ActionPalette::closed, this,
            &DesktopAnnotationController::refreshMask);
#endif
    onDesktopPaletteMaximized();

    // FIX #633: Ensure that these palettes stay on top of the other elements
    //mDesktopEraserPalette->raise();
    //mDesktopMarkerPalette->raise();
    //mDesktopPenPalette->raise();
}

DesktopAnnotationController::~DesktopAnnotationController() {
    delete mTransparentDrawingScene;
    delete mTransparentDrawingView;
}

void DesktopAnnotationController::updateColors() {
    if (Application::boardController->activeScene()->isDarkBackground()) {
        mTransparentDrawingScene->setBackground(true, PageBackground::plain);
    } else {
        mTransparentDrawingScene->setBackground(false, PageBackground::plain);
    }
}

DesktopPalette *DesktopAnnotationController::desktopPalette() {
    return mDesktopPalette;
}

QPainterPath DesktopAnnotationController::desktopPalettePath() const {
    QPainterPath result;
    if (mDesktopPalette && mDesktopPalette->isVisible()) {
        result.addRect(mDesktopPalette->geometry());
    }
    if (mDesktopPenPalette && mDesktopPenPalette->isVisible()) {
        result.addRect(mDesktopPenPalette->geometry());
    }
    if (mDesktopMarkerPalette && mDesktopMarkerPalette->isVisible()) {
        result.addRect(mDesktopMarkerPalette->geometry());
    }
    if (mDesktopEraserPalette && mDesktopEraserPalette->isVisible()) {
        result.addRect(mDesktopEraserPalette->geometry());
    }

    return result;
}

/**
 * \brief Toggle the visibility of the pen associated palette
 * @param checked as the visibility state
 */
void DesktopAnnotationController::desktopPenActionToggled(bool checked) {
    setAssociatedPalettePosition(mDesktopPenPalette, "actionPen");
    mDesktopPenPalette->setVisible(checked);
    mDesktopMarkerPalette->setVisible(false);
    mDesktopEraserPalette->setVisible(false);
}

/**
 * \brief Toggle the visibility of the marker associated palette
 * @param checked as the visibility state
 */
void DesktopAnnotationController::desktopMarkerActionToggled(bool checked) {
    setAssociatedPalettePosition(mDesktopMarkerPalette, "actionMarker");
    mDesktopMarkerPalette->setVisible(checked);
    mDesktopPenPalette->setVisible(false);
    mDesktopEraserPalette->setVisible(false);
}

/**
 * \brief Toggle the visibility of the eraser associated palette
 * @param checked as the visibility state
 */
void DesktopAnnotationController::desktopEraserActionToggled(bool checked) {
    setAssociatedPalettePosition(mDesktopEraserPalette, "actionEraser");
    mDesktopEraserPalette->setVisible(checked);
    mDesktopPenPalette->setVisible(false);
    mDesktopMarkerPalette->setVisible(false);
}

/**
 * \brief Set the location of the properties palette
 * @param palette as the palette
 * @param actionName as the name of the related action
 */
void DesktopAnnotationController::setAssociatedPalettePosition(ActionPalette *palette,
                                                               const QString &actionName) {
    QPoint desktopPalettePos = mDesktopPalette->geometry().topLeft();
    QList<QAction *> actions = mDesktopPalette->actions();
    int yPen = 0;

    foreach (QAction *act, actions) {
        if (act->objectName() == actionName) {
            int iAction = actions.indexOf(act);
            yPen = iAction * (mDesktopPalette->buttonSize().height() + 2 * mDesktopPalette->border() +
                              6); // This is the mysterious value (6)
            break;
        }
    }

    // First determine if the palette must be shown on the left or on the right
    if (desktopPalettePos.x() <= (mTransparentDrawingView->width() -
                                  (palette->width() + mDesktopPalette->width() + mRightPalette->width() +
                                   20))) // we take a small margin of 20 pixels
    {
        // Display it on the right
        desktopPalettePos += QPoint(mDesktopPalette->width(), yPen);
    } else {
        // Display it on the left
        desktopPalettePos += QPoint(0 - palette->width(), yPen);
    }

    palette->setCustomPosition(true);
    palette->move(desktopPalettePos);
}

void DesktopAnnotationController::eraseDesktopAnnotations() {
    if (mTransparentDrawingScene) {
        mTransparentDrawingScene->clearContent(GraphicsScene::clearAnnotations);
    }
}


BoardView *DesktopAnnotationController::drawingView() {
    return mTransparentDrawingView;
}


void DesktopAnnotationController::showWindow() {
    mDesktopPalette->setDisplaySelectButtonVisible(true);

    connect(Application::applicationController, &ApplicationController::desktopMode, mDesktopPalette,
            &DesktopPalette::setDisplaySelectButtonVisible, Qt::UniqueConnection);

    mDesktopPalette->show();

    bool showDisplay = Settings::settings()->webShowPageImmediatelyOnMirroredScreen->get().toBool();

    mDesktopPalette->showHideClick(showDisplay);
    mDesktopPalette->updateShowHideState(showDisplay);

    if (!mWindowPositionInitialized) {
        QRect desktopRect = VirtualDesktop::instance()->screenGeometry(mDesktopPalette->pos());

        mDesktopPalette->move(5, desktopRect.top() + 150);

        mWindowPositionInitialized = true;
        mDesktopPalette->maximizeMe();
    }

    updateBackground();

    mBoardStylusTool = DrawingController::drawingController()->stylusTool();

    DrawingController::drawingController()->setStylusTool(mDesktopStylusTool);

#ifndef Q_OS_LINUX
    PlatformUtils::showFullScreen(mTransparentDrawingView);
#else
    // this is necessary to avoid hiding the panels on Unity and Cinnamon
    // if finer control is necessary, use qgetenv("XDG_CURRENT_DESKTOP")
    mTransparentDrawingView->show();
#endif
    PlatformUtils::setDesktopMode(true);

    mDesktopPalette->appear();

#ifdef Q_OS_LINUX
    updateMask(true);
#endif
}


void DesktopAnnotationController::close() {
    if (mTransparentDrawingView)
        mTransparentDrawingView->hide();

    mDesktopPalette->hide();
}


void DesktopAnnotationController::stylusToolChanged(int tool) {
    Q_UNUSED(tool);

    updateBackground();
}


void DesktopAnnotationController::updateBackground() {
    QBrush newBrush;

    if (mIsFullyTransparent || DrawingController::drawingController()->stylusTool() == StylusTool::Selector) {
        newBrush = QBrush(Qt::transparent);
#ifdef Q_OS_LINUX
        updateMask(true);
#endif
    } else {
#if defined(Q_OS_OSX)
        newBrush = QBrush(QColor(127, 127, 127, 15));
#else
        newBrush = QBrush(QColor(127, 127, 127, 1));
#endif
#ifdef Q_OS_LINUX
        updateMask(false);
#endif
    }

    if (mTransparentDrawingScene && mTransparentDrawingScene->backgroundBrush() != newBrush)
        mTransparentDrawingScene->setBackgroundBrush(newBrush);
}


void DesktopAnnotationController::hideWindow() {
    if (mTransparentDrawingView)
        mTransparentDrawingView->hide();

    mDesktopPalette->hide();

    mDesktopStylusTool = DrawingController::drawingController()->stylusTool();
    DrawingController::drawingController()->setStylusTool(mBoardStylusTool);
}


void DesktopAnnotationController::goToBoard() {
    Application::applicationController->showBoard();
}

void DesktopAnnotationController::customCapture() {
    onToolClicked();
    mIsFullyTransparent = true;
    updateBackground();

    mDesktopPalette->disappearForCapture();
    CustomCaptureWindow customCaptureWindow(mDesktopPalette);
    // need to show the window before execute it to avoid some glitch on windows.

#ifndef Q_OS_WIN // Working only without this call on win32 desktop mode
    PlatformUtils::showFullScreen(&customCaptureWindow);
#endif

    if (customCaptureWindow.execute(getScreenPixmap()) == QDialog::Accepted) {
        QPixmap selectedPixmap = customCaptureWindow.getSelectedPixmap();
        emit imageCaptured(selectedPixmap, false);
    }

    mDesktopPalette->appear();

    mIsFullyTransparent = false;
    updateBackground();
}


void DesktopAnnotationController::windowCapture() {
    onToolClicked();
    mIsFullyTransparent = true;
    updateBackground();

    mDesktopPalette->disappearForCapture();

    WindowCapture util(mDesktopPalette, this);

    if (util.execute() == QDialog::Accepted) {
        QPixmap windowPixmap = util.getCapturedWindow();

        // on Mac OS X we can only know that user cancel the operatiion by checking is the image is null
        // because the screencapture utility always return code 0 event if user cancel the application
        if (!windowPixmap.isNull()) {
            emit imageCaptured(windowPixmap, false);
        }
    }

    mDesktopPalette->appear();

    mIsFullyTransparent = false;

    updateBackground();
}


void DesktopAnnotationController::screenCapture() {
    onToolClicked();
    mIsFullyTransparent = true;
    updateBackground();

    mDesktopPalette->disappearForCapture();

    QPixmap originalPixmap = getScreenPixmap();

    mDesktopPalette->appear();

    emit imageCaptured(originalPixmap, false);

    mIsFullyTransparent = false;

    updateBackground();
}


QPixmap DesktopAnnotationController::getScreenPixmap() {
    VirtualDesktop *desktop = VirtualDesktop::instance();
    QScreen *screen = Application::controlScreen();

    QRect rect = desktop->screenGeometry(QCursor::pos());

    return VirtualDesktop::grabScreen(screen, rect);
}


void DesktopAnnotationController::updateShowHideState(bool pEnabled) {
    mDesktopPalette->updateShowHideState(pEnabled);
}


void DesktopAnnotationController::screenLayoutChanged() {
    if (Application::applicationController && Application::applicationController->displayManager() &&
        Application::applicationController->displayManager()->hasDisplay()) {
        mDesktopPalette->setShowHideButtonVisible(true);
    } else {
        mDesktopPalette->setShowHideButtonVisible(false);
    }
}

/**
 * \brief Handles the pen action pressed event
 */
void DesktopAnnotationController::penActionPressed() {
    mbArrowClicked = false;
    mDesktopMarkerPalette->hide();
    mDesktopEraserPalette->hide();
    DrawingController::drawingController()->setStylusTool(StylusTool::Pen);
    mPenHoldTimer = QTime::currentTime();
    mPendingPenButtonPressed = true;

    // Check if the mouse cursor is on the little arrow
    QPoint cursorPos = QCursor::pos();
    QPoint palettePos = mDesktopPalette->pos();
    QPoint buttonPos = mDesktopPalette->buttonPos(Application::mainWindow->ui().actionPen);

    int iX = cursorPos.x() - (palettePos.x() + buttonPos.x()); // x position of the cursor in the palette
    int iY = cursorPos.y() - (palettePos.y() + buttonPos.y()); // y position of the cursor in the palette

    if (iX >= 30 && iX <= 40 && iY >= 30 && iY <= 40) {
        mbArrowClicked = true;
        penActionReleased();
    } else {
        mHoldTimerPen.start(PROPERTY_PALETTE_TIMER);
    }
}

/**
 * \brief Handles the pen action released event
 */
void DesktopAnnotationController::penActionReleased() {
    mHoldTimerPen.stop();
    if (mPendingPenButtonPressed) {
        if (mbArrowClicked || mPenHoldTimer.msecsTo(QTime::currentTime()) > PROPERTY_PALETTE_TIMER - 100) {
            togglePropertyPalette(mDesktopPenPalette);
        } else {
            Application::mainWindow->ui().actionPen->trigger();
        }
        mPendingPenButtonPressed = false;
    }
    Application::mainWindow->ui().actionPen->setChecked(true);

    switchCursor(StylusTool::Pen);
}

/**
 * \brief Handles the eraser action pressed event
 */
void DesktopAnnotationController::eraserActionPressed() {
    mbArrowClicked = false;
    mDesktopPenPalette->hide();
    mDesktopMarkerPalette->hide();
    DrawingController::drawingController()->setStylusTool(StylusTool::Eraser);
    mEraserHoldTimer = QTime::currentTime();
    mPendingEraserButtonPressed = true;

    // Check if the mouse cursor is on the little arrow
    QPoint cursorPos = QCursor::pos();
    QPoint palettePos = mDesktopPalette->pos();
    QPoint buttonPos = mDesktopPalette->buttonPos(Application::mainWindow->ui().actionEraser);

    int iX = cursorPos.x() - (palettePos.x() + buttonPos.x()); // x position of the cursor in the palette
    int iY = cursorPos.y() - (palettePos.y() + buttonPos.y()); // y position of the cursor in the palette

    if (iX >= 30 && iX <= 40 && iY >= 30 && iY <= 40) {
        mbArrowClicked = true;
        eraserActionReleased();
    } else {
        mHoldTimerEraser.start(PROPERTY_PALETTE_TIMER);
    }
}

/**
 * \brief Handles the eraser action released event
 */
void DesktopAnnotationController::eraserActionReleased() {
    mHoldTimerEraser.stop();
    if (mPendingEraserButtonPressed) {
        if (mbArrowClicked || mEraserHoldTimer.msecsTo(QTime::currentTime()) > PROPERTY_PALETTE_TIMER - 100) {
            togglePropertyPalette(mDesktopEraserPalette);
        } else {
            Application::mainWindow->ui().actionEraser->trigger();
        }
        mPendingEraserButtonPressed = false;
    }
    Application::mainWindow->ui().actionEraser->setChecked(true);

    switchCursor(StylusTool::Eraser);
}


/**
 * \brief Handles the marker action pressed event
 */
void DesktopAnnotationController::markerActionPressed() {
    mbArrowClicked = false;
    mDesktopPenPalette->hide();
    mDesktopEraserPalette->hide();
    DrawingController::drawingController()->setStylusTool(StylusTool::Marker);
    mMarkerHoldTimer = QTime::currentTime();
    mPendingMarkerButtonPressed = true;

    // Check if the mouse cursor is on the little arrow
    QPoint cursorPos = QCursor::pos();
    QPoint palettePos = mDesktopPalette->pos();
    QPoint buttonPos = mDesktopPalette->buttonPos(Application::mainWindow->ui().actionMarker);

    int iX = cursorPos.x() - (palettePos.x() + buttonPos.x()); // x position of the cursor in the palette
    int iY = cursorPos.y() - (palettePos.y() + buttonPos.y()); // y position of the cursor in the palette

    if (iX >= 30 && iX <= 40 && iY >= 30 && iY <= 40) {
        mbArrowClicked = true;
        markerActionReleased();
    } else {
        mHoldTimerMarker.start(PROPERTY_PALETTE_TIMER);
    }
}


/**
 * \brief Handles the marker action released event
 */
void DesktopAnnotationController::markerActionReleased() {
    mHoldTimerMarker.stop();
    if (mPendingMarkerButtonPressed) {
        if (mbArrowClicked || mMarkerHoldTimer.msecsTo(QTime::currentTime()) > PROPERTY_PALETTE_TIMER - 100) {
            togglePropertyPalette(mDesktopMarkerPalette);
        } else {
            Application::mainWindow->ui().actionMarker->trigger();
        }
        mPendingMarkerButtonPressed = false;
    }
    Application::mainWindow->ui().actionMarker->setChecked(true);

    switchCursor(StylusTool::Marker);
}

void DesktopAnnotationController::selectorActionPressed() {}

void DesktopAnnotationController::selectorActionReleased() {
    Application::mainWindow->ui().actionSelector->setChecked(true);
    switchCursor(StylusTool::Selector);
}


void DesktopAnnotationController::pointerActionPressed() {}

void DesktopAnnotationController::pointerActionReleased() {
    Application::mainWindow->ui().actionPointer->setChecked(true);
    switchCursor(StylusTool::Pointer);
}


/**
 * \brief Toggle the given palette visibility
 * @param palette as the given palette
 */
void DesktopAnnotationController::togglePropertyPalette(ActionPalette *palette) {
    if (NULL != palette) {
        bool bShow = !palette->isVisible();
        if (mDesktopPenPalette == palette) {
            desktopPenActionToggled(bShow);
        } else if (mDesktopMarkerPalette == palette) {
            desktopMarkerActionToggled(bShow);
        } else if (mDesktopEraserPalette == palette) {
            desktopEraserActionToggled(bShow);
        }
    }
}


void DesktopAnnotationController::switchCursor(const int tool) {
    mTransparentDrawingScene->setToolCursor(tool);
    mTransparentDrawingView->setToolCursor(tool);
}

/**
 * \brief Reconnect the pressed & released signals of the property palettes
 */
void DesktopAnnotationController::onDesktopPaletteMaximized() {
    // Pen
    ActionPaletteButton *pPenButton =
        mDesktopPalette->getButtonFromAction(Application::mainWindow->ui().actionPen);
    if (NULL != pPenButton) {
        connect(pPenButton, &QAbstractButton::pressed, this, &DesktopAnnotationController::penActionPressed);
        connect(pPenButton, &QAbstractButton::released, this,
                &DesktopAnnotationController::penActionReleased);
    }

    // Eraser
    ActionPaletteButton *pEraserButton =
        mDesktopPalette->getButtonFromAction(Application::mainWindow->ui().actionEraser);
    if (NULL != pEraserButton) {
        connect(pEraserButton, &QAbstractButton::pressed, this,
                &DesktopAnnotationController::eraserActionPressed);
        connect(pEraserButton, &QAbstractButton::released, this,
                &DesktopAnnotationController::eraserActionReleased);
    }

    // Marker
    ActionPaletteButton *pMarkerButton =
        mDesktopPalette->getButtonFromAction(Application::mainWindow->ui().actionMarker);
    if (NULL != pMarkerButton) {
        connect(pMarkerButton, &QAbstractButton::pressed, this,
                &DesktopAnnotationController::markerActionPressed);
        connect(pMarkerButton, &QAbstractButton::released, this,
                &DesktopAnnotationController::markerActionReleased);
    }

    // Pointer
    ActionPaletteButton *pSelectorButton =
        mDesktopPalette->getButtonFromAction(Application::mainWindow->ui().actionSelector);
    if (NULL != pSelectorButton) {
        connect(pSelectorButton, &QAbstractButton::pressed, this,
                &DesktopAnnotationController::selectorActionPressed);
        connect(pSelectorButton, &QAbstractButton::released, this,
                &DesktopAnnotationController::selectorActionReleased);
    }

    // Pointer
    ActionPaletteButton *pPointerButton =
        mDesktopPalette->getButtonFromAction(Application::mainWindow->ui().actionPointer);
    if (NULL != pPointerButton) {
        connect(pPointerButton, &QAbstractButton::pressed, this,
                &DesktopAnnotationController::pointerActionPressed);
        connect(pPointerButton, &QAbstractButton::released, this,
                &DesktopAnnotationController::pointerActionReleased);
    }
}

/**
 * \brief Disconnect the pressed & release signals of the property palettes
 * This is done to prevent memory leaks
 */
void DesktopAnnotationController::onDesktopPaletteMinimize() {
    // Pen
    ActionPaletteButton *pPenButton =
        mDesktopPalette->getButtonFromAction(Application::mainWindow->ui().actionPen);
    if (NULL != pPenButton) {
        disconnect(pPenButton, &QAbstractButton::pressed, this,
                   &DesktopAnnotationController::penActionPressed);
        disconnect(pPenButton, &QAbstractButton::released, this,
                   &DesktopAnnotationController::penActionReleased);
    }

    // Marker
    ActionPaletteButton *pMarkerButton =
        mDesktopPalette->getButtonFromAction(Application::mainWindow->ui().actionMarker);
    if (NULL != pMarkerButton) {
        disconnect(pMarkerButton, &QAbstractButton::pressed, this,
                   &DesktopAnnotationController::markerActionPressed);
        disconnect(pMarkerButton, &QAbstractButton::released, this,
                   &DesktopAnnotationController::markerActionReleased);
    }

    // Eraser
    ActionPaletteButton *pEraserButton =
        mDesktopPalette->getButtonFromAction(Application::mainWindow->ui().actionEraser);
    if (NULL != pEraserButton) {
        disconnect(pEraserButton, &QAbstractButton::pressed, this,
                   &DesktopAnnotationController::eraserActionPressed);
        disconnect(pEraserButton, &QAbstractButton::released, this,
                   &DesktopAnnotationController::eraserActionReleased);
    }

    // Selector —— onDesktopPaletteMaximized() 连了五组按钮，这里过去只断三组，
    // 于是每"最小化→再打开"一轮，选择/指针就多得一条连接，点一次触发多次。
    ActionPaletteButton *pSelectorButton =
        mDesktopPalette->getButtonFromAction(Application::mainWindow->ui().actionSelector);
    if (NULL != pSelectorButton) {
        disconnect(pSelectorButton, &QAbstractButton::pressed, this,
                   &DesktopAnnotationController::selectorActionPressed);
        disconnect(pSelectorButton, &QAbstractButton::released, this,
                   &DesktopAnnotationController::selectorActionReleased);
    }

    // Pointer
    ActionPaletteButton *pPointerButton =
        mDesktopPalette->getButtonFromAction(Application::mainWindow->ui().actionPointer);
    if (NULL != pPointerButton) {
        disconnect(pPointerButton, &QAbstractButton::pressed, this,
                   &DesktopAnnotationController::pointerActionPressed);
        disconnect(pPointerButton, &QAbstractButton::released, this,
                   &DesktopAnnotationController::pointerActionReleased);
    }
}

void DesktopAnnotationController::TransparentWidgetResized() {
    onTransparentWidgetResized();
}

/**
 * \brief Resize the library palette.
 */
void DesktopAnnotationController::onTransparentWidgetResized() {
    int rW = Application::boardController->paletteManager()->rightPalette()->width();
    int lW = Application::boardController->paletteManager()->leftPalette()->width();

    int rH = mTransparentDrawingView->height();

    Application::boardController->paletteManager()->rightPalette()->resize(rW + 1, rH);
    Application::boardController->paletteManager()->rightPalette()->resize(rW, rH);

    Application::boardController->paletteManager()->leftPalette()->resize(lW + 1, rH);
    Application::boardController->paletteManager()->leftPalette()->resize(lW, rH);
}

void DesktopAnnotationController::updateMask(bool bTransparent) {
    if (bTransparent) {
        // Here we have to generate a new mask This method is certainly resource
        // consuming but for the moment this is the only solution that I found.
        mMask = QPixmap(mTransparentDrawingView->width(), mTransparentDrawingView->height());
        mMask.fill(Qt::transparent);

        QPainter p;

        p.begin(&mMask);

        p.setPen(Qt::red);
        p.setBrush(QBrush(Qt::red));

        // Here we draw the widget mask
        if (mDesktopPalette->isVisible()) {
            p.drawRect(mDesktopPalette->geometry().x(), mDesktopPalette->geometry().y(),
                       mDesktopPalette->width(), mDesktopPalette->height());
        }
        if (Application::boardController->paletteManager()->mKeyboardPalette->isVisible()) {
            p.drawRect(Application::boardController->paletteManager()->mKeyboardPalette->geometry().x(),
                       Application::boardController->paletteManager()->mKeyboardPalette->geometry().y(),
                       Application::boardController->paletteManager()->mKeyboardPalette->width(),
                       Application::boardController->paletteManager()->mKeyboardPalette->height());
        }

        if (Application::boardController->paletteManager()->leftPalette()->isVisible()) {
            QRect leftPalette(Application::boardController->paletteManager()->leftPalette()->geometry().x(),
                              Application::boardController->paletteManager()->leftPalette()->geometry().y(),
                              Application::boardController->paletteManager()->leftPalette()->width(),
                              Application::boardController->paletteManager()->leftPalette()->height());

            QRect tabsPalette(
                Application::boardController->paletteManager()->leftPalette()->getTabPaletteRect());

            p.drawRect(leftPalette);
            p.drawRect(tabsPalette);
        }

        if (Application::boardController->paletteManager()->rightPalette()->isVisible()) {
            QRect rightPalette(Application::boardController->paletteManager()->rightPalette()->geometry().x(),
                               Application::boardController->paletteManager()->rightPalette()->geometry().y(),
                               Application::boardController->paletteManager()->rightPalette()->width(),
                               Application::boardController->paletteManager()->rightPalette()->height());

            QRect tabsPalette(
                Application::boardController->paletteManager()->rightPalette()->getTabPaletteRect());

            p.drawRect(rightPalette);
            p.drawRect(tabsPalette);
        }

#ifdef Q_OS_LINUX
        //Rquiered only for compiz wm
        //TODO. Window manager detection screen

        if (Application::boardController->paletteManager()->addItemPalette()->isVisible()) {
            p.drawRect(Application::boardController->paletteManager()->addItemPalette()->geometry());
        }

#endif

        p.end();

        // Then we add the annotations. We create another painter because we need to
        // apply transformations on it for coordinates matching
        QPainter annotationPainter;

        QTransform trans;
        trans.translate(mTransparentDrawingView->width() / 2, mTransparentDrawingView->height() / 2);

        annotationPainter.begin(&mMask);
        annotationPainter.setPen(Qt::red);
        annotationPainter.setBrush(Qt::red);

        annotationPainter.setTransform(trans);

        QList<QGraphicsItem *> allItems = mTransparentDrawingScene->items();

        for (int i = 0; i < allItems.size(); i++) {
            QGraphicsItem *pCrntItem = allItems.at(i);

            if (pCrntItem->isVisible() && pCrntItem->type() == GraphicsPolygonItem::Type) {
                QPainterPath crntPath = pCrntItem->shape();
                QRectF rect = crntPath.boundingRect();

                annotationPainter.drawRect(rect);
            }
        }

        annotationPainter.end();

        mTransparentDrawingView->setMask(mMask.mask());
    } else {
        // Remove the mask
        QPixmap noMask(mTransparentDrawingView->width(), mTransparentDrawingView->height());
        mTransparentDrawingView->setMask(noMask.mask());
    }
}

void DesktopAnnotationController::refreshMask() {
    if (mTransparentDrawingScene && mTransparentDrawingView->isVisible()) {
        if (mIsFullyTransparent ||
            DrawingController::drawingController()->stylusTool() == StylusTool::Selector
            //Needed to work correctly when another actions on stylus are checked
            || DrawingController::drawingController()->stylusTool() == StylusTool::Eraser ||
            DrawingController::drawingController()->stylusTool() == StylusTool::Pointer ||
            DrawingController::drawingController()->stylusTool() == StylusTool::Pen ||
            DrawingController::drawingController()->stylusTool() == StylusTool::Marker) {
            updateMask(true);
        }
    }
}

void DesktopAnnotationController::onToolClicked() {
    mDesktopEraserPalette->hide();
    mDesktopMarkerPalette->hide();
    mDesktopPenPalette->hide();
}
