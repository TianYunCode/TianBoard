#include "utils/UserNotices.h"

#include <QDebug>

namespace
{
    UserNotices::WarningPresenter     gWarningPresenter;
    UserNotices::ModalNoticePresenter gModalNoticePresenter;
    UserNotices::StatusPresenter      gStatusPresenter;

    /// @brief 没装呈现方式时只报一次，免得导出批量失败时刷屏。
    template <typename Presenter> void warnOnce(const Presenter &presenter, const char *what)
    {
        if (!presenter)
        {
            static bool reported = false;

            if (!reported)
            {
                reported = true;
                qWarning() << what << "没有装上呈现方式，这类提示只进日志（装配点应调用 UserNotices::install*）";
            }
        }
    }
} // namespace

namespace UserNotices
{
    void installWarningPresenter(WarningPresenter presenter)
    {
        gWarningPresenter = std::move(presenter);
    }

    void installModalNoticePresenter(ModalNoticePresenter presenter)
    {
        gModalNoticePresenter = std::move(presenter);
    }

    void installStatusPresenter(StatusPresenter presenter)
    {
        gStatusPresenter = std::move(presenter);
    }

    bool hasStatusPresenter()
    {
        return static_cast<bool>(gStatusPresenter);
    }

    void presentWarnings(const QString &title, const QStringList &messages)
    {
        warnOnce(gWarningPresenter, "导出告警清单");

        if (gWarningPresenter)
        {
            gWarningPresenter(title, messages);
        }
        else
        {
            qWarning() << title << messages;
        }
    }

    void presentModalNotice(const QString &title, const QString &text)
    {
        warnOnce(gModalNoticePresenter, "需要确认的消息");

        if (gModalNoticePresenter)
        {
            gModalNoticePresenter(title, text);
        }
        else
        {
            qWarning() << title << text;
        }
    }

    void presentStatusMessage(const QString &message, bool showBusyIndicator)
    {
        warnOnce(gStatusPresenter, "状态条消息");

        if (gStatusPresenter)
        {
            gStatusPresenter(message, showBusyIndicator);
        }
        else
        {
            // 进度消息是常规噪音，退化成 qInfo 而不是 qWarning：自检与冒烟跑在没装呈现方式
            // 时会一条一条过这里，警告级会把日志冲满。
            qInfo() << message;
        }
    }
} // namespace UserNotices
