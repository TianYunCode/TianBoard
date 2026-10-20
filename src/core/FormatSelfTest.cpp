#include "core/FormatSelfTest.h"

#include "core/PersistenceManager.h"
#include "content/SvgSubsetAdaptor.h"
#include "model/DocumentProxy.h"
#include "utils/Enums.h"
#include "whiteboard/GraphicsCompass.h"
#include "whiteboard/GraphicsGroupContainerItem.h"
#include "whiteboard/GraphicsPixmapItem.h"
#include "whiteboard/GraphicsPolygonItem.h"
#include "whiteboard/GraphicsProtractor.h"
#include "whiteboard/GraphicsRuler.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsStroke.h"
#include "whiteboard/GraphicsStrokesGroup.h"
#include "whiteboard/GraphicsTextItem.h"

#include <QCryptographicHash>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QGraphicsItem>
#include <QLineF>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QStringList>

#include <algorithm>
#include <cmath>

namespace
{
    /// @brief 页尺寸与笔迹配色：往返两端共用同一组常量，比对才有参照。
    const QSize  kPageSize(1024, 768);
    const QColor kStrokeColor(220, 40, 40);
    const QColor kStrokeOnDark(240, 120, 110);
    const QColor kStrokeOnLight(180, 20, 20);
    const qreal  kStrokeWidth = 7;

    /// @brief 按 0.1 取整，吸收写出器两位小数的坐标精度损失。
    qreal quantise(qreal v)
    {
        return std::round(v * 10.0) / 10.0;
    }

    QString fmt(qreal v)
    {
        return QString::number(quantise(v), 'f', 1);
    }

    /// @brief 图元在场景坐标系下的包围盒签名。
    QString rectOf(const QGraphicsItem *item)
    {
        QRectF r = item->sceneBoundingRect();
        return QStringLiteral("%1,%2,%3,%4").arg(fmt(r.x()), fmt(r.y()), fmt(r.width()), fmt(r.height()));
    }

    /// @brief 笔迹段的完整签名：几何、三种底色下的取色与笔宽。
    QString segmentFact(GraphicsPolygonItem *polygon)
    {
        return QStringLiteral("segment|%1|%2|%3|%4|%5")
            .arg(rectOf(polygon), polygon->color().name(QColor::HexRgb), polygon->colorOnDarkBackground().name(QColor::HexRgb),
                polygon->colorOnLightBackground().name(QColor::HexRgb), fmt(polygon->originalWidth()));
    }

    /// @brief 给位图内容做指纹，验的是像素真的存回了文件而不只是留了个引用。
    QString imageFingerprint(const QPixmap &pixmap)
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
     * @brief 把场景里“会写进页文件”的图元摊成可排序的文本行。
     *
     * 只统计可见且非背景的图元：擦除预览、笔迹指示圆那几件内部图元常驻场景但不可见，
     * 写出器也靠 isVisible() 把它们挡掉，这里必须用同一套口径。
     */
    QStringList sceneFacts(const GraphicsScene *scene)
    {
        QStringList facts;
        foreach (QGraphicsItem *item, scene->items())
        {
            if (!item->isVisible() || scene->isBackgroundObject(item))
            {
                continue;
            }

            if (GraphicsTextItem *text = dynamic_cast<GraphicsTextItem *>(item))
            {
                facts << QStringLiteral("text|%1|%2").arg(rectOf(text), text->toPlainText().simplified());
            }
            else if (GraphicsPixmapItem *pixmapItem = dynamic_cast<GraphicsPixmapItem *>(item))
            {
                facts << QStringLiteral("pixmap|%1|%2").arg(rectOf(pixmapItem), imageFingerprint(pixmapItem->pixmap()));
            }
            else if (dynamic_cast<GraphicsRuler *>(item))
            {
                facts << QStringLiteral("ruler|%1").arg(rectOf(item));
            }
            else if (dynamic_cast<GraphicsProtractor *>(item))
            {
                facts << QStringLiteral("protractor|%1").arg(rectOf(item));
            }
            else if (dynamic_cast<GraphicsCompass *>(item))
            {
                facts << QStringLiteral("compass|%1").arg(rectOf(item));
            }
            else if (dynamic_cast<GraphicsGroupContainerItem *>(item))
            {
                facts << QStringLiteral("group|%1").arg(rectOf(item));
            }
            else if (GraphicsPolygonItem *polygon = dynamic_cast<GraphicsPolygonItem *>(item))
            {
                facts << segmentFact(polygon);
            }
            else
            {
                facts << QStringLiteral("other|%1").arg(rectOf(item));
            }
        }
        facts.sort();
        return facts;
    }

