/**
 * @file UserNotices.h
 * @brief 服务层向用户报告问题的两个出口：导出告警清单与需要确认的一条消息。
 */

#pragma once

#include <QString>
#include <QStringList>

#include <functional>

/**
 * @brief 服务层不认识对话框，只把"该告诉用户什么"交出去，由装配层接上真正的界面。
 *
 * 详细说明：两个 presenter 都在启动时由装配点装上（app/main.cpp）——告警清单接
 * `ui/MessagesDialog`，需要确认的一条消息接一个模态提示框。没装上也不会静默丢信息：
 * 两个函数都退化成一 qWarning 日志，导出与文档仓库检查照常继续。
 * 生命周期：注册表是进程级的函数对象，不持有任何界面对象；present* 在 GUI 线程调用
 * （导出完成与仓库检查都发生在主线程）。
 */
namespace UserNotices
{
    /// @brief 告警清单的呈现方式：标题加若干条明细，非阻塞（对话框自己 show）。
    using WarningPresenter     = std::function<void(const QString &title, const QStringList &messages)>;

    /// @brief 一条需要用户确认的消息的呈现方式：标题加正文，阻塞到用户关闭。
    using ModalNoticePresenter = std::function<void(const QString &title, const QString &text)>;

    /// @brief 装上告警清单的呈现方式，只在装配阶段调用一次。
    void installWarningPresenter(WarningPresenter presenter);
    /// @brief 装上确认消息的呈现方式，只在装配阶段调用一次。
    void installModalNoticePresenter(ModalNoticePresenter presenter);

    /// @brief 报告一份导出告警清单；没装呈现方式时退化成日志。
    void presentWarnings(const QString &title, const QStringList &messages);
    /// @brief 报告一条需要确认的消息；没装呈现方式时退化成日志。
    void presentModalNotice(const QString &title, const QString &text);
} // namespace UserNotices
