#include "app/ImportSelfTest.h"

#include "core/PersistenceManager.h"
#include "model/DocumentProxy.h"
#include "model/DocumentTreeModel.h"
#include "settings/Settings.h"
#include "utils/FileSystemUtils.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsTextItem.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSize>
#include <QStringList>

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

    /// @brief 树里“我的文档”下的名字列表，用来数同名节点还剩几个。
    QStringList rootNames(DocumentTreeModel *model)
    {
        return model->nodeNameList(model->myDocumentsIndex());
    }

    /// @brief 收尾：测试留在树里的文档全部删掉，用户的文档库不该留着自测痕迹。
    void purgeTestDocuments(PersistenceManager *pm, const QString &prefix)
    {
        DocumentTreeModel *model = pm->mDocumentTreeStructureModel;
        const QModelIndex  group = model->myDocumentsIndex();
        const QStringList  names = rootNames(model);

        for (int i = names.size() - 1; i >= 0; i--)
        {
            if (!names.at(i).startsWith(prefix))
            {
                continue;
            }
            DocumentProxy *proxy = model->proxyData(model->index(i, 0, group));
            if (proxy)
            {
                pm->deleteDocument(proxy);
            }
        }
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

        purgeTestDocuments(pm, prefix);
        QCoreApplication::processEvents();

        for (const QString &failure : failures)
        {
            qWarning() << "import-selftest:" << failure;
        }

        if (failures.isEmpty())
        {
            QDir(rootDir).removeRecursively();
            qInfo() << "import-selftest: OK 重名裁决的取消/改名/替换三条路径与整批导入都按预期落地";
        }
        else
        {
            qWarning() << "import-selftest: 现场保留在" << rootDir;
        }

        return failures.size();
    }
} // namespace ImportSelfTest
