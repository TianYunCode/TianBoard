#include <QGraphicsItem>
#include <QRegularExpression>
#include <QRegExp>
#include <QPointF>
#include <QtWidgets>

#include "core/Application.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/FeaturesController.h"
#include "settings/Settings.h"
#include "whiteboard/ToolsManager.h"
#include "utils/FileSystemUtils.h"
#include "platform/PlatformUtils.h"

#include "core/DownloadManager.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsSvgItem.h"
#include "whiteboard/GraphicsPixmapItem.h"
#include "whiteboard/GraphicsWidgetItem.h"

#include "ui/FeaturesWidget.h"

const QString FeaturesController::virtualRootName = "root";
const QString FeaturesController::rootPath = "/" + virtualRootName;

const QString FeaturesController::appPath = rootPath + "/Applications";
const QString FeaturesController::audiosPath = rootPath + "/Audios";
const QString FeaturesController::moviesPath = rootPath + "/Movies";
const QString FeaturesController::picturesPath = rootPath + "/Pictures";
const QString FeaturesController::flashPath = rootPath + "/Animations";
const QString FeaturesController::interactPath = rootPath + "/Interactivities";
const QString FeaturesController::shapesPath = rootPath + "/Shapes";
const QString FeaturesController::trashPath = rootPath + "/Trash";
const QString FeaturesController::favoritePath = rootPath + "/Favorites";


void FeaturesComputingThread::scanFS(const QUrl &currentPath, const QString &currVirtualPath,
                                     const QSet<QUrl> &pFavoriteSet) {
    //    Q_ASSERT(QFileInfo(currentPath.toLocalFile()).exists());
    //    if(QFileInfo(currentPath.toLocalFile()).exists())
    //        return;

    QFileInfoList fileInfoList = FileSystemUtils::allElementsInDirectory(currentPath.toLocalFile());

    QFileInfoList::iterator fileInfo;
    for (fileInfo = fileInfoList.begin(); fileInfo != fileInfoList.end(); fileInfo += 1) {
        if (abort) {
            return;
        }

        QString fullFileName = fileInfo->absoluteFilePath();
        FeatureElementType featureType = FeaturesController::fileTypeFromUrl(fullFileName);
        QString fileName = fileInfo->fileName();

        QImage icon = FeaturesController::getIcon(fullFileName, featureType);

        if (fullFileName.contains(".thumbnail."))
            continue;

        Feature testFeature(currVirtualPath + "/" + fileName, icon, fileName,
                            QUrl::fromLocalFile(fullFileName), featureType);

        emit sendFeature(testFeature);
        emit featureSent();
        emit scanPath(fullFileName);

        if (pFavoriteSet.find(QUrl::fromLocalFile(fullFileName)) != pFavoriteSet.end()) {
            //TODO send favoritePath from the controller or make favoritePath public and static
            emit sendFeature(Feature(FeaturesController::favoritePath + "/" + fileName, icon, fileName,
                                     QUrl::fromLocalFile(fullFileName), featureType));
        }

        if (featureType == FEATURE_FOLDER) {
            scanFS(QUrl::fromLocalFile(fullFileName), currVirtualPath + "/" + fileName, pFavoriteSet);
        }
    }
}

void FeaturesComputingThread::scanAll(QList<QPair<QUrl, Feature>> pScanningData,
                                      const QSet<QUrl> &pFavoriteSet) {
    for (int i = 0; i < pScanningData.count(); i++) {
        if (abort) {
            return;
        }
        QPair<QUrl, Feature> curPair = pScanningData.at(i);

        emit scanCategory(curPair.second.getDisplayName());
        scanFS(curPair.first, curPair.second.getFullVirtualPath(), pFavoriteSet);
    }
}

int FeaturesComputingThread::featuresCount(const QUrl &pPath) {
    int noItems = 0;

    QFileInfoList fileInfoList = FileSystemUtils::allElementsInDirectory(pPath.toLocalFile());

    QFileInfoList::iterator fileInfo;
    for (fileInfo = fileInfoList.begin(); fileInfo != fileInfoList.end(); fileInfo += 1) {
        QString fullFileName = fileInfo->absoluteFilePath();
        FeatureElementType featureType = FeaturesController::fileTypeFromUrl(fullFileName);

        if (featureType != FEATURE_INVALID && !fullFileName.contains(".thumbnail.")) {
            noItems++;
        }

        if (featureType == FEATURE_FOLDER) {
            noItems += featuresCount(QUrl::fromLocalFile(fullFileName));
        }
    }

    return noItems;
}

int FeaturesComputingThread::featuresCountAll(QList<QPair<QUrl, Feature>> pScanningData) {
    int noItems = 0;
    for (int i = 0; i < pScanningData.count(); i++) {
        QPair<QUrl, Feature> curPair = pScanningData.at(i);
        noItems += featuresCount(curPair.first);
    }

    return noItems;
}

FeaturesComputingThread::FeaturesComputingThread(QObject *parent) : QThread(parent) {
    restart = false;
    abort = false;
}

