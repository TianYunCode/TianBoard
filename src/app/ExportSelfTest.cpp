#include "app/ExportSelfTest.h"

#include "content/ExportDocument.h"
#include "content/ExportFullPDF.h"
#include "content/ExportPDF.h"
#include "content/ImportDocument.h"
#include "core/PersistenceManager.h"
#include "model/DocumentProxy.h"
#include "model/DocumentTreeModel.h"
#include "model/IDocumentStore.h"
#include "platform/VirtualDesktop.h"
#include "utils/UserNotices.h"
#include "whiteboard/GraphicsPixmapItem.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsTextItem.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QImage>
#include <QPdfDocument>
#include <QPdfSelection>
#include <QPainter>
#include <QPen>
#include <QPixmap>

namespace
{
    /**
     * @brief 导出器报给状态条的消息清单（第 4 条用例装的收集器往这里加）。
     * @note 呈现方式注册表是进程级的，所以这里必须是命名空间作用域的容器而不是局部变量——
     *       捕获局部的话，注册表比它活得久，之后任何一条状态消息都是悬垂引用。
     */
    QStringList gStatusMessages;

    /// @brief 本条自检里裁决钩子被问了几次。
    int gAsked = 0;

    /**
     * @brief 造一个"改名"答案：把导入的这份登记成新名字。
     * @note .tbz 往返那一腿必须用它——答"替换"会把正在被这条自检使用的那份文档连节点一起
     *       删掉（节点析构删代理），手里的页场景立刻成悬垂指针。
     */
    PersistenceManager::DuplicateNameDecision renamedCopy(const QString &documentName)
    {
        PersistenceManager::DuplicateNameDecision decision;
        decision.proceed   = true;
        decision.finalName = documentName + QStringLiteral(" 导入");
        return decision;
    }

    /// @brief 造一个"替换"答案：finalName 落在同级名字里就是"顶掉那一份"。
    PersistenceManager::DuplicateNameDecision replaceExisting(const QString &documentName)
    {
        PersistenceManager::DuplicateNameDecision decision;
        decision.proceed   = true;
        decision.finalName = documentName;
        return decision;
    }

    /// @brief 每页的文字标记：导出器把哪一场景画进了哪一页，靠这两个串分辨。
    const QString kPageMarker[2] = {QStringLiteral("TIANBOARDPAGEZERO"), QStringLiteral("TIANBOARDPAGEONEONE")};

    /// @brief 造一块有区分度的位图：纯色压缩后指纹跟别的纯色没差别，加一条对角线才认得出串了页。
    QPixmap makeBlockPixmap(const QColor &color)
    {
        QPixmap pixmap(80, 60);
        pixmap.fill(color);

        QPainter painter(&pixmap);
        painter.setPen(QPen(color.darker(200), 5));
        painter.drawLine(0, 0, 79, 59);

        return pixmap;
    }

    /// @brief 字节指纹。
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

    /**
     * @brief 这一页"会被导出"的内容清单：文本与位图各按内容指纹列出。
     * @note .tbz 往返两端都用它比对；不比坐标，因为打包导入会把图元按原文恢复，
     *       真要盯住几何，格式自检那条已经盯了。
     */
    QStringList pageFacts(GraphicsScene *scene)
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

