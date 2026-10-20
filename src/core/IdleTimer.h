/**
 * @file IdleTimer.h
 * @brief 声明空闲检测器：输入静默后隐藏鼠标指针。
 */

#pragma once

#include <QObject>
#include <QDateTime>

class QEvent;

/**
 * @brief 空闲计时器：监听输入事件，空闲时把光标换成空白光标，再次输入时恢复。
 *
 * 详细说明：作为事件过滤器安装在应用对象上（由使用方安装），只在画板视图拥有焦点
 * 且没有等待光标时隐藏指针；对象生命周期由父对象管理。
 * 线程约束：依赖 GUI 事件循环与定时器，只能在 GUI 线程使用。
 */
class IdleTimer : public QObject {
    Q_OBJECT

public:
    /// 构造并启动周期为 100ms 的检测定时器。
    IdleTimer(QObject *parent = 0);
    virtual ~IdleTimer();

protected:
    /// 记录输入事件时间并在需要时恢复光标；其余事件继续转发。
    bool eventFilter(QObject *obj, QEvent *event);
    /// 周期检查空闲时长，决定是否隐藏光标。
    virtual void timerEvent(QTimerEvent *event);

private:
    QDateTime mLastInputEventTime;
    bool mCursorIsHidden;
};