void FeaturesComputingThread::compute(const QList<QPair<QUrl, Feature>> &pScanningData,
                                      QSet<QUrl> *pFavoritesSet) {
    QMutexLocker curLocker(&mMutex);

    mScanningData = pScanningData;
    mFavoriteSet = *pFavoritesSet;

    if (!isRunning()) {
        start(LowPriority);
    } else {
        restart = true;
        mWaitCondition.wakeOne();
    }
}

void FeaturesComputingThread::run() {
    forever {
        //        qDebug() << "Custom thread started execution";

        mMutex.lock();
        QList<QPair<QUrl, Feature>> searchData = mScanningData;
        QSet<QUrl> favoriteSet = mFavoriteSet;
        mMutex.unlock();

        if (abort) {
            return;
        }
        if (restart) {
            break;
        }

        //        QTime curTime = QTime::currentTime();
        int fsCnt = featuresCountAll(searchData);
        //        int msecsto = curTime.msecsTo(QTime::currentTime());
        //        qDebug() << "time on evaluation" << msecsto;

        emit maxFilesCountEvaluated(fsCnt);

        emit scanStarted();
        //        curTime = QTime::currentTime();
        scanAll(searchData, favoriteSet);
        //        qDebug() << "Time on finishing" << curTime.msecsTo(QTime::currentTime());
        emit scanFinished();

        mMutex.lock();
        if (!abort) {
            mWaitCondition.wait(&mMutex);
        }
        restart = false;
        mMutex.unlock();
    }
}

FeaturesComputingThread::~FeaturesComputingThread() {
    //    qDebug() <<  "thread destructor catched";

    mMutex.lock();
    abort = true;
    mWaitCondition.wakeOne();
    mMutex.unlock();

    wait();
}

Feature::Feature(const QString &url, const QImage &icon, const QString &name, const QUrl &realPath,
                 FeatureElementType type)
    : mThumbnail(icon), mDisplayName(name), mPath(realPath), elementType(type) {
    mName = getNameFromVirtualPath(url);
    virtualDir = getVirtualDirFromVirtualPath(url);
}

Feature::~Feature() {}

QString Feature::getNameFromVirtualPath(const QString &pVirtPath) {
    QString result;
    int slashPos = pVirtPath.lastIndexOf("/");
    if (slashPos != -1) {
        result = pVirtPath.right(pVirtPath.count() - slashPos - 1);
    } else {
        qDebug() << "Feature: incorrect virtual path parameter specified";
    }

    return result;
}

QString Feature::getVirtualDirFromVirtualPath(const QString &pVirtPath) {
    QString result;
    int slashPos = pVirtPath.lastIndexOf("/");
    if (slashPos != -1) {
        result = pVirtPath.left(slashPos);
    } else {
        qDebug() << "Feature: incorrect virtual path parameter specified";
    }

    return result;
}

QString Feature::getUrl() const {
    if (elementType == FEATURE_INTERNAL)
        return getFullPath().toString();

    return getFullPath().toLocalFile();
}

bool Feature::operator==(const Feature &f) const {
    return virtualDir == f.getVirtualPath() && mName == f.getName() && mPath == f.getFullPath() &&
           elementType == f.getType();
}

bool Feature::operator!=(const Feature &f) const {
    return !(*this == f);
}

bool Feature::isFolder() const {
    return elementType == FEATURE_CATEGORY || elementType == FEATURE_TRASH ||
           elementType == FEATURE_FAVORITE || elementType == FEATURE_FOLDER || elementType == FEATURE_SEARCH;
}

bool Feature::allowedCopy() const {
    return isFolder() && elementType != FEATURE_CATEGORY && elementType != FEATURE_SEARCH;
}

bool Feature::isDeletable() const {
    return elementType == FEATURE_ITEM || elementType == FEATURE_AUDIO || elementType == FEATURE_VIDEO ||
           elementType == FEATURE_IMAGE || elementType == FEATURE_FLASH ||
           elementType == FEATURE_FOLDER
           // TODO: 这是临时补丁，库条目的权限机制需要重新设计
           || getVirtualPath().startsWith("/root/Applications/Web");
}

bool Feature::inTrash() const {
    return getFullPath().toLocalFile().startsWith(
        QUrl::fromLocalFile(Settings::userTrashDirPath()).toLocalFile());
}

