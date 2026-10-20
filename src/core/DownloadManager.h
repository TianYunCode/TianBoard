/**
 * @file DownloadManager.h
 * @brief 声明网络/本地资源的下载队列、下载任务对象与下载完成后的分发协议。
 */

#pragma once

#include <QObject>
#include <QAtomicInt>
#include <QString>
#include <QThread>
#include <QVector>
#include <QDropEvent>

#include "core/HttpGet.h"

#define SIMULTANEOUS_DOWNLOAD 2

/**
 * @brief 一次下载任务的描述：来源、目标去向、进度与拖放落点。
 *
 * 详细说明：由调用方填充后交给 DownloadManager；结构体按值在队列中传递，
 * 因此所有字段必须保持可拷贝。srcUrl 在本地文件下载时会被改写为本地路径。
 */
struct sDownloadFileDesc
{
    /// 下载完成后的去向。
    enum eDestinations
    {
        board,         ///< 加入当前画板页面
        library,       ///< 存入文档库
        graphicsWidget ///< 作为图形控件嵌入文档
    };

    sDownloadFileDesc()
        : dest(board), id(0), totalSize(0), currentSize(0), modal(false), isBackground(false), dropActions(Qt::IgnoreAction),
          dropMouseButtons(Qt::NoButton), dropModifiers(Qt::NoModifier)
    {
    }

    eDestinations dest;
    QString       name;
    int           id;
    int           totalSize;
    int           currentSize;
    QString       srcUrl;
    QString       originalSrcUrl;
    QString       contentTypeHeader;
    bool          modal;
    QPointF       pos;
    QSize         size;
    bool          isBackground;

    QPoint                dropPoint;
    Qt::DropActions       dropActions;
    Qt::MouseButtons      dropMouseButtons;
    Qt::KeyboardModifiers dropModifiers;
};

/**
 * @brief 单个 HTTP 下载任务：包装 HttpGet，并把结果翻译成带任务 id 的信号。
 *
 * 详细说明：由 DownloadManager 创建并持有；完成或出错后由管理器销毁。
 * 线程约束：与管理器同处 GUI 线程。
 */
class DownloadHttpFile : public HttpGet
{
    Q_OBJECT
public:
    /**
     * @brief 构造下载任务。
     * @param fileId 任务编号，会原样出现在信号里。
     * @param parent 父对象。
     */
    DownloadHttpFile(int fileId, QObject *parent = 0);

signals:
    /// 下载进度变化（字节）。
    void downloadProgress(int id, qint64 current, qint64 total);
    /// 下载结束；contentUrl 为最终地址（可能经过重定向）。
    void downloadFinished(int id, bool pSuccess, QUrl sourceUrl, QUrl contentUrl, QString pContentTypeHeader, QByteArray pData, QPointF pPos,
        QSize pSize, bool isBackground);
    /// 下载出错。
    void downloadError(int id);

public slots:
    // 由 DownloadManager 持有并负责终结；取消与收尾都走这里。
    /// 取消当前请求；管理器随后会释放本对象。
    void cancel();

private slots:
    void onDownloadFinished(
        bool pSuccess, QUrl sourceUrl, QString pContentTypeHeader, QByteArray pData, QPointF pPos, QSize pSize, bool isBackground);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private:
    int  mId;
    bool mCancelled;
};

/**
 * @brief 把文档内的媒体文件从来源位置复制进文档包。复制本身是阻塞 I/O，放在工作线程；
 * 但文档路径、选中文档等 GUI 线程状态必须在主线程解析（见 prepare()）。
 * 对象自持生命周期：线程结束后自行销毁。
 */
class AsyncLocalFileDownloader : public QThread
{
    Q_OBJECT
public:
    /**
     * @brief 构造本地复制任务。
     * @param desc 下载描述，srcUrl 指向本地文件。
     * @param parent 父对象，通常是 DownloadManager。
     */
    AsyncLocalFileDownloader(sDownloadFileDesc desc, QObject *parent = 0);

    /**
     * @brief 在主线程准备好目标路径后启动工作线程。
     * @return 本对象；准备失败时返回空指针且不启动线程。
     * @note 只能从 GUI 线程调用。
     */
    AsyncLocalFileDownloader *download();
    /// 在工作线程中执行文件复制并发出完成信号。
    void run();
    /// 请求中止；复制完成后若发现已中止会删除残留目标文件。
    void abort();

signals:
    /// 复制结束；字段含义与 DownloadHttpFile::downloadFinished 一致，contentUrl 为目标文件地址。
    void signal_asyncCopyFinished(int id, bool pSuccess, QUrl sourceUrl, QUrl contentUrl, QString pContentTypeHeader, QByteArray pData, QPointF pPos,
        QSize pSize, bool isBackground);

private:
    // 只能在 GUI 线程调用：解析来源、取出当前文档、准备好目标路径。
    /// 解析来源与目标路径；来源不存在或目标路径为空时返回 false。
    bool prepare();

