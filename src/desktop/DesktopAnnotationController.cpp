#include "core/VirtualDesktop.h"

#include "desktop/DesktopAnnotationController.h"

#include "platform/PlatformUtils.h"

#include "core/Application.h"
#include "core/DisplayManager.h"
#include "core/ApplicationController.h"
#include "settings/Settings.h"

#include "ui/MainWindow.h"

#include "whiteboard/BoardView.h"
#include "whiteboard/DrawingController.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/BoardPaletteManager.h"

#include "whiteboard/GraphicsScene.h"

#include "ui/CustomCaptureWindow.h"
#include "platform/WindowCapture.h"
#include "desktop/DesktopPalette.h"
#include "desktop/DesktopPropertyPalette.h"

#include "ui/KeyboardPalette.h"

DesktopAnnotationController::DesktopAnnotationController(QObject *parent, RightPalette *rightPalette)
    : IDesktopAnnotation(parent), mTransparentDrawingView(0), mTransparentDrawingScene(0), mDesktopPalette(NULL), mDesktopPenPalette(NULL),
      mDesktopMarkerPalette(NULL), mDesktopEraserPalette(NULL), mRightPalette(rightPalette), mWindowPositionInitialized(false),
      mIsFullyTransparent(false), mDesktopToolsPalettePositioned(false), mPendingPenButtonPressed(false), mPendingMarkerButtonPressed(false),
      mPendingEraserButtonPressed(false), mbArrowClicked(false), mBoardStylusTool(DrawingController::drawingController()->stylusTool()),
      mDesktopStylusTool(DrawingController::drawingController()->stylusTool())
{
    mTransparentDrawingView = new BoardView(Application::boardController, static_cast<QWidget *>(0), false,
        true); // deleted in DesktopAnnotationController::destructor
    mTransparentDrawingView->setAttribute(Qt::WA_TranslucentBackground, true);
    mTransparentDrawingView->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Window | Qt::NoDropShadowWindowHint);
    mTransparentDrawingView->setCacheMode(QGraphicsView::CacheNone);
    mTransparentDrawingView->resize(VirtualDesktop::instance()->width(), VirtualDesktop::instance()->height());

    mTransparentDrawingView->setMouseTracking(true);

    mTransparentDrawingView->setAcceptDrops(true);

    QString backgroundStyle = "QWidget {background-color: rgba(127, 127, 127, 0)}";
    mTransparentDrawingView->setStyleSheet(backgroundStyle);

    mTransparentDrawingScene = new GraphicsScene(0, false);
    updateColors();

    mTransparentDrawingView->setScene(mTransparentDrawingScene);
    mTransparentDrawingScene->setDrawingMode(true);

    mDesktopPalette = new DesktopPalette(mTransparentDrawingView, rightPalette);
    // 工具栏必须保持浮动层级，才能盖在右侧库面板之上；改挂回普通子控件会被库面板压住。

    if (PlatformUtils::hasVirtualKeyboard())
    {
        connect(Application::boardController->paletteManager()->mKeyboardPalette, &KeyboardPalette::keyboardActivated, mTransparentDrawingView,
            &BoardView::virtualKeyboardActivated);
    }

    connect(mDesktopPalette, &DesktopPalette::boardClick, this, &DesktopAnnotationController::goToBoard);
    connect(mDesktopPalette, &DesktopPalette::customClick, this, &DesktopAnnotationController::customCapture);
    connect(mDesktopPalette, &DesktopPalette::windowClick, this, &DesktopAnnotationController::windowCapture);
    connect(mDesktopPalette, &DesktopPalette::screenClick, this, &DesktopAnnotationController::screenCapture);
    connect(Application::mainWindow->ui().actionPointer, &QAction::triggered, this,
        [this]
        {
            onToolClicked();
        });
    connect(Application::mainWindow->ui().actionSelector, &QAction::triggered, this,
        [this]
        {
            onToolClicked();
        });
    connect(mDesktopPalette, &FloatingPalette::maximized, this, &DesktopAnnotationController::onDesktopPaletteMaximized);
    connect(mDesktopPalette, &FloatingPalette::minimizeStart, this,
        [this]
        {
            onDesktopPaletteMinimize();
        });
    connect(mDesktopPalette, &FloatingPalette::mouseEntered, mTransparentDrawingScene, &GraphicsScene::hideTool);
    connect(mRightPalette, &DockPalette::mouseEntered, mTransparentDrawingScene, &GraphicsScene::hideTool);

    connect(mTransparentDrawingView, &BoardView::resized, this,
        [this]
        {
            onTransparentWidgetResized();
        });

    connect(DrawingController::drawingController(), &DrawingController::stylusToolChanged, this, &DesktopAnnotationController::stylusToolChanged);

    // 三个属性面板同样挂在透明视图下，跟随工具栏一起浮动。
    mDesktopPenPalette = new DesktopPenPalette(mTransparentDrawingView, rightPalette);

    connect(mDesktopPalette, &FloatingPalette::maximized, mDesktopPenPalette, &DesktopPenPalette::onParentMaximized);
    connect(mDesktopPalette, &FloatingPalette::minimizeStart, mDesktopPenPalette, &DesktopPenPalette::onParentMinimized);

    mDesktopMarkerPalette = new DesktopMarkerPalette(mTransparentDrawingView, rightPalette);
    mDesktopEraserPalette = new DesktopEraserPalette(mTransparentDrawingView, rightPalette);

    mDesktopPalette->setBackgroundBrush(Settings::settings()->opaquePaletteColor);
    mDesktopPenPalette->setBackgroundBrush(Settings::settings()->opaquePaletteColor);
    mDesktopMarkerPalette->setBackgroundBrush(Settings::settings()->opaquePaletteColor);
    mDesktopEraserPalette->setBackgroundBrush(Settings::settings()->opaquePaletteColor);

    // 属性面板的尺寸要到第一次显示时才计算；先 show 再 hide 强制完成布局，
    // 否则工具栏贴右时面板尺寸还是 0，定位会与工具栏重叠。
    mDesktopPenPalette->setVisible(true);
    mDesktopMarkerPalette->setVisible(true);
    mDesktopEraserPalette->setVisible(true);
    mDesktopPenPalette->setVisible(false);
    mDesktopMarkerPalette->setVisible(false);
    mDesktopEraserPalette->setVisible(false);

    connect(Application::mainWindow->ui().actionEraseDesktopAnnotations, &QAction::triggered, this,
        [this]
        {
            eraseDesktopAnnotations();
        });
    connect(Application::boardController, &BoardController::backgroundChanged, this, &DesktopAnnotationController::updateColors);
    connect(Application::boardController, &BoardController::activeSceneChanged, this, &DesktopAnnotationController::updateColors);
    connect(&mHoldTimerPen, &QTimer::timeout, this, &DesktopAnnotationController::penActionReleased);
    connect(&mHoldTimerMarker, &QTimer::timeout, this, &DesktopAnnotationController::markerActionReleased);
    connect(&mHoldTimerEraser, &QTimer::timeout, this, &DesktopAnnotationController::eraserActionReleased);

    onDesktopPaletteMaximized();
}

