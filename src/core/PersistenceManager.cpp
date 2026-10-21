#include "core/PersistenceManager.h"
#include "core/UserNotices.h"
#include <algorithm>

#include <QtXml>
#include <QVariant>
#include <QDomDocument>
#include <QXmlStreamWriter>
#include <QModelIndex>

#include "utils/FileSystemUtils.h"

#include "core/Application.h"
#include "settings/Settings.h"
#include "model/ForeignObjectsHandler.h"

#include "model/DocumentProxy.h"
#include "model/DocumentTreeModel.h"

#include "content/SvgSubsetAdaptor.h"
#include "content/ThumbnailAdaptor.h"
#include "content/MetadataDcSubsetAdaptor.h"

#include "whiteboard/GraphicsMediaItem.h"
#include "whiteboard/GraphicsWidgetItem.h"
#include "whiteboard/GraphicsPixmapItem.h"
#include "whiteboard/GraphicsSvgItem.h"

namespace
{
    /// 重名裁决钩子；由装配点装入界面实现，为空时重名一律不登记。
    PersistenceManager::DuplicateNameResolver gDuplicateNameResolver;
} // namespace

const QString PersistenceManager::imageDirectory   = "images";  // added to PersistenceManager::mAllDirectories
const QString PersistenceManager::objectDirectory  = "objects"; // added to PersistenceManager::mAllDirectories
const QString PersistenceManager::widgetDirectory  = "widgets"; // added to PersistenceManager::mAllDirectories
const QString PersistenceManager::videoDirectory   = "videos";  // added to PersistenceManager::mAllDirectories
const QString PersistenceManager::audioDirectory   = "audios";  // added to
const QString PersistenceManager::fileDirectory    = "files";   // Issue 1683 (Evolution) - AOU - 20131206

const QString PersistenceManager::myDocumentsName  = "MyDocuments";
const QString PersistenceManager::modelsName       = "Models";
const QString PersistenceManager::fFolders         = "folders.xml";
const QString PersistenceManager::tFolder          = "folder";
const QString PersistenceManager::aName            = "name";

PersistenceManager *PersistenceManager::sSingleton = 0;

PersistenceManager::PersistenceManager(QObject *pParent) : QObject(pParent), mHasPurgedDocuments(false)
{
    xmlFolderStructureFilename = "model";

    mDocumentSubDirectories << imageDirectory;
    mDocumentSubDirectories << objectDirectory;
    mDocumentSubDirectories << widgetDirectory;
    mDocumentSubDirectories << videoDirectory;
    mDocumentSubDirectories << audioDirectory;
    mDocumentSubDirectories << fileDirectory; // Issue 1683 (Evolution) - AOU - 20131206

    mDocumentRepositoryPath     = Settings::userDocumentDirectory();
    mFoldersXmlStorageName      = mDocumentRepositoryPath + "/" + fFolders;

    mDocumentTreeStructureModel = new DocumentTreeModel(this);
    createDocumentProxiesStructure();

    emit proxyListChanged();
}

PersistenceManager *PersistenceManager::persistenceManager()
{
    if (!sSingleton)
    {
        sSingleton = new PersistenceManager(Application::staticMemoryCleaner);
    }

    return sSingleton;
}

void PersistenceManager::destroy()
{
    if (sSingleton)
    {
        delete sSingleton;
    }
    sSingleton = NULL;
}

PersistenceManager::~PersistenceManager()
{
}

void PersistenceManager::createDocumentProxiesStructure(bool interactive)
{
    mDocumentRepositoryPath = Settings::userDocumentDirectory();

    QDir rootDir(mDocumentRepositoryPath);
    rootDir.mkpath(rootDir.path());

    QFileInfoList contentList = rootDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Time | QDir::Reversed);
    createDocumentProxiesStructure(contentList, interactive);

    if (QFileInfo(mFoldersXmlStorageName).exists())
    {
        QDomDocument xmlDom;
        QFile        inFile(mFoldersXmlStorageName);
        if (inFile.open(QIODevice::ReadOnly))
        {
            QString domString(inFile.readAll());

            QDomDocument::ParseResult parseResult = xmlDom.setContent(domString);
            if (parseResult)
            {
                loadFolderTreeFromXml("", xmlDom.firstChildElement());
            }
            else
            {
                qDebug() << "Error reading content of " << mFoldersXmlStorageName << Qt::endl
                         << "Error:" << inFile.errorString() << "Line:" << parseResult.errorLine << "Column:" << parseResult.errorColumn
                         << parseResult.errorMessage;
            }
            inFile.close();
        }
        else
        {
            qDebug() << "Error reading" << mFoldersXmlStorageName << Qt::endl << "Error:" << inFile.errorString();
        }
    }
}

