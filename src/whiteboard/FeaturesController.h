/**
 * @file FeaturesController.h
 * @brief 素材库（Features）的数据模型、后台扫描线程与控制器声明。
 */

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


/**
 * @brief 在后台线程扫描素材目录并把结果逐条发给界面的工作线程。
 *
 * compute() 放入扫描任务后由 run() 在独立线程消费；结果通过 sendFeature()
 * 跨线程排队回到 GUI 线程，结束后发出 scanFinished()。mMutex/mWaitCondition
 * 保护任务队列并支持重启与中止。
 *
 * 生命周期：作为 FeaturesController 的成员存在，随控制器析构而结束；
 * 扫描目录期间不要从 GUI 线程直接访问其内部数据。
 */
class FeaturesComputingThread : public QThread {
    Q_OBJECT
public:
    explicit FeaturesComputingThread(QObject *parent = 0);
    virtual ~FeaturesComputingThread();
    /**
     * @brief 提交扫描任务并唤醒线程。
     * @param pScanningData 待扫描的根路径与对应的 Feature 描述。
     * @param pFavoritesSet 收藏集合，扫描时用于标记收藏项。
     */
    void compute(const QList<QPair<QUrl, Feature>> &pScanningData, QSet<QUrl> *pFavoritesSet);

protected:
    /// @brief 线程主体：等待任务、执行扫描、发送结果，直到被要求退出。
    void run();

signals:
    /// @brief 扫描到一个素材条目。
    void sendFeature(Feature pFeature);
    /// @brief 一个条目的发送流程结束。
    void featureSent();
    /// @brief 扫描开始。
    void scanStarted();
    /// @brief 扫描结束。
    void scanFinished();
    /// @brief 统计出的最大文件数，供进度条使用。
    void maxFilesCountEvaluated(int max);
    /// @brief 当前扫描到的分类名。
    void scanCategory(const QString &str);
    /// @brief 当前扫描到的路径。
    void scanPath(const QString &str);

private:
    /// @brief 递归扫描单个目录。
    void scanFS(const QUrl &currentPath, const QString &currVirtualPath, const QSet<QUrl> &pFavoriteSet);
    /// @brief 依次扫描所有根路径。
    void scanAll(QList<QPair<QUrl, Feature>> pScanningData, const QSet<QUrl> &pFavoriteSet);
    /// @brief 统计目录下的素材数量。
    int featuresCount(const QUrl &pPath);
    /// @brief 统计所有根路径下的素材数量。
    int featuresCountAll(QList<QPair<QUrl, Feature>> pScanningData);

private:
    QMutex mMutex;                             ///< 保护任务字段的互斥量
    QWaitCondition mWaitCondition;             ///< 任务到达/退出的等待条件
    QUrl mScanningPath;                        ///< 当前扫描路径
    QString mScanningVirtualPath;              ///< 当前扫描对应的虚拟路径
    QList<QPair<QUrl, Feature>> mScanningData; ///< 待扫描任务
    QSet<QUrl> mFavoriteSet;                   ///< 收藏集合副本
    bool restart;                              ///< 是否请求重启扫描
    bool abort;                                ///< 是否请求中止扫描
};


/// @brief 素材库条目的类型。
enum FeatureElementType {
    FEATURE_CATEGORY,      ///< 分类根节点
    FEATURE_VIRTUALFOLDER, ///< 虚拟文件夹
    FEATURE_FOLDER,        ///< 真实文件夹
    FEATURE_INTERACTIVE,   ///< 交互式内容
    FEATURE_INTERNAL,      ///< 内部资源
    FEATURE_ITEM,          ///< 普通条目
    FEATURE_AUDIO,         ///< 音频
    FEATURE_VIDEO,         ///< 视频
    FEATURE_IMAGE,         ///< 图片
    FEATURE_FLASH,         ///< Flash 内容
    FEATURE_TRASH,         ///< 回收站
    FEATURE_FAVORITE,      ///< 收藏
    FEATURE_SEARCH,        ///< 搜索结果
    FEATURE_INVALID        ///< 无效条目
};

