#include "app/UndoRedoSelfTest.h"

#include "core/PersistenceManager.h"
#include "model/DocumentProxy.h"
#include "model/PackageLayout.hpp"
#include "whiteboard/BoardController.h"
#include "whiteboard/GraphicsPixmapItem.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsTextItem.h"
#include "whiteboard/Item.h"

#include <QCryptographicHash>
#include <QCoreApplication>
#include <QDir>
#include <QEvent>
#include <QImage>
#include <QUndoStack>
#include <QPainter>
#include <QPen>

namespace
{
    /// @brief 造一张有区分度的位图：纯色压缩后指纹跟别的纯色没差别，加一笔才认得出对象。
    QPixmap makePixmap(const QColor &color)
    {
        QPixmap pixmap(64, 48);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.fillRect(0, 0, 64, 48, color);
        painter.setPen(QPen(color.darker(250), 5));
        painter.drawLine(3, 3, 60, 44);

        return pixmap;
    }

    /// @brief 位图像素指纹（与格式/导出/导入自检同一口径）。
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

    /// @brief 场景里某类图元的个数（含背景与工具图元之外的普通对象；用增量比对，不算绝对值）。
    template <typename T> int countItems(GraphicsScene *scene)
    {
        int count = 0;
        if (!scene)
        {
            return 0;
        }

        foreach (QGraphicsItem *item, scene->items())
        {
            if (dynamic_cast<T *>(item))
            {
                ++count;
            }
        }

        return count;
    }

    /// @brief 按 UUID 找回场景里那张位图的内容指纹；找不回就返回空串。
    QString pixmapDigestByUuid(GraphicsScene *scene, const QUuid &uuid)
    {
        foreach (QGraphicsItem *item, scene->items())
        {
            GraphicsPixmapItem *pixmapItem = dynamic_cast<GraphicsPixmapItem *>(item);
            if (pixmapItem && pixmapItem->uuid() == uuid)
            {
                return pixmapDigest(pixmapItem->pixmap());
            }
        }

        return QString();
    }

    /// @brief 把已攒下的不一致逐条报出来再退场（自检的退出码就是条数）。
    int bail(const QStringList &failures)
    {
        for (const QString &failure : failures)
        {
            qWarning() << "undoredo-selftest:" << failure;
        }

        return failures.count();
    }
} // namespace

