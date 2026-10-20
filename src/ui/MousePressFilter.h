/**
 * @file MousePressFilter.h
 * @brief 事件过滤器声明：把“按住不动”的鼠标按下延迟转换为双击事件。
 */

#pragma once


#include <QtWidgets>

/**
 * @brief 将长按转换为双击的事件过滤器。
 *
 * 收到鼠标按下后不立刻放行，而是暂存一个双击事件并启动 1 秒计时；期间若出现
 * 松开或移动超过拖拽阈值就丢弃，否则把双击事件投递给原对象。设计意图是支持
 * 触控笔/手指长按触发双击语义。当前代码中只见 include，未见实例化，
 * 使用前需确认调用方的接入方式。
 */
class MousePressFilter : public QObject {
    Q_OBJECT

public:
    /// 构造过滤器；不安装到任何对象上，需调用方自行 installEventFilter。
    MousePressFilter(QObject *parent = 0);
    virtual ~MousePressFilter();

protected:
    /// 过滤鼠标按下/松开/移动三类事件，返回 false 表示不吞掉原事件。
    bool eventFilter(QObject *obj, QEvent *event);

protected slots:
    /// 计时结束：若按下事件仍待定，则把它作为双击事件投递出去。
    void mouseDownElapsed();

private:
    QObject *mObject;           ///< 待投递事件的接收对象。
    QMouseEvent *mPendingEvent; ///< 暂存的双击事件，松开或移动超阈值时销毁。
};