FeaturesController::FeaturesController(QWidget *pParentWidget)
    : QObject(pParentWidget), featuresList(0), mLastItemOffsetIndex(0) {
    //Initializing physical directories from Settings
    mUserAudioDirectoryPath = QUrl::fromLocalFile(Settings::settings()->userAudioDirectory());
    mUserVideoDirectoryPath = QUrl::fromLocalFile(Settings::settings()->userVideoDirectory());
    mUserPicturesDirectoryPath = QUrl::fromLocalFile(Settings::settings()->userImageDirectory());
    mUserInteractiveDirectoryPath = QUrl::fromLocalFile(Settings::settings()->userInteractiveDirectory());
    mUserAnimationDirectoryPath = QUrl::fromLocalFile(Settings::settings()->userAnimationDirectory());

    mLibPicturesDirectoryPath = QUrl::fromLocalFile(Settings::settings()->applicationImageLibraryDirectory());
    mLibAudiosDirectoryPath = QUrl::fromLocalFile(Settings::settings()->applicationAudiosLibraryDirectory());
    mLibVideosDirectoryPath = QUrl::fromLocalFile(Settings::settings()->applicationVideosLibraryDirectory());
    //mLibInteractiveDirectoryPath = QUrl::fromLocalFile(Settings::settings()->applicationInteractivesDirectory());
    mLibShapesDirectoryPath = QUrl::fromLocalFile(Settings::settings()->applicationShapeLibraryDirectory());
    trashDirectoryPath = QUrl::fromLocalFile(Settings::userTrashDirPath());

    rootElement = Feature(rootPath, QImage(":images/libpalette/home.png"), "root", QUrl());
    audiosElement = Feature(audiosPath, QImage(":images/libpalette/AudiosCategory.svg"), tr("Audios"),
                            mUserAudioDirectoryPath, FEATURE_CATEGORY);
    moviesElement = Feature(moviesPath, QImage(":images/libpalette/MoviesCategory.svg"), tr("Movies"),
                            mUserVideoDirectoryPath, FEATURE_CATEGORY);
    picturesElement = Feature(picturesPath, QImage(":images/libpalette/PicturesCategory.svg"), tr("Pictures"),
                              mUserPicturesDirectoryPath, FEATURE_CATEGORY);
    //interactElement = Feature( interactPath, QImage(":images/libpalette/InteractivesCategory.svg"), tr("Interactivities") ,  mLibInteractiveDirectoryPath, FEATURE_CATEGORY);
    applicationsElement = Feature(appPath, QImage(":images/libpalette/ApplicationsCategory.svg"),
                                  tr("Applications"), mUserInteractiveDirectoryPath, FEATURE_CATEGORY);
    shapesElement = Feature(shapesPath, QImage(":images/libpalette/ShapesCategory.svg"), tr("Shapes"),
                            mLibShapesDirectoryPath, FEATURE_CATEGORY);
    favoriteElement = Feature(favoritePath, QImage(":images/libpalette/FavoritesCategory.svg"),
                              tr("Favorites"), QUrl("favorites"), FEATURE_FAVORITE);

    trashElement = Feature(trashPath, QImage(":images/libpalette/TrashCategory.svg"), tr("Trash"),
                           trashDirectoryPath, FEATURE_TRASH);

    featuresList = new QList<Feature>();

    scanFS();

    featuresModel = new FeaturesModel(featuresList, this);
    //featuresModel->setSupportedDragActions(Qt::CopyAction | Qt::MoveAction);

    featuresProxyModel = new FeaturesProxyModel(this);
    featuresProxyModel->setFilterFixedString(rootPath);
    featuresProxyModel->setSourceModel(featuresModel);
    featuresProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    featuresSearchModel = new FeaturesSearchProxyModel(this);
    featuresSearchModel->setSourceModel(featuresModel);
    featuresSearchModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    featuresPathModel = new FeaturesPathProxyModel(this);
    featuresPathModel->setPath(rootPath);
    featuresPathModel->setSourceModel(featuresModel);

    connect(featuresModel, &FeaturesModel::dataRestructured, featuresProxyModel,
            &QSortFilterProxyModel::invalidate);
    connect(&mCThread, &FeaturesComputingThread::sendFeature, featuresModel, &FeaturesModel::addItem);
    connect(&mCThread, &FeaturesComputingThread::featureSent, this,
            &FeaturesController::featureAddedFromThread);
    connect(&mCThread, &FeaturesComputingThread::scanStarted, this, &FeaturesController::scanStarted);
    connect(&mCThread, &FeaturesComputingThread::scanFinished, this, &FeaturesController::scanFinished);
    connect(&mCThread, &FeaturesComputingThread::maxFilesCountEvaluated, this,
            &FeaturesController::maxFilesCountEvaluated);
    connect(&mCThread, &FeaturesComputingThread::scanCategory, this, &FeaturesController::scanCategory);
    connect(&mCThread, &FeaturesComputingThread::scanPath, this, &FeaturesController::scanPath);

    QTimer::singleShot(0, this, &FeaturesController::startThread);
}

void FeaturesController::startThread() {
    QList<QPair<QUrl, Feature>> computingData;

    computingData << QPair<QUrl, Feature>(mLibAudiosDirectoryPath, audiosElement)
                  << QPair<QUrl, Feature>(mLibVideosDirectoryPath, moviesElement)
                  << QPair<QUrl, Feature>(mLibPicturesDirectoryPath, picturesElement)

                  << QPair<QUrl, Feature>(mUserInteractiveDirectoryPath, applicationsElement)
                  << QPair<QUrl, Feature>(mUserAudioDirectoryPath, audiosElement)
                  << QPair<QUrl, Feature>(mUserPicturesDirectoryPath, picturesElement)
                  << QPair<QUrl, Feature>(mUserVideoDirectoryPath, moviesElement)

                  << QPair<QUrl, Feature>(mLibShapesDirectoryPath, shapesElement)
                  //<<  QPair<QUrl, Feature>(mLibInteractiveDirectoryPath, interactElement)
                  << QPair<QUrl, Feature>(trashDirectoryPath, trashElement);

    mCThread.compute(computingData, favoriteSet);
}

