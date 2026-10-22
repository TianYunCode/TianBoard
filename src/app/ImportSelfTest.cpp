#include "app/ImportSelfTest.h"

#include "content/SvgSubsetAdaptor.h"
#include "core/DocumentManager.h"
#include "core/PersistenceManager.h"
#include "model/DocumentProxy.h"
#include "model/DocumentTreeModel.h"
#include "model/PackageLayout.hpp"
#include "settings/Settings.h"
#include "utils/FileSystemUtils.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/GraphicsPDFItem.h"
#include "whiteboard/GraphicsPixmapItem.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsTextItem.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QPdfWriter>
#include <QPageSize>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QSize>
#include <QStringList>

#include <functional>

namespace
{
    /// 造页用的尺寸：与页面读写自检同一套，缩略图与页文件才有内容。
    const QSize kPageSize(1024, 768);

    /// @brief 桩裁决器被征询过一次：记下核心问的名字与当时的同级名字列表。
    struct Asked
    {
        QString     documentName;
        QStringList siblingNames;
    };

    /// 排好队的裁决答案，按次序发给核心；发完就空了。
    QList<PersistenceManager::DuplicateNameDecision> gScript;

    /// 核心每一次征询的记录，每个案子验完就清空。
    QList<Asked> gAsked;

    PersistenceManager::DuplicateNameDecision stubResolve(const QString &documentName, const QStringList &siblingNames)
    {
        gAsked.append(Asked{documentName, siblingNames});
        if (gScript.isEmpty())
        {
            // 答案用完了还被问：说明这条路径压根没人替用户想过，按取消处理并让计数断言露出它。
            return {};
        }
        return gScript.takeFirst();
    }

    PersistenceManager::DuplicateNameDecision proceed(const QString &finalName)
    {
        PersistenceManager::DuplicateNameDecision decision;
        decision.proceed   = true;
        decision.finalName = finalName;
        return decision;
    }

    PersistenceManager::DuplicateNameDecision cancelled()
    {
        return PersistenceManager::DuplicateNameDecision();
    }

    /// @brief 验上一个案子问了几次、问的是不是这份文档、同级列表里有没有撞名的旧文档。
    void expectAsked(QStringList &failures, const QString &caseName, int count, const QString &collidingName)
    {
        if (gAsked.size() != count)
        {
            failures << QStringLiteral("%1：应当征询 %2 次，实际 %3 次").arg(caseName).arg(count).arg(gAsked.size());
        }
        else if (count > 0)
        {
            const Asked &asked = gAsked.first();
            if (asked.documentName != collidingName)
            {
                failures << QStringLiteral("%1：问错了对象，问的是「%2」").arg(caseName, asked.documentName);
            }
            if (!asked.siblingNames.contains(collidingName))
            {
                failures << QStringLiteral("%1：同级名字列表里没有旧文档「%2」（实际 %3）")
                                .arg(caseName, collidingName, asked.siblingNames.join(QStringLiteral(", ")));
            }
        }
        gAsked.clear();
    }

    /// @brief 造一份真正落盘的文档包：登记进树、写一页、写元数据，之后可以整包复制出去当导入源。
    DocumentProxy *seedDocument(PersistenceManager *pm, const QString &dirPath, const QString &name, QStringList &failures)
    {
        DocumentProxy *proxy = pm->createDocument(QString(), name, false, dirPath, 1, false);
        if (!proxy)
        {
            failures << QStringLiteral("造底稿文档失败：%1").arg(dirPath);
            return nullptr;
        }

        // 场景交给持久化管理器的缓存收尾（persistDocumentScene 会把它插进缓存），这里不自己 delete。
        GraphicsScene *scene = new GraphicsScene(proxy, false);
        scene->setNominalSize(kPageSize);

        GraphicsTextItem *text = new GraphicsTextItem();
        text->setPlainText(QStringLiteral("导入自检页"));
        text->setPos(60, 60);
        scene->addItem(text);

        scene->setModified(true);
        pm->persistDocumentScene(proxy, scene, 0);
        pm->persistDocumentMetadata(proxy);

        if (!QFileInfo(dirPath + QStringLiteral("/metadata.rdf")).exists())
        {
            failures << QStringLiteral("底稿没有写出 metadata.rdf：%1").arg(dirPath);
        }
        return proxy;
    }

