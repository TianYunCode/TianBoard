#include "app/PageOperationsSelfTest.h"

#include "content/SvgSubsetAdaptor.h"
#include "core/PersistenceManager.h"
#include "model/DocumentProxy.h"
#include "model/IDocumentStore.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/GraphicsPixmapItem.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsTextItem.h"

#include <QCryptographicHash>
#include <QCoreApplication>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QPen>

namespace
{
    /// @brief 每页的标记文本；内容清单靠它认出这一页是谁。
    const char *const kPageMarkers[] = {"PAGE-ONE", "PAGE-TWO", "PAGE-THREE"};

    /// @brief 造一张有区分度的位图（纯色压缩后指纹没差别，加一笔才有）。
    QPixmap makePagePixmap(const QColor &color)
    {
        QPixmap pixmap(40, 30);
        pixmap.fill(color);

        QPainter painter(&pixmap);
        painter.setPen(QPen(color.darker(220), 4));
        painter.drawLine(2, 2, 37, 27);

        return pixmap;
    }

    /// @brief 位图像素指纹（与其它自检同一口径：先归一到 ARGB32 再取位）。
    QString pixmapDigest(const QPixmap &pixmap)
    {
        if (pixmap.isNull())
        {
            return QStringLiteral("<null>");
        }

        const QImage     image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
        const QByteArray bits(reinterpret_cast<const char *>(image.constBits()), static_cast<int>(image.sizeInBytes()));
        return QString::fromLatin1(QCryptographicHash::hash(bits, QCryptographicHash::Md5).toHex());
    }

    /**
     * @brief 这一页的内容清单：文本按原文、位图按像素指纹，排序后可直接比较。
     * @note 只数可见且非背景的对象（擦除预览这类内部图元常驻场景但不可见）。
     */
    QStringList pageSignature(GraphicsScene *scene)
    {
        QStringList facts;

        if (!scene)
        {
            facts << QStringLiteral("<没有场景>");
            return facts;
        }

        foreach (QGraphicsItem *item, scene->items())
        {
            if (!item->isVisible() || scene->isBackgroundObject(item))
            {
                continue;
            }

            if (GraphicsTextItem *text = dynamic_cast<GraphicsTextItem *>(item))
            {
                facts << QStringLiteral("text|%1").arg(text->toPlainText().simplified());
            }
            else if (GraphicsPixmapItem *pixmapItem = dynamic_cast<GraphicsPixmapItem *>(item))
            {
                facts << QStringLiteral("pixmap|%1").arg(pixmapDigest(pixmapItem->pixmap()));
            }
        }

        facts.sort();
        return facts;
    }

    /// @brief 内存里的这一页（走场景缓存）。
    QStringList liveSignature(PersistenceManager *pm, DocumentProxy *proxy, int index)
    {
        return pageSignature(pm->loadDocumentScene(proxy, index));
    }

    /// @brief 盘上的这一页（绕开缓存：读页文件重建场景）。
    QStringList coldSignature(DocumentProxy *proxy, int index)
    {
        const QByteArray pageXml = SvgSubsetAdaptor::loadSceneAsText(proxy, index);
        if (pageXml.isEmpty())
        {
            return QStringList() << QStringLiteral("<盘上没有这一页>");
        }

        return pageSignature(SvgSubsetAdaptor::loadScene(proxy, pageXml));
    }

    /**
     * @brief 逐步对账：页数对不对、每一页内存与盘上是否一致。
     * @param expectedCount 期望的页数；-1 表示这一步不检查数目。
     */
    void auditPages(
        QStringList &failures, BoardController *board, PersistenceManager *pm, DocumentProxy *proxy, const QString &stage, int expectedCount)
    {
        // 先把当前页落盘：应用本来就是"离开这一页/换文档/自动保存"时才写它，
        // 不对齐这一步就是拿"内存里刚改过的当前页"去比"还没写的盘上当前页"，报出来的是假不一致。
        board->persistCurrentScene(false, true);

        const int count = proxy->pageCount();

        if (expectedCount >= 0 && count != expectedCount)
        {
            failures << QStringLiteral("%1 之后页数应当是 %2，实际 %3").arg(stage).arg(expectedCount).arg(count);
        }

        for (int i = 0; i < count; i++)
        {
            const QStringList live = liveSignature(pm, proxy, i);
            const QStringList cold = coldSignature(proxy, i);

            if (live != cold)
            {
                failures << QStringLiteral("%1 之后第 %2 页内存与盘上不一致：内存 %3，盘上 %4")
                                .arg(stage)
                                .arg(i + 1)
                                .arg(live.join(QStringLiteral(",")), cold.join(QStringLiteral(",")));
            }
        }
    }

    /// @brief 把已攒下的不一致逐条报出来再退场（自检的退出码就是条数）。
    int bail(const QStringList &failures)
    {
        for (const QString &failure : failures)
        {
            qWarning() << "pages-selftest:" << failure;
        }

        return failures.count();
    }
} // namespace