DesktopAnnotationController::~DesktopAnnotationController()
{
    delete mTransparentDrawingScene;
    delete mTransparentDrawingView;
}

void DesktopAnnotationController::updateColors()
{
    if (Application::boardController->activeScene()->isDarkBackground())
    {
        mTransparentDrawingScene->setBackground(true, PageBackground::plain);
    }
    else
    {
        mTransparentDrawingScene->setBackground(false, PageBackground::plain);
    }
}

DesktopPalette *DesktopAnnotationController::desktopPalette()
{
    return mDesktopPalette;
}

void DesktopAnnotationController::desktopPenActionToggled(bool checked)
{
    setAssociatedPalettePosition(mDesktopPenPalette, "actionPen");
    mDesktopPenPalette->setVisible(checked);
    mDesktopMarkerPalette->setVisible(false);
    mDesktopEraserPalette->setVisible(false);
}

void DesktopAnnotationController::desktopMarkerActionToggled(bool checked)
{
    setAssociatedPalettePosition(mDesktopMarkerPalette, "actionMarker");
    mDesktopMarkerPalette->setVisible(checked);
    mDesktopPenPalette->setVisible(false);
    mDesktopEraserPalette->setVisible(false);
}

void DesktopAnnotationController::desktopEraserActionToggled(bool checked)
{
    setAssociatedPalettePosition(mDesktopEraserPalette, "actionEraser");
    mDesktopEraserPalette->setVisible(checked);
    mDesktopPenPalette->setVisible(false);
    mDesktopMarkerPalette->setVisible(false);
}

