#include "app/FormatSelfTest.h"

#include "core/PersistenceManager.h"
#include "content/ImportIwb.h"
#include "content/IwbDocumentAdaptor.h"
#include "content/SvgSubsetAdaptor.h"
#include "model/DocumentProxy.h"
#include "model/IDocumentFormatServices.h"
#include "settings/Settings.h"
#include "utils/Enums.h"
#include "utils/UserNotices.h"
#include "whiteboard/GraphicsAxes.h"
#include "whiteboard/GraphicsCompass.h"
#include "whiteboard/GraphicsGroupContainerItem.h"
#include "whiteboard/GraphicsPixmapItem.h"
#include "whiteboard/GraphicsPolygonItem.h"
#include "whiteboard/GraphicsProtractor.h"
#include "whiteboard/GraphicsRuler.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsStroke.h"
#include "whiteboard/GraphicsStrokesGroup.h"
#include "whiteboard/GraphicsSvgItem.h"
#include "whiteboard/GraphicsTextItem.h"

#include <QCryptographicHash>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QGraphicsItem>
#include <QLineF>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QPointF>
#include <QStringList>

#include <algorithm>
#include <cmath>

namespace
{
    /// @brief 自检期间收到的状态条消息条数；验的是装配点那条呈现通道接没接上。
    int gStatusMessages = 0;

    /// @brief 页尺寸与笔迹配色：往返两端共用同一组常量，比对才有参照。
    const QSize  kPageSize(1024, 768);
    const QColor kStrokeColor(220, 40, 40);
    const QColor kStrokeOnDark(240, 120, 110);
    const QColor kStrokeOnLight(180, 20, 20);
    const qreal  kStrokeWidth     = 7;

    /// @brief 自检文本；摆图元与比对 IWB 结果两端共用一份，不各写一遍字面量。
    const QString kSelftestText   = QStringLiteral("格式自检 Aa 一 1");