/**
 * @brief 素材库中的一个条目：路径、缩略图、显示名、类型与元数据。
 *
 * 值类型，可跨线程复制传递（已注册为 Qt 元类型）；不持有外部资源。
 */
class Feature {
public:
    Feature() { ; }
    /**
     * @param url 条目路径（保存在 virtualPath）。
     * @param icon 缩略图。
     * @param name 名称。
     * @param realPath 真实文件路径。
     * @param type 条目类型。
     */
    Feature(const QString &url, const QImage &icon, const QString &name, const QUrl &realPath,
            FeatureElementType type = FEATURE_CATEGORY);
    virtual ~Feature();
    /// @brief 条目名称。
    QString getName() const { return mName; }
    /// @brief 界面显示名（可能已做本地化或去扩展名）。
    QString getDisplayName() const { return mDisplayName; }
    /// @brief 缩略图。
    QImage getThumbnail() const { return mThumbnail; }
    /// @brief 所在虚拟目录。
    QString getVirtualPath() const { return virtualDir; }
    /// @brief 真实文件路径。
    QUrl getFullPath() const { return mPath; }
    /// @brief 虚拟目录 + 名称组成的完整虚拟路径。
    QString getFullVirtualPath() const { return virtualDir + "/" + mName; }
    /// @brief 用于打开/插入的 URL。
    QString getUrl() const;
    /// @brief 修改真实文件路径。
    void setFullPath(const QUrl &newPath) { mPath = newPath; }
    /// @brief 修改虚拟目录。
    void setFullVirtualPath(const QString &newVirtualPath) { virtualDir = newVirtualPath; }
    /// @brief 条目类型。
    FeatureElementType getType() const { return elementType; }

    /// @brief 是否代表文件夹。
    bool isFolder() const;
    /// @brief 是否允许复制。
    bool allowedCopy() const;
    /// @brief 是否允许删除。
    bool isDeletable() const;
    /// @brief 是否位于回收站。
    bool inTrash() const;
    /// @brief 按路径比较两个条目是否相同。
    bool operator==(const Feature &f) const;
    /// @brief 与 operator== 相反。
    bool operator!=(const Feature &f) const;
    /// @brief 附加元数据。
    const QMap<QString, QString> &getMetadata() const { return metadata; }
    /// @brief 设置附加元数据。
    void setMetadata(const QMap<QString, QString> &data) { metadata = data; }

private:
    QString getNameFromVirtualPath(const QString &pVirtPath);
    QString getVirtualDirFromVirtualPath(const QString &pVirtPath);

private:
    QString virtualDir;              ///< 虚拟目录
    QString virtualPath;             ///< 虚拟路径
    QImage mThumbnail;               ///< 缩略图
    QString mName;                   ///< 名称
    QString mDisplayName;            ///< 显示名
    QUrl mPath;                      ///< 真实路径
    FeatureElementType elementType;  ///< 条目类型
    QMap<QString, QString> metadata; ///< 附加元数据
};
Q_DECLARE_METATYPE(Feature)

/**
 * @brief 素材库控制器：管理用户/内置素材目录、收藏与回收站，驱动扫描线程并
 * 为素材库界面提供模型。
 *
 * 由素材库控件（FeaturesWidget）创建并持有；模型对象与代理模型都归本类所有。
 * 目录扫描在 FeaturesComputingThread 中执行，结果通过信号回到 GUI 线程后再
 * 写入 featuresList，避免界面线程阻塞。
 */
class FeaturesController : public QObject {
    friend class FeaturesWidget;

    Q_OBJECT

public:
    /// @brief 构造控制器并准备模型、目录与扫描线程。
    FeaturesController(QWidget *parentWidget);
    virtual ~FeaturesController();

    /// @brief 当前素材条目列表。
    QList<Feature> *getFeatures() const { return featuresList; }

    /// @brief 素材库根目录。
    const QString &getRootPath() const { return rootPath; }
    /// @brief 启动目录扫描（异步）。
    void scanFS();