void PersistenceManager::createDocumentProxiesStructure(const QFileInfoList &contentInfo, bool interactive)
{
    foreach (QFileInfo path, contentInfo)
    {
        QString fullPath                  = path.absoluteFilePath();

        QMap<QString, QVariant> metadatas = MetadataDcSubsetAdaptor::load(fullPath);

        QString docGroupName              = metadatas.value(Settings::documentGroupName, QString()).toString();
        QString docName                   = metadatas.value(Settings::documentName, QString()).toString();

        if (docName.isEmpty())
        {
            qDebug() << "Group name and document name are empty in "
                        "PersistenceManager::createDocumentProxiesStructure()";
            continue;
        }

        QModelIndex parentIndex = mDocumentTreeStructureModel->goTo(docGroupName);
        if (!parentIndex.isValid())
        {
            return;
        }

        DocumentProxy *docProxy = new DocumentProxy(fullPath, metadatas); // managed in DocumentTreeNode
        foreach (QString key, metadatas.keys())
        {
            docProxy->setMetaData(key, metadatas.value(key));
        }

        if (metadatas.contains(Settings::documentPageCount))
        {
            docProxy->setPageCount(metadatas.value(Settings::documentPageCount).toInt());
        }
        else
        {
            int pageCount = sceneCount(docProxy);
            docProxy->setPageCount(pageCount);
        }

        if (!interactive)
        {
            mDocumentTreeStructureModel->addDocument(docProxy, parentIndex);
        }
        else if (!registerDocumentProxy(docProxy))
        {
            // 用户没同意登记（或压根没装上重名裁决钩子）：把刚解压出来的这一份退回磁盘外，
            // 留着它既不在树里、又会在下次启动扫描时凭空冒出来。
            deleteDocument(docProxy);
        }
    }
}

void PersistenceManager::installDuplicateNameResolver(DuplicateNameResolver resolver)
{
    gDuplicateNameResolver = std::move(resolver);
}

bool PersistenceManager::registerDocumentProxy(DocumentProxy *pProxy)
{
    const QString     docGroupName = pProxy->metaData(Settings::documentGroupName).toString();
    const QModelIndex parentIndex  = mDocumentTreeStructureModel->goTo(docGroupName);
    if (!parentIndex.isValid())
    {
        return false;
    }

    const QStringList docList = mDocumentTreeStructureModel->nodeNameList(parentIndex);
    const QString     docName = pProxy->metaData(Settings::documentName).toString();

    if (!docList.contains(docName))
    {
        mDocumentTreeStructureModel->addDocument(pProxy, parentIndex);
        return true;
    }

    if (!gDuplicateNameResolver)
    {
        qWarning() << "PersistenceManager: 未装上重名裁决钩子，同名文档" << docName << "没有登记";
        return false;
    }

    const DuplicateNameDecision decision = gDuplicateNameResolver(docName, docList);
    if (!decision.proceed || decision.finalName.isEmpty())
    {
        // 空名字会被 addDocument() 直接挡掉，代理既不进树也不被释放，所以按取消处理。
        return false;
    }

    const int replacedRow = docList.indexOf(decision.finalName);

    pProxy->setMetaData(Settings::documentName, decision.finalName);

    if (replacedRow == -1)
    {
        mDocumentTreeStructureModel->addDocument(pProxy, parentIndex);
        return true;
    }

    const QModelIndex replaceIndex = mDocumentTreeStructureModel->index(replacedRow, 0, parentIndex);
    DocumentProxy    *outgoing     = mDocumentTreeStructureModel->proxyData(replaceIndex);
    const QModelIndex currentIndex = mDocumentTreeStructureModel->currentIndex();
    const bool        wasCurrent   = currentIndex.isValid() && outgoing == mDocumentTreeStructureModel->proxyData(currentIndex);

    // 先把新的挂上、再换旧的：界面在那一刻要能在树里找到接手文档的节点，否则"当前文档"
    // 会抱着一个马上被删掉的节点；旧文档则要在自己还活着的时候就被画板放下。
    mDocumentTreeStructureModel->addDocument(pProxy, parentIndex);

    if (wasCurrent)
    {
        emit currentDocumentReplaced(pProxy, outgoing);
    }

    if (outgoing)
    {
        deleteDocument(outgoing);
    }

    // 插入位置由排序决定，同名的一新一旧挨在一起时旧文档的行号可能被新文档顶动；删除前
    // 按代理重新找回它自己，别拿插入前算出来的行号去删——那会删错行，留下一个指向已删除
    // 代理的节点，代理模型接着就在这种错位的行映射里递归到栈溢出。
    const QModelIndex outgoingIndex = mDocumentTreeStructureModel->indexForProxy(outgoing);
    if (outgoingIndex.isValid())
    {
        mDocumentTreeStructureModel->removeRow(outgoingIndex.row(), outgoingIndex.parent());
    }

    return true;
}

QString PersistenceManager::adjustDocumentVirtualPath(const QString &str)
{
    QStringList pathList = str.split("/", Qt::SkipEmptyParts);

    if (pathList.isEmpty())
    {
        pathList.append(myDocumentsName);
    }

    if (pathList.first() != myDocumentsName && pathList.first() != Settings::trashedDocumentGroupNamePrefix && pathList.first() != modelsName)
    {
        pathList.prepend(myDocumentsName);
    }

    return pathList.join("/");
}

void PersistenceManager::closing()
{
    QDir rootDir(mDocumentRepositoryPath);
    rootDir.mkpath(rootDir.path());

    QFile outFile(mFoldersXmlStorageName);
    if (outFile.open(QIODevice::WriteOnly))
    {
        QXmlStreamWriter writer(&outFile);
        writer.setAutoFormatting(true);
        writer.writeStartDocument();
        writer.writeStartElement("content");
        saveFoldersTreeToXml(writer, QModelIndex());
        writer.writeEndElement();
        writer.writeEndDocument();

        outFile.close();
    }
    else
    {
        qDebug() << "failed to open document" << mFoldersXmlStorageName << "for writing" << Qt::endl << "Error string:" << outFile.errorString();
    }
}