    /**
     * @brief 一页里要塞进去的内嵌 SVG 图元内容。
     * @note 验的是"引用与字节都得落下"：SVG 图元的原始字节只在内存里，页文件里只留
     *       images/{uuid}.svg 这个引用，写出器早期只写引用不写字节——于是保存再打开，
     *       读回来的是一个渲染不出任何东西的空图元（实测：0.9×0.9，内容指纹是空串的 MD5），
     *       既不崩也不报错，用户只看到"对象没了"。
     */
    const QByteArray kSelftestSvg = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
                                    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"60\" height=\"40\">"
                                    "<rect x=\"2\" y=\"2\" width=\"56\" height=\"36\" fill=\"#00a000\"/></svg>";

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
            else if (GraphicsAxes *axes = dynamic_cast<GraphicsAxes *>(item))
            {
                facts << QStringLiteral("axes|%1|%2").arg(rectOf(axes), axes->showNumbers() ? QStringLiteral("on") : QStringLiteral("off"));
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
            else if (GraphicsSvgItem *svgItem = dynamic_cast<GraphicsSvgItem *>(item))
            {
                // 页文件里只有一个 images/{uuid}.svg 的引用，比对这些字节才验得出
                // 内容真写进了这一份文档，而不是留在内存或写去了别的文档。
                const QString digest = QString::fromLatin1(QCryptographicHash::hash(svgItem->fileData(), QCryptographicHash::Md5).toHex());
                facts << QStringLiteral("svg|%1|%2").arg(rectOf(svgItem), digest);
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
            else if (dynamic_cast<GraphicsRuler *>(item) || dynamic_cast<GraphicsProtractor *>(item) || dynamic_cast<GraphicsCompass *>(item) ||
                     dynamic_cast<GraphicsAxes *>(item))
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

    /// @brief 攒出一页有代表性的内容：笔迹、文本、图片、内嵌 SVG 与三件工具图元。
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
        text->setPlainText(kSelftestText);
        text->setPos(420, 120);
        scene->addItem(text);

        GraphicsPixmapItem *pixmapItem = scene->addPixmap(makeTestPixmap(), nullptr, QPointF(460, 300), 1.0, false, true);
        GraphicsSvgItem    *svgItem    = scene->addSvg(QUrl(), QPointF(700, 430), kSelftestSvg);

        // 再各留一份 deepCopy 出来的副本：粘贴/复制图元走的就是这条路，副本有自己新的 UUID，
        // 而这条路上没人替它把资源写进文档目录——写出器必须自己在保存这一页时补上字节，
        // 否则读回来就是一个空图元。旧写法让副本沿用源的 UUID，于是删掉任一份会把另一份
        // 引用的文件一起删掉（deepCopy 见 GraphicsPixmapItem/GraphicsSvgItem）。
        if (GraphicsPixmapItem *pixmapCopy = pixmapItem ? dynamic_cast<GraphicsPixmapItem *>(pixmapItem->deepCopy()) : nullptr)
        {
            pixmapCopy->setPos(QPointF(460, 470));
            scene->addItem(pixmapCopy);
        }
        if (GraphicsSvgItem *svgCopy = svgItem ? dynamic_cast<GraphicsSvgItem *>(svgItem->deepCopy()) : nullptr)
        {
            svgCopy->setPos(QPointF(770, 500));
            scene->addItem(svgCopy);
        }

        scene->addAxes(QPointF(900, 200));

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

    /**
     * @brief 把这份文档再过一遍 IWB：导出 .iwb、由导入器读成一份新文档、把页读回来。
     *
     * 这一段验的是互操作链路（`IwbDocumentAdaptor` + `IwbSubsetAdaptor`）而不是原生页格式。
     * IWB 只认 text / image / polyline / polygon / rect / group，尺子量角器圆规这些工具图元本来
     * 就不在规范里，所以这里只要求文本、图片与笔迹颜色活着回来，并检查图片不是空位图；
     * 页上的内嵌 SVG 对象在 IWB 里只能以位图身份回来，算进"图片"那一档。
     */
    QStringList iwbFacts(const QString &documentDirectory, QStringList &failures)
    {
        const QString packagePath = documentDirectory + QStringLiteral(".iwb");

        IwbDocumentAdaptor adaptor;
        if (!adaptor.convertPageToIwb(documentDirectory, packagePath))
        {
            failures << QStringLiteral("IWB 导出失败: %1").arg(adaptor.getConversionMessages().join(QStringLiteral(" | ")));
            return QStringList();
        }

        ImportIwb      importer;
        DocumentProxy *imported = importer.importFile(QFile(packagePath), QString());
        if (!imported)
        {
            failures << QStringLiteral("IWB 导入没产出文档（包大小 %1 字节）").arg(QFileInfo(packagePath).size());
            return QStringList();
        }
        if (imported->pageCount() != 1)
        {
            failures << QStringLiteral("IWB 导入后应有 1 页，实际 %1 页").arg(imported->pageCount());
        }
        if (gStatusMessages == 0)
        {
            failures << QStringLiteral("导入这一路一条状态消息都没送到界面：装配点的 UserNotices 呈现通道没接上");
        }

        QStringList    facts;
        GraphicsScene *scene = SvgSubsetAdaptor::loadScene(imported, 0);
        if (!scene)
        {
            failures << QStringLiteral("IWB 导入的文档读不回第 0 页");
        }
        else
        {
            facts = sceneFacts(scene);
        }

        PersistenceManager::persistenceManager()->deleteDocument(imported);
        QFile::remove(packagePath);
        return facts;
    }
} // namespace

namespace FormatSelfTest
{
    int run()
    {
        QStringList failures;

        // 先确认装配点已经给主窗口接过这条通道（app/main.cpp 的 installStatusPresenter）：
        // 没接的话消息不会崩，只会退化成一行 qInfo，用户在界面上什么也看不见——正是要判红的。
        if (!UserNotices::hasStatusPresenter())
        {
            failures << QStringLiteral("装配点没装上状态条呈现方式（app/main.cpp 的 installStatusPresenter）");
        }

        // 再换成一个计数器，验导入导出器确实还在往外发消息：把某条 presentStatusMessage
        // 删掉，下面 iwbFacts 那条断言立刻变红。
        gStatusMessages = 0;
        UserNotices::installStatusPresenter(
            [](const QString &, bool)
            {
                ++gStatusMessages;
            });

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

        // 这一页必须真的带上 SVG 图元：addSvg 落不下文件会返回空，那时存前存后都没有它，
        // 下面的比对就成了自己跟自己对空。
        if (!std::any_of(expectedFacts.cbegin(), expectedFacts.cend(),
                [](const QString &line)
                {
                    return line.startsWith(QStringLiteral("svg|"));
                }))
        {
            failures << QStringLiteral("自检这一页里没有 SVG 图元（addSvg 没成功），比对失去意义");
        }

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

        // 元数据这一腿：标题、组名与页数写在 metadata.rdf 里，是 .tbz 契约的一部分。
        // 写错一个 RDF 键名既不崩也不报错，表现只是“文档库里这份文档回到了默认名字”。
        // 走的是模型侧的自由函数——与 DocumentController 保存、DocumentProxy 打开时同一条路，
        // 顺带把装配点装上的格式服务也验一次（没装上时读回来的是空表）。
        const QString selftestTitle = QStringLiteral("格式自检标题");
        const QString selftestGroup = QStringLiteral("格式自检分组");

        proxy->setMetaData(Settings::documentName, selftestTitle);
        proxy->setMetaData(Settings::documentGroupName, selftestGroup);
        persistDocumentMetadata(proxy);

        DocumentProxy cold(documentDirectory); // 冷读：构造时从盘上的 RDF 取元数据

        if (cold.metaData(Settings::documentName).toString() != selftestTitle)
        {
            failures << QStringLiteral("元数据标题没读回来：%1").arg(cold.metaData(Settings::documentName).toString());
        }

        if (cold.metaData(Settings::documentGroupName).toString() != selftestGroup)
        {
            failures << QStringLiteral("元数据组名没读回来：%1").arg(cold.metaData(Settings::documentGroupName).toString());
        }

        if (cold.metaData(Settings::documentPageCount) != QVariant(proxy->pageCount()))
        {
            failures << QStringLiteral("元数据页数与实际页数不一致：%1 对 %2")
                            .arg(cold.metaData(Settings::documentPageCount).toString())
                            .arg(proxy->pageCount());
        }

        // 缩略图同样走这条桥：取回来的必须是真图，不能是空白。
        QList<const QPixmap *> thumbs;
        loadDocumentPageThumbnails(&cold, thumbs);

        for (const QPixmap *pix : thumbs)
        {
            if (!pix || pix->isNull() || pix->width() <= 0)
            {
                failures << QStringLiteral("桥取回来的缩略图是空图");
            }
        }
        qDeleteAll(thumbs);

        if (failures.isEmpty())
        {
            const QStringList iwb = iwbFacts(documentDirectory, failures);

            int texts             = 0;
            int images            = 0;
            int strokes           = 0;
            for (const QString &fact : iwb)
            {
                if (fact.startsWith(QStringLiteral("text|")) && fact.contains(kSelftestText))
                {
                    texts++;
                }
                else if (fact.startsWith(QStringLiteral("pixmap|")))
                {
                    images++;
                    if (fact.endsWith(QStringLiteral("|<null>")))
                    {
                        failures << QStringLiteral("IWB 导入的图片是空位图: %1").arg(fact);
                    }
                }
                else if (fact.startsWith(QStringLiteral("segment|")))
                {
                    strokes++;
                    if (!fact.contains(kStrokeColor.name(QColor::HexRgb)))
                    {
                        failures << QStringLiteral("IWB 往返把笔迹颜色改了: %1").arg(fact);
                    }
                }
            }

            if (texts != 1)
            {
                failures << QStringLiteral("IWB 往返后应有 1 条自检文本，实际 %1 条").arg(texts);
            }
            if (images != 4)
            {
                // IWB 里没有"矢量对象"这一类，页上的内嵌 SVG 过一趟 IWB 会变成一张位图；
                // buildScene 这一页摆的是位图 + 位图副本 + SVG + SVG 副本，四样都以 image 回来。
                failures << QStringLiteral("IWB 往返后应有 4 张图片（两张位图与两个被栅格化的 SVG），实际 %1 张").arg(images);
            }
            if (strokes < 1)
            {
                failures << QStringLiteral("IWB 往返后笔迹全丢了");
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