void DesktopAnnotationController::setAssociatedPalettePosition(ActionPalette *palette, const QString &actionName)
{
    QPoint           desktopPalettePos = mDesktopPalette->geometry().topLeft();
    QList<QAction *> actions           = mDesktopPalette->actions();
    int              yPen              = 0;

    foreach (QAction *act, actions)
    {
        if (act->objectName() == actionName)
        {
            int iAction = actions.indexOf(act);
            yPen        = iAction * (mDesktopPalette->buttonSize().height() + 2 * mDesktopPalette->border() +
                                        6); // 6 是按钮间的固定垂直间距（历史经验值，与工具栏布局对应）
            break;
        }
    }

    // 先判断面板放得下右边还是只能放左边：右侧要同时避开工具栏与主界面右侧栏，
    // 20 像素是留给面板与窗口边缘的余量。
    if (desktopPalettePos.x() <= (mTransparentDrawingView->width() - (palette->width() + mDesktopPalette->width() + mRightPalette->width() + 20)))
    {
        // 右侧放得下，紧贴工具栏右边缘。
        desktopPalettePos += QPoint(mDesktopPalette->width(), yPen);
    }
    else
    {
        // 右侧空间不足，翻到工具栏左侧。
        desktopPalettePos += QPoint(0 - palette->width(), yPen);
    }

    palette->setCustomPosition(true);
    palette->move(desktopPalettePos);
}

void DesktopAnnotationController::eraseDesktopAnnotations()
{
    if (mTransparentDrawingScene)
    {
        mTransparentDrawingScene->clearContent(GraphicsScene::clearAnnotations);
    }
}

QWidget *DesktopAnnotationController::drawingView()
{
    return mTransparentDrawingView;
}

void DesktopAnnotationController::showWindow()
{
    mDesktopPalette->setDisplaySelectButtonVisible(true);

    connect(Application::applicationController, &ApplicationController::desktopMode, mDesktopPalette, &DesktopPalette::setDisplaySelectButtonVisible,
        Qt::UniqueConnection);

    mDesktopPalette->show();

    bool showDisplay = Settings::settings()->webShowPageImmediatelyOnMirroredScreen->get().toBool();

    mDesktopPalette->showHideClick(showDisplay);
    mDesktopPalette->updateShowHideState(showDisplay);

    if (!mWindowPositionInitialized)
    {
        QRect desktopRect = VirtualDesktop::instance()->screenGeometry(mDesktopPalette->pos());

        mDesktopPalette->move(5, desktopRect.top() + 150);

        mWindowPositionInitialized = true;
        mDesktopPalette->maximizeMe();
    }

    updateBackground();

    // 进入桌面模式前保存画板工具，再切回桌面上次使用的工具，两套上下文互不覆盖。
    mBoardStylusTool = DrawingController::drawingController()->stylusTool();

    DrawingController::drawingController()->setStylusTool(mDesktopStylusTool);

    PlatformUtils::showFullScreen(mTransparentDrawingView);
    PlatformUtils::setDesktopMode(true);

    mDesktopPalette->appear();
}

void DesktopAnnotationController::closeWindow()
{
    if (mTransparentDrawingView)
    {
        mTransparentDrawingView->hide();
    }

    mDesktopPalette->hide();
}

void DesktopAnnotationController::stylusToolChanged(int tool)
{
    Q_UNUSED(tool);

    updateBackground();
}

void DesktopAnnotationController::updateBackground()
{
    QBrush newBrush;

    // 选择器与截图需要完全透明；其余工具用 alpha=1 的近透明灰，
    // 既让批注有底色可依，又基本不遮挡桌面。
    if (mIsFullyTransparent || DrawingController::drawingController()->stylusTool() == StylusTool::Selector)
    {
        newBrush = QBrush(Qt::transparent);
    }
    else
    {
        newBrush = QBrush(QColor(127, 127, 127, 1));
    }

    if (mTransparentDrawingScene && mTransparentDrawingScene->backgroundBrush() != newBrush)
    {
        mTransparentDrawingScene->setBackgroundBrush(newBrush);
    }
}