    /// @brief 渲染出来的页面上有多少非白像素：判"这页是不是白的"。
    int inkPixels(const QImage &image)
    {
        int ink = 0;

        for (int y = 0; y < image.height(); y++)
        {
            const QRgb *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));
            for (int x = 0; x < image.width(); x++)
            {
                const QRgb pixel = line[x];
                if (qAlpha(pixel) > 0 && (qRed(pixel) < 250 || qGreen(pixel) < 250 || qBlue(pixel) < 250))
                {
                    ++ink;
                }
            }
        }

        return ink;
    }

    /**
     * @brief 把一个刚导出的 PDF 读回来对账：页数、每页尺寸、每页文字、每页有没有墨。
     * @param failures 累积不一致的条数。
     * @param path PDF 路径。
     * @param scenes 期望的那几页（顺序即页序）。
     * @param label 这条断言属于哪个导出器，报错时能分辨。
     */
    void auditPdf(QStringList &failures, const QString &path, const QList<GraphicsScene *> &scenes, const QString &label)
    {
        QFileInfo info(path);
        if (!info.exists())
        {
            failures << QStringLiteral("%1 没产出文件: %2").arg(label, path);
            return;
        }
        if (info.size() < 1024)
        {
            failures << QStringLiteral("%1 只有 %2 字节，像是个空壳").arg(label).arg(info.size());
        }

        QPdfDocument document;
        const auto   error = document.load(path);
        if (error != QPdfDocument::Error::None)
        {
            failures << QStringLiteral("%1 产出的 PDF 读不回来（error=%2）").arg(label).arg(static_cast<int>(error));
            return;
        }

        if (document.pageCount() != scenes.size())
        {
            failures << QStringLiteral("%1 应有 %2 页，实际 %3 页").arg(label).arg(scenes.size()).arg(document.pageCount());
            return;
        }

        // 导出器按"场景像素 × 72/物理 DPI"定页面尺寸（点），这条把它的实际口径钉住：
        // docs/code-suspicions.md 里"导出 DPI 取哪块屏"还等产品定，先有可比较的现状。
        const qreal dpi         = VirtualDesktop::instance()->physicalDpi();
        const qreal scaleFactor = 72.0 / dpi;

        QStringList renderedInk;

        for (int i = 0; i < scenes.size(); i++)
        {
            const QSizeF pointSize     = document.pagePointSize(i);
            const QSize  expected      = scenes.at(i)->sceneSize();

            const qreal expectedWidth  = expected.width() * scaleFactor;
            const qreal expectedHeight = expected.height() * scaleFactor;

            if (qAbs(pointSize.width() - expectedWidth) > 2.0 || qAbs(pointSize.height() - expectedHeight) > 2.0)
            {
                failures << QStringLiteral("%1 第 %2 页尺寸不对: PDF 给的是 %3×%4 点，按场景 %5×%6 像素与 72/%7 DPI 应为 %8×%9 点")
                                .arg(label)
                                .arg(i + 1)
                                .arg(pointSize.width())
                                .arg(pointSize.height())
                                .arg(expected.width())
                                .arg(expected.height())
                                .arg(dpi)
                                .arg(expectedWidth)
                                .arg(expectedHeight);
            }

            const QString text = document.getAllText(i).text();
            if (!text.contains(kPageMarker[i]))
            {
                failures
                    << QStringLiteral("%1 第 %2 页里没有这页的文字（读到 %3 个字符），页与场景对不上号").arg(label).arg(i + 1).arg(text.length());
            }

            const QImage image = document.render(i, QSize(320, 240));
            if (image.isNull())
            {
                failures << QStringLiteral("%1 第 %2 页渲染不出图像").arg(label).arg(i + 1);
                continue;
            }

            const int ink = inkPixels(image);
            if (ink < 200)
            {
                failures << QStringLiteral("%1 第 %2 页几乎是白的（非白像素只有 %3 个）").arg(label).arg(i + 1).arg(ink);
            }

            renderedInk << QStringLiteral("%1").arg(ink);
        }

        // 两页内容不同，渲染结果必须可分辨——否则"两页其实画的是同一张"这种错就看不出来。
        if (renderedInk.size() == 2 && renderedInk.at(0) == renderedInk.at(1))
        {
            failures << QStringLiteral("%1 两页渲染出来的非白像素数完全一样（%2），怀疑每页画的是同一张场景").arg(label).arg(renderedInk.at(0));
        }
    }

    /// @brief 把已攒下的不一致逐条报出来再退场（自检的退出码就是条数）。
    int bail(const QStringList &failures)
    {
        for (const QString &failure : failures)
        {
            qWarning() << "export-selftest:" << failure;
        }

        return failures.count();
    }
} // namespace

