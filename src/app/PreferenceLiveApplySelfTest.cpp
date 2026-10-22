#include "app/PreferenceLiveApplySelfTest.h"

#include "core/PersistenceManager.h"
#include "model/DocumentProxy.h"
#include "settings/Setting.h"
#include "settings/Settings.h"
#include "utils/Enums.h"
#include "whiteboard/DrawingController.h"
#include "whiteboard/GraphicsScene.h"

#include <QCoreApplication>
#include <QDir>
#include <QGraphicsEllipseItem>

namespace
{
    /// @brief 数一数这一页上正显示着的预览圈（钢笔、荧光笔、橡皮三个圈都是 plain 的椭圆图元）。
    int visiblePreviewCircles(GraphicsScene *scene)
    {
        int count = 0;
        if (!scene)
        {
            return count;
        }

        foreach (QGraphicsItem *item, scene->tools())
        {
            // 指针那个圈挂在 Tool 层、hideTool() 也不收它，按层级键先排掉：预览圈三个都挂在 Control 层。
            if (item->data(GraphicsItemData::ItemLayerType).toInt() != ItemLayerType::Control)
            {
                continue;
            }

            if (item->isVisible() && dynamic_cast<QGraphicsEllipseItem *>(item))
            {
                ++count;
            }
        }

        return count;
    }

    /// @brief 把已攒下的不一致逐条报出来再退场（自检的退出码就是条数）。
    int bail(const QStringList &failures)
    {
        for (const QString &failure : failures)
        {
            qWarning() << "preflive-selftest:" << failure;
        }

        return failures.count();
    }
} // namespace