void FeaturesController::scanFS() {
    featuresList->clear();
    featuresList->append(rootElement);

    *featuresList << audiosElement << moviesElement
                  << picturesElement
                  //<< interactElement
                  << applicationsElement << shapesElement << favoriteElement << trashElement;

    //filling favoriteList
    loadFavoriteList();

    QList<ToolsManager::ToolDescriptor> tools = ToolsManager::manager()->allTools();

    foreach (ToolsManager::ToolDescriptor tool, tools) {
        featuresList->append(Feature(appPath + "/" + tool.label, tool.icon.toImage(), tool.label,
                                     QUrl(tool.id), FEATURE_INTERNAL));
        if (favoriteSet->find(QUrl(tool.id)) != favoriteSet->end()) {
            featuresList->append(Feature(favoritePath + "/" + tool.label, tool.icon.toImage(), tool.label,
                                         QUrl(tool.id), FEATURE_INTERNAL));
        }
    }
}
void FeaturesController::fileSystemScan(const QUrl &currentPath, const QString &currVirtualPath) {
    QFileInfoList fileInfoList = FileSystemUtils::allElementsInDirectory(currentPath.toLocalFile());

    QFileInfoList::iterator fileInfo;
    for (fileInfo = fileInfoList.begin(); fileInfo != fileInfoList.end(); fileInfo += 1) {
        QString fullFileName = fileInfo->absoluteFilePath();
        FeatureElementType featureType = fileTypeFromUrl(fullFileName);
        QString fileName = fileInfo->fileName();

        QImage icon = getIcon(fullFileName, featureType);

        if (fullFileName.contains(".thumbnail."))
            continue;

        Feature testFeature(currVirtualPath + "/" + fileName, icon, fileName,
                            QUrl::fromLocalFile(fullFileName), featureType);

        featuresList->append(testFeature);

        if (favoriteSet->find(QUrl::fromLocalFile(fullFileName)) != favoriteSet->end()) {
            featuresList->append(Feature(favoritePath + "/" + fileName, icon, fileName,
                                         QUrl::fromLocalFile(fullFileName), featureType));
        }

        if (featureType == FEATURE_FOLDER) {
            fileSystemScan(QUrl::fromLocalFile(fullFileName), currVirtualPath + "/" + fileName);
        }
    }
}

int FeaturesController::featuresCount(const QUrl &currPath) {
    int noItems = 0;

    QFileInfoList fileInfoList = FileSystemUtils::allElementsInDirectory(currPath.toLocalFile());

    QFileInfoList::iterator fileInfo;
    for (fileInfo = fileInfoList.begin(); fileInfo != fileInfoList.end(); fileInfo += 1) {
        QString fullFileName = fileInfo->absoluteFilePath();
        FeatureElementType featureType = fileTypeFromUrl(fullFileName);

        if (featureType != FEATURE_INVALID && !fullFileName.contains(".thumbnail.")) {
            noItems++;
        } else {
            continue;
        }

        if (featureType == FEATURE_FOLDER) {
            noItems += featuresCount(QUrl::fromLocalFile(fullFileName));
        }
    }

    return noItems;
}

void FeaturesController::loadFavoriteList() {
    favoriteSet = new QSet<QUrl>();
    QFile file(Settings::userDataDirectory() + "/favorites.dat");
    if (file.exists()) {
        file.open(QIODevice::ReadOnly);
        QDataStream in(&file);
        int elementsNumber;
        in >> elementsNumber;
        for (int i = 0; i < elementsNumber; ++i) {
            QUrl path;
            in >> path;
            favoriteSet->insert(path);
        }
    }
}

void FeaturesController::saveFavoriteList() {
    QFile file(Settings::userDataDirectory() + "/favorites.dat");
    file.resize(0);
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);
    out << favoriteSet->size();
    for (QSet<QUrl>::iterator it = favoriteSet->begin(); it != favoriteSet->end(); ++it) {
        out << (*it);
    }
    file.close();
}

QString FeaturesController::uniqNameForFeature(const Feature &feature, const QString &pName,
                                               const QString &pExtention) const {
    Q_ASSERT(featuresList);

    QStringList resultList;
    QString parentVirtualPath = feature.getFullVirtualPath();
    QString resultName;

    qDebug() << "start";
    foreach (Feature curFeature, *featuresList) {
        if (curFeature.getFullVirtualPath().startsWith(feature.getFullVirtualPath())) {
            QString curResultName = curFeature.getFullVirtualPath();
            if (!parentVirtualPath.endsWith("/")) {
                parentVirtualPath.append("/");
            }
            //Cut virtual path prevfix
            int i = curResultName.indexOf(feature.getFullVirtualPath());
            if (i != -1) {
                curResultName = curResultName.right(curFeature.getFullVirtualPath().count() - i -
                                                    parentVirtualPath.count());
            }
            //if directory has children, emptying the name;
            i = curResultName.indexOf("/");
            if (i != -1) {
                curResultName = "";
            }

            if (!curResultName.isEmpty()) {
                resultList.append(curResultName);
            }

            qDebug() << curResultName;
        }
    }

    if (!resultList.contains(pName + pExtention, Qt::CaseInsensitive)) {
        resultName = pName + pExtention;

    } else {
        for (int i = 0; i < 16777215; i++) {
            QString probeName = pName + "_" + QString::number(i) + pExtention;
            if (!resultList.contains(probeName, Qt::CaseInsensitive)) {
                resultName = probeName;
                break;
            }
        }
    }
    qDebug() << "result name is " << resultName;

    return resultName;
}