bool PersistenceManager::isSceneInCached(DocumentProxy *proxy, int index) const
{
    return mSceneCache.contains(proxy, index);
}

QStringList PersistenceManager::allShapes()
{
    QString shapeLibraryPath = Settings::settings()->applicationShapeLibraryDirectory();

    QDir dir(shapeLibraryPath);

    if (!dir.exists())
    {
        dir.mkpath(shapeLibraryPath);
    }

    QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    QStringList paths;

    foreach (QString file, files)
    {
        paths.append(shapeLibraryPath + QString("/") + file);
    }

    return paths;
}

QStringList PersistenceManager::allGips()
{
    QString gipLibraryPath = Settings::settings()->userGipLibraryDirectory();

    QDir dir(gipLibraryPath);

    QStringList files = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    QStringList paths;

    foreach (QString file, files)
    {
        QFileInfo fi(file);

        if (Settings::settings()->widgetFileExtensions.contains(fi.suffix()))
        {
            paths.append(dir.path() + QString("/") + file);
        }
    }

    return paths;
}

QStringList PersistenceManager::allImages(const QDir &dir)
{
    if (!dir.exists())
    {
        dir.mkpath(dir.path());
    }

    QStringList files = dir.entryList(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDir::Name);
    QStringList paths;

    foreach (QString file, files)
    {
        paths.append(dir.path() + QString("/") + file);
    }

    return paths;
}

QStringList PersistenceManager::allVideos(const QDir &dir)
{
    if (!dir.exists())
    {
        dir.mkpath(dir.path());
    }

    QStringList files = dir.entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    QStringList paths;

    foreach (QString file, files)
    {
        paths.append(dir.path() + QString("/") + file);
    }

    return paths;
}

QStringList PersistenceManager::allWidgets(const QDir &dir)
{
    if (!dir.exists())
    {
        dir.mkpath(dir.path());
    }

    QStringList files = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    QStringList paths;

    foreach (QString file, files)
    {
        QFileInfo fi(file);

        if (Settings::settings()->widgetFileExtensions.contains(fi.suffix()))
        {
            paths.append(dir.path() + QString("/") + file);
        }
    }

    return paths;
}

DocumentProxy *PersistenceManager::createDocument(
    const QString &pGroupName, const QString &pName, bool withEmptyPage, QString directory, int pageCount, bool promptDialogIfExists)
{
    DocumentProxy *doc;
    if (directory.length() != 0)
    {
        doc = new DocumentProxy(directory); // deleted in PersistenceManager::destructor
        doc->setPageCount(pageCount);
    }
    else
    {
        checkIfDocumentRepositoryExists();
        doc = new DocumentProxy();
    }

    if (pGroupName.length() > 0)
    {
        doc->setMetaData(Settings::documentGroupName, pGroupName);
    }

    if (pName.length() > 0)
    {
        doc->setMetaData(Settings::documentName, pName);
    }

    doc->setMetaData(Settings::documentVersion, Settings::currentFileVersion);
    QString currentDate = StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime());
    doc->setMetaData(Settings::documentUpdatedAt, currentDate);
    doc->setMetaData(Settings::documentDate, currentDate);

    if (withEmptyPage)
    {
        createDocumentSceneAt(doc, 0);
    }
    else
    {
        this->generatePathIfNeeded(doc);
        QDir dir(doc->persistencePath());
        if (!dir.mkpath(doc->persistencePath()))
        {
            return 0; // 仓库路径建不出来就别往下走，写文档只会留下一堆半截文件
        }
    }

    bool addDoc = false;
    if (!promptDialogIfExists)
    {
        addDoc = true;
        mDocumentTreeStructureModel->addDocument(doc);
    }
    else if (registerDocumentProxy(doc))
    {
        addDoc = true;
    }
    if (addDoc)
    {
        emit proxyListChanged();
    }
    else
    {
        deleteDocument(doc);
        doc = 0;
    }

    return doc;
}

DocumentProxy *PersistenceManager::createNewDocument(
    const QString &pGroupName, const QString &pName, bool withEmptyPage, QString directory, int pageCount, bool promptDialogIfExists)
{
    DocumentProxy *resultDoc = createDocument(pGroupName, pName, withEmptyPage, directory, pageCount, promptDialogIfExists);
    if (resultDoc)
    {
        mDocumentTreeStructureModel->markDocumentAsNew(resultDoc);
    }

    return resultDoc;
}

