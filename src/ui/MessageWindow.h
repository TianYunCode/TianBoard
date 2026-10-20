/**
 * @file MessageWindow.h
 * @brief 底部悬浮消息条的声明：显示一行提示文字，可选转圈动画，随后自动淡出。
 */

#pragma once


#include <QtWidgets>
#include <QHBoxLayout>
#include <QLabel>

#include <memory>

#include "ui/FloatingPalette.h"

class SpinningWheel;

/**
 * @brief 悬浮在窗口左下角的状态消息条。
 *
 * 界面上是半透明圆角背景上的一行白字，必要时左侧带一个旋转的加载指示器。
 * 由 BoardController 与 DocumentController 各创建一个，分别挂在自己的容器
 * （控制容器、文档缩略图）下方，用于提示“正在加载/保存”等短消息；显示后
 * 若不带转圈会在约 1 秒内逐级降低透明度并自动隐藏。它只是提示，不接收输入。
 */
class MessageWindow : public FloatingPalette {
    Q_OBJECT

public:
    /// 构造消息条；文字与转圈控件都以 parent 为父对象创建。
    MessageWindow(QWidget *parent = 0);
    virtual ~MessageWindow();

    /**
     * @brief 显示一条消息。
     * @param message 要显示的文本。
     * @param showSpinningWheel 为 true 时显示加载转圈并保持不自动隐藏。
     * @note 会立即重绘并处理挂起事件，保证消息在耗时操作开始前就可见。
     */
    void showMessage(const QString &message, bool showSpinningWheel = false);
    /// 立即隐藏消息并停止（可能的）淡出计时。
    void hideMessage();

protected:
    /// 淡出计时：每触发一次降低一档透明度，归零后隐藏。
    void timerEvent(QTimerEvent *event);

private:
    struct Private;
    std::unique_ptr<Private> d;
};