QString FeaturesController::adjustName(const QString &str) {
    if (str.isNull()) {
        return QString();
    }

    QString resultStr = str;
    QRegularExpression invalidSymbols("[/\\s:?*|<>\"]+");

    return resultStr.replace(invalidSymbols, "_");
}

void FeaturesController::addToFavorite(const QUrl &path) {
    QString filePath = fileNameFromUrl(path);
    if (favoriteSet->find(path) == favoriteSet->end()) {
        QFileInfo fileInfo(filePath);
        QString fileName = fileInfo.fileName();
        FeatureElementType type = fileTypeFromUrl(filePath);
        Feature elem(favoritePath + "/" + fileName, getIcon(filePath, type), fileName, path,
                     fileTypeFromUrl(filePath));
        favoriteSet->insert(path);
        saveFavoriteList();

        if (!elem.getVirtualPath().isEmpty() && !elem.getVirtualPath().isNull())
            featuresModel->addItem(elem);
    }
}

void FeaturesController::removeFromFavorite(const QUrl &path, bool deleteManualy) {
    if (favoriteSet->find(path) != favoriteSet->end()) {
        favoriteSet->erase(favoriteSet->find(path));
        saveFavoriteList();
    }

    if (deleteManualy) {
        featuresModel->deleteFavoriteItem(path.toString());
    }
}

QString FeaturesController::fileNameFromUrl(const QUrl &url) {
    QString fileName = url.toString();
    if (fileName.contains("tianboardtool://"))
        return fileName;
    return url.toLocalFile();
}


FeatureElementType FeaturesController::fileTypeFromUrl(const QString &path) {
    QFileInfo fileInfo(path);

    if (path.contains("tianboardtool://"))
        return FEATURE_INTERNAL;

    if (!fileInfo.exists()) {
        return FEATURE_INVALID;
    }

    FeatureElementType fileType = FEATURE_INVALID;
    QString fileName = fileInfo.fileName();
    QString mimeString = FileSystemUtils::mimeTypeFromFileName(fileName);

    if (mimeString.contains("application")) {
        if (mimeString.contains("application/search")) {
            fileType = FEATURE_SEARCH;
        } else if (mimeString.contains("application/x-shockwave-flash")) {
            fileType = FEATURE_FLASH;
        } else {
            fileType = FEATURE_INTERACTIVE;
        }
    } else if (mimeString.contains("audio")) {
        fileType = FEATURE_AUDIO;
    } else if (mimeString.contains("video")) {
        fileType = FEATURE_VIDEO;
    } else if (mimeString.contains("image")) {
        fileType = FEATURE_IMAGE;
    } else if (fileInfo.isDir()) {
        fileType = FEATURE_FOLDER;
    } else {
        fileType = FEATURE_INVALID;
    }

    return fileType;
}

QImage FeaturesController::getIcon(const QString &path, FeatureElementType pFType = FEATURE_INVALID) {
    if (pFType == FEATURE_FOLDER) {
        return QImage(":images/libpalette/folder.svg");
    } else if (pFType == FEATURE_INTERACTIVE || pFType == FEATURE_SEARCH) {
        return QImage(GraphicsWidgetItem::iconFilePath(QUrl::fromLocalFile(path)));
    } else if (pFType == FEATURE_INTERNAL) {
        return QImage(ToolsManager::manager()->iconFromToolId(path));
    } else if (pFType == FEATURE_FLASH) {
        return QImage(":images/libpalette/FlashIcon.svg");
    } else if (pFType == FEATURE_AUDIO) {
        return QImage(":images/libpalette/soundIcon.svg");
    } else if (pFType == FEATURE_VIDEO) {
        return QImage(":images/libpalette/movieIcon.svg");
    } else if (pFType == FEATURE_IMAGE) {
        QImage pix(path);
        if (pix.isNull()) {
            pix = QImage(":images/libpalette/notFound.png");
        } else {
            pix = pix.scaledToWidth(qMin(Settings::maxThumbnailWidth, pix.width()));
        }
        return pix;
    }

    return QImage(":images/libpalette/notFound.png");
}

bool FeaturesController::isDeletable(const QUrl &url) {
    FeatureElementType type = fileTypeFromUrl(fileNameFromUrl(url));
    return type == FEATURE_AUDIO || type == FEATURE_VIDEO || type == FEATURE_IMAGE || type == FEATURE_ITEM;
}

QString FeaturesController::categoryNameForVirtualPath(const QString &str) {
    QString result;
    int ind = str.lastIndexOf("/");
    if (ind != -1) {
        result = str.right(str.count() - ind - 1);
    }
    return result;
}