DocumentProxy *PersistenceManager::createDocumentFromDir(const QString &pDocumentDirectory, const QString &pGroupName, const QString &pName,
    bool withEmptyPage, bool addTitlePage, bool promptDialogIfExists)
{
    checkIfDocumentRepositoryExists();

    DocumentProxy *doc = new DocumentProxy(pDocumentDirectory); // deleted in PersistenceManager::destructor

    if (pGroupName.length() > 0)
    {
        doc->setMetaData(Settings::documentGroupName, pGroupName);
    }

    if (pName.length() > 0)
    {
        doc->setMetaData(Settings::documentName, pName);
    }

    QMap<QString, QVariant> metadatas = MetadataDcSubsetAdaptor::load(pDocumentDirectory);

    if (withEmptyPage)
    {
        createDocumentSceneAt(doc, 0);
    }
    if (addTitlePage)
    {
        persistDocumentScene(doc, mSceneCache.createScene(doc, 0, false), 0);
    }

    foreach (QString key, metadatas.keys())
    {
        doc->setMetaData(key, metadatas.value(key));
    }

    doc->setUuid(QUuid::createUuid());
    doc->setPageCount(sceneCount(doc));

    for (int i = 0; i < doc->pageCount(); i++)
    {
        SvgSubsetAdaptor::setSceneUuid(doc, i, QUuid::createUuid());
    }

    // 新建的文档已经落盘，但要不要留在树里取决于是否重名：
    // 调用方不想弹框就直接收，想弹框则等用户裁决，否决的话要把刚写的目录一起删掉。
    bool addDoc = false;
    if (!promptDialogIfExists)
    {
        addDoc = true;
        mDocumentTreeStructureModel->addDocument(doc);
    }
    else if (registerDocumentProxy(doc))
    {
        addDoc = true;
    }
    if (addDoc)
    {
        MetadataDcSubsetAdaptor::persist(doc);
        emit proxyListChanged();
        emit documentCreated(doc);
    }
    else
    {
        deleteDocument(doc);
        doc = 0;
    }

    return doc;
}

void PersistenceManager::deleteDocument(DocumentProxy *pDocumentProxy)
{
    checkIfDocumentRepositoryExists();

    emit documentWillBeDeleted(pDocumentProxy);

    if (QFileInfo(pDocumentProxy->persistencePath()).exists())
    {
        FileSystemUtils::deleteDir(pDocumentProxy->persistencePath());
    }

    mSceneCache.removeAllScenes(pDocumentProxy);

    pDocumentProxy->deleteLater();
}

DocumentProxy *PersistenceManager::duplicateDocument(DocumentProxy *pDocumentProxy)
{
    checkIfDocumentRepositoryExists();

    DocumentProxy *copy = new DocumentProxy(); // deleted in PersistenceManager::destructor

    generatePathIfNeeded(copy);

    FileSystemUtils::copyDir(pDocumentProxy->persistencePath(), copy->persistencePath());

    // 副本的每一页都要换新 UUID：源文档的页号不能被副本顶掉，否则场景缓存会认错页。
    for (int i = 0; i < copy->pageCount(); i++)
    {
        SvgSubsetAdaptor::setSceneUuid(copy, i, QUuid::createUuid());
    }

    foreach (QString key, pDocumentProxy->metaDatas().keys())
    {
        copy->setMetaData(key, pDocumentProxy->metaDatas().value(key));
    }

    copy->setMetaData(Settings::documentName, pDocumentProxy->metaData(Settings::documentName).toString() + " " + tr("(copy)"));

    copy->setUuid(QUuid::createUuid());

    persistDocumentMetadata(copy);

    copy->setPageCount(sceneCount(copy));

    emit proxyListChanged();

    emit documentCreated(copy);

    return copy;
}

void PersistenceManager::deleteDocumentScenes(DocumentProxy *proxy, const QList<int> &indexes)
{
    checkIfDocumentRepositoryExists();

    int pageCount = PersistenceManager::persistenceManager()->sceneCount(proxy);

    QList<int> compactedIndexes;

    foreach (int index, indexes)
    {
        if (!compactedIndexes.contains(index))
        {
            compactedIndexes.append(index);
        }
    }

    if (compactedIndexes.size() == pageCount)
    {
        deleteDocument(proxy);
        return;
    }

    if (compactedIndexes.size() == 0)
    {
        return;
    }

    foreach (int index, compactedIndexes)
    {
        emit documentSceneWillBeDeleted(proxy, index);
    }

    QString        sourceName    = proxy->metaData(Settings::documentName).toString();
    DocumentProxy *trashDocProxy = createDocument(Settings::trashedDocumentGroupNamePrefix /* + sourceGroupName*/, sourceName, false);

    foreach (int index, compactedIndexes)
    {
        GraphicsScene *scene = loadDocumentScene(proxy, index);
        if (scene)
        {
            // 这一页即将连带的媒体文件要搬进回收站那份文档，搬完才允许删原页
            foreach (QUrl relativeFile, scene->relativeDependencies())
            {
                QString source = scene->document()->persistencePath() + "/" + relativeFile.toString();
                QString target = trashDocProxy->persistencePath() + "/" + relativeFile.toString();

                QFileInfo fi(target);
                QDir      d = fi.dir();

                d.mkpath(d.absolutePath());
                QFile::copy(source, target);
            }

            insertDocumentSceneAt(trashDocProxy, scene, trashDocProxy->pageCount());
        }
    }

    for (int i = 1; i < pageCount; i++)
    {
        renamePage(trashDocProxy, i, i - 1);
    }

    foreach (int index, compactedIndexes)
    {
        QString svgFileName = proxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.svg", index);

        QFile::remove(svgFileName);

        QString thumbFileName = proxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", index);

        QFile::remove(thumbFileName);

        mSceneCache.removeScene(proxy, index);

        proxy->decPageCount();
    }

    std::sort(compactedIndexes.begin(), compactedIndexes.end());

    int offset = 1;

    for (int i = compactedIndexes.at(0) + 1; i < pageCount; i++)
    {
        if (compactedIndexes.contains(i))
        {
            offset++;
        }
        else
        {
            renamePage(proxy, i, i - offset);

            mSceneCache.moveScene(proxy, i, i - offset);
        }
    }
}