    sDownloadFileDesc mDesc;
    QAtomicInt        mAborting;
    QString           mFrom;
    QString           mTo;
};

/**
 * @brief 下载管理器：维护等待/进行中的下载队列，限制并发数并在完成后分发结果。
 *
 * 详细说明：进程级单例，由 Application 在退出时 destroy()；HTTP 任务与本地复制线程
 * 都以本对象为父，因此所有信号都回到 GUI 线程。并发上限由 SIMULTANEOUS_DOWNLOAD 决定。
 */
class DownloadManager : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief 构造管理器。
     * @param parent 父对象。
     * @param name 对象名，便于调试。
     */
    DownloadManager(QObject *parent = 0, const char *name = "DownloadManager");
    ~DownloadManager();
    /// 返回进程内唯一实例，首次调用时创建。
    static DownloadManager *downloadManager();
    /**
     * @brief 把一个任务加入等待队列。
     * @param desc 任务描述；其 id 会被覆盖为新分配的编号。
     * @return 分配到的任务编号。
     */
    int addFileToDownload(sDownloadFileDesc desc);
    /// 返回进行中的下载列表快照。
    QVector<sDownloadFileDesc> currentDownloads();
    /// 返回等待中的下载列表快照。
    QVector<sDownloadFileDesc> pendingDownloads();
    /// 取消所有等待与进行中的下载。
    void cancelDownloads();
    /// 取消指定任务。
    void cancelDownload(int id);

    /// 销毁单例并置空指针。
    static void destroy();

signals:
    /// 有新任务加入队列。
    void fileAddedToDownload();
    /// 任务进度更新（字节）。
    void downloadUpdated(int id, qint64 crnt, qint64 total);
    /// 任务结束（仅编号）。
    void downloadFinished(int id);
    /// HTTP 任务结束，携带内容与来源地址。
    void downloadFinished(bool pSuccess, int id, QUrl sourceUrl, QString pContentTypeHeader, QByteArray pData);
    /// 任务结束，携带完整描述。
    void downloadFinished(bool pSuccess, sDownloadFileDesc desc, QByteArray pData);
    /// 模态下载结束，界面可关闭等待框。
    void downloadModalFinished();
    /// 请求把下载结果加入画板。
    void addDownloadedFileToBoard(
        bool pSuccess, QUrl sourceUrl, QUrl contentUrl, QString pContentTypeHeader, QByteArray pData, QPointF pPos, QSize pSize, bool isBackground);
    /// 请求把下载结果存入文档库。
    void addDownloadedFileToLibrary(bool pSuccess, QUrl sourceUrl, QString pContentTypeHeader, QByteArray pData, QString pTitle);
    /// 取消全部下载的广播。
    void cancelAllDownloads();
    /// 所有下载均已结束。
    void allDownloadsFinished();

private slots:
    /// 有空闲槽位时把等待任务移入进行中列表。
    void onUpdateDownloadLists();
    /// 更新任务进度。
    void onDownloadProgress(int id, qint64 received, qint64 total);
    /// 统一处理任务结束：更新列表、发信号并回收任务对象。
    void onDownloadFinished(int id, bool pSuccess, QUrl sourceUrl, QUrl contentUrl, QString pContentTypeHeader, QByteArray pData, QPointF pPos,
        QSize pSize, bool isBackground);
    /// 任务出错时结束并回收。
    void onDownloadError(int id);

private:
    /// 清空队列并初始化并发槽位。
    void init();
    /// 把模态任务排到等待队列前面。
    void updateDownloadOrder();
    /// 更新指定任务的已下载字节数。
    void updateFileCurrentSize(int id, qint64 received = -1, qint64 total = -1);
    /// 按目标去向创建并启动实际下载任务。
    void startFileDownload(sDownloadFileDesc desc);
    /// 没有模态任务残留时发出 downloadModalFinished。
    void checkIfModalRemains();
    /// 结束下载流程，可按需取消。
    void finishDownloads(bool cancel = false);
    /// 释放指定任务对象并归还并发槽位。
    void releaseDownload(int id);

    QVector<sDownloadFileDesc> mCrntDL;
    QVector<sDownloadFileDesc> mPendingDL;
    int                        mLastID;
    QVector<int>               mDLAvailability;
    QMap<int, QObject *>       mDownloads;
};