QImage FeaturesController::createThumbnail(const QString &path) {
    QString thumbnailPath = path;
    QString mimetype = FileSystemUtils::mimeTypeFromFileName(path);

    if (mimetype.contains("audio")) {
        thumbnailPath = ":images/libpalette/soundIcon.svg";
    } else if (mimetype.contains("video")) {
        thumbnailPath = ":images/libpalette/movieIcon.svg";
    } else {
        QImage pix(path);
        if (!pix.isNull()) {
            pix = pix.scaledToWidth(qMin(Settings::maxThumbnailWidth, pix.width()), Qt::SmoothTransformation);
            return pix;

        } else {
            thumbnailPath = ":images/libpalette/notFound.png";
        }
    }

    return QImage(thumbnailPath);
}

void FeaturesController::importImage(const QImage &image, const QString &fileName) {
    importImage(image, currentElement, fileName);
}


void FeaturesController::importImage(const QImage &image, const Feature &destination,
                                     const QString &fileName) {
    QString mFileName = fileName;
    QString filePath;
    Feature dest = destination;
    if (mFileName.isNull()) {
        QDateTime now = QDateTime::currentDateTime();
        static int imageCounter = 0;
        mFileName = tr("ImportedImage") + "-" + now.toString("dd-MM-yyyy hh-mm-ss");

        filePath = dest.getFullPath().toLocalFile() + "/" + mFileName;

        if (QFile::exists(filePath + ".png"))
            mFileName += QString("-[%1]").arg(++imageCounter);
        else
            imageCounter = 0;

        mFileName += ".png";
    }


    if (!destination.getFullVirtualPath().startsWith(picturesElement.getFullVirtualPath(),
                                                     Qt::CaseInsensitive)) {
        dest = picturesElement;
    }

    filePath = dest.getFullPath().toLocalFile() + "/" + mFileName;
    image.save(filePath);

    QImage thumb = createThumbnail(filePath);
    Feature resultItem = Feature(dest.getFullVirtualPath() + "/" + mFileName, thumb, mFileName,
                                 QUrl::fromLocalFile(filePath), FEATURE_ITEM);

    featuresModel->addItem(resultItem);
}

QStringList FeaturesController::getFileNamesInFolders() {
    QStringList strList;

    Q_ASSERT(curListModel);

    for (int i = 0; i < curListModel->rowCount(QModelIndex()); i++) {
        QModelIndex ind = curListModel->index(i, 0);
        if (!ind.isValid()) {
            qDebug() << "incorrect model index catched";
            continue;
        }
        Feature curFeature = curListModel->data(ind, Qt::UserRole + 1).value<Feature>();
        if (curFeature.getType() == FEATURE_FOLDER) {
            strList << QFileInfo(curFeature.getFullPath().toLocalFile()).fileName();
        }
    }

    return strList;
}

void FeaturesController::addNewFolder(QString name) {
    QString path = currentElement.getFullPath().toLocalFile() + "/" + name;

    if ("/root" == currentElement.getFullVirtualPath()) {
        return;
    }

    if (!QFileInfo(path).exists()) {
        QDir().mkpath(path);
    }
    Feature newFeatureFolder =
        Feature(currentElement.getFullVirtualPath() + "/" + name, QImage(":images/libpalette/folder.svg"),
                name, QUrl::fromLocalFile(path), FEATURE_FOLDER);

    featuresModel->addItem(newFeatureFolder);
    featuresProxyModel->invalidate();
}

void FeaturesController::addItemToPage(const Feature &item) {
    Application::boardController->downloadURL(item.getFullPath());
}

void FeaturesController::addItemAsBackground(const Feature &item) {
    Application::boardController->downloadURL(item.getFullPath(), QString(), QPointF(), QSize(), true);
}

Feature FeaturesController::getDestinationFeatureForUrl(const QUrl &url) {
    QString mimetype = FileSystemUtils::mimeTypeFromFileName(url.toString());
    return getDestinationFeatureForMimeType(mimetype);
}

Feature FeaturesController::getDestinationFeatureForMimeType(const QString &pMmimeType) {
    if (pMmimeType.contains("audio"))
        return audiosElement;
    if (pMmimeType.contains("video"))
        return moviesElement;
    else if (pMmimeType.contains("image") || pMmimeType.isEmpty())
        return picturesElement;
    //else if ( pMmimeType.contains("application") )
    //{
    //    return interactElement;
    //}
    return Feature();
}