    /// @brief 把条目的内容插入当前页。
    void addItemToPage(const Feature &item);
    /// @brief 把条目的内容设为当前页背景。
    void addItemAsBackground(const Feature &item);
    /// @brief 当前在界面中选中的条目。
    const Feature &getCurrentElement() const { return currentElement; }
    /// @brief 记录界面当前选中的条目。
    void setCurrentElement(const Feature &elem) { currentElement = elem; }
    /// @brief 回收站条目。
    const Feature &getTrashElement() const { return trashElement; }

    /**
     * @brief 登记一个已下载的文件到素材库。
     * @param sourceUrl 来源地址。
     * @param pData 文件内容。
     * @param pContentSource 内容来源描述。
     * @param pTitle 标题（用于命名）。
     */
    void addDownloadedFile(const QUrl &sourceUrl, const QByteArray &pData, const QString pContentSource,
                           const QString pTitle);

    /// @brief 移动条目到目标文件夹，返回移动后的条目。
    Feature moveItemToFolder(const QUrl &url, const Feature &destination);
    /// @brief 复制条目到目标文件夹，返回复制后的条目。
    Feature copyItemToFolder(const QUrl &url, const Feature &destination);
    /// @brief 把外部数据移动到素材库目录。
    void moveExternalData(const QUrl &url, const Feature &destination);

    /// @brief 重新扫描并刷新模型。
    void rescanModel();
    /// @brief 按关键字过滤素材。
    void siftElements(const QString &pSiftValue);
    /// @brief 从模型索引还原出 Feature；结构较复杂，后续可拆分以降低维护成本。
    Feature getFeature(const QModelIndex &index, const QString &listName);
    /// @brief 搜索开始：切换列表模型到搜索结果。
    void searchStarted(const QString &pattern, QListView *pOnView);
    /// @brief 刷新全部模型与视图。
    void refreshModels();

    /// @brief 按 URL 删除条目（移入回收站或直接删除）。
    void deleteItem(const QUrl &url);
    /// @brief 按 Feature 删除条目。
    void deleteItem(const Feature &pFeature);
    /// @brief URL 是否位于回收站。
    bool isTrash(const QUrl &url);
    /**
     * @brief 把条目移入回收站。
     * @param feature 目标条目。
     * @param deleteManualy 是否为用户显式删除（影响确认/提示）。
     */
    void moveToTrash(Feature feature, bool deleteManualy = false);
    /// @brief 加入收藏。
    void addToFavorite(const QUrl &path);
    /// @brief 取消收藏。
    void removeFromFavorite(const QUrl &path, bool deleteManualy = false);
    /// @brief 把图像导入素材库图片目录。
    void importImage(const QImage &image, const QString &fileName = QString());
    /// @brief 把图像导入指定目录。
    void importImage(const QImage &image, const Feature &destination, const QString &fileName = QString());
    /// @brief 各素材目录下的文件名列表。
    QStringList getFileNamesInFolders();

    /// @brief 递归扫描文件系统（同步版本，供内部与导入流程使用）。
    void fileSystemScan(const QUrl &currPath, const QString &currVirtualPath);
    /// @brief 统计目录下素材数量。
    int featuresCount(const QUrl &currPath);
    /// @brief 按扩展名推断条目类型。
    static FeatureElementType fileTypeFromUrl(const QString &path);

    /// @brief 从 URL 提取文件名。
    static QString fileNameFromUrl(const QUrl &url);
    /// @brief 按路径与类型生成缩略图/图标。
    static QImage getIcon(const QString &path, FeatureElementType pFType);
    /// @brief 该 URL 是否允许删除。
    static bool isDeletable(const QUrl &url);
    /// @brief 由虚拟路径推出分类显示名。
    static QString categoryNameForVirtualPath(const QString &str);

    static const QString virtualRootName; ///< 虚拟根节点名称

