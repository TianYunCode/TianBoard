#include "app/EraserSelfTest.h"

#include "content/SvgSubsetAdaptor.h"
#include "core/PersistenceManager.h"
#include "model/DocumentProxy.h"
#include "settings/Settings.h"
#include "utils/Enums.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/DrawingController.h"
#include "whiteboard/GraphicsPolygonItem.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsStrokesGroup.h"

#include <QCoreApplication>
#include <QDir>
#include <QUndoStack>

namespace
{
    /// @brief 场景里所有笔迹多边形，换成场景坐标（擦除判据要按几何判，包围盒说不清"这一段没了"）。
    QList<QPolygonF> strokePolygons(GraphicsScene *scene)
    {
        QList<QPolygonF> polygons;
        if (!scene)
        {
            return polygons;
        }

        foreach (QGraphicsItem *item, scene->items())
        {
            GraphicsPolygonItem *polygonItem = dynamic_cast<GraphicsPolygonItem *>(item);
            if (polygonItem)
            {
                polygons << polygonItem->sceneTransform().map(polygonItem->polygon());
            }
        }

        return polygons;
    }

    /// @brief 有多少块笔迹多边形真的盖住这个点。
    int polygonsCovering(const QList<QPolygonF> &polygons, const QPointF &point)
    {
        int count = 0;
        for (const QPolygonF &polygon : polygons)
        {
            if (polygon.containsPoint(point, Qt::OddEvenFill))
            {
                ++count;
            }
        }

        return count;
    }

    /// @brief 这一页上有几个笔迹组（一笔一个，擦除不许把它拆开或多出来）。
    int strokeGroupCount(GraphicsScene *scene)
    {
        int count = 0;
        if (!scene)
        {
            return count;
        }

        foreach (QGraphicsItem *item, scene->items())
        {
            if (dynamic_cast<GraphicsStrokesGroup *>(item))
            {
                ++count;
            }
        }

        return count;
    }

    /// @brief 按界面上那条路走一趟输入设备：落笔、依次移到每个采样点、抬笔（工具由调用方先切好）。
    void pressMoveRelease(GraphicsScene *scene, const QList<QPointF> &points)
    {
        scene->inputDevicePress(points.first(), 1.0);

        for (int i = 1; i < points.size(); ++i)
        {
            scene->inputDeviceMove(points.at(i), 1.0);
        }

        scene->inputDeviceRelease();
    }

    /// @brief 把已攒下的不一致逐条报出来再退场（自检的退出码就是条数）。
    int bail(const QStringList &failures)
    {
        for (const QString &failure : failures)
        {
            qWarning() << "eraser-selftest:" << failure;
        }

        return failures.count();
    }
} // namespace