void PersistenceManager::duplicateDocumentScene(DocumentProxy *proxy, int index)
{
    checkIfDocumentRepositoryExists();

    int pageCount = PersistenceManager::persistenceManager()->sceneCount(proxy);

    for (int i = pageCount; i > index + 1; i--)
    {
        renamePage(proxy, i - 1, i);

        mSceneCache.moveScene(proxy, i - 1, i);
    }

    copyPage(proxy, index, index + 1);

    // TODO: 复制页面时磁盘上的对象只能靠“加载再存一遍”顺带搬走，缺一套正式的引用复制机制
    GraphicsScene *scene = loadDocumentScene(proxy, index + 1);
    if (!scene)
    {
        return;
    }

    foreach (QGraphicsItem *item, scene->items())
    {
        // 音频与视频图元各自回报 Type，qgraphicsitem_cast 认不出抽象基类，
        // 所以这里要按类型逐个问；早先正因为漏了视频分支，副本页会和原页共用同一个媒体文件。
        GraphicsMediaItem *mediaItem = dynamic_cast<GraphicsMediaItem *>(item);

        if (mediaItem)
        {
            QString source      = mediaItem->mediaFileUrl().toLocalFile();
            QString destination = source;
            QUuid   newUuid     = QUuid::createUuid();
            QString fileName    = QFileInfo(source).completeBaseName();
            destination         = destination.replace(fileName, newUuid.toString());
            QFile::copy(source, destination);
            mediaItem->setMediaFileUrl(QUrl::fromLocalFile(destination));
            continue;
        }

        GraphicsWidgetItem *widget = qgraphicsitem_cast<GraphicsWidgetItem *>(item);
        if (widget)
        {
            QUuid   newUUid               = QUuid::createUuid();
            QString newUUidString         = newUUid.toString().remove("{").remove("}");
            QString actualUuidString      = widget->uuid().toString().remove("{").remove("}");

            QString widgetSourcePath      = proxy->persistencePath() + "/" + PersistenceManager::widgetDirectory + "/{" + actualUuidString + "}.wgt";
            QString screenshotSourcePath  = proxy->persistencePath() + "/" + PersistenceManager::widgetDirectory + "/" + actualUuidString + ".png";

            QString widgetDestinationPath = widgetSourcePath;
            widgetDestinationPath         = widgetDestinationPath.replace(actualUuidString, newUUidString);
            QString screenshotDestinationPath = screenshotSourcePath;
            screenshotDestinationPath         = screenshotDestinationPath.replace(actualUuidString, newUUidString);

            FileSystemUtils::copyDir(widgetSourcePath, widgetDestinationPath);
            QFile::copy(screenshotSourcePath, screenshotDestinationPath);

            widget->setUuid(newUUid);

            widget->widgetUrl(QUrl::fromLocalFile(widgetDestinationPath));

            continue;
        }

        GraphicsPixmapItem *pixmapItem = qgraphicsitem_cast<GraphicsPixmapItem *>(item);
        if (pixmapItem)
        {
            QString source      = proxy->persistencePath() + "/" + PersistenceManager::imageDirectory + "/" + pixmapItem->uuid().toString() + ".png";
            QString destination = source;
            QUuid   newUuid     = QUuid::createUuid();
            QString fileName    = QFileInfo(source).completeBaseName();
            destination         = destination.replace(fileName, newUuid.toString());
            QFile::copy(source, destination);
            pixmapItem->setUuid(newUuid);
            continue;
        }

        GraphicsSvgItem *svgItem = qgraphicsitem_cast<GraphicsSvgItem *>(item);
        if (svgItem)
        {
            QString source      = proxy->persistencePath() + "/" + PersistenceManager::imageDirectory + "/" + svgItem->uuid().toString() + ".svg";
            QString destination = source;
            QUuid   newUuid     = QUuid::createUuid();
            QString fileName    = QFileInfo(source).completeBaseName();
            destination         = destination.replace(fileName, newUuid.toString());
            QFile::copy(source, destination);
            svgItem->setUuid(newUuid);
            continue;
        }
    }
    scene->setModified(true);

    persistDocumentScene(proxy, scene, index + 1);

    proxy->incPageCount();

    emit documentSceneCreated(proxy, index + 1);
}