    /// @brief 绑定素材列表视图。
    void assignFeaturesListView(FeaturesListView *pList);
    /// @brief 绑定路径列表视图。
    void assignPathListView(FeaturesListView *pList);

public:
    static const QString rootPath;     ///< 素材库根目录
    static const QString audiosPath;   ///< 用户音频目录
    static const QString moviesPath;   ///< 用户视频目录
    static const QString picturesPath; ///< 用户图片目录
    static const QString appPath;      ///< 内置应用/交互内容目录
    static const QString flashPath;    ///< Flash 内容目录
    static const QString shapesPath;   ///< 形状素材目录
    static const QString interactPath; ///< 交互素材目录
    static const QString trashPath;    ///< 回收站目录
    static const QString favoritePath; ///< 收藏虚拟目录标识

signals:
    /// @brief 最大文件数已统计，供进度条使用。
    void maxFilesCountEvaluated(int pLimit);
    /// @brief 扫描开始。
    void scanStarted();
    /// @brief 扫描结束。
    void scanFinished();
    /// @brief 后台线程新增了一个条目。
    void featureAddedFromThread();
    /// @brief 扫描到分类。
    void scanCategory(const QString &);
    /// @brief 扫描到路径。
    void scanPath(const QString &);

public slots:
    /// @brief 新建文件夹；素材库控件会转发此调用，跨类连接必须公开。
    void addNewFolder(QString name);

private slots:
    /// @brief 启动/重启扫描线程。
    void startThread();

private:
    FeaturesItemDelegate *itemDelegate;         ///< 素材条目绘制委托
    FeaturesPathItemDelegate *pathItemDelegate; ///< 路径条目绘制委托

    FeaturesModel *featuresModel;                  ///< 素材模型
    FeaturesProxyModel *featuresProxyModel;        ///< 素材排序/过滤代理
    FeaturesSearchProxyModel *featuresSearchModel; ///< 搜索代理
    FeaturesPathProxyModel *featuresPathModel;     ///< 路径代理

    QAbstractItemModel *curListModel; ///< 当前列表使用的模型
    FeaturesComputingThread mCThread; ///< 后台扫描线程

private:
    /// @brief 生成缩略图。
    static QImage createThumbnail(const QString &path);
    /// @brief 加载收藏列表。
    void loadFavoriteList();
    /// @brief 保存收藏列表。
    void saveFavoriteList();
    /// @brief 为目标条目生成不冲突的名称。
    QString uniqNameForFeature(const Feature &feature, const QString &pName = "Imported",
                               const QString &pExtention = "") const;
    /// @brief 规范化名称（去除非法字符等）。
    QString adjustName(const QString &str);

    QList<Feature> *featuresList; ///< 素材条目列表

    QUrl mUserAudioDirectoryPath;       ///< 用户音频目录
    QUrl mUserVideoDirectoryPath;       ///< 用户视频目录
    QUrl mUserPicturesDirectoryPath;    ///< 用户图片目录
    QUrl mUserInteractiveDirectoryPath; ///< 用户交互内容目录
    QUrl mUserAnimationDirectoryPath;   ///< 用户动画目录

    QString libraryPath;            ///< 素材库路径
    QUrl mLibPicturesDirectoryPath; ///< 内置图片目录
    QUrl mLibAudiosDirectoryPath;   ///< 内置音频目录
    QUrl mLibVideosDirectoryPath;   ///< 内置视频目录
    QUrl mLibShapesDirectoryPath;   ///< 内置形状目录

    QUrl trashDirectoryPath; ///< 回收站目录

    int mLastItemOffsetIndex; ///< 上一次插入的偏移索引
    Feature currentElement;   ///< 当前选中条目

    Feature rootElement;         ///< 根节点
    Feature favoriteElement;     ///< 收藏节点
    Feature audiosElement;       ///< 音频节点
    Feature moviesElement;       ///< 视频节点
    Feature picturesElement;     ///< 图片节点
    Feature applicationsElement; ///< 应用节点
    Feature shapesElement;       ///< 形状节点

    QSet<QUrl> *favoriteSet; ///< 收藏集合

public:
    Feature trashElement; ///< 回收站节点
    /// @brief 根据 URL 推断其应放入的目标分类。
    Feature getDestinationFeatureForUrl(const QUrl &url);
    /// @brief 根据 MIME 类型推断其应放入的目标分类。
    Feature getDestinationFeatureForMimeType(const QString &pMmimeType);
};
