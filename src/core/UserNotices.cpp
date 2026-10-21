#include "core/UserNotices.h"

#include <QDebug>

namespace
{
    UserNotices::WarningPresenter     gWarningPresenter;
    UserNotices::ModalNoticePresenter gModalNoticePresenter;

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
} // namespace UserNotices
