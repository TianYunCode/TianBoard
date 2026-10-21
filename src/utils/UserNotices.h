/**
 * @file UserNotices.h
 * @brief 服务层向用户报告的三个出口：导出告警清单、需要确认的一条消息、状态条上的一条进度。
 */

#pragma once

#include <QString>
#include <QStringList>

#include <functional>

/**
 * @brief 服务层不认识对话框，只把"该告诉用户什么"交出去，由装配层接上真正的界面。
 *
 * 详细说明：三个 presenter 都在启动时由装配点装上（app/main.cpp）——告警清单接
 * `ui/MessagesDialog`，需要确认的一条消息接一个模态提示框，进度接主窗口的状态条。
 * 没装上也不会静默丢信息：告警与确认消息退化成一 qWarning 日志，进度退化成 qInfo。
 * 这里住 utils 而不是 core：整个门面只有 QString 与 std::function，没有任何服务依赖，
 * 而它的调用方从 content 的导入导出器一直到 api 的小部件门面——放在 core 就等于要求
 * 每一个只想说一句"导出成功"的格式代码先认识应用对象。
 * 生命周期：注册表是进程级的函数对象，不持有任何界面对象；present* 在 GUI 线程调用
 * （导出完成、仓库检查与导入进度都发生在主线程）。
 */
namespace UserNotices
{
    /// @brief 告警清单的呈现方式：标题加若干条明细，非阻塞（对话框自己 show）。
    using WarningPresenter     = std::function<void(const QString &title, const QStringList &messages)>;

    /// @brief 一条需要用户确认的消息的呈现方式：标题加正文，阻塞到用户关闭。
    using ModalNoticePresenter = std::function<void(const QString &title, const QString &text)>;

    /// @brief 状态条进度的呈现方式：一条文本加一个"忙"标记（转起来的那个圈）。
    using StatusPresenter      = std::function<void(const QString &message, bool showBusyIndicator)>;

    /// @brief 装上告警清单的呈现方式，只在装配阶段调用一次。
    void installWarningPresenter(WarningPresenter presenter);
    /// @brief 装上确认消息的呈现方式，只在装配阶段调用一次。
    void installModalNoticePresenter(ModalNoticePresenter presenter);
    /// @brief 装上状态条进度的呈现方式，只在装配阶段调用一次。
    void installStatusPresenter(StatusPresenter presenter);
    /// @brief 状态条呈现方式是否已装上；自检用它确认装配点没漏（漏了消息只会默默进日志）。
    bool hasStatusPresenter();

    /// @brief 报告一份导出告警清单；没装呈现方式时退化成日志。
    void presentWarnings(const QString &title, const QStringList &messages);
    /// @brief 报告一条需要确认的消息；没装呈现方式时退化成日志。
    void presentModalNotice(const QString &title, const QString &text);
    /// @brief 报告一条状态条进度（导入导出的第几页这类）；没装呈现方式时退化成 qInfo。
    void presentStatusMessage(const QString &message, bool showBusyIndicator = false);
} // namespace UserNotices