namespace UndoRedoSelfTest
{
    int run()
    {
        QStringList failures;

        BoardController *board = BoardController::instance();
        QUndoStack      *stack = board ? BoardController::undoStack() : nullptr;
        if (!stack)
        {
            qWarning() << "undoredo-selftest: 撤销栈没建出来（BoardController::undoStack() 返回空）";
            return 1;
        }

        PersistenceManager *pm = PersistenceManager::persistenceManager();

        const QString rootDir =
            QDir(QDir::tempPath()).filePath(QStringLiteral("TianBoard-UndoRedoSelfTest-%1").arg(QCoreApplication::applicationPid()));
        QDir(rootDir).removeRecursively();
        QDir().mkpath(rootDir);

        DocumentProxy *first  = pm->createDocument(QString(), QStringLiteral("UndoRedoSelfTestA"), true, QDir(rootDir).filePath("A"), 0, false);
        DocumentProxy *second = pm->createDocument(QString(), QStringLiteral("UndoRedoSelfTestB"), true, QDir(rootDir).filePath("B"), 0, false);
        if (!first || !second)
        {
            failures << QStringLiteral("两份临时文档没都建出来（A %1，B %2）").arg(first ? "有" : "无", second ? "有" : "无");
            return bail(failures);
        }

        board->setActiveDocumentScene(first, 0);
        GraphicsScene *scene = board->activeScene();
        if (!scene || scene->document() != first)
        {
            failures << QStringLiteral("画板的活动场景不是临时文档 A 的第 0 页");
            return bail(failures);
        }

        // 换文档会把撤销栈清掉——撤销栈归画板之后（2026-10-22），这条生命周期必须还是这样。
        if (stack->count() != 0)
        {
            failures << QStringLiteral("切到新文档之后撤销栈没被清空（还剩 %1 条）").arg(stack->count());
        }

        // ---- 1. 加一张位图：入栈、撤销移出、重做放回同一个对象 ----
        const int pixBaseline          = countItems<GraphicsPixmapItem>(scene);

        GraphicsPixmapItem *pixmapItem = scene->addPixmap(makePixmap(QColor(210, 60, 40)), nullptr, QPointF(200, 200), 1.0, false, true);
        if (!pixmapItem)
        {
            failures << QStringLiteral("位图没加进场景");
            return bail(failures);
        }

        const QUuid   pixmapUuid   = pixmapItem->uuid();
        const QString pixmapWanted = pixmapDigest(pixmapItem->pixmap());

        if (stack->count() != 1)
        {
            failures << QStringLiteral("加一张位图应当压入 1 条撤销命令，实际栈里 %1 条").arg(stack->count());
        }
        if (countItems<GraphicsPixmapItem>(scene) != pixBaseline + 1)
        {
            failures << QStringLiteral("位图没出现在场景里");
        }

        stack->undo();
        if (countItems<GraphicsPixmapItem>(scene) != pixBaseline)
        {
            failures << QStringLiteral("撤销之后场景里还数得到那张位图");
        }
        if (!stack->canRedo())
        {
            failures << QStringLiteral("撤销之后 canRedo() 是假的");
        }

        stack->redo();
        if (countItems<GraphicsPixmapItem>(scene) != pixBaseline + 1)
        {
            failures << QStringLiteral("重做之后位图没回到场景");
        }
        if (pixmapDigestByUuid(scene, pixmapUuid) != pixmapWanted)
        {
            failures << QStringLiteral("重做回来的不是同一个位图对象（按 UUID 找不回或像素变了）");
        }

        // ---- 2. 删掉它（界面上那条删除路），撤销要连对象带内容一起回来 ----
        pixmapItem->remove(true);
        if (countItems<GraphicsPixmapItem>(scene) != pixBaseline)
        {
            failures << QStringLiteral("删除之后场景里还数得到那张位图");
        }

        stack->undo();
        const QString restored = pixmapDigestByUuid(scene, pixmapUuid);
        if (restored.isEmpty())
        {
            failures << QStringLiteral("撤销删除之后原图元没回场景（按 UUID 找不回）");
        }
        else if (restored != pixmapWanted)
        {
            failures << QStringLiteral("撤销删除回来的位图内容变了（%1 对 %2）").arg(restored, pixmapWanted);
        }

        stack->redo();
        if (countItems<GraphicsPixmapItem>(scene) != pixBaseline)
        {
            failures << QStringLiteral("再重做删除之后位图又留在场景里");
        }

        // ---- 3. 文本走同一条回路 ----
        const int         textBaseline = countItems<GraphicsTextItem>(scene);
        GraphicsTextItem *textItem     = scene->addText(QStringLiteral("撤销自检文本"), QPointF(120, 120));
        if (!textItem)
        {
            failures << QStringLiteral("文本图元没加进场景");
        }
        else
        {
            if (countItems<GraphicsTextItem>(scene) != textBaseline + 1)
            {
                failures << QStringLiteral("文本没出现在场景里");
            }

            stack->undo();
            if (countItems<GraphicsTextItem>(scene) != textBaseline)
            {
                failures << QStringLiteral("撤销之后文本还留在场景里");
            }

            stack->redo();
            if (countItems<GraphicsTextItem>(scene) != textBaseline + 1)
            {
                failures << QStringLiteral("重做之后文本没回来");
            }
        }

        // ---- 4. ClearUndoStack：栈清空，场景不受影响 ----
        const int pixAfterAll  = countItems<GraphicsPixmapItem>(scene);
        const int textAfterAll = countItems<GraphicsTextItem>(scene);

        board->ClearUndoStack();
        if (stack->count() != 0)
        {
            failures << QStringLiteral("ClearUndoStack 之后栈里还剩 %1 条").arg(stack->count());
        }
        if (countItems<GraphicsPixmapItem>(scene) != pixAfterAll || countItems<GraphicsTextItem>(scene) != textAfterAll)
        {
            failures << QStringLiteral("ClearUndoStack 动到了场景里的图元（位图 %1→%2，文本 %3→%4）")
                            .arg(pixAfterAll)
                            .arg(countItems<GraphicsPixmapItem>(scene))
                            .arg(textAfterAll)
                            .arg(countItems<GraphicsTextItem>(scene));
        }

        // ---- 5. 换到另一份文档：撤销栈跟着清 ----
        board->setActiveDocumentScene(second, 0);
        if (stack->count() != 0)
        {
            failures << QStringLiteral("换文档之后撤销栈还剩 %1 条（上一条文档的撤销记录不该跟着过来）").arg(stack->count());
        }

        // ---- 6. 画一笔之后删掉**当前这一页**：撤销栈不许抱着随场景销毁的图元 ----
        // 这是界面上走得到的一串：在新页上画东西（压一条命令）→ 在翻页条上删掉这一页
        // → 按 Ctrl+Z。删页把那页的场景 deleteLater，命令里的图元指针与场景指针于是都成了悬垂的。
        // 实测：去掉 `ClearUndoStack` 末尾那次 `undoStack()->clear()`，这一串直接段错误——
        // 栈是 cdb 抓的：`deleteScene → setActiveDocumentScene → ClearUndoStack → findUniquesItems
        // → QGraphicsItem::parentItem` 读已释放的图元。原因是那个循环会把"既不在场景也不在剪贴板"
        // 的图元 delete 掉，命令却还抱着它们。所以"顺手清栈"不是收尾，是这条链路的承重墙。
        board->addScene();
        GraphicsScene *lastPage          = board->activeScene();
        const int      pagesBeforeDelete = second->pageCount();

        if (!lastPage || pagesBeforeDelete < 2)
        {
            failures << QStringLiteral("没法量删页之后的撤销：新页 %1，文档页数 %2").arg(lastPage ? "有" : "无", pagesBeforeDelete);
        }
        else
        {
            const int deleteIndex = board->activeSceneIndex();

            lastPage->addPixmap(makePixmap(QColor(40, 90, 200)), nullptr, QPointF(180, 260), 1.0, false, true);
            if (stack->count() != 1)
            {
                failures << QStringLiteral("在新页上画一张位图应当压入 1 条撤销命令，实际 %1 条").arg(stack->count());
            }

            board->deleteScene(deleteIndex);

            // 把挂起的 DeferredDelete 真的投递出去：Qt 明写 processEvents() 不处理 DeferredDelete，
            // 只有回到主事件循环或显式 sendPostedEvents 才释放——不补这一手，那一页的场景其实还没没，
            // 悬垂指针也没变成野内存，这一案就量不到它想量的东西。
            QCoreApplication::processEvents();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            QCoreApplication::processEvents();

            if (second->pageCount() != pagesBeforeDelete - 1)
            {
                failures << QStringLiteral("删掉一页之后页数应当是 %1，实际 %2").arg(pagesBeforeDelete - 1).arg(second->pageCount());
            }

            // 命令必须在场景真的没了之前就被丢掉：留着它们，下面那两下与后面任何一次
            // ClearUndoStack 的遍历都是在读悬垂指针。
            const int countAfterDelete = stack->count();
            if (countAfterDelete != 0)
            {
                failures << QStringLiteral("删掉当前页之后撤销栈里还剩 %1 条——那些命令记的是随场景一起销毁的图元，"
                                           "下一次遍历就是 use-after-free（实测去掉那次 clear 时这里直接段错误）")
                                .arg(countAfterDelete);
            }
            else
            {
                // 栈确实空了才走界面那条撤销/重做路：这两下应当无害地什么都不做
                stack->undo();
                stack->redo();
            }

            // 画板还得继续能用：在剩下的那一页上再画一笔，命令要压得进、也撤得掉
            GraphicsScene *survivor = board->activeScene();
            if (!survivor)
            {
                failures << QStringLiteral("删页之后画板没有活动场景了");
            }
            else
            {
                const int pixBefore = countItems<GraphicsPixmapItem>(survivor);

                survivor->addPixmap(makePixmap(QColor(60, 180, 90)), nullptr, QPointF(300, 300), 1.0, false, true);
                if (stack->count() != 1)
                {
                    failures << QStringLiteral("删页之后再画一笔应当压入 1 条命令，实际 %1 条").arg(stack->count());
                }
                if (countItems<GraphicsPixmapItem>(survivor) != pixBefore + 1)
                {
                    failures << QStringLiteral("删页之后再画一笔没落到剩下的那一页上");
                }

                stack->undo();
                if (countItems<GraphicsPixmapItem>(survivor) != pixBefore)
                {
                    failures << QStringLiteral("删页之后新画的这一笔撤销不掉");
                }
            }
        }

        if (failures.isEmpty())
        {
            qInfo() << "undoredo-selftest: OK 加、撤、重做、删、清空、换文档与删当前页之后重画都按预期落地";
        }

        const int result = bail(failures);

        pm->deleteDocument(second);
        pm->deleteDocument(first);
        QDir(rootDir).removeRecursively();

        return result;
    }
} // namespace UndoRedoSelfTest