    /// @brief 顶层图元按自有 z 值排出的类别序列，用来验叠放次序没在往返中被打乱。
    QStringList sceneOrder(const GraphicsScene *scene)
    {
        QList<QGraphicsItem *> tops;
        foreach (QGraphicsItem *item, scene->items())
        {
            if (item->isVisible() && !item->parentItem() && !scene->isBackgroundObject(item))
            {
                tops << item;
            }
        }

        std::stable_sort(tops.begin(), tops.end(),
            [](const QGraphicsItem *a, const QGraphicsItem *b)
            {
                return a->data(GraphicsItemData::ItemOwnZValue).toReal() < b->data(GraphicsItemData::ItemOwnZValue).toReal();
            });

        QStringList kinds;
        foreach (QGraphicsItem *item, tops)
        {
            if (dynamic_cast<GraphicsTextItem *>(item))
            {
                kinds << QStringLiteral("text");
            }
            else if (dynamic_cast<GraphicsPixmapItem *>(item))
            {
                kinds << QStringLiteral("pixmap");
            }
            else if (dynamic_cast<GraphicsStrokesGroup *>(item))
            {
                kinds << QStringLiteral("stroke");
            }
            else if (dynamic_cast<GraphicsGroupContainerItem *>(item))
            {
                kinds << QStringLiteral("group");
            }
            else if (dynamic_cast<GraphicsRuler *>(item) || dynamic_cast<GraphicsProtractor *>(item) || dynamic_cast<GraphicsCompass *>(item))
            {
                kinds << QStringLiteral("tool");
            }
            else
            {
                kinds << QStringLiteral("other");
            }
        }
        return kinds;
    }

    /// @brief 造一张有内容的测试位图，纯色压缩后指纹没有区分度。
    QPixmap makeTestPixmap()
    {
        QImage image(64, 48, QImage::Format_ARGB32);
        image.fill(QColor(30, 90, 200));
        QPainter painter(&image);
        painter.setPen(QColor(255, 230, 0));
        painter.drawLine(2, 44, 60, 6);
        painter.setPen(QColor(255, 255, 255));
        painter.drawRect(8, 8, 40, 24);
        painter.end();
        return QPixmap::fromImage(image);
    }

    /**
     * @brief 按绘制管线的方式摆一笔：逐段 nominal line、共享 stroke、收进笔迹组。
     *
     * 三件事都必须照做，否则写出器认不出这是一笔：段要 setStroke（否则 stroke->polygons()
     * 为空，走不到 polyline 分支）、段要 setStrokesGroup（组信息写在 g 元素上），
     * 且所有段同宽且都是 nominal line（否则 hasPressure() 为真，改写成带压感的 polygon）。
     */
    void addFreehandStroke(GraphicsScene *scene, const QList<QPointF> &points)
    {
        GraphicsStroke       *stroke = new GraphicsStroke(scene);
        GraphicsStrokesGroup *group  = new GraphicsStrokesGroup();

        for (int i = 0; i + 1 < points.size(); i++)
        {
            GraphicsPolygonItem *polygon = new GraphicsPolygonItem(QLineF(points.at(i), points.at(i + 1)), kStrokeWidth);
            polygon->setColor(kStrokeColor);
            polygon->setColorOnDarkBackground(kStrokeOnDark);
            polygon->setColorOnLightBackground(kStrokeOnLight);
            polygon->setStroke(stroke);
            polygon->setStrokesGroup(group);
            group->addToGroup(polygon);
        }

        scene->addItem(group);
    }

    /// @brief 攒出一页有代表性的内容：笔迹、文本、图片与三件工具图元。
    GraphicsScene *buildScene(DocumentProxy *proxy)
    {
        GraphicsScene *scene = new GraphicsScene(proxy, false);
        scene->setNominalSize(kPageSize);
        scene->setBackground(false, PageBackground::crossed);
        scene->setBackgroundGridSize(40);

        QList<QPointF> points;
        points << QPointF(120, 140) << QPointF(190, 260) << QPointF(260, 150) << QPointF(330, 270);
        addFreehandStroke(scene, points);

        GraphicsTextItem *text = new GraphicsTextItem();
        text->setPlainText(QStringLiteral("格式自检 Aa 一 1"));
        text->setPos(420, 120);
        scene->addItem(text);

        scene->addPixmap(makeTestPixmap(), nullptr, QPointF(460, 300), 1.0, false, true);

        scene->addRuler(QPointF(300, 620));
        scene->addProtractor(QPointF(560, 620));
        scene->addCompass(QPointF(820, 600));

        return scene;
    }

    /// @brief 找出文档目录里写出来的页文件；自检只放一页，多于一页就说明状态不干净。
    QStringList pageFiles(const QString &documentDirectory)
    {
        return QDir(documentDirectory).entryList({QStringLiteral("page*.svg")}, QDir::Files, QDir::Name);
    }
} // namespace

