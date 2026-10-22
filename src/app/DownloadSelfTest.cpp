#include "app/DownloadSelfTest.h"

#include "app/SelfTestAssets.h"
#include "core/Application.h"
#include "core/ApplicationController.h"
#include "core/DisplayManager.h"
#include "core/DownloadManager.h"
#include "core/PersistenceManager.h"
#include "model/DocumentProxy.h"
#include "model/PackageLayout.hpp"
#include "ui/DockDownloadWidget.h"
#include "ui/MainWindow.h"
#include "whiteboard/BoardController.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QStringList>
#include <QUrl>

#include <functional>

namespace
{
    /**
     * @brief 有界地等事件循环把跨线程的下载完成信号送进来。
     * @param settled 判定“等到了”的谓词。
     * @return 等到为 true；200 轮 × 50ms（约 10 秒）后仍不满足为 false。
     */
    bool waitUntil(const std::function<bool()> &settled)
    {
        for (int tick = 0; tick < 200 && !settled(); ++tick)
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        }

        return settled();
    }

    /// @brief 右侧面板上那个下载标签页：它是“画板有没有应答下载起止”唯一看得见的地方。
    DockDownloadWidget *downloadTab()
    {
        return Application::mainWindow ? Application::mainWindow->findChild<DockDownloadWidget *>() : nullptr;
    }

    /// @brief 下载标签页此刻该不该出现（调色板自己记的逻辑可见状态）。
    bool downloadTabShown()
    {
        DockDownloadWidget *tab = downloadTab();
        return tab && tab->visibleState();
    }

    /// @brief 文档包里 audios/ 下的 wav 文件名。
    QStringList landedAudioFiles(DocumentProxy *proxy)
    {
        const QDir audioDir(QDir(proxy->persistencePath()).filePath(PackageLayout::audioDirectory));
        return audioDir.entryList({QStringLiteral("*.wav")}, QDir::Files, QDir::Name);
    }

    /// @brief 把已攒下的不一致逐条报出来再退场（自检的退出码就是条数）。
    int bail(const QStringList &failures)
    {
        for (const QString &failure : failures)
        {
            qWarning() << "download-selftest:" << failure;
        }

        return failures.count();
    }
} // namespace