namespace EraserSelfTest
{
    int run()
    {
        QStringList failures;

        BoardController *board = BoardController::instance();
        QUndoStack      *stack = BoardController::undoStack();
        if (!board || !stack)
        {
            qWarning() << "eraser-selftest: 画板或撤销栈还没装配";
            return 1;
        }

        Settings *settings               = Settings::settings();
        const int toolBefore             = DrawingController::drawingController()->stylusTool();

        // 擦除带的宽度是设置除以缩放算出来的，直接拿它摆探针点：探针要落在带的外面，
        // 但整笔又必须跨过带，否则"带里没了、带外还在"这两条判据都是空话。
        const int eraserWidthIndexBefore = settings->eraserWidthIndex();
        settings->setEraserWidthIndex(Width::Medium);
        const qreal eraserWidth = settings->currentEraserWidth() / board->systemScaleFactor() / board->currentZoom();
        const int   half        = static_cast<int>(eraserWidth / 2.0) + 8;
        const int   span        = half + 90;

        const QPointF cut(400, 200);
        const QPointF leftEnd(cut.x() - half - 45, cut.y());
        const QPointF rightEnd(cut.x() + half + 45, cut.y());
        const QPointF otherStroke(cut.x(), 500);

        const QString documentDirectory =
            QDir(QDir::tempPath()).filePath(QStringLiteral("TianBoard-EraserSelfTest-%1").arg(QCoreApplication::applicationPid()));
        QDir(documentDirectory).removeRecursively();

        DocumentProxy *proxy =
            PersistenceManager::persistenceManager()->createDocument(QString(), QStringLiteral("EraserSelfTest"), false, documentDirectory, 1, false);
        if (!proxy)
        {
            qWarning() << "eraser-selftest: 建临时文档失败";
            settings->setEraserWidthIndex(eraserWidthIndexBefore);
            DrawingController::drawingController()->setStylusTool(toolBefore);
            return 1;
        }

        // 撤销要量得到，所以这一页开自带的撤销栈；场景的父对象是文档代理，随代理一起销毁。
        GraphicsScene *scene = new GraphicsScene(proxy, true);
        scene->setNominalSize(QSize(1024, 768));
        scene->setBackground(false, PageBackground::plain);

        const int commandsBefore = stack->count();

        // ---- 1. 画两笔：一笔要被切开，另一笔作"不许误伤"的对照 ----
        DrawingController::drawingController()->setStylusTool(StylusTool::Pen);
        pressMoveRelease(scene, {QPointF(cut.x() - span, cut.y()), cut, QPointF(cut.x() + span, cut.y())});
        pressMoveRelease(scene, {QPointF(otherStroke.x() - span, otherStroke.y()), otherStroke, QPointF(otherStroke.x() + span, otherStroke.y())});

        if (stack->count() != commandsBefore + 2)
        {
            failures << QStringLiteral("画两笔应当压入 2 条撤销命令，实际压入 %1 条").arg(stack->count() - commandsBefore);
        }
        if (strokeGroupCount(scene) != 2)
        {
            failures << QStringLiteral("两笔应当是两个笔迹组，实际 %1 个").arg(strokeGroupCount(scene));
        }

        QList<QPolygonF> polygons = strokePolygons(scene);
        if (polygons.isEmpty())
        {
            failures << QStringLiteral("两笔画完，场景里一块笔迹多边形都没有——后面全都不用量了");
            stack->clear();
            settings->setEraserWidthIndex(eraserWidthIndexBefore);
            DrawingController::drawingController()->setStylusTool(toolBefore);
            PersistenceManager::persistenceManager()->deleteDocument(proxy);
            QDir(documentDirectory).removeRecursively();
            return bail(failures);
        }

        // 前置对照：擦之前三个探针点都得真有几何盖着，否则"擦完没了"是废话。
        for (const QPointF &probe : {leftEnd, cut, rightEnd})
        {
            if (polygonsCovering(polygons, probe) == 0)
            {
                failures << QStringLiteral("擦除之前 (%1,%2) 上本来就该有笔迹，实际没有——这一笔没画到该到的位置").arg(probe.x()).arg(probe.y());
            }
        }
        if (polygonsCovering(polygons, otherStroke) == 0)
        {
            failures << QStringLiteral("对照那一笔在 (%1,%2) 上没有几何，误伤判据验不了").arg(otherStroke.x()).arg(otherStroke.y());
        }

        const int polygonsBeforeErase = polygons.size();

        // ---- 2. 竖着擦过第一笔的中段：一次抬笔只许压一条命令 ----
        DrawingController::drawingController()->setStylusTool(StylusTool::Eraser);
        pressMoveRelease(scene, {QPointF(cut.x(), cut.y() - half), cut, QPointF(cut.x(), cut.y() + half)});

        if (stack->count() != commandsBefore + 3)
        {
            failures << QStringLiteral("擦一道应当只压入 1 条撤销命令（累计 3 条），实际累计 %1 条").arg(stack->count() - commandsBefore);
        }

        polygons = strokePolygons(scene);
        if (polygonsCovering(polygons, cut) != 0)
        {
            failures << QStringLiteral("擦除带中心 (%1,%2) 还留着 %3 块笔迹多边形——擦了个寂寞")
                            .arg(cut.x())
                            .arg(cut.y())
                            .arg(polygonsCovering(polygons, cut));
        }
        if (polygonsCovering(polygons, leftEnd) == 0 || polygonsCovering(polygons, rightEnd) == 0)
        {
            failures << QStringLiteral("擦掉中段之后带两头应当还在：左 %1 块、右 %2 块")
                            .arg(polygonsCovering(polygons, leftEnd))
                            .arg(polygonsCovering(polygons, rightEnd));
        }
        if (polygonsCovering(polygons, otherStroke) == 0)
        {
            failures << QStringLiteral("橡皮擦过 y=%1 这一道，把 y=%2 那笔也误伤了").arg(cut.y()).arg(otherStroke.y());
        }
        if (strokeGroupCount(scene) != 2)
        {
            failures << QStringLiteral("擦除把笔迹组数目改成了 %1（应当还是 2，一笔一组）").arg(strokeGroupCount(scene));
        }

        // ---- 3. 撤销那一条：整段擦除必须原样还回来 ----
        stack->undo();

        polygons = strokePolygons(scene);
        if (polygonsCovering(polygons, cut) == 0)
        {
            failures << QStringLiteral("撤销一次擦除之后，中段 (%1,%2) 没有恢复出任何笔迹——一次 Ctrl+Z 没能还回整段擦除").arg(cut.x()).arg(cut.y());
        }
        if (polygons.size() != polygonsBeforeErase)
        {
            failures
                << QStringLiteral("撤销擦除之后笔迹多边形是 %1 块，擦除之前是 %2 块——还原不是原样").arg(polygons.size()).arg(polygonsBeforeErase);
        }

        // ---- 4. 重做：又擦掉了 ----
        stack->redo();

        polygons = strokePolygons(scene);
        if (polygonsCovering(polygons, cut) != 0)
        {
            failures << QStringLiteral("重做擦除之后中段 (%1,%2) 还有几何").arg(cut.x()).arg(cut.y());
        }

        // ---- 5. 存盘 → 绕开场景缓存冷读：盘上的那一页必须就是擦过的样子 ----
        scene->setModified(true);
        PersistenceManager::persistenceManager()->persistDocumentScene(proxy, scene, 0);

        const QByteArray pageXml = SvgSubsetAdaptor::loadSceneAsText(proxy, 0);
        if (pageXml.isEmpty())
        {
            failures << QStringLiteral("擦完之后存的那一页读出来是空的");
        }

        GraphicsScene *reloaded = pageXml.isEmpty() ? nullptr : SvgSubsetAdaptor::loadScene(proxy, pageXml);
        if (!pageXml.isEmpty() && !reloaded)
        {
            failures << QStringLiteral("读取器没能从盘上的页重建场景（%1 字节）").arg(pageXml.size());
        }
        if (reloaded)
        {
            const QList<QPolygonF> reloadedPolygons = strokePolygons(reloaded);

            if (polygonsCovering(reloadedPolygons, cut) != 0)
            {
                failures << QStringLiteral("冷读回来中段 (%1,%2) 又长出了 %3 块笔迹——盘上写的还是擦之前的内容")
                                .arg(cut.x())
                                .arg(cut.y())
                                .arg(polygonsCovering(reloadedPolygons, cut));
            }
            if (polygonsCovering(reloadedPolygons, leftEnd) == 0 || polygonsCovering(reloadedPolygons, rightEnd) == 0)
            {
                failures << QStringLiteral("冷读回来带两头丢了：左 %1 块、右 %2 块")
                                .arg(polygonsCovering(reloadedPolygons, leftEnd))
                                .arg(polygonsCovering(reloadedPolygons, rightEnd));
            }
            if (strokeGroupCount(reloaded) != 2)
            {
                failures << QStringLiteral("冷读回来的笔迹组是 %1 个（应当 2 个）").arg(strokeGroupCount(reloaded));
            }
        }

        stack->clear();
        settings->setEraserWidthIndex(eraserWidthIndexBefore);
        DrawingController::drawingController()->setStylusTool(toolBefore);

        PersistenceManager::persistenceManager()->deleteDocument(proxy);
        QDir(documentDirectory).removeRecursively();

        if (failures.isEmpty())
        {
            qInfo() << "eraser-selftest: OK 橡皮切开中段不误伤、一次擦除一条命令、撤销原样还原、存读一致";
        }

        return bail(failures);
    }
} // namespace EraserSelfTest