namespace FormatSelfTest
{
    int run()
    {
        QStringList failures;

        // 按进程号命名，重复运行或并行跑都不会撞到上一次的文档；开工前先清干净。
        const QString documentDirectory =
            QDir(QDir::tempPath()).filePath(QStringLiteral("TianBoard-FormatSelfTest-%1").arg(QCoreApplication::applicationPid()));
        QDir(documentDirectory).removeRecursively();
        QDir().mkpath(documentDirectory);

        DocumentProxy *proxy =
            PersistenceManager::persistenceManager()->createDocument(QString(), QStringLiteral("FormatSelfTest"), false, documentDirectory, 1, false);
        if (!proxy)
        {
            qWarning() << "format-selftest: 建临时文档失败";
            return 1;
        }

        GraphicsScene    *scene              = buildScene(proxy);
        const QStringList expectedFacts      = sceneFacts(scene);
        const QStringList expectedOrder      = sceneOrder(scene);
        const bool        expectedDark       = scene->isDarkBackground();
        const int         expectedBackground = static_cast<int>(scene->pageBackground());
        const int         expectedGridSize   = scene->backgroundGridSize();

        scene->setModified(true);
        PersistenceManager::persistenceManager()->persistDocumentScene(proxy, scene, 0);

        const QStringList pages = pageFiles(documentDirectory);
        if (pages.size() != 1)
        {
            failures << QStringLiteral("页文件数量不对，期望 1 个，实际 %1 个").arg(pages.size());
        }

        // 冷读：绕开场景缓存，直接拿盘上的字节走一遍读取器。
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
            const QStringList gotFacts = sceneFacts(reloaded);
            const QStringList gotOrder = sceneOrder(reloaded);

            if (gotFacts != expectedFacts)
            {
                foreach (const QString &line, expectedFacts)
                {
                    if (!gotFacts.contains(line))
                    {
                        failures << QStringLiteral("读回后缺少: %1").arg(line);
                    }
                }
                foreach (const QString &line, gotFacts)
                {
                    if (!expectedFacts.contains(line))
                    {
                        failures << QStringLiteral("读回后多出: %1").arg(line);
                    }
                }
            }

            if (gotOrder != expectedOrder)
            {
                failures << QStringLiteral("叠放次序变了: 存前 %1 / 读回 %2")
                                .arg(expectedOrder.join(QStringLiteral(">")), gotOrder.join(QStringLiteral(">")));
            }

            if (reloaded->isDarkBackground() != expectedDark)
            {
                failures << QStringLiteral("深浅底丢了: 存前 %1 读回 %2").arg(expectedDark).arg(reloaded->isDarkBackground());
            }
            if (static_cast<int>(reloaded->pageBackground()) != expectedBackground)
            {
                failures
                    << QStringLiteral("页面背景类型丢了: 存前 %1 读回 %2").arg(expectedBackground).arg(static_cast<int>(reloaded->pageBackground()));
            }
            if (reloaded->backgroundGridSize() != expectedGridSize)
            {
                failures << QStringLiteral("网格尺寸丢了: 存前 %1 读回 %2").arg(expectedGridSize).arg(reloaded->backgroundGridSize());
            }

            // 第二个循环必须落在同一个点上：累积型误差只看一轮常常看不出来（每存开一次
            // 就胖一个笔宽的尺子就是这样），再走一轮存读、要求结果与第一轮完全相同。
            reloaded->setModified(true);
            PersistenceManager::persistenceManager()->persistDocumentScene(proxy, reloaded, 0);

            const QByteArray pageXmlAgain = SvgSubsetAdaptor::loadSceneAsText(proxy, 0);
            GraphicsScene   *second       = SvgSubsetAdaptor::loadScene(proxy, pageXmlAgain);
            if (!second)
            {
                failures << QStringLiteral("第二轮读不回场景（%1 字节）").arg(pageXmlAgain.size());
            }
            else
            {
                const QStringList secondFacts = sceneFacts(second);
                const QStringList secondOrder = sceneOrder(second);

                if (secondFacts != gotFacts)
                {
                    foreach (const QString &line, gotFacts)
                    {
                        if (!secondFacts.contains(line))
                        {
                            failures << QStringLiteral("第二个循环里变了: %1").arg(line);
                        }
                    }
                    foreach (const QString &line, secondFacts)
                    {
                        if (!gotFacts.contains(line))
                        {
                            failures << QStringLiteral("第二个循环里多出: %1").arg(line);
                        }
                    }
                }
                if (secondOrder != gotOrder)
                {
                    failures << QStringLiteral("第二个循环里叠放次序变了: %1").arg(secondOrder.join(QStringLiteral(">")));
                }
            }

            const QStringList pagesAfterTwo = pageFiles(documentDirectory);
            if (pagesAfterTwo.size() != 1)
            {
                failures << QStringLiteral("两轮存完页文件应有 1 个，实际 %1 个").arg(pagesAfterTwo.size());
            }
        }

        for (const QString &failure : failures)
        {
            qWarning() << "format-selftest:" << failure;
        }

        if (failures.isEmpty())
        {
            qInfo() << "format-selftest: OK" << expectedFacts.size() << "项图元特征往返一致";
            QDir(documentDirectory).removeRecursively();
        }
        else
        {
            qWarning() << "format-selftest: 现场保留在" << documentDirectory;
        }

        return failures.size();
    }
} // namespace FormatSelfTest