    /// @brief 把底稿整包复制成一份“等着被导入”的目录：与底稿同名，但还没进文档树。
    bool clonePackage(const QString &fromDir, const QString &toDir)
    {
        QDir(toDir).removeRecursively();
        return FileSystemUtils::copyDir(fromDir, toDir);
    }

    /**
     * @brief 现场造一份两页 PDF：每页一块不同颜色的实心背景，导入回来才分得清页有没有串。
     * @note 不依赖仓库里任何样例文件——导入这条路的输入必须是自检自己写的，才知道是谁坏了。
     */
    bool writeTwoPagePdf(const QString &path)
    {
        QPdfWriter writer(path);
        writer.setPageSize(QPageSize(QSizeF(300, 200), QPageSize::Point));
        writer.setPageMargins(QMarginsF(0, 0, 0, 0));

        QPainter painter;
        if (!painter.begin(&writer))
        {
            return false;
        }

        painter.fillRect(QRectF(0, 0, 300, 200), QColor(220, 40, 40));
        painter.setPen(QPen(QColor(255, 255, 255), 6));
        painter.drawLine(20, 20, 280, 180);

        writer.newPage();

        painter.fillRect(QRectF(0, 0, 300, 200), QColor(40, 120, 220));
        painter.setPen(QPen(QColor(0, 0, 0), 6));
        painter.drawLine(280, 20, 20, 180);
        painter.end();

        return QFileInfo(path).size() > 512;
    }

    /// @brief 字节指纹；空内容的 MD5 是 d41d8cd9…，读回一个空东西时一眼看得出来。
    QString digest(const QByteArray &bytes)
    {
        return QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Md5).toHex());
    }

    /// @brief 位图像素指纹（与格式/导出自检同一口径：先归一到 ARGB32 再取位）。
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

    /// @brief 数一数场景里某类图元有几个（含背景对象：导入进来的 PDF/图片就是背景对象）。
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

    /// @brief 数一下画板容器里那份缩略图列表有多长（越界读到空为止，另有上限防脱缰）。
    int thumbCount(BoardController *board, int stopAfter)
    {
        int thumbs = 0;
        while (thumbs < stopAfter && board->pageAt(thumbs))
        {
            thumbs++;
        }
        return thumbs;
    }

    /// @brief 树里“我的文档”下的名字列表，用来数同名节点还剩几个。
    QStringList rootNames(DocumentTreeModel *model)
    {
        return model->nodeNameList(model->myDocumentsIndex());
    }

    /// @brief 收尾：测试留在树里的文档与分组全部删掉，用户的文档库不该留着自测痕迹。
    void purgeTestDocuments(PersistenceManager *pm, const QString &prefix)
    {
        DocumentTreeModel *model                       = pm->mDocumentTreeStructureModel;

        // 深度优先：先清子节点，父分组的行号才会在删除过程中保持不动。
        std::function<void(const QModelIndex &)> purge = [&](const QModelIndex &parent)
        {
            const QStringList names = model->nodeNameList(parent);
            for (int i = names.size() - 1; i >= 0; i--)
            {
                const QModelIndex child = model->index(i, 0, parent);
                if (model->isCatalog(child))
                {
                    purge(child);
                    if (names.at(i).startsWith(prefix))
                    {
                        model->removeRow(i, parent);
                    }
                }
                else if (names.at(i).startsWith(prefix))
                {
                    DocumentProxy *proxy = model->proxyData(child);
                    if (proxy)
                    {
                        pm->deleteDocument(proxy);
                    }
                }
            }
        };

        purge(model->myDocumentsIndex());
    }
} // namespace

