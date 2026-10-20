/**
 * @file WebDownloadManager.h
 * @brief 声明下载域部件：单条下载项 DownloadItem、下载对话框 WebDownloadManager 及列表模型 DownloadModel。
 */

#pragma once

#include <QtNetwork>
#include <QtWidgets>
#include <QFileIconProvider>

#include <memory>


/**
 * @brief 单条下载项：一个下载任务的 UI 与网络状态机。
 *
 * 状态由按钮可见性组合表达：Stop 可用表示传输中；Try Again 可用表示失败或已停止；
 * 两者都隐藏表示成功完成。“重试”会重新发起 QNetworkRequest 并复用本控件。
 * 下载项由 WebDownloadManager 创建，父对象是管理器，删除由列表模型负责。
 */
class DownloadItem : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief 构造下载项并绑定网络回复。
     * @param reply 网络回复，可为空（从历史记录恢复时）。
     * @param requestFileName 是否先弹出保存文件对话框。
     * @param parent 父控件。
     * @param customDownloadPath 自定义下载目录，空则用设置或桌面。
     */
    DownloadItem(QNetworkReply *reply = 0, bool requestFileName = false, QWidget *parent = 0,
                 QString customDownloadPath = QString());
    /** @brief 析构；网络回复随父对象销毁。 */
    ~DownloadItem() override;
    /** @brief 进度条可见即视为正在下载。 */
    bool downloading() const;
    /** @brief Stop 与 Try Again 都隐藏即视为下载成功。 */
    bool downloadedSuccessfully() const;

    /** @brief 传输仍在进行（Stop 按钮可用）。 */
    bool isActive() const;
    /** @brief 失败或已停止，可重试。 */
    bool retryAvailable() const;

    /** @brief 设置显示的文件名。 */
    void setFileName(const QString &fileName);
    /** @brief 设置文件类型图标。 */
    void setFileIconPixmap(const QPixmap &pixmap);
    /**
     * @brief 把条目置为“已结束”状态。
     * @param completed true 表示成功完成，false 表示被中断。
     */
    void setDownloadCompleted(bool completed);
    /** @brief 返回信息行文本。 */
    QString infoText() const;

    /** @brief 下载来源 URL。 */
    QUrl m_url;

    /** @brief 输出文件。 */
    QFile m_output;
    /** @brief 当前网络回复。 */
    QNetworkReply *m_reply;
signals:
    /** @brief 下载状态变化，管理器据此刷新行高、图标与删除策略。 */
    void statusChanged();

private slots:
    /** @brief 用户点击停止：中止回复并切换到可重试状态。 */
    void stop();
    /** @brief 用户点击重试：重新发起请求并重置输出文件。 */
    void tryAgain();
    /** @brief 用系统默认程序打开已下载文件。 */
    void open();

    /** @brief 读取网络数据并写入输出文件。 */
    void downloadReadyRead();
    /** @brief 网络错误时显示错误并允许重试。 */
    void error(QNetworkReply::NetworkError code);
    /** @brief 更新进度条（总大小未知时显示忙碌状态）。 */
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    /** @brief 元数据变化，当前未处理。 */
    void metaDataChanged();
    /** @brief 下载结束：关闭文件、解包部件并刷新状态。 */
    void finished();

private:
    /** @brief 解析默认文件名，必要时弹出保存对话框并打开输出文件。 */
    void getFileName();
    /** @brief 绑定网络回复信号并重置界面。 */
    void init();
    /** @brief 根据已收字节、总大小与耗时刷新信息行。 */
    void updateInfoLabel();
    /** @brief 把字节数格式化为带单位的可读文本。 */
    QString dataString(int size) const;

    /** @brief 在指定目录中生成不冲突的保存文件名。 */
    QString saveFileName(const QString &directory) const;

    /** @brief 是否要求用户选择保存位置。 */
    bool mRequestFileName;
    /** @brief 已接收字节数。 */
    qint64 mBytesReceived;
    /** @brief 下载计时器，用于估算速度与剩余时间。 */
    QElapsedTimer mDownloadTime;
    /** @brief 自定义下载目录。 */
    QString mCustomDownloadPath;

    /** @brief 私有实现。 */
    struct Private;
    std::unique_ptr<Private> d;
};

class AutoSaver;
class DownloadModel;

