#include "app/ClipboardSelfTest.h"

#include "content/SvgSubsetAdaptor.h"
#include "core/PersistenceManager.h"
#include "model/DocumentProxy.h"
#include "model/MimeTypes.hpp"
#include "model/PackageLayout.hpp"
#include "whiteboard/BoardController.h"
#include "whiteboard/GraphicsPixmapItem.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsSvgItem.h"
#include "whiteboard/Item.h"
#include "whiteboard/MimeDataGraphicsItem.h"

#include <QApplication>
#include <QClipboard>
#include <QCryptographicHash>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QPen>

namespace
{
    /// @brief 有区分度的测试位图：纯色压缩后指纹跟别的纯色没差别，画几笔才验得出像素真过了一趟盘。
    QPixmap makeTestPixmap()
    {
        QPixmap pixmap(64, 48);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setPen(QPen(QColor(10, 90, 200), 6));
        painter.drawLine(4, 4, 59, 43);
        painter.fillRect(22, 14, 18, 14, QColor(220, 30, 30));
        painter.setPen(QPen(QColor(20, 160, 60), 3));
        painter.drawLine(59, 6, 6, 42);

        return pixmap;
    }

    /// @brief 内嵌 SVG 的内容；粘贴与存读两端比的就是这些字节。
    const QByteArray kSelftestSvg = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                                    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"60\" height=\"40\">"
                                    "<circle cx=\"30\" cy=\"20\" r=\"16\" fill=\"#c04000\"/></svg>";

    /// @brief 字节指纹：空内容的 MD5 是 d41d8cd9…，读回一个空图元时它一眼就能看出来。
    QString digest(const QByteArray &bytes)
    {
        return QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Md5).toHex());
    }

    /// @brief 位图像素指纹（与格式自检同一口径：先归一到 ARGB32 再取位）。
    QString pixmapDigest(const QPixmap &pixmap)
    {
        if (pixmap.isNull())
        {
            return QStringLiteral("<null>");
        }

        const QImage     image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
        const QByteArray bits(reinterpret_cast<const char *>(image.constBits()), static_cast<int>(image.sizeInBytes()));
        return digest(bits);
    }

    /// @brief 这一页上"会写进页文件的对象图元"该有几个、内容是什么：类型 + 内容指纹。
    QStringList objectFacts(const GraphicsScene *scene)
    {
        QStringList facts;

        foreach (QGraphicsItem *item, scene->items())
        {
            if (!item->isVisible() || scene->isBackgroundObject(item))
            {
                continue;
            }

            if (const GraphicsPixmapItem *pixmapItem = dynamic_cast<const GraphicsPixmapItem *>(item))
            {
                facts << QStringLiteral("pixmap|%1").arg(pixmapDigest(pixmapItem->pixmap()));
            }
            else if (const GraphicsSvgItem *svgItem = dynamic_cast<const GraphicsSvgItem *>(item))
            {
                facts << QStringLiteral("svg|%1").arg(digest(svgItem->fileData()));
            }
        }

        facts.sort();
        return facts;
    }

    /// @brief 这批对象图元的 UUID 清单；副本与原件撞车在这条列表上一眼看得出来。
    QStringList objectUuids(const GraphicsScene *scene)
    {
        QStringList uuids;

        foreach (QGraphicsItem *item, scene->items())
        {
            if (!item->isVisible() || scene->isBackgroundObject(item))
            {
                continue;
            }

            const GraphicsPixmapItem *pixmapItem = dynamic_cast<const GraphicsPixmapItem *>(item);
            const GraphicsSvgItem    *svgItem    = dynamic_cast<const GraphicsSvgItem *>(item);
            const Item               *boardItem  = dynamic_cast<const Item *>(item);

            if ((pixmapItem || svgItem) && boardItem)
            {
                uuids << boardItem->uuid().toString();
            }
        }

        uuids.sort();
        return uuids;
    }

    /// @brief 文档图片目录下的文件名清单。
    QStringList imageFiles(const QString &documentDirectory)
    {
        return QDir(documentDirectory + "/" + PackageLayout::imageDirectory).entryList(QDir::Files, QDir::Name);
    }

    /// @brief 把已攒下的不一致逐条报出来再退场（自检的退出码就是条数）。
    int bail(const QStringList &failures)
    {
        for (const QString &failure : failures)
        {
            qWarning() << "clipboard-selftest:" << failure;
        }

        return failures.count();
    }
} // namespace

