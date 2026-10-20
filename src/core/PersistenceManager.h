/**
 * @file PersistenceManager.h
 * @brief 声明文档包持久化单例：文档/页面的创建、读取、复制、删除与场景缓存。
 */

#pragma once

#include <QtCore>

#include "core/SceneCache.h"

class QDomNode;
class QDomElement;
class Document;
class DocumentProxy;
class GraphicsScene;
class DocumentTreeNode;
class DocumentTreeModel;

/**
 * @brief 文档仓库的唯一入口：把内存中的文档代理与磁盘上的文档包对应起来。
 *
 * 详细说明：进程级单例，父对象为 Application::staticMemoryCleaner，首次调用
 * persistenceManager() 时创建；持有文档树模型与场景缓存（LRU 式），文档代理归
 * 树节点所有。所有路径都在构造时从设置读取，运行期通过关闭/重开来重建。
 * 线程约束：全部操作都访问文件系统与 GUI 模型，只能在 GUI 线程调用；
 * documentFilePath() 例外地只解析路径，供工作线程准备复制目标。
 */
class PersistenceManager : public QObject {
    Q_OBJECT

private:
    PersistenceManager(QObject *pParent = 0);
    static PersistenceManager *sSingleton;

public:
    virtual ~PersistenceManager();

    /// 文档包内存放图片的子目录名。
    static const QString imageDirectory;
    /// 文档包内存放普通对象的子目录名。
    static const QString objectDirectory;
    /// 文档包内存放视频的子目录名。
    static const QString videoDirectory;
    /// 文档包内存放音频的子目录名。
    static const QString audioDirectory;
    /// 文档包内存放控件的子目录名。
    static const QString widgetDirectory;
    /// 文档包内存放其它附件的子目录名。
    static const QString fileDirectory;

    /// 文档树中“我的文档”根分组的名称。
    static const QString myDocumentsName;
    /// 文档树中“模型”根分组的名称。
    static const QString modelsName;
    /// 目录结构 XML 的文件名。
    static const QString fFolders;
    /// 目录结构 XML 中分组元素的标签名。
    static const QString tFolder;
    /// 目录结构 XML 中名称属性的属性名。
    static const QString aName;

    /// 返回进程内唯一实例，首次调用时创建并扫描文档仓库。
    static PersistenceManager *persistenceManager();
    /// 销毁单例并置空指针。
    static void destroy();

    /**
     * @brief 在文档仓库中新建一个文档包。
     * @param pGroupName 目标分组。
     * @param pName 文档名；为空时由实现生成。
     * @param withEmptyPage 是否附带一个空白页。
     * @param directory 指定目录；为空时在仓库下生成唯一路径。
     * @param pageCount 预置页数。
     * @param promptDialogIfExists 重名时是否询问替换。
     * @return 新建的文档代理。
     */
    virtual DocumentProxy *createDocument(const QString &pGroupName = "", const QString &pName = "",
                                          bool withEmptyPage = true, QString directory = QString(),
                                          int pageCount = 0, bool promptDialogIfExists = false);

    /**
     * @brief createDocument 的别名，供需要明确语义的调用点使用。
     * @param pGroupName 目标分组。
     * @param pName 文档名。
     * @param withEmptyPage 是否附带空白页。
     * @param directory 指定目录。
     * @param pageCount 预置页数。
     * @param promptDialogIfExists 重名时是否询问替换。
     * @return 新建的文档代理。
     */
    virtual DocumentProxy *createNewDocument(const QString &pGroupName = "", const QString &pName = "",
                                             bool withEmptyPage = true, QString directory = QString(),
                                             int pageCount = 0, bool promptDialogIfExists = false);

    /**
     * @brief 把已存在的目录注册为文档（用于打开外部文档包）。
     * @param pDocumentDirectory 文档目录。
     * @param pGroupName 目标分组。
     * @param pName 文档名。
     * @param withEmptyPage 是否在没有任何页面时补一个空白页。
     * @param addTitlePage 是否额外生成标题页。
     * @param promptDialogIfExists 重名时是否询问替换。
     * @return 文档代理；目录无效时为空。
     */
    virtual DocumentProxy *createDocumentFromDir(const QString &pDocumentDirectory,
                                                 const QString &pGroupName = "", const QString &pName = "",
                                                 bool withEmptyPage = false, bool addTitlePage = false,
                                                 bool promptDialogIfExists = false);

    /// 把文档元数据写回磁盘。
    virtual DocumentProxy *persistDocumentMetadata(DocumentProxy *pDocumentProxy);

    /// 复制文档包并登记副本。
    virtual DocumentProxy *duplicateDocument(DocumentProxy *pDocumentProxy);

    /// 删除文档包及其在树中的节点。
    virtual void deleteDocument(DocumentProxy *pDocumentProxy);

    /**
     * @brief 删除文档中的若干页面。
     * @param pDocumentProxy 目标文档。
     * @param indexes 待删除页面下标。
     */
    virtual void deleteDocumentScenes(DocumentProxy *pDocumentProxy, const QList<int> &indexes);

