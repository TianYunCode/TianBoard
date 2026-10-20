/**
 * @file SpinningWheel.h
 * @brief 旋转加载指示器的声明：12 根短线绕中心旋转的等待动画。
 */

#pragma once


#include <QWidget>

/**
 * @brief 小型的旋转等待指示器。
 *
 * 界面上是一个 16x16 的转轮，12 根短线按透明度渐变并逐帧旋转。由
 * MessageWindow 创建，放在提示文字左侧；只在显示加载类消息时通过
 * startAnimation() 启动，消息隐藏时停止。
 */
class SpinningWheel : public QWidget {
    Q_OBJECT

public:
    /// 构造静止状态的转轮（初始相位 9）。
    SpinningWheel(QWidget *parent = 0);
    virtual ~SpinningWheel();
    /// 绘制 12 根透明度递增的短线，并按当前相位整体旋转。
    virtual void paintEvent(QPaintEvent *event);

public slots:
    /// 以约 42ms 的间隔启动旋转；已启动时不重复计时。
    void startAnimation();
    /// 停止旋转并保留当前相位。
    void stopAnimation();

protected:
    /// 固定建议尺寸 16x16。
    virtual QSize sizeHint() const;
    /// 每帧递增相位并请求重绘。
    virtual void timerEvent(QTimerEvent *event);

private:
    QAtomicInt mPosition; ///< 当前相位（对 12 取模），用原子变量避免线程告警。
    int mTimerID;         ///< 动画定时器 id，0 表示未运行。
};