void FeaturesController::addDownloadedFile(const QUrl &sourceUrl, const QByteArray &pData,
                                           const QString pContentSource, const QString pTitle) {
    Feature dest = getDestinationFeatureForMimeType(pContentSource);

    if (dest == Feature())
        return;

    QString fileName;
    QString filePath;

    //Audio item
    if (dest == picturesElement) {
        QString UniqName = uniqNameForFeature(dest, adjustName(pTitle), ".jpg");
        fileName = !UniqName.isNull()
                       ? UniqName
                       : tr("ImportedImage") + "-" +
                             QDateTime::currentDateTime().toString("dd-MM-yyyy hh-mm-ss") + ".jpg";

        filePath = dest.getFullPath().toLocalFile() + "/" + fileName;

        QImage::fromData(pData).save(filePath);

        Feature downloadedFeature =
            Feature(dest.getFullVirtualPath() + "/" + fileName, getIcon(filePath, fileTypeFromUrl(filePath)),
                    fileName, QUrl::fromLocalFile(filePath), FEATURE_ITEM);
        if (downloadedFeature != Feature()) {
            featuresModel->addItem(downloadedFeature);
        }

    } else {
        fileName = QFileInfo(sourceUrl.toString()).fileName();
        filePath = dest.getFullPath().toLocalFile() + "/" + fileName;

        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(pData);
            file.close();

            Feature downloadedFeature = Feature(dest.getFullVirtualPath() + "/" + fileName,
                                                getIcon(filePath, fileTypeFromUrl(filePath)), fileName,
                                                QUrl::fromLocalFile(filePath), FEATURE_ITEM);
            if (downloadedFeature != Feature()) {
                featuresModel->addItem(downloadedFeature);
            }
        }
    }
}

Feature FeaturesController::moveItemToFolder(const QUrl &url, const Feature &destination) {
    /*Feature newElement = copyItemToFolder( url, destination );
    deleteItem( url );
    return newElement;*/
    QString sourcePath = url.toLocalFile();

    Q_ASSERT(QFileInfo(sourcePath).exists());

    Feature possibleDest = getDestinationFeatureForUrl(url);

    Feature dest = destination;

    if (destination != trashElement && !destination.getFullVirtualPath().startsWith(
                                           possibleDest.getFullVirtualPath(), Qt::CaseInsensitive)) {
        dest = possibleDest;
    }

    QString name = QFileInfo(sourcePath).fileName();
    QString destPath = dest.getFullPath().toLocalFile();
    QString destVirtualPath = dest.getFullVirtualPath();
    QString newFullPath = destPath + "/" + name;
    if (sourcePath.compare(newFullPath, Qt::CaseInsensitive)) {
        QFile(sourcePath).copy(newFullPath);
        deleteItem(url);
    }

    QImage thumb = getIcon(newFullPath);

    FeatureElementType type = FEATURE_ITEM;
    if (FileSystemUtils::mimeTypeFromFileName(newFullPath).contains("application"))
        type = FEATURE_INTERACTIVE;
    Feature newElement(destVirtualPath + "/" + name, thumb, name, QUrl::fromLocalFile(newFullPath), type);
    return newElement;
}

void FeaturesController::rescanModel() {
    featuresModel->removeRows(0, featuresList->count());

    //Could implement infolder scanning for better perfomance
    scanFS();
    refreshModels();
}

void FeaturesController::siftElements(const QString &pSiftValue) {
    featuresProxyModel->setFilterFixedString(pSiftValue);
    featuresProxyModel->invalidate();

    featuresPathModel->setPath(pSiftValue);
    featuresPathModel->invalidate();
}

Feature FeaturesController::getFeature(const QModelIndex &index, const QString &listName) {
    //    QSortFilterProxyModel *model = qobject_cast<QSortFilterProxyModel *>(pOnView->model());
    QAbstractItemModel *model = 0;
    if (listName == FeaturesWidget::objNamePathList) {
        model = featuresPathModel;
    } else if (listName == FeaturesWidget::objNameFeatureList) {
        model = curListModel;
    }

    if (model) {
        return model->data(index, Qt::UserRole + 1).value<Feature>();
    }

    return Feature();


    //    return pOnView->model()->data(index, Qt::UserRole + 1).value<Feature>();  /*featuresSearchModel->data(index, Qt::UserRole + 1).value<Feature>()*/;
}

void FeaturesController::searchStarted(const QString &pattern, QListView *pOnView) {
    if (pattern.isEmpty()) {
        pOnView->setModel(featuresProxyModel);
        featuresProxyModel->invalidate();
        curListModel = featuresProxyModel;
    } else if (pattern.size() > 1) {
        //        featuresSearchModel->setFilterPrefix(currentElement.getFullVirtualPath());
        featuresSearchModel->setFilterWildcard("*" + pattern + "*");
        pOnView->setModel(featuresSearchModel);
        featuresSearchModel->invalidate();
        curListModel = featuresSearchModel;
    }
}

void FeaturesController::refreshModels() {
    featuresProxyModel->invalidate();
    featuresSearchModel->invalidate();
    featuresPathModel->invalidate();
}

Feature FeaturesController::copyItemToFolder(const QUrl &url, const Feature &destination) {
    QString sourcePath = url.toLocalFile();

    Q_ASSERT(QFileInfo(sourcePath).exists());

    Feature possibleDest = getDestinationFeatureForUrl(url);

    Feature dest = destination;

    if (destination != trashElement && !destination.getFullVirtualPath().startsWith(
                                           possibleDest.getFullVirtualPath(), Qt::CaseInsensitive)) {
        dest = possibleDest;
    }

    QString name = QFileInfo(sourcePath).fileName();
    QString destPath = dest.getFullPath().toLocalFile();
    QString destVirtualPath = dest.getFullVirtualPath();
    QString newFullPath = destPath + "/" + name;
    if (!sourcePath.compare(newFullPath, Qt::CaseInsensitive))
        QFile(sourcePath).copy(newFullPath);

    QImage thumb = getIcon(newFullPath);

    FeatureElementType type = FEATURE_ITEM;
    if (FileSystemUtils::mimeTypeFromFileName(newFullPath).contains("application"))
        type = FEATURE_INTERACTIVE;
    Feature newElement(destVirtualPath + "/" + name, thumb, name, QUrl::fromLocalFile(newFullPath), type);
    return newElement;
}

