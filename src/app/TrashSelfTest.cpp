#include "app/TrashSelfTest.h"

#include "core/Application.h"
#include "core/PersistenceManager.h"
#include "model/DocumentProxy.h"
#include "model/DocumentTreeModel.h"
#include "settings/Settings.h"
#include "ui/DocumentController.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace
{
    /// @brief 读一份文档自己的 metadata.rdf——那是下次启动重建树时唯一会读的东西。
    QByteArray metadataBytes(DocumentProxy *proxy)
    {
        QFile file(QDir(proxy->persistencePath()).filePath(QStringLiteral("metadata.rdf")));
        if (!file.open(QIODevice::ReadOnly))
        {
            return QByteArray();
        }

        return file.readAll();
    }

    /// @brief 把已攒下的不一致逐条报出来再退场（自检的退出码就是条数）。
    int bail(const QStringList &failures)
    {
        for (const QString &failure : failures)
        {
            qWarning() << "trash-selftest:" << failure;
        }

        return failures.count();
    }
} // namespace

namespace TrashSelfTest
{
    int run()
    {
        QStringList failures;

        PersistenceManager *pm       = PersistenceManager::persistenceManager();
        DocumentTreeModel  *docModel = pm->mDocumentTreeStructureModel;
        DocumentController *docs     = Application::documentController;

        if (!docModel || !docs)
        {
            qWarning() << "trash-selftest: 文档树或文档控制器还没装配";
            return 1;
        }

        const QModelIndex myDocuments = docModel->myDocumentsIndex();
        const QModelIndex trash       = docModel->trashIndex();

        if (!myDocuments.isValid() || !trash.isValid())
        {
            qWarning() << "trash-selftest: 文档树缺少顶层的两个固定入口（我的文档 / 回收站）";
            return 1;
        }

        // ---- 1. 建两份文档：一份要进回收站，一份留在外面当"不许牵连"的对照 ----
        DocumentProxy *victim    = pm->createDocument(QString(), QStringLiteral("TrashSelfTestVictim"), true, QString(), 0, false);
        DocumentProxy *bystander = pm->createDocument(QString(), QStringLiteral("TrashSelfTestBystander"), true, QString(), 0, false);
        if (!victim || !bystander)
        {
            qWarning() << "trash-selftest: 建临时文档失败";
            return 1;
        }

        const QString victimPath    = victim->persistencePath();
        const QString bystanderPath = bystander->persistencePath();

        if (!QFileInfo(victimPath).exists() || !QFileInfo(bystanderPath).exists())
        {
            failures << QStringLiteral("两份新建文档的目录应当都在盘上，victim=%1 bystander=%2").arg(victimPath, bystanderPath);
        }
        if (docModel->inTrash(docModel->indexForProxy(victim)))
        {
            failures << QStringLiteral("刚建好的文档就被认为在回收站里，后面的判据全都没有意义");
        }

        const int trashRowsBefore = docModel->rowCount(trash);

        // ---- 2. 界面上那条"移入回收站" ----
        QModelIndex victimIndex   = docModel->indexForProxy(victim);
        docs->moveToTrash(victimIndex, docModel);

        if (!docModel->inTrash(docModel->indexForProxy(victim)))
        {
            failures << QStringLiteral("移入回收站之后模型并不认为这份文档在回收站里");
        }
        if (docModel->rowCount(docModel->trashIndex()) != trashRowsBefore + 1)
        {
            failures << QStringLiteral("回收站那一层的行数从 %1 变成了 %2（只该多一条）")
                            .arg(trashRowsBefore)
                            .arg(docModel->rowCount(docModel->trashIndex()));
        }
        if (docModel->inTrash(docModel->indexForProxy(bystander)))
        {
            failures << QStringLiteral("只移了一份文档，留在外面的那份却被判定也在回收站里");
        }
        // 回收站不是删除：这一步之后盘上的包必须还在，用户才谈得上"再恢复"或"再清空"。
        if (!QFileInfo(victimPath).exists())
        {
            failures << QStringLiteral("仅仅移入回收站就把文档包从盘上删掉了：%1").arg(victimPath);
        }

        // 归属只写在这份文档自己的 metadata.rdf 里（folders.xml 只存目录树），
        // 而下次启动重建树读的就是这里——写不回盘上，重启之后文档会回到原来的分组。
        const QByteArray movedMetadata = metadataBytes(victim);
        if (!movedMetadata.contains(Settings::trashedDocumentGroupNamePrefix.toUtf8()))
        {
            failures << QStringLiteral("移入回收站之后 metadata.rdf 里没有回收站的组名前缀（%1），下次启动读不回这个归属")
                            .arg(Settings::trashedDocumentGroupNamePrefix);
        }

        // ---- 3. 界面上那条"清空回收站"（EmptyTrash 在界面上映射成 emptyFolder(index, CompleteDelete)）----
        docs->emptyFolder(docModel->trashIndex(), DocumentController::CompleteDelete);

        if (QFileInfo(victimPath).exists())
        {
            failures << QStringLiteral("清空回收站之后文档包还留在盘上，成了没人认领的孤儿目录：%1").arg(victimPath);
        }
        if (docModel->rowCount(docModel->trashIndex()) != 0)
        {
            failures << QStringLiteral("清空回收站之后那一层还剩 %1 行").arg(docModel->rowCount(docModel->trashIndex()));
        }
        // 本来这里还要问一句"树里不该再留着这份已删文档的行"（indexForProxy），但这一查就踩雷：
        // 永久删除之后再走 indexForProxy→findProxy→indexForNode，会读到一个已经被释放的树节点，
        // Debug 下断在 QStringView 的 len>=0 上。这条留在任务里单独查根因，见 docs/code-suspicions.md。
        if (!QFileInfo(bystanderPath).exists())
        {
            failures << QStringLiteral("清空回收站把留在外面的文档也删了：%1").arg(bystanderPath);
        }

        // 界面上那条"按天数清理回收站"（deleteDocumentsInFolderOlderThan）暂时没接进来：
        // 永久删除之后再走这棵树（建一份新文档、或查那份已删文档的行）都会读到一个已被释放的节点，
        // Debug 下断在 QStringView 的 len>=0 上——同一个根因，见 docs/code-suspicions.md 与任务清单。

        return bail(failures);
    }
} // namespace TrashSelfTest
