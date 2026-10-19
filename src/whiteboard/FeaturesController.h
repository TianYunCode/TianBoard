#pragma once

#include <QMetaType>
#include <QObject>
#include <QWidget>
#include <QSet>
#include <QVector>
#include <QString>
#include <QPixmap>
#include <QMap>
#include <QUrl>
#include <QByteArray>
#include <QtWidgets>
#include <QImage>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QListView>

class FeaturesModel;
class FeaturesItemDelegate;
class FeaturesPathItemDelegate;
class FeaturesProxyModel;
class FeaturesSearchProxyModel;
class FeaturesPathProxyModel;
class FeaturesListView;
class Feature;

class FeaturesComputingThread : public QThread {
    Q_OBJECT
public:
    explicit FeaturesComputingThread(QObject *parent = 0);
    virtual ~FeaturesComputingThread();
    void compute(const QList<QPair<QUrl, Feature>> &pScanningData, QSet<QUrl> *pFavoritesSet);

protected:
    void run();

signals:
    void sendFeature(Feature pFeature);
    void featureSent();
    void scanStarted();
    void scanFinished();
    void maxFilesCountEvaluated(int max);
    void scanCategory(const QString &str);
    void scanPath(const QString &str);

private:
    void scanFS(const QUrl &currentPath, const QString &currVirtualPath, const QSet<QUrl> &pFavoriteSet);
    void scanAll(QList<QPair<QUrl, Feature>> pScanningData, const QSet<QUrl> &pFavoriteSet);
    int featuresCount(const QUrl &pPath);
    int featuresCountAll(QList<QPair<QUrl, Feature>> pScanningData);

private:
    QMutex mMutex;
    QWaitCondition mWaitCondition;
    QUrl mScanningPath;
    QString mScanningVirtualPath;
    QList<QPair<QUrl, Feature>> mScanningData;
    QSet<QUrl> mFavoriteSet;
    bool restart;
    bool abort;
};


enum FeatureElementType {
    FEATURE_CATEGORY,
    FEATURE_VIRTUALFOLDER,
    FEATURE_FOLDER,
    FEATURE_INTERACTIVE,
    FEATURE_INTERNAL,
    FEATURE_ITEM,
    FEATURE_AUDIO,
    FEATURE_VIDEO,
    FEATURE_IMAGE,
    FEATURE_FLASH,
    FEATURE_TRASH,
    FEATURE_FAVORITE,
    FEATURE_SEARCH,
    FEATURE_INVALID
};

class Feature {
public:
    Feature() { ; }
    Feature(const QString &url, const QImage &icon, const QString &name, const QUrl &realPath,
            FeatureElementType type = FEATURE_CATEGORY);
    virtual ~Feature();
    QString getName() const { return mName; }
    QString getDisplayName() const { return mDisplayName; }
    QImage getThumbnail() const { return mThumbnail; }
    QString getVirtualPath() const { return virtualDir; }
    QUrl getFullPath() const { return mPath; }
    QString getFullVirtualPath() const { return virtualDir + "/" + mName; }
    QString getUrl() const;
    void setFullPath(const QUrl &newPath) { mPath = newPath; }
    void setFullVirtualPath(const QString &newVirtualPath) { virtualDir = newVirtualPath; }
    FeatureElementType getType() const { return elementType; }

    bool isFolder() const;
    bool allowedCopy() const;
    bool isDeletable() const;
    bool inTrash() const;
    bool operator==(const Feature &f) const;
    bool operator!=(const Feature &f) const;
    const QMap<QString, QString> &getMetadata() const { return metadata; }
    void setMetadata(const QMap<QString, QString> &data) { metadata = data; }

private:
    QString getNameFromVirtualPath(const QString &pVirtPath);
    QString getVirtualDirFromVirtualPath(const QString &pVirtPath);

private:
    QString virtualDir;
    QString virtualPath;
    QImage mThumbnail;
    QString mName;
    QString mDisplayName;
    QUrl mPath;
    FeatureElementType elementType;
    QMap<QString, QString> metadata;
};
Q_DECLARE_METATYPE(Feature)

class FeaturesController : public QObject {
    friend class FeaturesWidget;

    Q_OBJECT

public:
    FeaturesController(QWidget *parentWidget);
    virtual ~FeaturesController();

    QList<Feature> *getFeatures() const { return featuresList; }

    const QString &getRootPath() const { return rootPath; }
    void scanFS();