/**
 * @brief 下载管理器对话框：维护下载列表、删除策略与持久化记录。
 *
 * 以 QDialog 形式存在，由 BrowserWindow::downloadManager() 作为进程级单例创建。
 * 下载请求统一经 NetworkAccessManager 重新抓取，再交给 DownloadItem；设置通过
 * AutoSaver 延迟保存。列表模型 DownloadModel 直接引用本类的 mDownloads。
 */
class WebDownloadManager : public QDialog {
    Q_OBJECT
    Q_PROPERTY(RemovePolicy removePolicy READ removePolicy WRITE setRemovePolicy)
    Q_ENUMS(RemovePolicy)

public:
    /** @brief 下载记录移除策略。 */
    enum RemovePolicy {
        Never,              ///< 从不自动移除
        Exit,               ///< 退出程序时移除
        SuccessFullDownload ///< 下载成功后移除
    };

    /** @brief 构造下载对话框并加载历史记录。 */
    WebDownloadManager(QWidget *parent = 0);
    /** @brief 析构：保存设置并释放图标提供器。 */
    ~WebDownloadManager();
    /** @brief 返回仍在传输的下载数。 */
    int activeDownloads() const;

    /** @brief 返回当前移除策略。 */
    RemovePolicy removePolicy() const;
    /** @brief 设置移除策略并标记待保存。 */
    void setRemovePolicy(RemovePolicy policy);

public slots:
    /**
     * @brief 发起下载。
     * @param request 网络请求。
     * @param requestFileName 是否先让用户选择保存位置。
     */
    void download(const QNetworkRequest &request, bool requestFileName = false);

    /** @brief 以 URL 发起下载。 */
    inline void download(const QUrl &url, bool requestFileName = false) {
        download(QNetworkRequest(url), requestFileName);
    }

    /**
     * @brief 处理 WebEngine 无法直接渲染的内容：按原 URL 重新下载。
     */
    void handleUnsupportedContent(QNetworkReply *reply, bool requestFileName = false,
                                  QString customDownloadPath = QString());

    /** @brief 移除所有已结束的下载项。 */
    void cleanup();

private slots:
    /** @brief 把移除策略与下载记录写入 QSettings。 */
    void save() const;
    /** @brief 下载项状态变化时刷新图标、行高并按策略移除。 */
    void updateRow();
    /** @brief 为网络回复创建下载项。 */
    void processDownloadedContent(QNetworkReply *reply, bool requestFileName,
                                  QString customDownloadPath = QString());

private:
    /** @brief 把下载项加入列表与模型。 */
    void addItem(DownloadItem *item);
    /** @brief 刷新“N 个下载”文本。 */
    void updateItemCount();
    /** @brief 从 QSettings 恢复下载记录。 */
    void load();

    /** @brief 延迟保存器。 */
    AutoSaver *mAutoSaver;
    /** @brief 下载列表模型。 */
    DownloadModel *mModel;
    /** @brief 网络访问管理器（全局共享）。 */
    QNetworkAccessManager *mManager;
    /** @brief 文件类型图标提供器，按需创建。 */
    QFileIconProvider *mIconProvider;
    /** @brief 下载项列表，模型直接读取。 */
    QList<DownloadItem *> mDownloads;
    /** @brief 当前移除策略。 */
    RemovePolicy m_RemovePolicy;
    friend class DownloadModel;

    /** @brief 是否正在从设置加载（加载期间不弹出对话框）。 */
    bool mIsLoading;

    /** @brief 私有实现。 */
    struct Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief 下载列表模型：只读列表，向视图提供提示文本并代理删除操作。
 *
 * 数据本身保存在 WebDownloadManager::mDownloads 中，本模型只转发行数与删除；
 * 因此与管理器强绑定，由管理器创建。
 */
class DownloadModel : public QAbstractListModel {
    friend class WebDownloadManager;
    Q_OBJECT

public:
    /** @brief 构造模型并绑定管理器。 */
    DownloadModel(WebDownloadManager *downloadManager, QObject *parent = 0);
    /** @brief 仅提供未完成下载的 ToolTip 文本。 */
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    /** @brief 返回下载项数量；有效父索引下为 0。 */
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    /** @brief 删除指定范围中已结束的下载项。 */
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());

private:
    /** @brief 所属下载管理器。 */
    WebDownloadManager *mDownloadManager;
};
