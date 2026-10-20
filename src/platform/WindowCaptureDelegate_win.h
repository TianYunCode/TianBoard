/**
 * @file WindowCaptureDelegate_win.h
 * @brief 声明 Windows 下的窗口选择抓取委托。
 */

#pragma once

#include <QtWidgets>

/**
 * @brief Windows 平台上的窗口选择与抓取委托。
 *
 * 位于 Win32 平台层，被 WindowCapture 实例化：通过事件过滤器跟踪鼠标所在窗口，
 * 绘制高亮选择框，确认后抓取该窗口内容。mIsCapturing 期间独占输入，
 * 因此只能在 GUI 线程使用，且不应在捕获过程中销毁本对象。
 */
class WindowCaptureDelegate : public QObject {
    Q_OBJECT

public:
    /** @brief 构造委托，捕获流程由 execute() 启动。 */
    WindowCaptureDelegate(QObject *parent = 0);
    /** @brief 析构；若正在捕获，应确保输入已被释放。 */
    ~WindowCaptureDelegate();

    /** @brief 进入窗口选择流程，返回平台约定的结果码。 */
    int execute();
    /** @brief 返回抓取到的窗口图像；未抓取时为空。 */
    const QPixmap getCapturedWindow();

private:
    /** @brief 跟踪鼠标移动与点击，驱动选择与确认。 */
    bool eventFilter(QObject *target, QEvent *event);
    /** @brief 根据鼠标位置更新当前候选窗口。 */
    void processPos(QPoint pPoint);
    /** @brief 在屏幕上绘制当前候选窗口的选择框。 */
    void drawSelectionRect();

    bool mIsCapturing;       ///< 是否处于捕获流程中。
    bool mCancel;            ///< 用户是否已取消。
    QPoint mLastPoint;       ///< 上一次鼠标位置。
    HWND mCurrentWindow;     ///< 当前候选窗口句柄。
    QPixmap mCapturedPixmap; ///< 抓取结果。
};