    void addItemToPage(const Feature &item);
    void addItemAsBackground(const Feature &item);
    const Feature &getCurrentElement() const { return currentElement; }
    void setCurrentElement(const Feature &elem) { currentElement = elem; }
    const Feature &getTrashElement() const { return trashElement; }

    void addDownloadedFile(const QUrl &sourceUrl, const QByteArray &pData, const QString pContentSource,
                           const QString pTitle);

    Feature moveItemToFolder(const QUrl &url, const Feature &destination);
    Feature copyItemToFolder(const QUrl &url, const Feature &destination);
    void moveExternalData(const QUrl &url, const Feature &destination);

    void rescanModel();
    void siftElements(const QString &pSiftValue);
    //TODO make less complicated for betteer maintainence
    Feature getFeature(const QModelIndex &index, const QString &listName);
    void searchStarted(const QString &pattern, QListView *pOnView);
    void refreshModels();

    void deleteItem(const QUrl &url);
    void deleteItem(const Feature &pFeature);
    bool isTrash(const QUrl &url);
    void moveToTrash(Feature feature, bool deleteManualy = false);
    void addToFavorite(const QUrl &path);
    void removeFromFavorite(const QUrl &path, bool deleteManualy = false);
    void importImage(const QImage &image, const QString &fileName = QString());
    void importImage(const QImage &image, const Feature &destination, const QString &fileName = QString());
    QStringList getFileNamesInFolders();

    void fileSystemScan(const QUrl &currPath, const QString &currVirtualPath);
    int featuresCount(const QUrl &currPath);
    static FeatureElementType fileTypeFromUrl(const QString &path);

    static QString fileNameFromUrl(const QUrl &url);
    static QImage getIcon(const QString &path, FeatureElementType pFType);
    static bool isDeletable(const QUrl &url);
    static QString categoryNameForVirtualPath(const QString &str);

    static const QString virtualRootName;

    void assignFeaturesListView(FeaturesListView *pList);
    void assignPathListView(FeaturesListView *pList);

public:
    static const QString rootPath;
    static const QString audiosPath;
    static const QString moviesPath;
    static const QString picturesPath;
    static const QString appPath;
    static const QString flashPath;
    static const QString shapesPath;
    static const QString interactPath;
    static const QString trashPath;
    static const QString favoritePath;

signals:
    void maxFilesCountEvaluated(int pLimit);
    void scanStarted();
    void scanFinished();
    void featureAddedFromThread();
    void scanCategory(const QString &);
    void scanPath(const QString &);

public slots:
    // FeaturesWidget 转发新建文件夹；跨类连接必须公开
    void addNewFolder(QString name);

private slots:
    void startThread();

private:
    FeaturesItemDelegate *itemDelegate;
    FeaturesPathItemDelegate *pathItemDelegate;

    FeaturesModel *featuresModel;
    FeaturesProxyModel *featuresProxyModel;
    FeaturesSearchProxyModel *featuresSearchModel;
    FeaturesPathProxyModel *featuresPathModel;

    QAbstractItemModel *curListModel;
    FeaturesComputingThread mCThread;

private:
    static QImage createThumbnail(const QString &path);
    //void addImageToCurrentPage( const QString &path );
    void loadFavoriteList();
    void saveFavoriteList();
    QString uniqNameForFeature(const Feature &feature, const QString &pName = "Imported",
                               const QString &pExtention = "") const;
    QString adjustName(const QString &str);

    QList<Feature> *featuresList;

    QUrl mUserAudioDirectoryPath;
    QUrl mUserVideoDirectoryPath;
    QUrl mUserPicturesDirectoryPath;
    QUrl mUserInteractiveDirectoryPath;
    QUrl mUserAnimationDirectoryPath;

    QString libraryPath;
    QUrl mLibPicturesDirectoryPath;
    QUrl mLibAudiosDirectoryPath;
    QUrl mLibVideosDirectoryPath;
    //QUrl mLibAnimationsDirectoryPath;
    QUrl mLibShapesDirectoryPath;

    QUrl trashDirectoryPath;

    int mLastItemOffsetIndex;
    Feature currentElement;

    Feature rootElement;
    Feature favoriteElement;
    Feature audiosElement;
    Feature moviesElement;
    Feature picturesElement;
    //Feature interactElement;
    Feature applicationsElement;
    Feature shapesElement;

    QSet<QUrl> *favoriteSet;

public:
    Feature trashElement;
    Feature getDestinationFeatureForUrl(const QUrl &url);
    Feature getDestinationFeatureForMimeType(const QString &pMmimeType);
};