namespace ExportSelfTest
{
    int run()
    {
        QStringList failures;

        const QString documentDirectory =
            QDir(QDir::tempPath()).filePath(QStringLiteral("TianBoard-ExportSelfTest-%1").arg(QCoreApplication::applicationPid()));
        QDir(documentDirectory).removeRecursively();
        QDir().mkpath(documentDirectory);

        // pageCount 传 0：withEmptyPage 已经会插一页并把计数加一，两个参数是叠加的，
        // 都传非零会让文档比实际页数多一页（多出来那页没有页文件，导出时会被补成空白页）。
        DocumentProxy *proxy =
            PersistenceManager::persistenceManager()->createDocument(QString(), QStringLiteral("ExportSelfTest"), true, documentDirectory, 0, false);
        if (!proxy)
        {
            qWarning() << "export-selftest: 建临时文档失败";
            return 1;
        }

        IDocumentStore *store = documentStore();

        GraphicsScene *page0  = store->loadDocumentScene(proxy, 0);
        GraphicsScene *page1  = store->createDocumentSceneAt(proxy, 1);
        if (!page0 || !page1)
        {
            failures << QStringLiteral("两页场景没建起来（第 0 页 %1，第 1 页 %2）").arg(page0 ? "有" : "无", page1 ? "有" : "无");
            return bail(failures);
        }

        if (proxy->pageCount() != 2)
        {
            failures << QStringLiteral("临时文档应有 2 页，实际 %1 页").arg(proxy->pageCount());
        }

        // 两页各放一块不同的位图与一段自己的文字；第 0 页设成深色底加网格，
        // 用来查导出器有没有把"临时改成白底"这件事恢复回去。
        page0->setBackground(true, PageBackground::crossed);
        page0->addText(kPageMarker[0], QPointF(60, 60));
        page0->addPixmap(makeBlockPixmap(QColor(220, 40, 40)), nullptr, QPointF(500, 400), 1.0, false, true);

        page1->addText(kPageMarker[1], QPointF(60, 300));
        page1->addPixmap(makeBlockPixmap(QColor(40, 120, 220)), nullptr, QPointF(700, 200), 1.0, false, true);

        const QStringList facts0 = pageFacts(page0);
        const QStringList facts1 = pageFacts(page1);
        if (facts0 == facts1)
        {
            failures << QStringLiteral("两页内容一模一样，后面的对账没有区分度");
        }

        // 落盘：.tbz 打包的是文档目录，页文件不在盘上就没东西可打包。
        PersistenceManager::persistenceManager()->persistDocumentScene(proxy, page0, 0);
        PersistenceManager::persistenceManager()->persistDocumentScene(proxy, page1, 1);

        const QList<GraphicsScene *> pages                 = {page0, page1};
        const int                    pageCountBeforeExport = proxy->pageCount();

        // ---- 1. 两个 PDF 导出器 ----
        const QString simplePdfPath                        = QDir(documentDirectory).filePath(QStringLiteral("export-simple.pdf"));
        const QString fullPdfPath                          = QDir(documentDirectory).filePath(QStringLiteral("export-full.pdf"));

        ExportPDF simpleExporter;
        if (!simpleExporter.persistsDocument(proxy, simplePdfPath))
        {
            failures << QStringLiteral("ExportPDF::persistsDocument 报失败");
        }
        auditPdf(failures, simplePdfPath, pages, QStringLiteral("ExportPDF"));

        ExportFullPDF fullExporter;
        if (!fullExporter.persistsDocument(proxy, fullPdfPath))
        {
            failures << QStringLiteral("ExportFullPDF::persistsDocument 报失败");
        }
        auditPdf(failures, fullPdfPath, pages, QStringLiteral("ExportFullPDF"));

        // 导出器为了白底会把场景临时改掉，改完必须还原，否则用户切回画板看到的就是白底。
        if (!page0->isDarkBackground())
        {
            failures << QStringLiteral("导出之后第 0 页的深色底没恢复");
        }
        if (static_cast<int>(page0->pageBackground()) != static_cast<int>(PageBackground::crossed))
        {
            failures << QStringLiteral("导出之后第 0 页的网格背景没恢复（实际 %1）").arg(static_cast<int>(page0->pageBackground()));
        }

        // ExportFullPDF 的中间产物是 *_overlay.pdf：合并器没接，它靠改名收尾，
        // 残留一个 overlay 文件就说明那条改名路没走干净。
        const QString overlayPath = QString(fullPdfPath).replace(QStringLiteral(".pdf"), QStringLiteral("_overlay.pdf"));
        if (QFileInfo::exists(overlayPath))
        {
            failures << QStringLiteral("ExportFullPDF 留下了中间的 overlay 文件: %1").arg(overlayPath);
        }

        // 导出必须是只读的：页数一变就说明它在替用户改文档。
        if (proxy->pageCount() != pageCountBeforeExport)
        {
            failures << QStringLiteral("导一趟 PDF 文档就胖了：导出前 %1 页、导出后 %2 页").arg(pageCountBeforeExport).arg(proxy->pageCount());
        }

        // ---- 2. .tbz 打包再导入回来 ----
        // 先答"改名"：这一腿要拿自己那份文档当参照，答"替换"会把它连节点一起删掉。
        gAsked = 0;
        PersistenceManager::installDuplicateNameResolver(
            [](const QString &documentName, const QStringList &)
            {
                ++gAsked;
                return renamedCopy(documentName);
            });

        const QString packagePath = QDir(documentDirectory).filePath(QStringLiteral("export-roundtrip.tbz"));

        ExportDocument packageExporter;
        if (!packageExporter.persistsDocument(proxy, packagePath))
        {
            failures << QStringLiteral("ExportDocument::persistsDocument 报失败");
        }

        const QFileInfo packageInfo(packagePath);
        if (!packageInfo.exists() || packageInfo.size() < 1024)
        {
            failures << QStringLiteral(".tbz 没产出或太小（存在 %1，%2 字节）").arg(packageInfo.exists() ? "是" : "否").arg(packageInfo.size());
        }
        else
        {
            ImportDocument packageImporter;
            DocumentProxy *reopened = packageImporter.importFile(QFile(packagePath), QString());
            if (!reopened)
            {
                failures << QStringLiteral(".tbz 导入不回来任何东西（包 %1 字节）").arg(packageInfo.size());
            }
            else
            {
                if (reopened->pageCount() != 2)
                {
                    failures << QStringLiteral(".tbz 导入回来应有 2 页，实际 %1 页").arg(reopened->pageCount());
                }

                for (int i = 0; i < reopened->pageCount() && i < 2; i++)
                {
                    GraphicsScene    *scene = documentStore()->loadDocumentScene(reopened, i);
                    const QStringList got   = scene ? pageFacts(scene) : QStringList();
                    const QStringList want  = (i == 0) ? facts0 : facts1;
                    if (got != want)
                    {
                        failures << QStringLiteral(".tbz 往返后第 %1 页内容对不上（出去 %2 项，回来 %3 项）")
                                        .arg(i + 1)
                                        .arg(want.count())
                                        .arg(got.count());
                    }
                }

                PersistenceManager::persistenceManager()->deleteDocument(reopened);
            }
        }

        // ---- 3. 同一份包再导入一次，这回答"替换" ----
        // 这一条过去直接崩：DocumentTreeModel::mCurrentNode 构造函数从没给过值，Debug 下它是
        // 调试堆的 0xCD… 填充，removeRows 里那句 `mCurrentNode && …` 恒真，紧接着拿它去走父链
        // 就是野指针解引用。答"替换"必然走 removeRow，所以这条正是那个初始化的回归钉。
        // 走完这一步，第 1 步那份 proxy 连同它的节点已经被删掉（节点析构删代理），下面不再碰它。
        gAsked = 0;
        PersistenceManager::installDuplicateNameResolver(
            [](const QString &documentName, const QStringList &)
            {
                ++gAsked;
                return replaceExisting(documentName);
            });

        ImportDocument replaceImporter;
        DocumentProxy *replacing = replaceImporter.importFile(QFile(packagePath), QString());
        if (!replacing)
        {
            failures << QStringLiteral("答\"替换\"之后 .tbz 没能登记进文档树");
        }
        else
        {
            if (gAsked == 0)
            {
                failures << QStringLiteral("同名替换那条路一次都没向裁决钩子要过答案");
            }

            DocumentTreeModel *docModel = documentStore()->documentTreeModel();
            int                sameName = 0;
            for (const QString &name : docModel->nodeNameList(docModel->myDocumentsIndex()))
            {
                if (name == QStringLiteral("ExportSelfTest"))
                {
                    ++sameName;
                }
            }
            if (sameName != 1)
            {
                failures << QStringLiteral("替换之后\"我的文档\"根下叫 ExportSelfTest 的只剩/多了 %1 份（应为 1）").arg(sameName);
            }
        }

        PersistenceManager::installDuplicateNameResolver(nullptr);
        if (replacing)
        {
            PersistenceManager::persistenceManager()->deleteDocument(replacing);
        }

        // ---- 4. 文档声称有页、盘上一页都读不到时：导出不许改文档，也不许一声不吭 ----
        // 旧写法读不到某一页就当场 createDocumentSceneAt(proxy, 0) 补一页、再把第 1 页的内容
        // 挂到请求的那个页号上：同一份文档连导两趟，pageCount 一路涨、页号全体后移，
        // "第 N 页"其实是第 1 页。导出必须是只读操作。
        // 这一条用"声称两页、一页都没有"的文档来验（场景缓存里没有那几页，才会真去读盘）。
        const QString phantomDirectory = QDir(documentDirectory).filePath(QStringLiteral("phantom"));
        QDir().mkpath(phantomDirectory);

        gStatusMessages.clear();
        UserNotices::installStatusPresenter(
            [](const QString &message, bool)
            {
                gStatusMessages << message;
            });

        DocumentProxy *phantom = PersistenceManager::persistenceManager()->createDocument(
            QString(), QStringLiteral("ExportSelfTestPhantom"), false, phantomDirectory, 2, false);
        if (!phantom)
        {
            failures << QStringLiteral("建那份声称两页、盘上一页都没有的临时文档失败");
        }
        else
        {
            const QString phantomPdfPath = QDir(documentDirectory).filePath(QStringLiteral("export-phantom.pdf"));
            ExportPDF     phantomExporter;
            phantomExporter.persistsDocument(phantom, phantomPdfPath);

            if (phantom->pageCount() != 2)
            {
                failures << QStringLiteral("导一趟没有可读页的文档之后页数被改成 %1（导出必须是只读的）").arg(phantom->pageCount());
            }
            if (!QDir(phantomDirectory).entryList({QStringLiteral("page*.svg")}, QDir::Files).isEmpty())
            {
                failures << QStringLiteral("导出替没有可读页的文档补出了页文件");
            }

            // 匹配的是 tr() 的源文本：这条自检跑的是英文源，不走翻译。
            bool toldUser = false;
            for (const QString &message : gStatusMessages)
            {
                if (message.contains(QStringLiteral("no readable page")))
                {
                    toldUser = true;
                }
            }
            if (!toldUser)
            {
                failures << QStringLiteral("一页都没画出来，导出器却一声不吭（用户只会拿到一个空 PDF）");
            }

            QFile::remove(phantomPdfPath);
        }

        QFile::remove(simplePdfPath);
        QFile::remove(fullPdfPath);
        QFile::remove(packagePath);

        // 不再 deleteDocument(proxy)：走到这一步它多半已经随节点析构掉了，
        // 真留下的那份也由 PersistenceManager 的析构统一回收。
        QDir(documentDirectory).removeRecursively();

        if (failures.isEmpty())
        {
            qInfo() << "export-selftest: OK PDF 两个导出器、.tbz 往返、同名替换与缺页只读五条都对得上账";
        }

        return bail(failures);
    }
} // namespace ExportSelfTest