namespace PreferenceLiveApplySelfTest
{
    int run()
    {
        QStringList failures;

        Settings *settings                   = Settings::settings();

        const QVariant circleBefore          = settings->showPenPreviewCircle->get();
        const QVariant previewFromSizeBefore = settings->penPreviewFromSize->get();

        // 阈值先压到 0：圈出不出现还有一条「笔宽够不够」的门槛挡着，不排掉它，下面每次
        // "应当出现"都可能被笔宽拦住，量出来的东西说不清是谁的锅。
        settings->penPreviewFromSize->setInt(0);
        const qreal penWidth          = settings->currentPenWidth();

        // 悬空移动（不落笔）才会走预览圈这条路，所以把工具按界面上那条路切到钢笔。
        DrawingController *dc         = DrawingController::drawingController();
        const int          toolBefore = dc->stylusTool();
        dc->setStylusTool(StylusTool::Pen);

        const QString documentDirectory =
            QDir(QDir::tempPath()).filePath(QStringLiteral("TianBoard-PreferenceLiveApplySelfTest-%1").arg(QCoreApplication::applicationPid()));
        QDir(documentDirectory).removeRecursively();

        DocumentProxy *proxy = PersistenceManager::persistenceManager()->createDocument(
            QString(), QStringLiteral("PreferenceLiveApplySelfTest"), false, documentDirectory, 0, false);
        if (!proxy)
        {
            qWarning() << "preflive-selftest: 建临时文档失败";
            dc->setStylusTool(toolBefore);
            settings->penPreviewFromSize->set(previewFromSizeBefore);
            return 1;
        }

        // 两页的差别只在"建它的时候设置是什么"。界面上的勾选框写的就是 setPenPreviewCircle
        // 这个槽，这里也走它，不绕过去直接碰 Setting 指针。
        settings->setPenPreviewCircle(false);
        GraphicsScene *pageBuiltWhileOff = new GraphicsScene(proxy, false); // 父对象是文档代理，随代理一起销毁

        settings->setPenPreviewCircle(true);
        GraphicsScene *pageBuiltWhileOn = new GraphicsScene(proxy, false);

        // ---- 1. 对照页：在「打开」状态下建的一页，笔一悬过去就该有圈 ----
        // 这一条改前也是绿的。它存在的意义是证明下面那条判据不是空转：同一套观察办法，
        // 页是开着时建的就能看到圈，那么"看不见"确实是页本身没有圈可显示。
        pageBuiltWhileOn->hideTool();
        if (visiblePreviewCircles(pageBuiltWhileOn) != 0)
        {
            failures << QStringLiteral("对照页在笔悬过去之前不该有任何可见的预览圈");
        }
        pageBuiltWhileOn->inputDeviceMove(QPointF(120, 140));
        if (visiblePreviewCircles(pageBuiltWhileOn) != 1)
        {
            failures << QStringLiteral("设置本来就开着、又是开着时建的页，笔一悬过去却没有预览圈（笔宽 %1）").arg(penWidth);
        }

        // ---- 2. 「关闭」状态下建的那一页：把设置打开之后，这一页也得当场有圈 ----
        // 这是界面上看得到的一类失灵：圈这个图元以前只在建页时按设置决定建不建，于是一个
        // 在关闭状态下建好的页永远没有圈，把勾选框打开只对新页生效；反过来取消勾选只是"下一次
        // 悬笔不再显示"，看着像是当场生效。改前这一条是红的。
        pageBuiltWhileOff->hideTool();
        if (visiblePreviewCircles(pageBuiltWhileOff) != 0)
        {
            failures << QStringLiteral("这一页在笔悬过去之前不该有任何可见的预览圈");
        }
        pageBuiltWhileOff->inputDeviceMove(QPointF(200, 220));
        if (visiblePreviewCircles(pageBuiltWhileOff) != 1)
        {
            failures << QStringLiteral("偏好里把「钢笔预览圈」打开之后，在关闭状态下已经建好的那一页画不出圈"
                                       "（圈这个图元压根没被建出来，改设置只对新页生效；笔宽 %1）")
                            .arg(penWidth);
        }

        // ---- 3. 阈值抬高：笔宽不够就不许有圈 ----
        const int tooWideThreshold = int(penWidth) + 20;
        settings->penPreviewFromSize->setInt(tooWideThreshold);
        pageBuiltWhileOn->hideTool();
        pageBuiltWhileOn->inputDeviceMove(QPointF(260, 280));
        if (visiblePreviewCircles(pageBuiltWhileOn) != 0)
        {
            failures << QStringLiteral("阈值抬到 %1 之后（笔宽 %2）预览圈还显示着").arg(tooWideThreshold).arg(penWidth);
        }

        // ---- 4. 阈值调回：圈又当场出现（这一维每次悬笔都在读）----
        settings->penPreviewFromSize->setInt(0);
        pageBuiltWhileOn->inputDeviceMove(QPointF(300, 320));
        if (visiblePreviewCircles(pageBuiltWhileOn) != 1)
        {
            failures << QStringLiteral("阈值调回 0 之后预览圈没能重新显示");
        }

        // ---- 5. 取消勾选：这一页上正显示着的圈必须当场没掉 ----
        // 圈已经显示着的时候把勾选框取消，鼠标离开这一页之前圈一直挂着，也是同一类失灵：
        // 显示与否没被每次算干净。改前这一条同样是红的。
        settings->setPenPreviewCircle(false);
        pageBuiltWhileOn->inputDeviceMove(QPointF(340, 360));
        if (visiblePreviewCircles(pageBuiltWhileOn) != 0)
        {
            failures << QStringLiteral("偏好里取消「钢笔预览圈」之后，笔还悬在页上圈却没掉");
        }

        settings->setPenPreviewCircle(circleBefore.toBool());
        settings->penPreviewFromSize->set(previewFromSizeBefore);
        dc->setStylusTool(toolBefore);

        PersistenceManager::persistenceManager()->deleteDocument(proxy);
        QDir(documentDirectory).removeRecursively();

        if (failures.isEmpty())
        {
            qInfo() << "preflive-selftest: OK 钢笔预览圈随偏好当场起停，阈值每次悬笔都算";
        }

        return bail(failures);
    }
} // namespace PreferenceLiveApplySelfTest
