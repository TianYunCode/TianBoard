#include "core/SnapSelfTest.h"

#include "core/Application.h"
#include "core/PersistenceManager.h"
#include "model/DocumentProxy.h"
#include "utils/Enums.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/GraphicsPolygonItem.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsStroke.h"
#include "whiteboard/GraphicsStrokesGroup.h"

#include <QDir>
#include <QGraphicsRectItem>
#include <QPen>

namespace
{
    /// @brief 比较两个坐标，网格运算只要求 1e-6 内一致。
    bool same(QPointF a, QPointF b)
    {
        return std::abs(a.x() - b.x()) < 1e-6 && std::abs(a.y() - b.y()) < 1e-6;
    }
} // namespace

namespace SnapSelfTest
{
    int run()
    {
        QStringList failures;

        const QString documentDirectory =
            QDir(QDir::tempPath()).filePath(QStringLiteral("TianBoard-SnapSelfTest-%1").arg(QCoreApplication::applicationPid()));
        QDir(documentDirectory).removeRecursively();

        DocumentProxy *proxy =
            PersistenceManager::persistenceManager()->createDocument(QString(), QStringLiteral("SnapSelfTest"), false, documentDirectory, 0, false);
        if (!proxy)
        {
            qWarning() << "snap-selftest: 建临时文档失败";
            return 1;
        }

        GraphicsScene *scene = new GraphicsScene(proxy, false); // 父对象是文档代理，随代理一起销毁
        scene->setNominalSize(QSize(1024, 768));
        scene->setBackground(false, PageBackground::plain);
        scene->setBackgroundGridSize(50);

        const bool snappingWasOn = Application::boardController->isSnapping();
        Application::boardController->setSnapping(true);

        if (!scene->isSnapping())
        {
            failures << QStringLiteral("开了控制器开关，场景却没认为吸附生效");
        }

        // 空白底两轴都吸：112/74 都在半格（25）以内，应吸到 100/50。
        const QPointF plain = scene->snapToGrid(QPointF(112, 74));
        if (!same(plain, QPointF(-12, -24)))
        {
            failures << QStringLiteral("空白底吸附结果不对: (%1,%2) 期望 (-12,-24)").arg(plain.x()).arg(plain.y());
        }

        // 横线稿纸只吸 Y。
        scene->setBackground(false, PageBackground::ruled);
        const QPointF ruled = scene->snapToGrid(QPointF(112, 74));
        if (!same(ruled, QPointF(0, -24)))
        {
            failures << QStringLiteral("稿纸背景不该吸 X，实得 (%1,%2)").arg(ruled.x()).arg(ruled.y());
        }
        scene->setBackground(false, PageBackground::plain);

        // 多角点取吸得力最强的那个角，并把角标带回来。
        int     cornerIndex   = -1;
        QPointF cornersOffset = scene->snapCorners({QPointF(124, 50), QPointF(103, 50)}, &cornerIndex);
        if (!same(cornersOffset, QPointF(-3, 0)) || cornerIndex != 1)
        {
            failures << QStringLiteral("角点择优错了: offset=(%1,%2) corner=%3，期望 (-3,0) corner=1")
                            .arg(cornersOffset.x())
                            .arg(cornersOffset.y())
                            .arg(cornerIndex);
        }

        // 带画笔的图形要按笔宽缩回一半：Qt 的 boundingRect 本来就把画笔算进去了
        // （这个 100x60 的框带 6 号笔会报成 106x66），吸回去之后才是几何外框。
        QGraphicsRectItem rectItem(0, 0, 100, 60);
        rectItem.setPen(QPen(Qt::black, 6));
        const QRectF shrunk = GraphicsScene::itemRectForSnapping(&rectItem);
        if (!same(shrunk.topLeft(), QPointF(0, 0)) || !same(shrunk.bottomRight(), QPointF(100, 60)))
        {
            failures
                << QStringLiteral("笔宽缩回没回到几何外框: %1,%2 %3x%4").arg(shrunk.x()).arg(shrunk.y()).arg(shrunk.width()).arg(shrunk.height());
        }

        // 只有一段的直线笔迹按两个端点算框，而不是斜线的方形包围盒。
        GraphicsStroke       *stroke  = new GraphicsStroke(scene);
        GraphicsStrokesGroup *group   = new GraphicsStrokesGroup();
        GraphicsPolygonItem  *segment = new GraphicsPolygonItem(QLineF(10, 10, 90, 50), 4);
        segment->setStroke(stroke);
        segment->setStrokesGroup(group);
        group->addToGroup(segment);
        const QRectF lineRect = GraphicsScene::itemRectForSnapping(group);
        if (!same(lineRect.topLeft(), QPointF(10, 10)) || !same(lineRect.bottomRight(), QPointF(90, 50)))
        {
            failures << QStringLiteral("单段直线笔迹没按端点取框: %1,%2 %3x%4")
                            .arg(lineRect.x())
                            .arg(lineRect.y())
                            .arg(lineRect.width())
                            .arg(lineRect.height());
        }

        // 关掉开关后必须一点都不吸。
        Application::boardController->setSnapping(false);
        if (!same(scene->snapToGrid(QPointF(112, 74)), QPointF(0, 0)))
        {
            failures << QStringLiteral("吸附关着还在吸");
        }

        Application::boardController->setSnapping(snappingWasOn);

        for (const QString &failure : failures)
        {
            qWarning() << "snap-selftest:" << failure;
        }

        if (failures.isEmpty())
        {
            qInfo() << "snap-selftest: OK";
        }

        PersistenceManager::persistenceManager()->deleteDocument(proxy);
        QDir(documentDirectory).removeRecursively();

        return failures.size();
    }
} // namespace SnapSelfTest