namespace ClipboardSelfTest
{
    int run()
    {
        QStringList failures;

        BoardController *board = BoardController::instance();
        if (!board)
        {
            qWarning() << "clipboard-selftest: 画板总控器还没装配，这条自检跑不了";
            return 1;
        }

        const QString documentDirectory =
            QDir(QDir::tempPath()).filePath(QStringLiteral("TianBoard-ClipboardSelfTest-%1").arg(QCoreApplication::applicationPid()));
        QDir(documentDirectory).removeRecursively();
        QDir().mkpath(documentDirectory);

        // pageCount 传 0：withEmptyPage 已经插了一页，两个参数是叠加的（见 PersistenceManager::createDocument）。
        DocumentProxy *proxy = PersistenceManager::persistenceManager()->createDocument(
            QString(), QStringLiteral("ClipboardSelfTest"), true, documentDirectory, 0, false);
        if (!proxy)
        {
            qWarning() << "clipboard-selftest: 建临时文档失败";
            return 1;
        }

        board->setActiveDocumentScene(proxy, 0);

        GraphicsScene *scene = board->activeScene();
        if (!scene || scene->document() != proxy)
        {
            failures << QStringLiteral("切到临时文档之后，画板的活动场景不是它第 0 页（场景 %1）").arg(scene ? "非空" : "为空");
            return bail(failures);
        }

        // ---- 1. 两张原件：资源文件此刻就该在盘上 ----
        GraphicsPixmapItem *pixmapItem = scene->addPixmap(makeTestPixmap(), nullptr, QPointF(120, 120), 1.0, false, true);
        GraphicsSvgItem    *svgItem    = scene->addSvg(QUrl(), QPointF(420, 300), kSelftestSvg);

        if (!pixmapItem || !svgItem)
        {
            failures << QStringLiteral("原件图元没建起来（位图 %1，SVG %2）").arg(pixmapItem ? "有" : "无", svgItem ? "有" : "无");
            return bail(failures);
        }

        const QString pixmapUuid         = pixmapItem->uuid().toString();
        const QString svgUuid            = svgItem->uuid().toString();

        const QStringList originalImages = imageFiles(documentDirectory);
        if (originalImages.size() != 2)
        {
            failures << QStringLiteral("两张原件都落盘之后 images/ 应有 2 个文件，实际 %1 个: %2")
                            .arg(originalImages.size())
                            .arg(originalImages.join(QStringLiteral(", ")));
        }

        // ---- 2. copy()：剪贴板拿到的必须是副本，UUID 不许跟原件相同 ----
        scene->clearSelection();
        pixmapItem->setSelected(true);
        svgItem->setSelected(true);

        board->copy();

        const QMimeData *clipboardData = QApplication::clipboard()->mimeData();
        if (!clipboardData || !clipboardData->hasFormat(MimeTypes::boardPageItem))
        {
            failures << QStringLiteral("copy() 没往剪贴板放画板图元数据（boardPageItem 格式缺失）");
        }
        else
        {
            const MimeDataGraphicsItem *mimeItems = qobject_cast<const MimeDataGraphicsItem *>(clipboardData);
            if (!mimeItems)
            {
                failures << QStringLiteral("剪贴板上的数据不是 MimeDataGraphicsItem，粘贴端会 qobject_cast 出空指针");
            }
            else if (mimeItems->items().size() != 2)
            {
                failures << QStringLiteral("剪贴板里应有 2 个图元，实际 %1 个").arg(mimeItems->items().size());
            }
            else
            {
                for (Item *item : mimeItems->items())
                {
                    const QString uuid = item->uuid().toString();
                    if (uuid == pixmapUuid || uuid == svgUuid)
                    {
                        failures << QStringLiteral("剪贴板里的副本跟原件共用 UUID %1——删原件就把副本引用的文件一起删掉").arg(uuid);
                    }
                }
            }
        }

        // ---- 3. paste()：这一页上应多出两个内容一致的图元 ----
        const QStringList beforeFacts = objectFacts(scene);
        board->paste();

        const QStringList wanted     = QStringList() << QStringLiteral("pixmap|%1").arg(pixmapDigest(makeTestPixmap()))
                                                     << QStringLiteral("svg|%1").arg(digest(kSelftestSvg));
        const QStringList afterFacts = objectFacts(scene);

        for (const QString &fact : wanted)
        {
            if (afterFacts.count(fact) < beforeFacts.count(fact) + 1)
            {
                failures << QStringLiteral("粘贴之后这个图元没多出来（%1：粘贴前 %2 个，粘贴后 %3 个）")
                                .arg(fact)
                                .arg(beforeFacts.count(fact))
                                .arg(afterFacts.count(fact));
            }
        }

        const QStringList pastedUuids = objectUuids(scene);
        if (pastedUuids.size() != 4)
        {
            failures << QStringLiteral("场景里应有 4 个对象图元（两张原件与两份副本），实际 %1 个: %2")
                            .arg(pastedUuids.size())
                            .arg(pastedUuids.join(QStringLiteral(", ")));
        }

        QStringList uniqueUuids = pastedUuids;
        uniqueUuids.removeDuplicates();
        if (uniqueUuids.size() != pastedUuids.size())
        {
            failures << QStringLiteral("原件与副本撞在同一个 UUID 上（%1）——deleteItem 按 UUID 清盘，删一份会把另一份的文件带走")
                            .arg(pastedUuids.join(QStringLiteral(", ")));
        }

        // ---- 4. 删掉两张原件：走界面上那条删除路（Item::remove → 撤销命令持有），
        //          再到 ClearUndoStack() 这个真正的释放点（不在场景也不在剪贴板上的图元
        //          才被 deleteItem → clearSource 按 UUID 清盘）。副本引用的文件不许被带走。
        pixmapItem->remove(true);
        svgItem->remove(true);
        board->ClearUndoStack();

        const QStringList afterDelete = imageFiles(documentDirectory);
        if (!afterDelete.isEmpty())
        {
            failures << QStringLiteral("删掉原件并清空撤销栈之后，原件自己的资源文件应当消失，盘上还留着 %1 个: %2")
                            .arg(afterDelete.size())
                            .arg(afterDelete.join(QStringLiteral(", ")));
        }

        // ---- 5. 存盘 → 冷读：两份副本必须带着自己的内容回来 ----
        PersistenceManager::persistenceManager()->persistDocumentScene(proxy, scene, 0);

        const QByteArray pageXml = SvgSubsetAdaptor::loadSceneAsText(proxy, 0);
        if (pageXml.isEmpty())
        {
            failures << QStringLiteral("盘上的页读出来是空的");
        }

        GraphicsScene *reloaded = pageXml.isEmpty() ? nullptr : SvgSubsetAdaptor::loadScene(proxy, pageXml);
        if (!pageXml.isEmpty() && !reloaded)
        {
            failures << QStringLiteral("读取器没能从盘上的页重建场景（%1 字节）").arg(pageXml.size());
        }
        if (reloaded)
        {
            const QStringList gotFacts = objectFacts(reloaded);
            for (const QString &fact : wanted)
            {
                if (!gotFacts.contains(fact))
                {
                    failures << QStringLiteral("删掉原件再存读之后副本丢了: %1").arg(fact);
                }
            }

            const QStringList gotImages = imageFiles(documentDirectory);
            if (gotImages.size() != 2)
            {
                failures << QStringLiteral("存读之后 images/ 应只剩副本那两份文件，实际 %1 个: %2")
                                .arg(gotImages.size())
                                .arg(gotImages.join(QStringLiteral(", ")));
            }
        }

        // 剪贴板里存的是这条链路的副本，别把它留给用户的 Ctrl+V。
        QApplication::clipboard()->clear();

        PersistenceManager::persistenceManager()->deleteDocument(proxy);
        QDir(documentDirectory).removeRecursively();

        if (failures.isEmpty())
        {
            qInfo() << "clipboard-selftest: OK 复制→粘贴→删原件→存读一条链路一致";
        }

        return bail(failures);
    }
} // namespace ClipboardSelfTest