namespace ImportSelfTest
{
    int run()
    {
        QStringList failures;

        PersistenceManager *pm    = PersistenceManager::persistenceManager();
        DocumentTreeModel  *model = pm->mDocumentTreeStructureModel;

        const QString prefix      = QStringLiteral("导入自检%1-").arg(QCoreApplication::applicationPid());
        const QString baseName    = prefix + QStringLiteral("底稿");
        QString seedDir = QDir(QDir::tempPath()).filePath(QStringLiteral("TianBoard-ImportSelfTest-%1/A").arg(QCoreApplication::applicationPid()));
        const QString rootDir = QFileInfo(seedDir).absolutePath();

        QDir(rootDir).removeRecursively();
        QDir().mkpath(rootDir);

        PersistenceManager::installDuplicateNameResolver(&stubResolve);

        // 画板正在展示哪一份文档由模型的“当前节点”决定；顶掉当前文档这一支必须让它指向底稿。
        DocumentProxy *seed = seedDocument(pm, seedDir, baseName, failures);
        if (!seed)
        {
            qWarning() << "import-selftest: 造不出底稿文档，自检无法进行";
            return failures.isEmpty() ? 1 : failures.size();
        }
        model->setCurrentDocument(seed);

        // ---- 1. 名字不撞：一次都不该问用户 ----
        {
            gScript.clear();
            const QString name = prefix + QStringLiteral("无冲突");
            const QString dir  = QDir(rootDir).filePath(QStringLiteral("E"));

            DocumentProxy *doc = pm->createDocument(QString(), name, false, dir, 1, true);
            if (!doc)
            {
                failures << QStringLiteral("无冲突的新文档不该被拦下来");
            }
            else if (!rootNames(model).contains(name))
            {
                failures << QStringLiteral("无冲突的新文档没有登记进树");
            }
            expectAsked(failures, QStringLiteral("不撞名"), 0, name);

            if (doc)
            {
                pm->deleteDocument(doc);
            }
        }

        // ---- 2. 取消：这一份整包退回磁盘外，底稿不受影响 ----
        {
            gScript.clear();
            gScript << cancelled();
            const QString dir  = QDir(rootDir).filePath(QStringLiteral("B"));

            DocumentProxy *doc = pm->createDocument(QString(), baseName, false, dir, 1, true);
            if (doc)
            {
                failures << QStringLiteral("用户取消后 createDocument 还返回了文档");
            }
            expectAsked(failures, QStringLiteral("取消"), 1, baseName);

            if (QFileInfo(dir).exists())
            {
                failures << QStringLiteral("取消导入后磁盘上残留了没人认领的文档包：%1").arg(dir);
            }
            if (!QFileInfo(seedDir).exists())
            {
                failures << QStringLiteral("取消导入把底稿也带没了：%1").arg(seedDir);
            }
            if (rootNames(model).count(baseName) != 1)
            {
                failures << QStringLiteral("取消导入后树里的底稿应有 1 份，实际 %1 份").arg(rootNames(model).count(baseName));
            }
        }

        // ---- 3. 改名新建：两份共存，旧的一份都不许少 ----
        {
            gScript.clear();
            gScript << proceed(prefix + QStringLiteral("改名后"));
            const QString dir         = QDir(rootDir).filePath(QStringLiteral("C"));
            const QString renamedName = prefix + QStringLiteral("改名后");

            if (!clonePackage(seedDir, dir))
            {
                failures << QStringLiteral("复制导入包失败：%1 -> %2").arg(seedDir, dir);
            }

            DocumentProxy *doc = pm->createDocumentFromDir(dir, QString(), QString(), false, false, true);
            if (!doc)
            {
                failures << QStringLiteral("改名新建被误判成取消");
            }
            else
            {
                if (doc->metaData(Settings::documentName).toString() != renamedName)
                {
                    failures << QStringLiteral("改名后登记的名字是「%1」").arg(doc->metaData(Settings::documentName).toString());
                }
                if (doc->persistencePath() != dir)
                {
                    failures << QStringLiteral("改名后的文档指向了另一个目录：%1").arg(doc->persistencePath());
                }
                if (!QFileInfo(dir).exists())
                {
                    failures << QStringLiteral("改名新建把刚导入的包删掉了：%1").arg(dir);
                }
                // 落盘的元数据必须跟着改名，否则下次启动扫描又变回旧名字、白改一次。
                DocumentProxy cold(dir);
                if (cold.metaData(Settings::documentName).toString() != renamedName)
                {
                    failures << QStringLiteral("改名没写回盘上的元数据，冷读回来是「%1」").arg(cold.metaData(Settings::documentName).toString());
                }
            }
            expectAsked(failures, QStringLiteral("改名新建"), 1, baseName);

            const QStringList names = rootNames(model);
            if (!names.contains(renamedName))
            {
                failures << QStringLiteral("改名后的文档没在树里");
            }
            if (names.count(baseName) != 1)
            {
                failures << QStringLiteral("改名新建把底稿挤掉了，树里的底稿有 %1 份").arg(names.count(baseName));
            }
        }

        // ---- 4. 同名替换：旧的删干净、新的顶上，且当前文档要换到人 ----
        {
            gScript.clear();
            gScript << proceed(baseName);
            const QString dir             = QDir(rootDir).filePath(QStringLiteral("D"));

            int            replacedCount  = 0;
            DocumentProxy *signalIncoming = nullptr;
            DocumentProxy *signalOutgoing = nullptr;
            QObject        watcher;
            QObject::connect(pm, &PersistenceManager::currentDocumentReplaced, &watcher,
                [&replacedCount, &signalIncoming, &signalOutgoing](DocumentProxy *incomingProxy, DocumentProxy *outgoingProxy)
                {
                    replacedCount++;
                    signalIncoming = incomingProxy;
                    signalOutgoing = outgoingProxy;
                });

            if (!clonePackage(seedDir, dir))
            {
                failures << QStringLiteral("复制导入包失败：%1 -> %2").arg(seedDir, dir);
            }

            DocumentProxy *doc = pm->createDocumentFromDir(dir, QString(), QString(), false, false, true);
            if (!doc)
            {
                failures << QStringLiteral("同名替换被误判成取消");
            }
            else if (doc->persistencePath() != dir)
            {
                failures << QStringLiteral("替换后的文档不是新导入的那一份：%1").arg(doc->persistencePath());
            }

            expectAsked(failures, QStringLiteral("同名替换"), 1, baseName);

            if (replacedCount != 1)
            {
                failures << QStringLiteral("顶掉当前文档时该回报一次事实，实际 %1 次").arg(replacedCount);
            }
            if (signalIncoming && doc && signalIncoming != doc)
            {
                failures << QStringLiteral("回报的接手文档不是新导入的那一份");
            }
            if (signalOutgoing != seed)
            {
                failures << QStringLiteral("回报被顶掉的旧文档不是底稿那一份");
            }
            if (model->proxyData(model->currentIndex()) != doc)
            {
                failures << QStringLiteral("替换后当前文档没跟着换成新导入的那一份");
            }

            const QStringList names = rootNames(model);
            if (names.count(baseName) != 1)
            {
                failures << QStringLiteral("替换后树里叫这个名字的应有 1 份，实际 %1 份").arg(names.count(baseName));
            }
            if (QFileInfo(seedDir).exists())
            {
                failures << QStringLiteral("被替换掉的旧文档包还留在磁盘上：%1").arg(seedDir);
            }
            if (!QFileInfo(dir).exists())
            {
                failures << QStringLiteral("替换把新导入的包也删掉了：%1").arg(dir);
            }

            // 底稿已经被替换掉了，后面的案子拿接手后的这一份当新的底稿继续复制。
            seedDir = dir;
        }

        // ---- 5. .tbx 那一腿：整批登记走同一个裁决，取消的一份不许漏在树外 ----
        {
            gScript.clear();
            gScript << cancelled();
            const QString dir = QDir(rootDir).filePath(QStringLiteral("F"));

            if (!clonePackage(seedDir, dir))
            {
                failures << QStringLiteral("复制导入包失败：%1 -> %2").arg(seedDir, dir);
            }

            QFileInfoList batch;
            batch << QFileInfo(dir);
            pm->createDocumentProxiesStructure(batch, true);

            expectAsked(failures, QStringLiteral("整批导入取消"), 1, baseName);
            if (QFileInfo(dir).exists())
            {
                failures << QStringLiteral("整批导入取消后残留了没登记的文档包：%1").arg(dir);
            }
        }
        {
            gScript.clear();
            gScript << proceed(prefix + QStringLiteral("整批改名"));
            const QString dir         = QDir(rootDir).filePath(QStringLiteral("G"));
            const QString batchedName = prefix + QStringLiteral("整批改名");

            if (!clonePackage(seedDir, dir))
            {
                failures << QStringLiteral("复制导入包失败：%1 -> %2").arg(seedDir, dir);
            }

            QFileInfoList batch;
            batch << QFileInfo(dir);
            pm->createDocumentProxiesStructure(batch, true);

            expectAsked(failures, QStringLiteral("整批导入改名"), 1, baseName);
            if (!rootNames(model).contains(batchedName))
            {
                failures << QStringLiteral("整批导入里改名后的文档没登记进树");
            }
        }

        // ---- 6. 没装裁决钩子：撞名一律不登记，且不许崩 ----
        {
            gScript.clear();
            const QString dir = QDir(rootDir).filePath(QStringLiteral("H"));

            if (!clonePackage(seedDir, dir))
            {
                failures << QStringLiteral("复制导入包失败：%1 -> %2").arg(seedDir, dir);
            }

            PersistenceManager::installDuplicateNameResolver(nullptr);

            QFileInfoList batch;
            batch << QFileInfo(dir);
            pm->createDocumentProxiesStructure(batch, true);

            if (!gAsked.isEmpty())
            {
                failures << QStringLiteral("没装钩子时不该有任何征询记录");
            }
            if (!rootNames(model).contains(baseName))
            {
                failures << QStringLiteral("没装钩子时把同名的旧文档挤掉了");
            }
            if (QFileInfo(dir).exists())
            {
                failures << QStringLiteral("没装钩子时不登记的文档包该一起退回磁盘外：%1").arg(dir);
            }
        }

        // ---- 7. 删掉当前文档所在的那一行：模型不许抱着一个已经释放的节点 ----
        {
            const QString name     = prefix + QStringLiteral("当前文档");
            const QString dir      = QDir(rootDir).filePath(QStringLiteral("I"));

            DocumentProxy *current = seedDocument(pm, dir, name, failures);
            if (current)
            {
                model->setCurrentDocument(current);

                const QModelIndex index = model->indexForProxy(current);
                model->removeRow(index.row(), index.parent());

                // 节点随这一行一起被 delete；当前节点要是还指着它，之后每一次比对都拿的是
                // 一个已释放的地址——地址被新节点复用时会把"当前文档"标到别人身上。
                if (model->currentNode() != nullptr)
                {
                    failures << QStringLiteral("删掉当前文档那一行之后，当前节点还指着已删除的节点");
                }
                if (model->currentIndex().isValid())
                {
                    failures << QStringLiteral("当前节点没清掉时 currentIndex() 还返回了一个有效索引");
                }
            }
        }

        // ---- 8. 带新分组的导入包：分组要跟着补建出来，文档不许掉进“我的文档”根 ----
        {
            gScript.clear();
            const QString group      = prefix + QStringLiteral("新分组");
            const QString name       = prefix + QStringLiteral("分组里的文档");
            const QString seedPath   = QDir(rootDir).filePath(QStringLiteral("J"));
            const QString importPath = QDir(rootDir).filePath(QStringLiteral("K"));

            DocumentProxy *grouped   = pm->createDocument(group, name, false, seedPath, 1, false);
            if (!grouped)
            {
                failures << QStringLiteral("造带分组的底稿失败：%1").arg(seedPath);
            }
            else
            {
                pm->persistDocumentMetadata(grouped);
                if (!clonePackage(seedPath, importPath))
                {
                    failures << QStringLiteral("复制导入包失败：%1 -> %2").arg(seedPath, importPath);
                }
                // 把分组连同里面那份文档从树里撤掉，模拟“装这个包的时候机器上还没有这个分组”。
                const QModelIndex groupIndex = model->goTo(group);
                if (groupIndex.isValid())
                {
                    model->removeRow(groupIndex.row(), groupIndex.parent());
                }
                if (rootNames(model).contains(group))
                {
                    failures << QStringLiteral("撤分组没成功，后面的案子量不出东西");
                }

                QFileInfoList batch;
                batch << QFileInfo(importPath);
                pm->createDocumentProxiesStructure(batch, true);

                const QModelIndex revived = model->goTo(group);
                if (!revived.isValid() || !model->nodeNameList(revived).contains(name))
                {
                    failures << QStringLiteral("导入没把分组「%1」连同里面那份文档补回来").arg(group);
                }
                if (rootNames(model).contains(name))
                {
                    failures << QStringLiteral("带分组的文档掉进了“我的文档”根，而不是它自己的分组");
                }
            }
        }
        // ---- 9. 导入一份真 PDF：页数、每页一个 PDF 图元、那份 PDF 有没有被搬进文档目录 ----
        // 导入这条路过编译与冒烟从没验过产物：页数为 0、图元没建出来、或者页文件里的引用
        // 指向一个没被拷贝进文档的 PDF，界面都只是"导入成功"然后页上一片空白。
        {
            gScript.clear();

            const QString pdfPath = QDir(rootDir).filePath(prefix + QStringLiteral("导入-PDF.pdf"));
            if (!writeTwoPagePdf(pdfPath))
            {
                failures << QStringLiteral("现场造那份两页 PDF 失败: %1").arg(pdfPath);
            }
            else
            {
                DocumentProxy *imported = DocumentManager::documentManager()->importFile(QFile(pdfPath), QString());
                if (!imported)
                {
                    failures << QStringLiteral("导入一份两页 PDF 没产出文档（DocumentManager::importFile 返回空）");
                }
                else
                {
                    if (imported->pageCount() != 2)
                    {
                        failures << QStringLiteral("PDF 导入后应有 2 页，实际 %1 页").arg(imported->pageCount());
                    }
                    for (int i = 0; i < 2 && i < imported->pageCount(); i++)
                    {
                        const int items = countItems<GraphicsPDFItem>(pm->loadDocumentScene(imported, i));
                        if (items != 1)
                        {
                            failures << QStringLiteral("PDF 导入后第 %1 页里的 PDF 图元不是 1 个（实际 %2 个）").arg(i + 1).arg(items);
                        }
                    }

                    const QStringList objects =
                        QDir(imported->persistencePath() + "/" + PackageLayout::objectDirectory).entryList({QStringLiteral("*.pdf")}, QDir::Files);
                    if (objects.isEmpty())
                    {
                        failures << QStringLiteral("导入的 PDF 没被搬进文档目录的 %1/ 下，页文件里的引用是空头").arg(PackageLayout::objectDirectory);
                    }
                    // 注意这条断言钉的是"文档自包含"，不是"导入那一步做了拷贝"：实测把
                    // DocumentManager 里那次 folderToCopy 短路掉它照样绿——每存一页时
                    // pdfItemToLinkedPDF 都会把缺少的 PDF 重新落盘（位图与 SVG 现在同理）。
                    if (!rootNames(model).contains(imported->name()))
                    {
                        failures << QStringLiteral("PDF 导入出来的文档没登记进树: %1").arg(imported->name());
                    }

                    // 冷读第 0 页：绕开场景缓存，从盘上的页文件重建，PDF 图元必须还在。
                    DocumentProxy cold(imported->persistencePath());
                    if (countItems<GraphicsPDFItem>(SvgSubsetAdaptor::loadScene(&cold, 0)) != 1)
                    {
                        failures << QStringLiteral("PDF 文档存盘再冷读回来，第 0 页的 PDF 图元没了");
                    }
                }
            }
        }

        // ---- 10. 导入一张真 PNG：一页、一张位图、像素一致、资源落进 images/ ----
        {
            gScript.clear();

            const QString pngPath = QDir(rootDir).filePath(prefix + QStringLiteral("导入-图片.png"));

            QPixmap source(48, 36);
            source.fill(Qt::transparent);
            {
                QPainter painter(&source);
                painter.fillRect(0, 0, 48, 36, QColor(30, 200, 90));
                painter.setPen(QPen(QColor(200, 30, 30), 4));
                painter.drawLine(2, 2, 45, 33);
            }

            if (!source.save(pngPath, "PNG"))
            {
                failures << QStringLiteral("现场造那张 PNG 失败: %1").arg(pngPath);
            }
            else
            {
                DocumentProxy *imported = DocumentManager::documentManager()->importFile(QFile(pngPath), QString());
                if (!imported)
                {
                    failures << QStringLiteral("导入一张 PNG 没产出文档");
                }
                else
                {
                    if (imported->pageCount() != 1)
                    {
                        failures << QStringLiteral("图片导入后应有 1 页，实际 %1 页").arg(imported->pageCount());
                    }

                    const QString want = pixmapDigest(source);

                    QString        got;
                    GraphicsScene *scene = pm->loadDocumentScene(imported, 0);
                    foreach (QGraphicsItem *item, scene ? scene->items() : QList<QGraphicsItem *>())
                    {
                        if (GraphicsPixmapItem *pixmapItem = dynamic_cast<GraphicsPixmapItem *>(item))
                        {
                            got = pixmapDigest(pixmapItem->pixmap());
                        }
                    }
                    if (got.isEmpty())
                    {
                        failures << QStringLiteral("图片导入后第 0 页里没有位图图元");
                    }
                    else if (got != want)
                    {
                        failures << QStringLiteral("图片导入回来的像素跟源文件不一致（%1 对 %2）").arg(got, want);
                    }

                    const QStringList images =
                        QDir(imported->persistencePath() + "/" + PackageLayout::imageDirectory).entryList({QStringLiteral("*.png")}, QDir::Files);
                    if (images.isEmpty())
                    {
                        failures << QStringLiteral("导入的位图没落进文档的 %1/ 目录").arg(PackageLayout::imageDirectory);
                    }

                    DocumentProxy  cold(imported->persistencePath());
                    QString        coldDigest;
                    GraphicsScene *coldScene = SvgSubsetAdaptor::loadScene(&cold, 0);
                    foreach (QGraphicsItem *item, coldScene ? coldScene->items() : QList<QGraphicsItem *>())
                    {
                        if (GraphicsPixmapItem *pixmapItem = dynamic_cast<GraphicsPixmapItem *>(item))
                        {
                            coldDigest = pixmapDigest(pixmapItem->pixmap());
                        }
                    }
                    if (coldDigest != want)
                    {
                        failures << QStringLiteral("图片文档存盘再冷读回来，位图不是那张源图（%1 对 %2）").arg(coldDigest, want);
                    }
                }
            }
        }

        // ---- 11. 往**当前打开的那份文档**追加一张图片（画板里拖文件进来走的就是这条路）----
        {
            gScript.clear();

            BoardController *board = BoardController::instance();
            if (!board)
            {
                failures << QStringLiteral("画板总控器还没装配，量不了「追加进当前文档」这条路");
            }
            else
            {
                const QString targetName = prefix + QStringLiteral("追加目标");
                // withEmptyPage=true：画板要能真的装下这份文档（pageCount=1 但没有页文件时
                // loadDocumentScene 返回空，setActiveDocumentScene 就不切文档），本条判据量的
                // 就是"追加进当前打开的那份文档"，文档必须先是画板当前那一份。
                DocumentProxy *target    = pm->createDocument(QString(), targetName, true, QDir(rootDir).filePath(QStringLiteral("L")), 0, false);
                const QString  pngPath   = QDir(rootDir).filePath(prefix + QStringLiteral("追加-图片.png"));

                QPixmap source(48, 36);
                source.fill(Qt::transparent);
                {
                    QPainter painter(&source);
                    painter.fillRect(0, 0, 48, 36, QColor(240, 120, 20));
                    painter.setPen(QPen(QColor(20, 40, 200), 4));
                    painter.drawLine(2, 2, 45, 33);
                }

                if (!target)
                {
                    failures << QStringLiteral("造不出「追加进当前文档」那条路的 target 文档");
                }
                else if (!source.save(pngPath, "PNG"))
                {
                    failures << QStringLiteral("现场造那张要追加的 PNG 失败: %1").arg(pngPath);
                }
                else
                {
                    board->setActiveDocumentScene(target, 0);

                    if (board->selectedDocument() != target)
                    {
                        failures << QStringLiteral("画板没能切到「追加目标」这份文档，后面量的就不是「追加进当前文档」这条路");
                    }

                    const int pagesBefore = target->pageCount();
                    const int imported    = DocumentManager::documentManager()->addFilesToDocument(target, {pngPath});

                    if (imported != 1)
                    {
                        failures << QStringLiteral("往当前文档追加一张图片应当报告导入 1 个，实际 %1").arg(imported);
                    }
                    if (target->pageCount() != pagesBefore + 1)
                    {
                        failures << QStringLiteral("追加一张图片之后页数应当是 %1，实际 %2").arg(pagesBefore + 1).arg(target->pageCount());
                    }
                    else
                    {
                        const QString  want  = pixmapDigest(source);
                        GraphicsScene *scene = pm->loadDocumentScene(target, pagesBefore);
                        QString        got;

                        foreach (QGraphicsItem *item, scene ? scene->items() : QList<QGraphicsItem *>())
                        {
                            if (GraphicsPixmapItem *pixmapItem = dynamic_cast<GraphicsPixmapItem *>(item))
                            {
                                got = pixmapDigest(pixmapItem->pixmap());
                            }
                        }

                        if (got.isEmpty())
                        {
                            failures << QStringLiteral("追加进来的那一页上没有位图图元");
                        }
                        else if (got != want)
                        {
                            failures << QStringLiteral("追加的那一页像素不是源图（%1 对 %2）").arg(got, want);
                        }

                        const QStringList images =
                            QDir(target->persistencePath() + "/" + PackageLayout::imageDirectory).entryList({QStringLiteral("*.png")}, QDir::Files);
                        if (images.isEmpty())
                        {
                            failures << QStringLiteral("追加的位图没落进这份文档的 %1/ 目录").arg(PackageLayout::imageDirectory);
                        }

                        // 画板那一侧的缩略图列表必须跟着长一条：这条判据就是"追加完请画板补一张图"
                        // 那句的对照物——core 现在只报"这份文档多了一页"，由装配点装上的实现去判断
                        // 这份文档是不是画板当前那一份。
                        const int thumbs = thumbCount(board, target->pageCount() + 8);
                        if (thumbs != target->pageCount())
                        {
                            failures << QStringLiteral("追加一页之后画板的缩略图列表有 %1 条，页数是 %2——两边必须一样多")
                                            .arg(thumbs)
                                            .arg(target->pageCount());
                        }

                        // 反向：往**另一份**文档追加一页，画板那份列表不许跟着长
                        // （旧写法是无条件插进画板的容器，被挤位的正是当前这份文档的图）。
                        DocumentProxy *other = pm->createDocument(
                            QString(), prefix + QStringLiteral("追加旁支"), true, QDir(rootDir).filePath(QStringLiteral("M")), 0, false);
                        if (!other)
                        {
                            failures << QStringLiteral("造不出「往别的文档追加」那一案要用的文档");
                        }
                        else
                        {
                            board->setActiveDocumentScene(target, 0);
                            if (board->selectedDocument() != target)
                            {
                                failures << QStringLiteral("画板没能停在「追加目标」这份文档，「往别的文档追加」那一案量不到东西");
                            }
                            else if (DocumentManager::documentManager()->addFilesToDocument(other, {pngPath}) != 1)
                            {
                                failures << QStringLiteral("往另一份文档追加图片没有报告导入成功");
                            }
                            else if (other->pageCount() != 2)
                            {
                                failures << QStringLiteral("另一份文档追加一页之后应当有 2 页，实际 %1").arg(other->pageCount());
                            }
                            else
                            {
                                const int thumbsNow = thumbCount(board, target->pageCount() + 8);
                                if (thumbsNow != target->pageCount())
                                {
                                    failures << QStringLiteral("往别的文档追加一页，画板那份缩略图列表却被改成了 %1 条（当前文档只有 %2 页）")
                                                    .arg(thumbsNow)
                                                    .arg(target->pageCount());
                                }
                            }
                        }
                    }
                }
            }
        }

        purgeTestDocuments(pm, prefix);
        QCoreApplication::processEvents();

        for (const QString &failure : failures)
        {
            qWarning() << "import-selftest:" << failure;
        }

        if (failures.isEmpty())
        {
            QDir(rootDir).removeRecursively();
            qInfo() << "import-selftest: OK 重名裁决的取消/改名/替换三条路径与整批导入都按预期落地，PDF 与图片导入的产物也对得上，"
                       "往当前文档追加一页时长的是画板那份缩略图、往别的文档追加时它不动";
        }
        else
        {
            qWarning() << "import-selftest: 现场保留在" << rootDir;
        }

        return failures.size();
    }
} // namespace ImportSelfTest
