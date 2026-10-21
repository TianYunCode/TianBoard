/**
 * @file AutoSaver.h
 * @brief 声明去抖式自动保存定时器。
 */

#pragma once

#include <QtCore>

/**
 * @brief 把连续变更合并成一次延迟保存的定时器。
 *
 * 详细说明：文档类对象在每次编辑后调用 changeOccurred()，本类在最后一次变更后等待
 * 固定时间再通过元对象系统调用父对象的 `save()` 槽；父对象在构造时传入，且必须
 * 提供该槽，本类不拥有父对象。线程约束：只能在 GUI 线程使用，QBasicTimer 要求同线程。
 */
class AutoSaver : public QObject
{
    Q_OBJECT;

public:
    /**
     * @brief 构造自动保存器。
     * @param parent 被保存对象，必须非空并提供可调用的 `save()` 槽。
     */
    AutoSaver(QObject *parent);
    /// 若仍有未落盘的变更，仅打印警告，不强行保存。
    ~AutoSaver();

    /// 立即执行一次待定的保存；没有活动的定时器时直接返回。
    void saveIfNeccessary();

public slots:
    /// 记录一次变更并按去抖/最长等待策略安排保存。
    void changeOccurred();

protected:
    /// 定时器到期时触发保存，其它定时器事件交还基类。
    void timerEvent(QTimerEvent *event);

private:
    QBasicTimer   mTimer;
    QElapsedTimer mFirstChange;
};
