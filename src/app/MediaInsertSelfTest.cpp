#include "app/MediaInsertSelfTest.h"

#include "content/SvgSubsetAdaptor.h"
#include "core/PersistenceManager.h"
#include "model/DocumentProxy.h"
#include "model/PackageLayout.hpp"
#include "whiteboard/BoardController.h"
#include "whiteboard/GraphicsMediaItem.h"
#include "whiteboard/Item.h"
#include "whiteboard/GraphicsScene.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>

namespace
{
    /// @brief 合成一个合法的 8bit 单声道 WAV（0.1 秒）；当作用户桌面上那个"要拖进画板的音频"。
    QByteArray buildWav()
    {
        const int  sampleCount = 800;
        QByteArray body(sampleCount, '\0');
        for (int i = 0; i < sampleCount; ++i)
        {
            body[i] = static_cast<char>(128 + (i % 32) - 16);
        }

        QByteArray  wav;
        QDataStream out(&wav, QIODevice::WriteOnly);
        out.setByteOrder(QDataStream::LittleEndian);

        out.writeRawData("RIFF", 4);
        out << qint32(36 + body.size());
        out.writeRawData("WAVE", 4);
        out.writeRawData("fmt ", 4);
        out << qint32(16) << quint16(1) << quint16(1) << qint32(8000) << qint32(8000) << quint16(1) << quint16(8);
        out.writeRawData("data", 4);
        out << qint32(body.size());
        out.writeRawData(body.constData(), body.size());

        return wav;
    }

    /// @brief 场景里媒体图元的个数：只数顶层图元（每个媒体图元还挂着子图元，一起数会翻倍）。
    int countMediaItems(GraphicsScene *scene)
    {
        int count = 0;

        if (!scene)
        {
            return 0;
        }

        foreach (QGraphicsItem *item, scene->items())
        {
            // 只数挂在场景顶层的媒体图元：媒体图元自己挂的子图元也会被 dynamic_cast
            // 认成媒体图元，混进来数目就翻倍（实测过）。
            if (item->parentItem() == nullptr && dynamic_cast<GraphicsMediaItem *>(item))
            {
                ++count;
            }
        }

        return count;
    }

    /**
     * @brief 把一个放在文档之外的 WAV 插进当前页，然后要求它真的成为文档的一部分。
     * @param withData 是否把字节交给插入接口——两条分支的差别就是这条自检的核心。
     */
    void insertAndAudit(QStringList &failures, BoardController *board, PersistenceManager *pm, DocumentProxy *proxy, const QString &outsideDir,
        const QString &fileName, bool withData)
    {
        const QString stage      = withData ? QStringLiteral("带着字节插入") : QStringLiteral("只给文件路径插入");

        const QString sourcePath = QDir(outsideDir).filePath(fileName);
        {
            QFile sourceFile(sourcePath);
            if (!sourceFile.open(QIODevice::WriteOnly))
            {
                failures << QStringLiteral("%1：造不出源媒体文件 %2").arg(stage, sourcePath);
                return;
            }
            sourceFile.write(buildWav());
        }

        const QUrl       sourceUrl = QUrl::fromLocalFile(sourcePath);
        const QByteArray payload   = withData ? buildWav() : QByteArray();

        Item *created =
            board->downloadFinished(true, sourceUrl, QUrl(), QStringLiteral("audio/x-wav"), payload, QPointF(300, 300), QSize(), false, false);

        GraphicsMediaItem *mediaItem = dynamic_cast<GraphicsMediaItem *>(created);
        if (!mediaItem)
        {
            failures << QStringLiteral("%1：音频没插成媒体图元（返回 %2）").arg(stage, created ? "不是媒体图元" : "空");
            return;
        }

        // 文档必须拥有自己那一份媒体：页文件里的引用是 audios/<文件名> 这种文档内相对路径，
        // 文件不在文档目录里，重开这份文档就听不到声音（复制时会换成按 UUID 命名的新文件，
        // 所以这里查的是"图元指向的路径在不在文档里"，不是文件名有没有保住）。
        const QString mediaLocalPath = mediaItem->mediaFileUrl().toLocalFile();
        if (!mediaLocalPath.startsWith(proxy->persistencePath()) || !QFile::exists(mediaLocalPath))
        {
            failures << QStringLiteral("%1：音频没有被复制进文档，图元指向的是 %2——页里的相对引用会悬空，重开就没声音了")
                            .arg(stage, mediaItem->mediaFileUrl().toString());
        }

        const QString copiedName = mediaLocalPath.isEmpty() ? fileName : QFileInfo(mediaLocalPath).fileName();

        pm->persistDocumentScene(proxy, board->activeScene(), board->activeSceneIndex());

        const QByteArray pageXml = SvgSubsetAdaptor::loadSceneAsText(proxy, board->activeSceneIndex());
        if (!pageXml.contains(copiedName.toUtf8()))
        {
            failures << QStringLiteral("%1：页文件里没有指向 %2 的引用").arg(stage, copiedName);
        }

        GraphicsScene *reloaded  = pageXml.isEmpty() ? nullptr : SvgSubsetAdaptor::loadScene(proxy, pageXml);
        const int      liveCount = countMediaItems(board->activeScene());
        if (countMediaItems(reloaded) != liveCount)
        {
            failures
                << QStringLiteral("%1：存盘再冷读回来，页里的音频图元从 %2 个变成 %3 个").arg(stage).arg(liveCount).arg(countMediaItems(reloaded));
        }
        else if (reloaded)
        {
            foreach (QGraphicsItem *item, reloaded->items())
            {
                GraphicsMediaItem *reloadedMedia = dynamic_cast<GraphicsMediaItem *>(item);
                if (!reloadedMedia)
                {
                    continue;
                }

                const QString resolved = reloadedMedia->mediaFileUrl().toLocalFile();
                if (!resolved.startsWith(proxy->persistencePath()) || !QFile::exists(resolved))
                {
                    failures
                        << QStringLiteral("%1：冷读回来的音频指向文档外面或根本不存在的文件 %2").arg(stage, reloadedMedia->mediaFileUrl().toString());
                }
            }
        }
    }

