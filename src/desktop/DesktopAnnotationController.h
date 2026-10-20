/**
 * @file DesktopAnnotationController.h
 * @brief 声明桌面批注控制器，管理透明绘图层、桌面工具栏及其属性面板。
 */

#pragma once

#include <QtWidgets>
#include <QTime>
#include <QTimer>

#include "ui/RightPalette.h"

class DesktopPalette;
class BoardView;
class GraphicsScene;
class DesktopPenPalette;
class DesktopMarkerPalette;
class DesktopEraserPalette;
class ActionPalette;
class MainWindow;
class RightPalette;

#define PROPERTY_PALETTE_TIMER 1000

/**
 * @brief 桌面批注的总控：在桌面之上铺一层透明画布，并协调工具栏、属性面板与区域截图。
 *
 * 构造时创建覆盖整个虚拟桌面的无边框透明 BoardView 及其 GraphicsScene，场景背景在
 * “完全透明”与“极淡灰”之间切换，用于在桌面上直接书写；笔/橡皮/标记面板是该视图的子控件。
 * 由 ApplicationController 持有（mUninoteController），随应用生命周期存在，不随进出桌面模式销毁：
 * showWindow() 保存画板当前工具并切到桌面工具，hideWindow() 再恢复，避免两套上下文互相污染。
 * 工具栏与属性面板都浮在透明画布上，位置由桌面工具栏与右侧栏宽度共同决定。
 * 线程约束：全部操作在 GUI 线程；截图期间会先隐藏工具栏，抓完再恢复。
 */
class DesktopAnnotationController : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造控制器并搭建透明画布、桌面工具栏与笔/标记/橡皮属性面板。
     * @param parent 父对象，通常为 ApplicationController。
     * @param rightPalette 主界面右侧栏，用于计算面板避让偏移。
     */
    DesktopAnnotationController(QObject *parent, RightPalette *rightPalette);

    /** @brief 析构；先删场景再删视图，避免视图析构时访问已释放的场景。 */
    virtual ~DesktopAnnotationController();

    /** @brief 进入桌面批注：显示工具栏并全屏铺开透明画布，同时保存画板工具、切到桌面工具。 */
    void showWindow();

    /** @brief 退出桌面批注：隐藏画布与工具栏，并恢复进入前的画板工具。 */
    void hideWindow();

    /** @brief 桌面工具栏对象，供外部连接/定位。 */
    DesktopPalette *desktopPalette();

    /** @brief 透明绘图层视图，供外部截图或设置父窗口。 */
    BoardView *drawingView();

    /** @brief 请求按当前视图高度重排左右侧栏；转发给 onTransparentWidgetResized()。 */
    void TransparentWidgetResized();

public slots:
    /** @brief 显示器布局变化：有可用副屏时显示“在副屏显示画板”按钮。 */
    void screenLayoutChanged();

    /** @brief 回到白板主界面；由桌面工具栏的“显示 TianBoard”按钮触发。 */
    void goToBoard();

    /** @brief 区域截图：隐藏工具栏，框选后把图像作为抓图结果发出。 */
    void customCapture();

    /** @brief 窗口截图：隐藏工具栏，选择窗口后把抓到的窗口图像发出。 */
    void windowCapture();

    /** @brief 全屏截图：隐藏工具栏，抓取当前屏幕后立即发出结果。 */
    void screenCapture();

    /** @brief 同步工具栏上显示/隐藏副屏按钮的状态。 */
    void updateShowHideState(bool pEnabled);

    /** @brief 隐藏透明画布与桌面工具栏；不恢复画板工具，供退出路径复用。 */
    void close();

    /** @brief 工具切换后刷新场景背景（选择器需要完全透明）。 */
    void stylusToolChanged(int tool);

    /** @brief 按当前状态重设透明场景背景刷。 */
    void updateBackground();

signals:
    /** @brief 截图完成；pageMode 表示是否按整页插入（桌面截图当前均为 false）。 */
    void imageCaptured(const QPixmap &pCapturedPixmap, bool pageMode);

    /** @brief 请求退出桌面模式、恢复白板显示。 */
    void restoreBoard();