namespace PageOperationsSelfTest
{
    int run()
    {
        QStringList failures;

        BoardController *board = BoardController::instance();
        if (!board)
        {
            qWarning() << "pages-selftest: 画板总控器还没装配";
            return 1;
        }

        PersistenceManager *pm = PersistenceManager::persistenceManager();

        const QString documentDirectory =
            QDir(QDir::tempPath()).filePath(QStringLiteral("TianBoard-PagesSelfTest-%1").arg(QCoreApplication::applicationPid()));
        QDir(documentDirectory).removeRecursively();
        QDir().mkpath(documentDirectory);

        DocumentProxy *proxy = pm->createDocument(QString(), QStringLiteral("PagesSelfTest"), true, documentDirectory, 0, false);
        if (!proxy)
        {
            qWarning() << "pages-selftest: 建临时文档失败";
            return 1;
        }

        board->setActiveDocumentScene(proxy, 0);

        // ---- 1. 三页，每页一段自己的文本与一张自己的位图 ----
        for (int i = 0; i < 3; i++)
        {
            if (i > 0)
            {
                board->addScene();
                if (board->activeSceneIndex() != i)
                {
                    failures << QStringLiteral("第 %1 次新增页之后活动页号是 %2，应当是 %3").arg(i).arg(board->activeSceneIndex() + 1).arg(i + 1);
                }
            }

            GraphicsScene *scene = board->activeScene();
            if (!scene)
            {
                failures << QStringLiteral("第 %1 页没拿到活动场景").arg(i + 1);
                break;
            }

            scene->addText(QString::fromLatin1(kPageMarkers[i]), QPointF(80 + 30 * i, 90));
            scene->addPixmap(makePagePixmap(QColor(60 + 70 * i, 180 - 40 * i, 90)), nullptr, QPointF(400, 300), 1.0, false, true);
        }

        auditPages(failures, board, pm, proxy, QStringLiteral("新增两页"), 3);

        // ---- 2. 复制第 0 页：副本落在第 1 页，页数 +1，两页内容都该是第 0 页那套 ----
        const QStringList pageZeroBefore = liveSignature(pm, proxy, 0);

        board->duplicateScene(0);
        auditPages(failures, board, pm, proxy, QStringLiteral("复制第 1 页"), 4);

        if (proxy->pageCount() == 4 && liveSignature(pm, proxy, 1) != pageZeroBefore)
        {
            failures << QStringLiteral("复制出来的第 2 页内容与第 1 页不同（副本应当是原件的拷贝）");
        }

        // ---- 3. 把第 0 页移到第 2 页：内容按"取出再插到目标位"重排 ----
        QStringList beforeMove;
        for (int i = 0; i < proxy->pageCount(); i++)
        {
            beforeMove << liveSignature(pm, proxy, i).join(QStringLiteral(";"));
        }

        QStringList   expectedMove = beforeMove;
        const QString moved        = expectedMove.takeAt(0);
        expectedMove.insert(2, moved);

        board->moveSceneToIndex(0, 2);
        auditPages(failures, board, pm, proxy, QStringLiteral("把第 1 页移到第 3 页"), 4);

        if (proxy->pageCount() == 4)
        {
            QStringList afterMove;
            for (int i = 0; i < proxy->pageCount(); i++)
            {
                afterMove << liveSignature(pm, proxy, i).join(QStringLiteral(";"));
            }

            if (afterMove != expectedMove)
            {
                failures << QStringLiteral("移动页之后的页序不是「取出第 1 页、插到第 3 位」的结果：期望 %1，实际 %2")
                                .arg(expectedMove.join(QStringLiteral(" | ")), afterMove.join(QStringLiteral(" | ")));
            }
        }

        // ---- 4. 删掉中间那一页：剩下的内容一个不多一个不少 ----
        QStringList beforeDelete;
        for (int i = 0; i < proxy->pageCount(); i++)
        {
            beforeDelete << liveSignature(pm, proxy, i).join(QStringLiteral(";"));
        }

        QStringList expectedDelete = beforeDelete;
        expectedDelete.removeAt(1);

        board->deleteScene(1);
        auditPages(failures, board, pm, proxy, QStringLiteral("删掉第 2 页"), 3);

        if (proxy->pageCount() == 3)
        {
            QStringList afterDelete;
            for (int i = 0; i < proxy->pageCount(); i++)
            {
                afterDelete << liveSignature(pm, proxy, i).join(QStringLiteral(";"));
            }

            if (afterDelete != expectedDelete)
            {
                failures << QStringLiteral("删掉第 2 页之后剩下的页不是原来那三页：期望 %1，实际 %2")
                                .arg(expectedDelete.join(QStringLiteral(" | ")), afterDelete.join(QStringLiteral(" | ")));
            }
        }

        if (failures.isEmpty())
        {
            qInfo() << "pages-selftest: OK 新增、复制、移动、删除四步之后每一页内存与盘上都一致";
        }

        const int result = bail(failures);

        pm->deleteDocument(proxy);
        if (failures.isEmpty())
        {
            QDir(documentDirectory).removeRecursively();
        }
        else
        {
            qWarning() << "pages-selftest: 现场保留在" << documentDirectory;
        }

        return result;
    }
} // namespace PageOperationsSelfTest
