/**
 * @file IDocumentStore.h
 * @brief 文档仓库契约：格式层（content 的导入导出器）需要的那几件事，由 core 的服务实现。
 */

#pragma once

#include <QList>
#include <QString>
#include <QUuid>

#include "model/DocumentTreeModel.h"

class DocumentProxy;
class GraphicsScene;

/**
 * @brief 把一份 .tbz 文档"落到仓库里"所需的服务：路径分配、建文档、搬对象、取页面场景。
 *
 * 详细说明：接口住在 model，实现是 `core/PersistenceManager`（它继承本接口并在构造时把自己
 * 装上）。以前 content 的导入导出器直接 include `core/PersistenceManager.h`，于是文档格式层
 * 反向认识应用服务层，构成 `content` ↔ `core` 这组双向 include；而现在 content 本来就认识
 * model（DocumentProxy 就在 model），走这份契约只留向下的边。
 *
 * 这里只列**格式层与模型真正用到的**那些事，不是 PersistenceManager 的全集：它的界面交互、目录树
 * 重建、撤销栈与缓存淘汰一概不在契约内，那些是服务自己的事。
 *
 * 生命周期：实现者是进程级单例 `PersistenceManager`，构造时装上、析构时摘下（摘之前会核对
 * 装的确实是它自己，避免留下悬垂指针）。`documentStore()` 拿不到实现时直接 `qFatal`——
 * 导入导出只在应用装配完成之后才被调用，拿不到就是装配漏了，空指针崩溃可诊断得多。
 * 线程约束：全部在 GUI 线程调用（导入导出与文档库刷新都在主线程）。
 */
class IDocumentStore
{
public:
    virtual ~IDocumentStore()                                                                              = default;

    /// @brief 在文档仓库下生成一个不与既有文档冲突的目录路径。
    virtual QString generateUniqueDocumentPath()                                                           = 0;

    /// @brief 在指定基础目录下生成一个不冲突的文档目录路径。
    virtual QString generateUniqueDocumentPath(const QString &baseFolder)                                  = 0;

    /**
     * @brief 把一个已经在磁盘上的文档目录登记进仓库。
     * @return 文档代理；目录无效时为空。
     */
    virtual DocumentProxy *createDocumentFromDir(const QString &documentDirectory, const QString &groupName = "", const QString &name = "",
        bool withEmptyPage = false, bool addTitlePage = false, bool promptDialogIfExists = false)          = 0;

    /// @brief 把目录内容（图片等）导入为目标文档的页面；至少导入一个文件才算成功。
    virtual bool addDirectoryContentToDocument(const QString &documentRootFolder, DocumentProxy *document) = 0;

    /**
     * @brief 把一个外部文件放进文档包的指定子目录。
     * @param subdir 目标子目录名，取值来自 `PackageLayout`。
     * @param destinationPath 输出参数：文件在包内落到的绝对路径。
     * @param data 给定则写入这份字节，而不是去读 path 指向的文件。
     */
    virtual bool addFileToDocument(
        DocumentProxy *document, QString path, const QString &subdir, QUuid objectUuid, QString &destinationPath, QByteArray *data = nullptr) = 0;

    /// @brief 取一页场景，必要时从磁盘加载；加载失败时为空。
    virtual GraphicsScene *loadDocumentScene(DocumentProxy *document, int sceneIndex)                                                         = 0;

    /// @brief 只取已缓存的场景，未缓存时为空，不触发加载。
    virtual GraphicsScene *getDocumentScene(DocumentProxy *document, int sceneIndex)                                                          = 0;

    /// @brief 在指定下标新建一页场景；所有权归场景缓存。
    virtual GraphicsScene *createDocumentSceneAt(DocumentProxy *document, int index, bool useUndoRedoStack = true)                            = 0;

    /// @brief 复制文档里某一页，插在其后。
    virtual void duplicateDocumentScene(DocumentProxy *document, int index)                                                                   = 0;

    /// @brief 把一页挪到另一个下标。
    virtual void moveSceneToIndex(DocumentProxy *document, int source, int target)                                                            = 0;

    /// @brief 删掉若干页。
    virtual void deleteDocumentScenes(DocumentProxy *document, const QList<int> &indexes)                                                     = 0;

    /**
     * @brief 把一页原样复制到另一份文档的指定下标（文档库里的拖放）。
     * @note 完成后由实现方发 documentSceneCreated，让认识画板的那一层决定要不要刷新视图。
     */
    virtual void copyDocumentScene(DocumentProxy *from, int fromIndex, DocumentProxy *to, int toIndex)                                        = 0;

    /// @brief 整份复制一个文档（含目录与元数据）；失败时返回空。
    virtual DocumentProxy *duplicateDocument(DocumentProxy *document)                                                                         = 0;

    /// @brief 文档代理被替换后，把仓库里以旧代理为键的东西迁到新代理上。
    virtual void reassignDocProxy(DocumentProxy *newDocument, DocumentProxy *oldDocument)                                                     = 0;

    /// @brief 文档目录树模型；导出文档集时需要按树里的位置拼文件名。
    virtual DocumentTreeModel *documentTreeModel()                                                                                            = 0;
};

/// @brief 装上仓库实现；由 PersistenceManager 在自己的构造/析构里完成。
void installDocumentStore(IDocumentStore *store);

/// @brief 取实现；没装上就 qFatal（导入导出只可能发生在装配完成之后）。
IDocumentStore *documentStore();