void PersistenceManager::copyDocumentScene(DocumentProxy *from, int fromIndex, DocumentProxy *to, int toIndex)
{
    if (from == to && toIndex <= fromIndex)
    {
        qDebug() << "operation is not supported" << Q_FUNC_INFO;
        return;
    }

    checkIfDocumentRepositoryExists();

    for (int i = to->pageCount(); i > toIndex; i--)
    {
        renamePage(to, i - 1, i);
        mSceneCache.moveScene(to, i - 1, i);
    }

    ForeighnObjectsHandler hl;
    hl.copyPage(QUrl::fromLocalFile(from->persistencePath()), fromIndex, QUrl::fromLocalFile(to->persistencePath()), toIndex);

    to->incPageCount();

    QString thumbTmp(from->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", fromIndex));
    QString thumbTo(to->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", toIndex));

    QFile::remove(thumbTo);
    QFile::copy(thumbTmp, thumbTo);

    Q_ASSERT(QFileInfo(thumbTmp).exists());
    Q_ASSERT(QFileInfo(thumbTo).exists());

    // 回报事实，不动界面：谁在展示这份文档，谁自己把缩略图条按盘上的内容重建。
    // 以前这里直接拿 Application::documentController 往缩略图列表里插一张图，复制到的
    // 文档不是屏幕上那一份时，插进去的图就落在了别的文档的条上。
    emit documentSceneCreated(to, toIndex);
}

GraphicsScene *PersistenceManager::createDocumentSceneAt(DocumentProxy *proxy, int index, bool useUndoRedoStack)
{
    int count = sceneCount(proxy);

    for (int i = count - 1; i >= index; i--)
    {
        renamePage(proxy, i, i + 1);
    }

    mSceneCache.shiftUpScenes(proxy, index, count - 1);

    GraphicsScene *newScene = mSceneCache.createScene(proxy, index, useUndoRedoStack);

    newScene->setBackground(Settings::settings()->isDarkBackground(), Settings::settings()->Settings::pageBackground());

    newScene->setBackgroundGridSize(Settings::settings()->crossSize);

    persistDocumentScene(proxy, newScene, index);

    proxy->incPageCount();

    emit documentSceneCreated(proxy, index);

    return newScene;
}

void PersistenceManager::insertDocumentSceneAt(DocumentProxy *proxy, GraphicsScene *scene, int index, bool persist)
{
    scene->setDocument(proxy);

    int count = sceneCount(proxy);

    for (int i = count - 1; i >= index; i--)
    {
        renamePage(proxy, i, i + 1);
    }

    mSceneCache.shiftUpScenes(proxy, index, count - 1);

    mSceneCache.insert(proxy, index, scene);

    if (persist)
    {
        persistDocumentScene(proxy, scene, index);
    }

    proxy->incPageCount();

    emit documentSceneCreated(proxy, index);
}

void PersistenceManager::moveSceneToIndex(DocumentProxy *proxy, int source, int target)
{
    checkIfDocumentRepositoryExists();

    if (source == target)
    {
        return;
    }

    QFile svgTmp(proxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.svg", source));
    svgTmp.rename(proxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.tmp", target));

    QFile thumbTmp(proxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", source));
    thumbTmp.rename(proxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.thumbnail.tmp", target));

    if (source < target)
    {
        for (int i = source + 1; i <= target; i++)
        {
            renamePage(proxy, i, i - 1);
        }
    }
    else
    {
        for (int i = source - 1; i >= target; i--)
        {
            renamePage(proxy, i, i + 1);
        }
    }

    QFile svg(proxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.tmp", target));
    svg.rename(proxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.svg", target));

    QFile thumb(proxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.thumbnail.tmp", target));
    thumb.rename(proxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", target));

    mSceneCache.moveScene(proxy, source, target);
}

GraphicsScene *PersistenceManager::loadDocumentScene(DocumentProxy *proxy, int sceneIndex)
{
    if (mSceneCache.contains(proxy, sceneIndex))
    {
        return mSceneCache.value(proxy, sceneIndex);
    }
    else
    {
        GraphicsScene *scene = SvgSubsetAdaptor::loadScene(proxy, sceneIndex);
        if (!scene)
        {
            createDocumentSceneAt(proxy, 0);
            scene = SvgSubsetAdaptor::loadScene(proxy, 0);
        }

        if (scene)
        {
            mSceneCache.insert(proxy, sceneIndex, scene);
        }

        return scene;
    }
}

void PersistenceManager::reassignDocProxy(DocumentProxy *newDocument, DocumentProxy *oldDocument)
{
    return mSceneCache.reassignDocProxy(newDocument, oldDocument);
}

void PersistenceManager::persistDocumentScene(DocumentProxy *pDocumentProxy, GraphicsScene *pScene, const int pSceneIndex)
{
    checkIfDocumentRepositoryExists();

    pScene->deselectAllItems();

    generatePathIfNeeded(pDocumentProxy);

    QDir dir(pDocumentProxy->persistencePath());
    dir.mkpath(pDocumentProxy->persistencePath());

    if (pDocumentProxy->isModified())
    {
        MetadataDcSubsetAdaptor::persist(pDocumentProxy);
    }

    if (pScene->isModified())
    {
        SvgSubsetAdaptor::persistScene(pDocumentProxy, pScene, pSceneIndex);

        ThumbnailAdaptor::persistScene(pDocumentProxy, pScene, pSceneIndex);

        pScene->setModified(false);
    }

    mSceneCache.insert(pDocumentProxy, pSceneIndex, pScene);
}

DocumentProxy *PersistenceManager::persistDocumentMetadata(DocumentProxy *pDocumentProxy)
{
    MetadataDcSubsetAdaptor::persist(pDocumentProxy);

    emit documentMetadataChanged(pDocumentProxy);

    return pDocumentProxy;
}

void PersistenceManager::renamePage(DocumentProxy *pDocumentProxy, const int sourceIndex, const int targetIndex)
{
    QFile svg(pDocumentProxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.svg", sourceIndex));
    svg.rename(pDocumentProxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.svg", targetIndex));

    QFile thumb(pDocumentProxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", sourceIndex));
    thumb.rename(pDocumentProxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", targetIndex));
}

void PersistenceManager::copyPage(DocumentProxy *pDocumentProxy, const int sourceIndex, const int targetIndex)
{
    QFile svg(pDocumentProxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.svg", sourceIndex));
    svg.copy(pDocumentProxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.svg", targetIndex));

    SvgSubsetAdaptor::setSceneUuid(pDocumentProxy, targetIndex, QUuid::createUuid());

    QFile thumb(pDocumentProxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", sourceIndex));
    thumb.copy(pDocumentProxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", targetIndex));
}

int PersistenceManager::sceneCount(const DocumentProxy *proxy)
{
    const QString pPath       = proxy->persistencePath();

    int  pageIndex            = 0;
    bool moreToProcess        = true;
    bool addedMissingZeroPage = false;

    while (moreToProcess)
    {
        QString fileName = pPath + FileSystemUtils::digitFileFormat("/page%1.svg", pageIndex);

        QFile file(fileName);

        if (file.exists())
        {
            pageIndex++;
        }
        else
        {
            moreToProcess = false;
        }
    }

    if (pageIndex == 1 && addedMissingZeroPage)
    {
        // 上面那个加号只为判断“除了补出来的 0 页还有没有别的页”；
        // 走到这里说明整本文档其实一页都没有。
        return 0;
    }

    return pageIndex;
}

QStringList PersistenceManager::getSceneFileNames(const QString &folder)
{
    QDir dir(folder, "page???.svg", QDir::Name, QDir::Files);
    return dir.entryList();
}

QString PersistenceManager::generateUniqueDocumentPath(const QString &baseFolder)
{
    QDateTime now     = QDateTime::currentDateTime();
    QString   dirName = now.toString("yyyy-MM-dd hh-mm-ss.zzz");

    return baseFolder + QString("/TianBoard Document %1").arg(dirName);
}

QString PersistenceManager::generateUniqueDocumentPath()
{
    return generateUniqueDocumentPath(Settings::userDocumentDirectory());
}

void PersistenceManager::generatePathIfNeeded(DocumentProxy *pDocumentProxy)
{
    if (pDocumentProxy->persistencePath().length() == 0)
    {
        pDocumentProxy->setPersistencePath(generateUniqueDocumentPath());
    }
}

bool PersistenceManager::addDirectoryContentToDocument(const QString &documentRootFolder, DocumentProxy *pDocument)
{
    QStringList sourceScenes = getSceneFileNames(documentRootFolder);
    if (sourceScenes.empty())
    {
        return false;
    }

    int targetPageCount = pDocument->pageCount();

    for (int sourceIndex = 0; sourceIndex < sourceScenes.size(); sourceIndex++)
    {
        int targetIndex = targetPageCount + sourceIndex;

        QFile svg(documentRootFolder + "/" + sourceScenes[sourceIndex]);
        if (!svg.copy(pDocument->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.svg", targetIndex)))
        {
            return false;
        }

        SvgSubsetAdaptor::setSceneUuid(pDocument, targetIndex, QUuid::createUuid());

        QFile thumb(documentRootFolder + FileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", sourceIndex));
        // 缩略图复制失败可以忽略：缺了会在下次生成时补出来，不影响正文。
        thumb.copy(pDocument->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", targetIndex));
    }

    foreach (QString dir, mDocumentSubDirectories)
    {
        qDebug() << "copying " << documentRootFolder << "/" << dir << " to " << pDocument->persistencePath() << "/" + dir;

        QDir srcDir(documentRootFolder + "/" + dir);
        if (srcDir.exists())
        {
            if (!FileSystemUtils::copyDir(documentRootFolder + "/" + dir, pDocument->persistencePath() + "/" + dir))
            {
                return false;
            }
        }
    }

    pDocument->setPageCount(sceneCount(pDocument));

    return true;
}

bool PersistenceManager::isEmpty(DocumentProxy *pDocumentProxy)
{
    if (!pDocumentProxy)
    {
        return true;
    }

    if (pDocumentProxy->pageCount() > 1)
    {
        return false;
    }

    GraphicsScene *theSoleScene = SvgSubsetAdaptor::loadScene(pDocumentProxy, 0);

    bool empty                  = false;

    if (theSoleScene)
    {
        empty = theSoleScene->isEmpty();
        delete theSoleScene;
    }
    else
    {
        empty = true;
    }

    return empty;
}

void PersistenceManager::purgeEmptyDocuments()
{
    QList<DocumentProxy *> toBeDeleted;

    foreach (DocumentProxy *docProxy, mDocumentTreeStructureModel->newDocuments())
    {
        if (isEmpty(docProxy))
        {
            toBeDeleted << docProxy;
        }
    }

    foreach (DocumentProxy *docProxy, toBeDeleted)
    {
        deleteDocument(docProxy);
    }
}

QString PersistenceManager::documentFilePath(DocumentProxy *pDocumentProxy, const QString &subdir, const QUuid &objectUuid, const QString &suffix)
{
    if (!pDocumentProxy || objectUuid.isNull())
    {
        return QString();
    }

    const QString documentRoot    = pDocumentProxy->persistencePath();
    const QString destinationPath = documentRoot + "/" + subdir + "/" + objectUuid.toString() + "." + suffix;

    if (QFile::exists(destinationPath))
    {
        return QString();
    }

    QDir dir;
    dir.mkdir(documentRoot + "/" + subdir);
    if (!QFile::exists(documentRoot + "/" + subdir))
    {
        return QString();
    }

    return destinationPath;
}

bool PersistenceManager::addFileToDocument(
    DocumentProxy *pDocumentProxy, QString path, const QString &subdir, QUuid objectUuid, QString &destinationPath, QByteArray *data)
{
    Q_ASSERT(path.length());
    QFileInfo fi(path);

    if (data == NULL && !fi.exists())
    {
        return false;
    }

    destinationPath = documentFilePath(pDocumentProxy, subdir, objectUuid, fi.suffix());
    if (destinationPath.isEmpty())
    {
        return false;
    }

    if (data == NULL)
    {
        QFile source(path);
        return source.copy(destinationPath);
    }

    QFile newFile(destinationPath);
    if (!newFile.open(QIODevice::WriteOnly))
    {
        return false;
    }

    const qint64 n = newFile.write(*data);
    newFile.flush();
    newFile.close();
    return n == data->size();
}

bool PersistenceManager::addGraphicsWidgetToDocument(DocumentProxy *pDocumentProxy, QString path, QUuid objectUuid, QString &destinationPath)
{
    QFileInfo fi(path);

    if (!fi.exists() || !pDocumentProxy || objectUuid.isNull())
    {
        return false;
    }

    QString widgetRootDir = path;
    QString extension     = QFileInfo(widgetRootDir).suffix();

    destinationPath       = pDocumentProxy->persistencePath() + "/" + widgetDirectory + "/" + objectUuid.toString() + "." + extension;

    if (!QFile::exists(destinationPath))
    {
        QDir dir;
        if (!dir.mkpath(destinationPath))
        {
            return false;
        }
        return FileSystemUtils::copyDir(widgetRootDir, destinationPath);
    }
    else
    {
        return false;
    }
}

void PersistenceManager::documentRepositoryChanged(const QString &path)
{
    Q_UNUSED(path);
    checkIfDocumentRepositoryExists();
}

void PersistenceManager::checkIfDocumentRepositoryExists()
{
    QDir rp(mDocumentRepositoryPath);

    if (!rp.exists())
    {
        // 文档仓库目录不在了（被挪走或网盘掉线），只能提示用户重新指定

        QString humanPath = QDir::cleanPath(mDocumentRepositoryPath);
        humanPath         = QDir::toNativeSeparators(humanPath);

        UserNotices::presentModalNotice(
            tr("Document Repository Loss"), tr("TianBoard has lost access to the document repository '%1'. Unfortunately the application "
                                               "must shut down to avoid data corruption. Latest changes may be lost as well.")
                                                .arg(humanPath));

        Application::quit();
    }
}

void PersistenceManager::saveFoldersTreeToXml(QXmlStreamWriter &writer, const QModelIndex &parentIndex)
{
    for (int i = 0; i < mDocumentTreeStructureModel->rowCount(parentIndex); i++)
    {
        QModelIndex currentIndex = mDocumentTreeStructureModel->index(i, 0, parentIndex);
        if (mDocumentTreeStructureModel->isCatalog(currentIndex))
        {
            writer.writeStartElement(tFolder);
            writer.writeAttribute(aName, mDocumentTreeStructureModel->nodeFromIndex(currentIndex)->nodeName());
            saveFoldersTreeToXml(writer, currentIndex);
            writer.writeEndElement();
        }
    }
}

void PersistenceManager::loadFolderTreeFromXml(const QString &path, const QDomElement &element)
{
    QDomElement iterElement = element.firstChildElement();
    while (!iterElement.isNull())
    {
        QString leafPath;
        if (tFolder == iterElement.tagName())
        {
            leafPath = iterElement.attribute(aName);

            if (!leafPath.isEmpty())
            {
                mDocumentTreeStructureModel->goTo(path + "/" + leafPath);
                if (!iterElement.firstChildElement().isNull())
                {
                    loadFolderTreeFromXml(path + "/" + leafPath, iterElement);
                }
            }
        }
        iterElement = iterElement.nextSiblingElement();
    }
}

bool PersistenceManager::mayHaveVideo(DocumentProxy *pDocumentProxy)
{
    QDir videoDir(pDocumentProxy->persistencePath() + "/" + PersistenceManager::videoDirectory);

    return videoDir.exists() && videoDir.entryInfoList().length() > 0;
}

bool PersistenceManager::mayHaveAudio(DocumentProxy *pDocumentProxy)
{
    QDir audioDir(pDocumentProxy->persistencePath() + "/" + PersistenceManager::audioDirectory);

    return audioDir.exists() && audioDir.entryInfoList().length() > 0;
}

bool PersistenceManager::mayHavePDF(DocumentProxy *pDocumentProxy)
{
    QDir objectDir(pDocumentProxy->persistencePath() + "/" + PersistenceManager::objectDirectory);

    QStringList filters;
    filters << "*.pdf";

    return objectDir.exists() && objectDir.entryInfoList(filters).length() > 0;
}

bool PersistenceManager::mayHaveSVGImages(DocumentProxy *pDocumentProxy)
{
    QDir imageDir(pDocumentProxy->persistencePath() + "/" + PersistenceManager::imageDirectory);

    QStringList filters;
    filters << "*.svg";

    return imageDir.exists() && imageDir.entryInfoList(filters).length() > 0;
}

bool PersistenceManager::mayHaveWidget(DocumentProxy *pDocumentProxy)
{
    QDir widgetDir(pDocumentProxy->persistencePath() + "/" + PersistenceManager::widgetDirectory);

    return widgetDir.exists() && widgetDir.entryInfoList(QDir::Dirs).length() > 0;
}