    /**
     * @brief 在文档内复制一页，副本插入到原页之后。
     * @param pDocumentProxy 目标文档。
     * @param index 源页下标。
     */
    virtual void duplicateDocumentScene(DocumentProxy *pDocumentProxy, int index);

    /**
     * @brief 把一页从一个文档复制到另一个文档。
     * @param from 源文档。
     * @param fromIndex 源页下标。
     * @param to 目标文档。
     * @param toIndex 目标插入位置。
     */
    virtual void copyDocumentScene(DocumentProxy *from, int fromIndex, DocumentProxy *to, int toIndex);

    /**
     * @brief 把内存中的页面写入磁盘。
     * @param pDocumentProxy 页面所属文档。
     * @param pScene 页面场景。
     * @param pSceneIndex 页面下标。
     */
    virtual void persistDocumentScene(DocumentProxy *pDocumentProxy, GraphicsScene *pScene,
                                      const int pSceneIndex);

    /**
     * @brief 在指定下标创建一个新的空白页面。
     * @param pDocumentProxy 目标文档。
     * @param index 插入位置。
     * @param useUndoRedoStack 是否把操作记入撤销栈。
     * @return 新页面场景，所有权归场景缓存。
     */
    virtual GraphicsScene *createDocumentSceneAt(DocumentProxy *pDocumentProxy, int index,
                                                 bool useUndoRedoStack = true);

    /**
     * @brief 把一个已有场景插入文档的指定下标。
     * @param pDocumentProxy 目标文档。
     * @param scene 待插入场景。
     * @param index 插入位置。
     * @param persist 是否立即写盘。
     */
    virtual void insertDocumentSceneAt(DocumentProxy *pDocumentProxy, GraphicsScene *scene, int index,
                                       bool persist = true);

    /**
     * @brief 移动页面。
     * @param pDocumentProxy 目标文档。
     * @param source 源下标。
     * @param target 目标下标。
     */
    virtual void moveSceneToIndex(DocumentProxy *pDocumentProxy, int source, int target);

    /**
     * @brief 从缓存或磁盘加载指定页面。
     * @param pDocumentProxy 页面所属文档。
     * @param sceneIndex 页面下标。
     * @return 页面场景；加载失败时为空。
     */
    virtual GraphicsScene *loadDocumentScene(DocumentProxy *pDocumentProxy, int sceneIndex);
    /// 返回已缓存的页面场景；未缓存时为空，不会触发加载。
    GraphicsScene *getDocumentScene(DocumentProxy *pDocumentProxy, int sceneIndex) {
        return mSceneCache.value(pDocumentProxy, sceneIndex);
    }
    /// 文档代理被替换后，把缓存中的键迁移到新代理。
    void reassignDocProxy(DocumentProxy *newDocument, DocumentProxy *oldDocument);

    //        QList<QPointer<DocumentProxy> > documentProxies;
    DocumentTreeNode *mDocumentTreeStructure;
    DocumentTreeModel *mDocumentTreeStructureModel;

    /// 扫描仓库中所有 SVG 页面文件。
    virtual QStringList allShapes();
    /// 扫描仓库中所有图形对象文件。
    virtual QStringList allGips();
    /// 列出目录中的图片文件。
    virtual QStringList allImages(const QDir &dir);
    /// 列出目录中的视频文件。
    virtual QStringList allVideos(const QDir &dir);
    /// 列出目录中的控件文件。
    virtual QStringList allWidgets(const QDir &dir);

    /// 在文档仓库下生成一个不冲突的文档目录路径。
    QString generateUniqueDocumentPath();
    /**
     * @brief 在指定基础目录下生成一个不冲突的文档目录路径。
     * @param baseFolder 基础目录。
     */
    QString generateUniqueDocumentPath(const QString &baseFolder);

    /**
     * @brief 把目录内容（图片等）导入为文档页面。
     * @param documentRootFolder 源目录。
     * @param pDocument 目标文档。
     * @return 是否至少导入了一个文件。
     */
    bool addDirectoryContentToDocument(const QString &documentRootFolder, DocumentProxy *pDocument);

    /// 扫描用户文档目录，重建文档树。
    void createDocumentProxiesStructure(bool interactive = false);
    /**
     * @brief 依据给定目录列表重建文档树。
     * @param contentInfo 文档目录列表。
     * @param interactive 重名时是否弹窗询问。
     */
    void createDocumentProxiesStructure(const QFileInfoList &contentInfo, bool interactive = false);
    /**
     * @brief 文档重名时弹窗让用户选择替换或保留。
     * @param pProxy 待登记的文档。
     * @return 对话框结果。
     */
    QDialog::DialogCode processInteractiveReplacementDialog(DocumentProxy *pProxy);

    /// 返回文档包的固定子目录名列表。
    QStringList documentSubDirectories() { return mDocumentSubDirectories; }