void DesktopAnnotationController::hideWindow()
{
    if (mTransparentDrawingView)
    {
        mTransparentDrawingView->hide();
    }

    mDesktopPalette->hide();

    // 记住桌面工具并恢复进入前的画板工具，避免桌面上的选择器/指针状态带回白板。
    mDesktopStylusTool = DrawingController::drawingController()->stylusTool();
    DrawingController::drawingController()->setStylusTool(mBoardStylusTool);
}

void DesktopAnnotationController::goToBoard()
{
    Application::applicationController->showBoard();
}

void DesktopAnnotationController::customCapture()
{
    onToolClicked();
    // 抓图前把批注层彻底透明化，抓完再按当前工具恢复背景。
    mIsFullyTransparent = true;
    updateBackground();

    // 截图期间工具栏与淡灰背景都必须先撤掉，否则会一起被抓进画面。
    mDesktopPalette->disappearForCapture();
    CustomCaptureWindow customCaptureWindow(mDesktopPalette);
    // 窗口要先显示再进入模态循环，否则 Windows 上首次绘制会闪一下或留下残影。

    if (customCaptureWindow.execute(getScreenPixmap()) == QDialog::Accepted)
    {
        QPixmap selectedPixmap = customCaptureWindow.getSelectedPixmap();
        emit    imageCaptured(selectedPixmap, false);
    }

    mDesktopPalette->appear();

    mIsFullyTransparent = false;
    updateBackground();
}

void DesktopAnnotationController::windowCapture()
{
    onToolClicked();
    mIsFullyTransparent = true;
    updateBackground();

    mDesktopPalette->disappearForCapture();

    WindowCapture util(mDesktopPalette, this);

    if (util.execute() == QDialog::Accepted)
    {
        QPixmap windowPixmap = util.getCapturedWindow();

        // 取消选择或抓取失败都会得到空图，空图不发出，避免把空白插进文档。
        if (!windowPixmap.isNull())
        {
            emit imageCaptured(windowPixmap, false);
        }
    }

    mDesktopPalette->appear();

    mIsFullyTransparent = false;

    updateBackground();
}

