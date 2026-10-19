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

class PersistenceManager : public QObject
{
    Q_OBJECT

    private:
        PersistenceManager(QObject *pParent = 0);
        static PersistenceManager* sSingleton;

    public:
        virtual ~PersistenceManager();

        static const QString imageDirectory;
        static const QString objectDirectory;
        static const QString videoDirectory;
        static const QString audioDirectory;
        static const QString widgetDirectory;
        static const QString fileDirectory;

        static const QString myDocumentsName;
        static const QString modelsName;
        static const QString fFolders;
        static const QString tFolder;
        static const QString aName;

        static PersistenceManager* persistenceManager();
        static void destroy();

        virtual DocumentProxy* createDocument(const QString& pGroupName = ""
                , const QString& pName = ""
                , bool withEmptyPage = true
                , QString directory =QString()
                , int pageCount = 0
                , bool promptDialogIfExists = false);

        virtual DocumentProxy *createNewDocument(const QString& pGroupName = ""
                , const QString& pName = ""
                , bool withEmptyPage = true
                , QString directory =QString()
                , int pageCount = 0
                , bool promptDialogIfExists = false);

        virtual DocumentProxy* createDocumentFromDir(const QString& pDocumentDirectory
                                                       , const QString& pGroupName = ""
                , const QString& pName = ""
                , bool withEmptyPage = false
                , bool addTitlePage = false
                , bool promptDialogIfExists = false);

        virtual DocumentProxy* persistDocumentMetadata(DocumentProxy* pDocumentProxy);

        virtual DocumentProxy* duplicateDocument(DocumentProxy* pDocumentProxy);

        virtual void deleteDocument(DocumentProxy* pDocumentProxy);

        virtual void deleteDocumentScenes(DocumentProxy* pDocumentProxy, const QList<int>& indexes);

        virtual void duplicateDocumentScene(DocumentProxy* pDocumentProxy, int index);

        virtual void copyDocumentScene(DocumentProxy *from, int fromIndex, DocumentProxy *to, int toIndex);

        virtual void persistDocumentScene(DocumentProxy* pDocumentProxy,
                GraphicsScene* pScene, const int pSceneIndex);

        virtual GraphicsScene* createDocumentSceneAt(DocumentProxy* pDocumentProxy, int index, bool useUndoRedoStack = true);

        virtual void insertDocumentSceneAt(DocumentProxy* pDocumentProxy, GraphicsScene* scene, int index, bool persist = true);

        virtual void moveSceneToIndex(DocumentProxy* pDocumentProxy, int source, int target);

        virtual GraphicsScene* loadDocumentScene(DocumentProxy* pDocumentProxy, int sceneIndex);
        GraphicsScene *getDocumentScene(DocumentProxy* pDocumentProxy, int sceneIndex) {return mSceneCache.value(pDocumentProxy, sceneIndex);}
        void reassignDocProxy(DocumentProxy *newDocument, DocumentProxy *oldDocument);

//        QList<QPointer<DocumentProxy> > documentProxies;
        DocumentTreeNode *mDocumentTreeStructure;
        DocumentTreeModel *mDocumentTreeStructureModel;

        virtual QStringList allShapes();
        virtual QStringList allGips();
        virtual QStringList allImages(const QDir& dir);
        virtual QStringList allVideos(const QDir& dir);
        virtual QStringList allWidgets(const QDir& dir);

        QString generateUniqueDocumentPath();
        QString generateUniqueDocumentPath(const QString& baseFolder);

        bool addDirectoryContentToDocument(const QString& documentRootFolder, DocumentProxy* pDocument);

        void createDocumentProxiesStructure(bool interactive = false);
        void createDocumentProxiesStructure(const QFileInfoList &contentInfo, bool interactive = false);
        QDialog::DialogCode processInteractiveReplacementDialog(DocumentProxy *pProxy);

        QStringList documentSubDirectories()
        {
            return mDocumentSubDirectories;
        }

        virtual bool isEmpty(DocumentProxy* pDocumentProxy);
        virtual void purgeEmptyDocuments();

        bool addGraphicsWidgetToDocument(DocumentProxy *mDocumentProxy, QString path, QUuid objectUuid, QString& destinationPath);
        // 只解析并准备文档包内的目标路径（含建子目录），不搬运数据；失败返回空串。
        // 拆开是为了让大文件复制能离开 GUI 线程，而文档状态仍只在主线程读取。
        QString documentFilePath(DocumentProxy* pDocumentProxy, const QString& subdir, const QUuid& objectUuid, const QString& suffix);
        bool addFileToDocument(DocumentProxy* pDocumentProxy, QString path, const QString& subdir,  QUuid objectUuid, QString& destinationPath, QByteArray* data = NULL);

        bool mayHaveVideo(DocumentProxy* pDocumentProxy);
        bool mayHaveAudio(DocumentProxy* pDocumentProxy);
        bool mayHavePDF(DocumentProxy* pDocumentProxy);
        bool mayHaveSVGImages(DocumentProxy* pDocumentProxy);
        bool mayHaveWidget(DocumentProxy* pDocumentProxy);

        QString adjustDocumentVirtualPath(const QString &str);

        void closing();
        bool isSceneInCached(DocumentProxy *proxy, int index) const;

    signals:
        void proxyListChanged();

        void documentCreated(DocumentProxy* pDocumentProxy);
        void documentMetadataChanged(DocumentProxy* pDocumentProxy);
        void documentWillBeDeleted(DocumentProxy* pDocumentProxy);

        void documentSceneCreated(DocumentProxy* pDocumentProxy, int pIndex);
        void documentSceneWillBeDeleted(DocumentProxy* pDocumentProxy, int pIndex);

	private:
        int sceneCount(const DocumentProxy* pDocumentProxy);
        static QStringList getSceneFileNames(const QString& folder);
        void renamePage(DocumentProxy* pDocumentProxy,
                        const int sourceIndex, const int targetIndex);
        void copyPage(DocumentProxy* pDocumentProxy,
                      const int sourceIndex, const int targetIndex);
        void generatePathIfNeeded(DocumentProxy* pDocumentProxy);
        void checkIfDocumentRepositoryExists();

        void saveFoldersTreeToXml(QXmlStreamWriter &writer, const QModelIndex &parentIndex);
        void loadFolderTreeFromXml(const QString &path, const QDomElement &element);

        QString xmlFolderStructureFilename;

        SceneCache mSceneCache;
        QStringList mDocumentSubDirectories;
        QMutex mDeletedListMutex;
        bool mHasPurgedDocuments;
        QString mDocumentRepositoryPath;
        QString mFoldersXmlStorageName;

    private slots:
        void documentRepositoryChanged(const QString& path);

};

