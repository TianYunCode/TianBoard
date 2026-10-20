/**
 * @file BlackoutWidget.h
 * @brief 提供全屏黑屏遮罩窗口，用于临时遮住屏幕内容。
 */

#pragma once

#include <QWidget>

#include <memory>

/**
 * @brief 全屏黑屏遮罩窗口。
 *
 * 界面上是一块纯黑背景，中间是产品标志和“Click to Return to Application”提示。
 * DisplayManager::blackout() 在创建后调用 show()，并连接 activity() 以便在用户
 * 点击或按键时解除遮罩；对象设置了 WA_DeleteOnClose，由关闭动作自行销毁。
 */
class BlackoutWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造黑屏窗口并搭建标志与提示标签。
     * @param parent 父控件；通常为空以作为独立全屏窗口
     */
    BlackoutWidget(QWidget *parent = nullptr);
    ~BlackoutWidget() override;

    /**
     * @brief 显示或隐藏中央的标志与返回提示。
     * @param visible true 显示，false 只保留黑色背景
     */
    void setReturnControlsVisible(bool visible);

    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

signals:
    /** @brief 用户要求退出遮罩（点击或按下 B 键）。 */
    void activity();

public slots:
    /** @brief 发出 activity()，由外部决定如何关闭遮罩。 */
    void doActivity();

private:
    struct Private;
    std::unique_ptr<Private> d;
};