void DesktopAnnotationController::screenCapture()
{
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

QPixmap DesktopAnnotationController::getScreenPixmap()
{
    VirtualDesktop *desktop = VirtualDesktop::instance();
    QScreen        *screen  = Application::controlScreen();

    // 以光标所在屏幕为准，多显示器下截的是用户正在操作的那一块。
    QRect rect              = desktop->screenGeometry(QCursor::pos());

    return VirtualDesktop::grabScreen(screen, rect);
}

void DesktopAnnotationController::updateShowHideState(bool pEnabled)
{
    mDesktopPalette->updateShowHideState(pEnabled);
}

void DesktopAnnotationController::screenLayoutChanged()
{
    // 只有检测到可用副屏时才显示“副屏显示白板”按钮，否则该按钮没有意义。
    if (Application::applicationController && Application::applicationController->displayManager() &&
        Application::applicationController->displayManager()->hasDisplay())
    {
        mDesktopPalette->setShowHideButtonVisible(true);
    }
    else
    {
        mDesktopPalette->setShowHideButtonVisible(false);
    }
}

void DesktopAnnotationController::penActionPressed()
{
    mbArrowClicked = false;
    mDesktopMarkerPalette->hide();
    mDesktopEraserPalette->hide();
    DrawingController::drawingController()->setStylusTool(StylusTool::Pen);
    mPenHoldTimer            = QTime::currentTime();
    mPendingPenButtonPressed = true;

    // 按钮右下角的小箭头是“展开属性面板”的热区，用按下时的光标位置判断，避免拖动后误判。
    QPoint cursorPos         = QCursor::pos();
    QPoint palettePos        = mDesktopPalette->pos();
    QPoint buttonPos         = mDesktopPalette->buttonPos(Application::mainWindow->ui().actionPen);

    int iX                   = cursorPos.x() - (palettePos.x() + buttonPos.x()); // 光标相对按钮左上角的横向偏移
    int iY                   = cursorPos.y() - (palettePos.y() + buttonPos.y()); // 光标相对按钮左上角的纵向偏移

    if (iX >= 30 && iX <= 40 && iY >= 30 && iY <= 40)
    {
        mbArrowClicked = true;
        penActionReleased();
    }
    else
    {
        mHoldTimerPen.start(PROPERTY_PALETTE_TIMER);
    }
}

void DesktopAnnotationController::penActionReleased()
{
    mHoldTimerPen.stop();
    if (mPendingPenButtonPressed)
    {
        // 长按（含定时器已接近超时）或点小箭头都展开面板，短按则执行工具动作。
        if (mbArrowClicked || mPenHoldTimer.msecsTo(QTime::currentTime()) > PROPERTY_PALETTE_TIMER - 100)
        {
            togglePropertyPalette(mDesktopPenPalette);
        }
        else
        {
            Application::mainWindow->ui().actionPen->trigger();
        }
        mPendingPenButtonPressed = false;
    }
    Application::mainWindow->ui().actionPen->setChecked(true);

    switchCursor(StylusTool::Pen);
}

void DesktopAnnotationController::eraserActionPressed()
{
    mbArrowClicked = false;
    mDesktopPenPalette->hide();
    mDesktopMarkerPalette->hide();
    DrawingController::drawingController()->setStylusTool(StylusTool::Eraser);
    mEraserHoldTimer            = QTime::currentTime();
    mPendingEraserButtonPressed = true;

    // 与笔按钮相同的判定：只有点在按钮右下角的小箭头上才直接展开面板。
    QPoint cursorPos            = QCursor::pos();
    QPoint palettePos           = mDesktopPalette->pos();
    QPoint buttonPos            = mDesktopPalette->buttonPos(Application::mainWindow->ui().actionEraser);

    int iX                      = cursorPos.x() - (palettePos.x() + buttonPos.x()); // 光标相对按钮左上角的横向偏移
    int iY                      = cursorPos.y() - (palettePos.y() + buttonPos.y()); // 光标相对按钮左上角的纵向偏移

    if (iX >= 30 && iX <= 40 && iY >= 30 && iY <= 40)
    {
        mbArrowClicked = true;
        eraserActionReleased();
    }
    else
    {
        mHoldTimerEraser.start(PROPERTY_PALETTE_TIMER);
    }
}

void DesktopAnnotationController::eraserActionReleased()
{
    mHoldTimerEraser.stop();
    if (mPendingEraserButtonPressed)
    {
        // 长按或点小箭头展开橡皮属性面板，短按只切工具。
        if (mbArrowClicked || mEraserHoldTimer.msecsTo(QTime::currentTime()) > PROPERTY_PALETTE_TIMER - 100)
        {
            togglePropertyPalette(mDesktopEraserPalette);
        }
        else
        {
            Application::mainWindow->ui().actionEraser->trigger();
        }
        mPendingEraserButtonPressed = false;
    }
    Application::mainWindow->ui().actionEraser->setChecked(true);

    switchCursor(StylusTool::Eraser);
}

void DesktopAnnotationController::markerActionPressed()
{
    mbArrowClicked = false;
    mDesktopPenPalette->hide();
    mDesktopEraserPalette->hide();
    DrawingController::drawingController()->setStylusTool(StylusTool::Marker);
    mMarkerHoldTimer            = QTime::currentTime();
    mPendingMarkerButtonPressed = true;

    // 与笔按钮相同的判定：只有点在按钮右下角的小箭头上才直接展开面板。
    QPoint cursorPos            = QCursor::pos();
    QPoint palettePos           = mDesktopPalette->pos();
    QPoint buttonPos            = mDesktopPalette->buttonPos(Application::mainWindow->ui().actionMarker);

    int iX                      = cursorPos.x() - (palettePos.x() + buttonPos.x()); // 光标相对按钮左上角的横向偏移
    int iY                      = cursorPos.y() - (palettePos.y() + buttonPos.y()); // 光标相对按钮左上角的纵向偏移

    if (iX >= 30 && iX <= 40 && iY >= 30 && iY <= 40)
    {
        mbArrowClicked = true;
        markerActionReleased();
    }
    else
    {
        mHoldTimerMarker.start(PROPERTY_PALETTE_TIMER);
    }
}

void DesktopAnnotationController::markerActionReleased()
{
    mHoldTimerMarker.stop();
    if (mPendingMarkerButtonPressed)
    {
        // 长按或点小箭头展开标记属性面板，短按只切工具。
        if (mbArrowClicked || mMarkerHoldTimer.msecsTo(QTime::currentTime()) > PROPERTY_PALETTE_TIMER - 100)
        {
            togglePropertyPalette(mDesktopMarkerPalette);
        }
        else
        {
            Application::mainWindow->ui().actionMarker->trigger();
        }
        mPendingMarkerButtonPressed = false;
    }
    Application::mainWindow->ui().actionMarker->setChecked(true);

    switchCursor(StylusTool::Marker);
}

void DesktopAnnotationController::selectorActionPressed()
{
}

void DesktopAnnotationController::selectorActionReleased()
{
    Application::mainWindow->ui().actionSelector->setChecked(true);
    switchCursor(StylusTool::Selector);
}

void DesktopAnnotationController::pointerActionPressed()
{
}

void DesktopAnnotationController::pointerActionReleased()
{
    Application::mainWindow->ui().actionPointer->setChecked(true);
    switchCursor(StylusTool::Pointer);
}

void DesktopAnnotationController::togglePropertyPalette(ActionPalette *palette)
{
    if (NULL != palette)
    {
        bool bShow = !palette->isVisible();
        if (mDesktopPenPalette == palette)
        {
            desktopPenActionToggled(bShow);
        }
        else if (mDesktopMarkerPalette == palette)
        {
            desktopMarkerActionToggled(bShow);
        }
        else if (mDesktopEraserPalette == palette)
        {
            desktopEraserActionToggled(bShow);
        }
    }
}

void DesktopAnnotationController::switchCursor(const int tool)
{
    mTransparentDrawingScene->setToolCursor(tool);
    mTransparentDrawingView->setToolCursor(tool);
}

void DesktopAnnotationController::onDesktopPaletteMaximized()
{
    // 工具栏每次展开都会重建按钮，这里重新连接按下/释放信号；
    // 收起时由 onDesktopPaletteMinimize() 成对断开，否则连接会越积越多。
    // 笔
    ActionPaletteButton *pPenButton = mDesktopPalette->getButtonFromAction(Application::mainWindow->ui().actionPen);
    if (NULL != pPenButton)
    {
        connect(pPenButton, &QAbstractButton::pressed, this, &DesktopAnnotationController::penActionPressed);
        connect(pPenButton, &QAbstractButton::released, this, &DesktopAnnotationController::penActionReleased);
    }

    // 橡皮
    ActionPaletteButton *pEraserButton = mDesktopPalette->getButtonFromAction(Application::mainWindow->ui().actionEraser);
    if (NULL != pEraserButton)
    {
        connect(pEraserButton, &QAbstractButton::pressed, this, &DesktopAnnotationController::eraserActionPressed);
        connect(pEraserButton, &QAbstractButton::released, this, &DesktopAnnotationController::eraserActionReleased);
    }

    // 标记
    ActionPaletteButton *pMarkerButton = mDesktopPalette->getButtonFromAction(Application::mainWindow->ui().actionMarker);
    if (NULL != pMarkerButton)
    {
        connect(pMarkerButton, &QAbstractButton::pressed, this, &DesktopAnnotationController::markerActionPressed);
        connect(pMarkerButton, &QAbstractButton::released, this, &DesktopAnnotationController::markerActionReleased);
    }

    // 选择器
    ActionPaletteButton *pSelectorButton = mDesktopPalette->getButtonFromAction(Application::mainWindow->ui().actionSelector);
    if (NULL != pSelectorButton)
    {
        connect(pSelectorButton, &QAbstractButton::pressed, this, &DesktopAnnotationController::selectorActionPressed);
        connect(pSelectorButton, &QAbstractButton::released, this, &DesktopAnnotationController::selectorActionReleased);
    }

    // 指针
    ActionPaletteButton *pPointerButton = mDesktopPalette->getButtonFromAction(Application::mainWindow->ui().actionPointer);
    if (NULL != pPointerButton)
    {
        connect(pPointerButton, &QAbstractButton::pressed, this, &DesktopAnnotationController::pointerActionPressed);
        connect(pPointerButton, &QAbstractButton::released, this, &DesktopAnnotationController::pointerActionReleased);
    }
}

void DesktopAnnotationController::onDesktopPaletteMinimize()
{
    // 断开连接既防重复触发，也避免按钮销毁后仍保留指向本控制器的连接。
    // 笔
    ActionPaletteButton *pPenButton = mDesktopPalette->getButtonFromAction(Application::mainWindow->ui().actionPen);
    if (NULL != pPenButton)
    {
        disconnect(pPenButton, &QAbstractButton::pressed, this, &DesktopAnnotationController::penActionPressed);
        disconnect(pPenButton, &QAbstractButton::released, this, &DesktopAnnotationController::penActionReleased);
    }

    // 标记
    ActionPaletteButton *pMarkerButton = mDesktopPalette->getButtonFromAction(Application::mainWindow->ui().actionMarker);
    if (NULL != pMarkerButton)
    {
        disconnect(pMarkerButton, &QAbstractButton::pressed, this, &DesktopAnnotationController::markerActionPressed);
        disconnect(pMarkerButton, &QAbstractButton::released, this, &DesktopAnnotationController::markerActionReleased);
    }

    // 橡皮
    ActionPaletteButton *pEraserButton = mDesktopPalette->getButtonFromAction(Application::mainWindow->ui().actionEraser);
    if (NULL != pEraserButton)
    {
        disconnect(pEraserButton, &QAbstractButton::pressed, this, &DesktopAnnotationController::eraserActionPressed);
        disconnect(pEraserButton, &QAbstractButton::released, this, &DesktopAnnotationController::eraserActionReleased);
    }

    // 选择器：展开时连了五组按钮，这里必须同样断五组。过去只断三组，
    // 每经历一轮“最小化→再展开”，选择器/指针就多一条连接，点一次触发多次。
    ActionPaletteButton *pSelectorButton = mDesktopPalette->getButtonFromAction(Application::mainWindow->ui().actionSelector);
    if (NULL != pSelectorButton)
    {
        disconnect(pSelectorButton, &QAbstractButton::pressed, this, &DesktopAnnotationController::selectorActionPressed);
        disconnect(pSelectorButton, &QAbstractButton::released, this, &DesktopAnnotationController::selectorActionReleased);
    }

    // 指针
    ActionPaletteButton *pPointerButton = mDesktopPalette->getButtonFromAction(Application::mainWindow->ui().actionPointer);
    if (NULL != pPointerButton)
    {
        disconnect(pPointerButton, &QAbstractButton::pressed, this, &DesktopAnnotationController::pointerActionPressed);
        disconnect(pPointerButton, &QAbstractButton::released, this, &DesktopAnnotationController::pointerActionReleased);
    }
}

void DesktopAnnotationController::relayout()
{
    onTransparentWidgetResized();
}

void DesktopAnnotationController::onTransparentWidgetResized()
{
    int rW = Application::boardController->paletteManager()->rightPalette()->width();
    int lW = Application::boardController->paletteManager()->leftPalette()->width();

    int rH = mTransparentDrawingView->height();

    // 先加 1 像素再恢复原宽，强制侧栏按新高度重新布局；单纯同尺寸 resize 会被 Qt 忽略。
    Application::boardController->paletteManager()->rightPalette()->resize(rW + 1, rH);
    Application::boardController->paletteManager()->rightPalette()->resize(rW, rH);

    Application::boardController->paletteManager()->leftPalette()->resize(lW + 1, rH);
    Application::boardController->paletteManager()->leftPalette()->resize(lW, rH);
}

void DesktopAnnotationController::onToolClicked()
{
    mDesktopEraserPalette->hide();
    mDesktopMarkerPalette->hide();
    mDesktopPenPalette->hide();
}