    /// @brief 把已攒下的不一致逐条报出来再退场（自检的退出码就是条数）。
    int bail(const QStringList &failures)
    {
        for (const QString &failure : failures)
        {
            qWarning() << "media-selftest:" << failure;
        }

        return failures.count();
    }
} // namespace

namespace MediaInsertSelfTest
{
    int run()
    {
        QStringList failures;

        BoardController *board = BoardController::instance();
        if (!board)
        {
            qWarning() << "media-selftest: 画板总控器还没装配";
            return 1;
        }

        PersistenceManager *pm = PersistenceManager::persistenceManager();

        const QString scratch = QDir(QDir::tempPath()).filePath(QStringLiteral("TianBoard-MediaSelfTest-%1").arg(QCoreApplication::applicationPid()));
        QDir(scratch).removeRecursively();

        const QString outsideDir = QDir(scratch).filePath(QStringLiteral("outside"));
        QDir().mkpath(outsideDir);

        DocumentProxy *proxy = pm->createDocument(QString(), QStringLiteral("MediaSelfTest"), true, QDir(scratch).filePath("doc"), 0, false);
        if (!proxy)
        {
            qWarning() << "media-selftest: 建临时文档失败";
            return 1;
        }

        board->setActiveDocumentScene(proxy, 0);
        if (!board->activeScene())
        {
            failures << QStringLiteral("画板没有活动场景，插不了媒体");
            return bail(failures);
        }

        // 两条分支都要把媒体变成文档的一部分；差别只在有没有把字节交给插入接口。
        insertAndAudit(failures, board, pm, proxy, outsideDir, QStringLiteral("tone-with-data.wav"), true);
        insertAndAudit(failures, board, pm, proxy, outsideDir, QStringLiteral("tone-path-only.wav"), false);

        if (proxy->pageCount() == 1 && countMediaItems(board->activeScene()) != 2)
        {
            failures << QStringLiteral("这一页上应当有两个音频图元，实际 %1 个").arg(countMediaItems(board->activeScene()));
        }

        if (failures.isEmpty())
        {
            qInfo() << "media-selftest: OK 两条插入分支都把音频变成了文档的一部分";
        }

        const int result = bail(failures);

        pm->deleteDocument(proxy);
        if (failures.isEmpty())
        {
            QDir(scratch).removeRecursively();
        }
        else
        {
            qWarning() << "media-selftest: 现场保留在" << scratch;
        }

        return result;
    }
} // namespace MediaInsertSelfTest