protected:
    /** @brief 抓取光标所在屏幕的当前画面，用于区域/全屏截图。 */
    QPixmap getScreenPixmap();

    BoardView *mTransparentDrawingView;
    GraphicsScene *mTransparentDrawingScene;

private slots:
    /** @brief 跟随画板背景明暗调整透明场景的底色。 */
    void updateColors();

    /** @brief 笔属性面板的显示切换，同时隐藏其他属性面板。 */
    void desktopPenActionToggled(bool checked);

    /** @brief 标记属性面板的显示切换，同时隐藏其他属性面板。 */
    void desktopMarkerActionToggled(bool checked);

    /** @brief 橡皮属性面板的显示切换，同时隐藏其他属性面板。 */
    void desktopEraserActionToggled(bool checked);

    /** @brief 清空桌面画布上的全部批注。 */
    void eraseDesktopAnnotations();

    /** @brief 笔按钮按下：切到笔工具并按“长按/点箭头”决定是否展开属性面板。 */
    void penActionPressed();

    /** @brief 标记按钮按下：切到标记工具并按“长按/点箭头”决定是否展开属性面板。 */
    void markerActionPressed();

    /** @brief 橡皮按钮按下：切到橡皮工具并按“长按/点箭头”决定是否展开属性面板。 */
    void eraserActionPressed();

    /** @brief 笔按钮释放：短按触发动作，长按或点箭头展开属性面板。 */
    void penActionReleased();

    /** @brief 标记按钮释放：短按触发动作，长按或点箭头展开属性面板。 */
    void markerActionReleased();

    /** @brief 橡皮按钮释放：短按触发动作，长按或点箭头展开属性面板。 */
    void eraserActionReleased();

    /** @brief 选择器按钮按下；当前无需处理。 */
    void selectorActionPressed();

    /** @brief 选择器按钮释放：勾选动作并切换光标。 */
    void selectorActionReleased();

    /** @brief 指针按钮按下；当前无需处理。 */
    void pointerActionPressed();

    /** @brief 指针按钮释放：勾选动作并切换光标。 */
    void pointerActionReleased();

    /** @brief 同步透明视图与场景的鼠标光标。 */
    void switchCursor(int tool);

    /** @brief 工具栏展开后重新连接各按钮的按下/释放信号。 */
    void onDesktopPaletteMaximized();

    /** @brief 工具栏收起时断开按钮信号，避免重复连接。 */
    void onDesktopPaletteMinimize();

    /** @brief 透明视图尺寸变化后强制重排左右侧栏。 */
    void onTransparentWidgetResized();

    /** @brief 点击任意工具后收起三个属性面板。 */
    void onToolClicked();

private:
    /** @brief 把属性面板定位到桌面工具栏对应按钮的左侧或右侧。 */
    void setAssociatedPalettePosition(ActionPalette *palette, const QString &actionName);

    /** @brief 切换指定属性面板的可见性（内部转调对应的 toggled 槽）。 */
    void togglePropertyPalette(ActionPalette *palette);

    DesktopPalette *mDesktopPalette;
    DesktopPenPalette *mDesktopPenPalette;
    DesktopMarkerPalette *mDesktopMarkerPalette;
    DesktopEraserPalette *mDesktopEraserPalette;

    RightPalette *mRightPalette;

    QTime mPenHoldTimer;
    QTime mMarkerHoldTimer;
    QTime mEraserHoldTimer;
    QTimer mHoldTimerPen;
    QTimer mHoldTimerMarker;
    QTimer mHoldTimerEraser;

    bool mWindowPositionInitialized;
    bool mIsFullyTransparent;
    bool mDesktopToolsPalettePositioned;
    bool mPendingPenButtonPressed;
    bool mPendingMarkerButtonPressed;
    bool mPendingEraserButtonPressed;
    bool mbArrowClicked;

    int mBoardStylusTool;
    int mDesktopStylusTool;
};