namespace DownloadSelfTest
{
    int run()
    {
        QStringList failures;

        BoardController *board = BoardController::instance();
        if (!board)
        {
            qWarning() << "download-selftest: 画板总控器还没装配";
            return 1;
        }

        const QString scratch =
            QDir(QDir::tempPath()).filePath(QStringLiteral("TianBoard-DownloadSelfTest-%1").arg(QCoreApplication::applicationPid()));
        QDir(scratch).removeRecursively();
        const QString outsideDir = QDir(scratch).filePath(QStringLiteral("outside"));
        QDir().mkpath(outsideDir);

        PersistenceManager *pm    = PersistenceManager::persistenceManager();
        DocumentProxy      *proxy = pm->createDocument(QString(), QStringLiteral("DownloadSelfTest"), true, QDir(scratch).filePath("doc"), 0, false);
        if (!proxy)
        {
            qWarning() << "download-selftest: 建临时文档失败";
            return 1;
        }

        board->setActiveDocumentScene(proxy, 0);

        DownloadManager *dm = DownloadManager::downloadManager();

        // ---- 1. 目标文档提供者：本地复制在进队列前就要知道往哪份文档的包里放 ----
        if (!DownloadManager::targetDocument())
        {
            failures << QStringLiteral("下载服务给不出目标文档——装配点没装 installTargetDocumentProvider，"
                                       "拖进来的文件无处可去（旧写法是下载服务直接命令画板）");
        }
        else if (DownloadManager::targetDocument() != board->selectedDocument())
        {
            failures << QStringLiteral("下载的目标文档不是画板当前打开的那一份");
        }

        if (!downloadTab() || downloadTabShown())
        {
            failures << QStringLiteral("下载标签页的初始状态不对（找不到=%1，一开始就显示=%2）")
                            .arg(downloadTab() ? "否" : "是", downloadTabShown() ? "是" : "否");
        }

        // 自己再挂一份应答，量的是“服务有没有把事实报出来”；上面那个标签页量的是“画板有没有应答”。
        int                           queuedCount  = 0;
        int                           drainedCount = 0;
        int                           liftedCount  = 0;
        const QMetaObject::Connection queued       = QObject::connect(dm, &DownloadManager::downloadsQueued,
            [&queuedCount]
            {
                ++queuedCount;
            });
        const QMetaObject::Connection drained      = QObject::connect(dm, &DownloadManager::downloadsDrained,
            [&drainedCount]
            {
                ++drainedCount;
            });

        DisplayManager               *display = Application::applicationController ? Application::applicationController->displayManager() : nullptr;
        const QMetaObject::Connection lifted  = display ? QObject::connect(display, &DisplayManager::blackoutLifted,
                                                              [&liftedCount]
                                                              {
                                                                 ++liftedCount;
                                                              })
                                                        : QMetaObject::Connection();

        // ---- 2. 端到端：文档外的 WAV 排队 → 复制进包 → 队列收尾 ----
        const QString sourcePath              = QDir(outsideDir).filePath(QStringLiteral("dragged-in.wav"));
        if (!SelfTestAssets::writeBytes(sourcePath, SelfTestAssets::toneWav()))
        {
            failures << QStringLiteral("造不出源文件 %1").arg(sourcePath);
        }
        else
        {
            sDownloadFileDesc desc;
            desc.dest            = sDownloadFileDesc::board;
            desc.name            = QFileInfo(sourcePath).fileName();
            desc.srcUrl          = QUrl::fromLocalFile(sourcePath).toString();

            const int downloadId = dm->addFileToDownload(desc);

            // 入队那条事实是同步发出来的：addFileToDownload 返回时画板应当已经把页签放出来了。
            if (queuedCount != 1)
            {
                failures << QStringLiteral("一次入队应当回报“有下载入队”一次，实际 %1 次").arg(queuedCount);
            }
            if (!downloadTabShown())
            {
                failures << QStringLiteral("入队后下载标签页没出现——画板没应答“有下载入队”这条事实");
            }

            if (!waitUntil(
                    [&drainedCount]
                    {
                        return drainedCount > 0;
                    }))
            {
                failures << QStringLiteral("本地下载 %1 走完却没有回报“队列排空”——收尾那条事实丢了").arg(downloadId);
            }

            if (drainedCount != 1)
            {
                failures << QStringLiteral("一次下载收尾应当回报“队列排空”一次，实际 %1 次").arg(drainedCount);
            }
            if (drainedCount > 0 && downloadTabShown())
            {
                failures << QStringLiteral("队列都排空了，下载标签页还赖在右侧面板上");
            }

            const QStringList landed = landedAudioFiles(proxy);
            if (landed.isEmpty())
            {
                failures << QStringLiteral("文档包的 audios/ 里没有留下任何 wav——下载没把文件放进这份文档");
            }

            bool byteMatch = false;
            for (const QString &name : landed)
            {
                QFile copied(QDir(proxy->persistencePath()).filePath(QDir(PackageLayout::audioDirectory).filePath(name)));
                if (copied.open(QIODevice::ReadOnly) && copied.readAll() == SelfTestAssets::toneWav())
                {
                    byteMatch = true;
                }
            }
            if (!byteMatch)
            {
                failures << QStringLiteral("audios/ 里的 %1 个文件没有一个和源 WAV 的字节一致").arg(landed.count());
            }

            if (!QFile::exists(sourcePath))
            {
                failures << QStringLiteral("下载把文档外的源文件 %1 弄没了——本地复制只该复制，不该搬走").arg(sourcePath);
            }
        }

        // ---- 3. 黑屏解除这条事实：解除动作只报事实，解冻网页小部件由画板应答 ----
        if (!display)
        {
            failures << QStringLiteral("拿不到多屏管理器，黑屏解除那条事实无从可查");
        }
        else
        {
            display->unBlackout(); // 当前没有黑屏表面，这一调就是纯“解除”，顺便量它报不报事实
            if (liftedCount != 1)
            {
                failures << QStringLiteral("解除黑屏应当回报一次“黑屏已解除”，实际 %1 次").arg(liftedCount);
            }
        }

        QObject::disconnect(queued);
        QObject::disconnect(drained);
        if (lifted)
        {
            QObject::disconnect(lifted);
        }

        if (failures.isEmpty())
        {
            qInfo() << "download-selftest: OK 下载起止与黑屏解除都有人应答，本地拖进来的文件按字节进了文档包";
        }

        const int result = bail(failures);

        pm->deleteDocument(proxy);
        if (failures.isEmpty())
        {
            QDir(scratch).removeRecursively();
        }
        else
        {
            qWarning() << "download-selftest: 现场保留在" << scratch;
        }

        return result;
    }
} // namespace DownloadSelfTest
