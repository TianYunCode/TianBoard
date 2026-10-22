#include "app/AutoSaveSelfTest.h"

#include "core/ApplicationController.h"
#include "settings/Setting.h"
#include "settings/Settings.h"
#include "utils/FileSystemUtils.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/GraphicsScene.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPainter>
#include <QPixmap>
#include <QTimer>

namespace
{
    /// @brief 数一数总控器名下有几个正在计时的定时器（自动保存那一个是唯一该有的）。
    QList<QTimer *> activeAutosaveTimers(BoardController *board)
    {
        QList<QTimer *> active;

        foreach (QTimer *timer, board->findChildren<QTimer *>())
        {
            if (timer->isActive())
            {
                active << timer;
            }
        }

        return active;
    }

    /// @brief 当前那一页的页文件路径。
    QString currentPageFile(BoardController *board)
    {
        return board->selectedDocument()->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.svg", board->activeSceneIndex());
    }

    /// @brief 把已攒下的不一致逐条报出来再退场（自检的退出码就是条数）。
    int bail(const QStringList &failures)
    {
        for (const QString &failure : failures)
        {
            qWarning() << "autosave-selftest:" << failure;
        }

        return failures.count();
    }
} // namespace

namespace AutoSaveSelfTest
{
    int run()
    {
        QStringList failures;

        BoardController *board = BoardController::instance();
        if (!board)
        {
            qWarning() << "autosave-selftest: 画板总控器还没装配";
            return 1;
        }

        Setting  *intervalSetting = Settings::settings()->autoSaveInterval;
        const int originalMinutes = intervalSetting->get().toInt();

        // ---- 1. 开机之后就该有一个正在计时的定时器，间隔按设置的分钟换算 ----
        // 这一条量的是过去真正的坑：定时器只在 appMainModeChanged() 里创建，
        // 而开机就是画板模式、一整会话不换模式的用户永远走不到那个槽。
        QList<QTimer *> timers    = activeAutosaveTimers(board);
        if (timers.count() != 1)
        {
            failures << QStringLiteral("画板装配之后应当有 1 个正在计时的自动保存定时器，实际 %1 个").arg(timers.count());
        }
        else if (originalMinutes > 0 && timers.first()->interval() != originalMinutes * 60 * 1000)
        {
            failures << QStringLiteral("自动保存间隔不对：设置是 %1 分钟，定时器却是 %2 毫秒").arg(originalMinutes).arg(timers.first()->interval());
        }

        // ---- 2. 改设置**当场**就要生效：不调任何模式回调 ----
        // 旧写法里重排只发生在 appMainModeChanged()，所以改了间隔（或改成关闭）之后，
        // 画板还要等下一次换主模式才按新值走——这里刻意不喂那次回调，量的就是"立刻"。
        intervalSetting->setInt(2);

        timers = activeAutosaveTimers(board);
        if (timers.count() != 1)
        {
            failures << QStringLiteral("改设置之后应当还有 1 个活动的自动保存定时器，实际 %1 个").arg(timers.count());
        }
        else if (timers.first()->interval() != 2 * 60 * 1000)
        {
            failures << QStringLiteral("设置改成 2 分钟之后定时器还是 %1 毫秒（改完没立刻重排）").arg(timers.first()->interval());
        }

        // ---- 3. 设成 0 就是关掉：当场不许留一个在跑的定时器 ----
        intervalSetting->setInt(0);

        timers = activeAutosaveTimers(board);
        if (!timers.isEmpty())
        {
            failures << QStringLiteral("间隔设成 0（关闭自动保存）之后还有 %1 个定时器在跑").arg(timers.count());
        }

        // ---- 4. 恢复原值，定时器要当场重新起来 ----
        intervalSetting->setInt(originalMinutes > 0 ? originalMinutes : 3);

        timers = activeAutosaveTimers(board);
        if (timers.count() != 1)
        {
            failures << QStringLiteral("恢复设置之后自动保存定时器没能重新跑起来（%1 个）").arg(timers.count());
        }

        // ---- 4b. 主模式这一维仍然管得住：离开画板模式就停，回到画板模式就跑 ----
        board->appMainModeChanged(ApplicationController::Document);
        timers = activeAutosaveTimers(board);
        if (!timers.isEmpty())
        {
            failures << QStringLiteral("切出画板模式之后还有 %1 个自动保存定时器在跑").arg(timers.count());
        }

        board->appMainModeChanged(ApplicationController::Board);
        timers = activeAutosaveTimers(board);
        if (timers.count() != 1)
        {
            failures << QStringLiteral("回到画板模式之后自动保存定时器没能重新跑起来（%1 个）").arg(timers.count());
        }

        // ---- 5. saveData 必须真把这一页写到盘上 ----
        const QString pageFile = currentPageFile(board);

        qint64 sizeBefore      = QFileInfo(pageFile).size();
        if (sizeBefore <= 0)
        {
            failures << QStringLiteral("当前页的页文件不在或为空，没法比保存前后的差别: %1").arg(pageFile);
        }
        else
        {
            QPixmap dot(24, 24);
            dot.fill(QColor(180, 40, 160));
            QPainter painter(&dot);
            painter.setPen(QPen(QColor(20, 20, 200), 3));
            painter.drawLine(2, 2, 21, 21);
            painter.end();

            GraphicsScene *scene = board->activeScene();
            if (!scene)
            {
                failures << QStringLiteral("画板没有活动场景，saveData 这条验不了");
            }
            else
            {
                scene->addPixmap(dot, nullptr, QPointF(300, 300), 1.0, false, true);

                board->saveData(BoardController::sf_none);

                const qint64 sizeAfter = QFileInfo(pageFile).size();
                if (sizeAfter <= sizeBefore)
                {
                    failures << QStringLiteral("saveData 之后页文件没有变长（%1 → %2 字节），这一页大概没真落盘").arg(sizeBefore).arg(sizeAfter);
                }
            }
        }

        if (failures.isEmpty())
        {
            qInfo() << "autosave-selftest: OK 定时器随设置起停，saveData 真落盘";
        }

        return bail(failures);
    }
} // namespace AutoSaveSelfTest