    /// 判断文档是否没有任何页面。
    virtual bool isEmpty(DocumentProxy *pDocumentProxy);
    /// 删除仓库中所有空文档。
    virtual void purgeEmptyDocuments();

    /**
     * @brief 把控件文件加入文档包。
     * @param mDocumentProxy 目标文档。
     * @param path 源文件路径。
     * @param objectUuid 目标文件标识。
     * @param destinationPath 输出：文档包内的相对路径。
     * @return 成功返回 true。
     */
    bool addGraphicsWidgetToDocument(DocumentProxy *mDocumentProxy, QString path, QUuid objectUuid,
                                     QString &destinationPath);
    // 只解析并准备文档包内的目标路径（含建子目录），不搬运数据；失败返回空串。
    // 拆开是为了让大文件复制能离开 GUI 线程，而文档状态仍只在主线程读取。
    /**
     * @brief 解析文档包内的目标路径并确保子目录存在，不复制数据。
     * @param pDocumentProxy 目标文档。
     * @param subdir 子目录名，通常取本类的目录常量。
     * @param objectUuid 目标文件标识。
     * @param suffix 文件扩展名（不含点）。
     * @return 绝对路径；文档无效时为空串。
     */
    QString documentFilePath(DocumentProxy *pDocumentProxy, const QString &subdir, const QUuid &objectUuid,
                             const QString &suffix);
    /**
     * @brief 把文件复制进文档包。
     * @param pDocumentProxy 目标文档。
     * @param path 源文件路径。
     * @param subdir 目标子目录。
     * @param objectUuid 目标文件标识。
     * @param destinationPath 输出：目标路径。
     * @param data 非空时改为从内存写入。
     * @return 成功返回 true。
     */
    bool addFileToDocument(DocumentProxy *pDocumentProxy, QString path, const QString &subdir,
                           QUuid objectUuid, QString &destinationPath, QByteArray *data = NULL);

    /// 判断文档是否可能包含视频（用于界面提示）。
    bool mayHaveVideo(DocumentProxy *pDocumentProxy);
    /// 判断文档是否可能包含音频。
    bool mayHaveAudio(DocumentProxy *pDocumentProxy);
    /// 判断文档是否可能包含 PDF。
    bool mayHavePDF(DocumentProxy *pDocumentProxy);
    /// 判断文档是否可能包含 SVG 图片。
    bool mayHaveSVGImages(DocumentProxy *pDocumentProxy);
    /// 判断文档是否可能包含控件。
    bool mayHaveWidget(DocumentProxy *pDocumentProxy);

    /// 把文档树路径补全为“我的文档/…”形式的虚拟路径。
    QString adjustDocumentVirtualPath(const QString &str);

    /// 退出前把目录树结构写入 XML。
    void closing();
    /// 判断页面是否已在场景缓存中。
    bool isSceneInCached(DocumentProxy *proxy, int index) const;

signals:
    /// 文档列表发生变化。
    void proxyListChanged();

    /// 新文档已登记。
    void documentCreated(DocumentProxy *pDocumentProxy);
    /// 文档元数据已改变。
    void documentMetadataChanged(DocumentProxy *pDocumentProxy);
    /// 文档即将被删除。
    void documentWillBeDeleted(DocumentProxy *pDocumentProxy);

    /// 文档中新建了页面。
    void documentSceneCreated(DocumentProxy *pDocumentProxy, int pIndex);
    /// 文档中的页面即将被删除。
    void documentSceneWillBeDeleted(DocumentProxy *pDocumentProxy, int pIndex);

private:
    /// 统计文档包内的页面文件数。
    int sceneCount(const DocumentProxy *pDocumentProxy);
    /// 列出目录中的页面文件名。
    static QStringList getSceneFileNames(const QString &folder);
    /// 页面在文档包内改名，用于移动/复制。
    void renamePage(DocumentProxy *pDocumentProxy, const int sourceIndex, const int targetIndex);
    /// 页面文件在文档包内复制。
    void copyPage(DocumentProxy *pDocumentProxy, const int sourceIndex, const int targetIndex);
    /// 文档缺少路径时补一个唯一路径。
    void generatePathIfNeeded(DocumentProxy *pDocumentProxy);
    /// 确保文档仓库目录存在。
    void checkIfDocumentRepositoryExists();

    /// 递归把目录树写入 XML。
    void saveFoldersTreeToXml(QXmlStreamWriter &writer, const QModelIndex &parentIndex);
    /// 从 XML 递归恢复目录树。
    void loadFolderTreeFromXml(const QString &path, const QDomElement &element);

    QString xmlFolderStructureFilename;

    SceneCache mSceneCache;
    QStringList mDocumentSubDirectories;
    QMutex mDeletedListMutex;
    bool mHasPurgedDocuments;
    QString mDocumentRepositoryPath;
    QString mFoldersXmlStorageName;

private slots:
    /// 文档仓库路径设置变化后重建结构。
    void documentRepositoryChanged(const QString &path);
};