void FeaturesController::moveExternalData(const QUrl &url, const Feature &destination) {
    QString sourcePath = url.toLocalFile();

    Q_ASSERT(QFileInfo(sourcePath).exists());

    Feature possibleDest = getDestinationFeatureForUrl(url);

    Feature dest = destination;

    if (destination != trashElement && destination != Feature()
        /*&& !destination.getFullVirtualPath().startsWith( possibleDest.getFullVirtualPath(), Qt::CaseInsensitive )*/) {
        dest = possibleDest;
    }

    FeatureElementType type = fileTypeFromUrl(sourcePath);

    if (type == FEATURE_FOLDER) {
        return;
    }

    QString name = QFileInfo(sourcePath).fileName();
    QString destPath = dest.getFullPath().toLocalFile();
    QString destVirtualPath = dest.getFullVirtualPath();
    QString newFullPath = destPath + "/" + name;

    if (!sourcePath.compare(newFullPath, Qt::CaseInsensitive) ||
        !FileSystemUtils::copy(sourcePath, newFullPath)) {
        return;
    }

    Q_ASSERT(QFileInfo(newFullPath).exists());

    QImage thumb = getIcon(newFullPath, type);
    Feature newElement(destVirtualPath + "/" + name, thumb, name, QUrl::fromLocalFile(newFullPath), type);

    featuresModel->addItem(newElement);
}

void FeaturesController::deleteItem(const QUrl &url) {
    QString path = url.toLocalFile();
    Q_ASSERT(QFileInfo(path).exists());

    QString thumbnailPath = FileSystemUtils::thumbnailPath(path);
    if (thumbnailPath.length() && QFileInfo(thumbnailPath).exists()) {
        if (QFileInfo(thumbnailPath).isFile()) {
            QFile::remove(thumbnailPath);
        } else if (QFileInfo(thumbnailPath).isDir()) {
            if (!FileSystemUtils::deleteDir(thumbnailPath)) {
                qDebug() << "Not able to delete directory";
            }
        }
    }
    if (QFileInfo(path).isFile()) {
        QFile::remove(path);

    } else if (QFileInfo(path).isDir()) {
        FileSystemUtils::deleteDir(path);
    }
}

void FeaturesController::deleteItem(const Feature &pFeature) {
    QUrl sourceUrl = pFeature.getFullPath();
    featuresModel->deleteItem(pFeature);
    deleteItem(sourceUrl);
}

bool FeaturesController::isTrash(const QUrl &url) {
    return url.toLocalFile().startsWith(trashDirectoryPath.toLocalFile());
}
void FeaturesController::moveToTrash(Feature feature, bool deleteManualy) {
    featuresModel->moveData(feature, trashElement, Qt::MoveAction, deleteManualy);
    removeFromFavorite(feature.getFullPath());
    featuresModel->deleteFavoriteItem(FeaturesController::fileNameFromUrl(feature.getFullPath()));
}

FeaturesController::~FeaturesController() {
    if (featuresList) {
        delete featuresList;
    }
}

void FeaturesController::assignFeaturesListView(FeaturesListView *pList) {
    pList->setDragDropMode(QAbstractItemView::DragDrop);
    pList->setSelectionMode(QAbstractItemView::ContiguousSelection);

    pList->setResizeMode(QListView::Adjust);
    pList->setViewMode(QListView::IconMode);

    pList->setIconSize(QSize(FeaturesWidget::defaultThumbnailSize, FeaturesWidget::defaultThumbnailSize));
    pList->setGridSize(
        QSize(FeaturesWidget::defaultThumbnailSize + 20, FeaturesWidget::defaultThumbnailSize + 20));

    itemDelegate = new FeaturesItemDelegate(this, pList);
    pList->setItemDelegate(itemDelegate);

    pList->setModel(featuresProxyModel);
    curListModel = featuresProxyModel;
}

void FeaturesController::assignPathListView(FeaturesListView *pList) {
    pList->setViewMode(QListView::IconMode);
    pList->setIconSize(
        QSize(FeaturesWidget::defaultThumbnailSize - 10, FeaturesWidget::defaultThumbnailSize - 10));
    pList->setGridSize(
        QSize(FeaturesWidget::defaultThumbnailSize + 0, FeaturesWidget::defaultThumbnailSize - 10));
    pList->setFixedHeight(60);
    pList->setSelectionMode(QAbstractItemView::NoSelection);
    pList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    pList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    pList->setFlow(QListView::LeftToRight);
    pList->setWrapping(false);
    pList->setDragDropMode(QAbstractItemView::DropOnly);

    pList->setModel(featuresPathModel);

    pathItemDelegate = new FeaturesPathItemDelegate(this);
    pList->setItemDelegate(pathItemDelegate);
}
