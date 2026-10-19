#pragma once

#include <QObject>
#include <QAtomicInt>
#include <QString>
#include <QThread>
#include <QVector>
#include <QDropEvent>

#include "core/HttpGet.h"

#define SIMULTANEOUS_DOWNLOAD 2

struct sDownloadFileDesc {
    enum eDestinations { board, library, graphicsWidget };

    sDownloadFileDesc()
        : dest(board),
          id(0),
          totalSize(0),
          currentSize(0),
          modal(false),
          isBackground(false),
          dropActions(Qt::IgnoreAction),
          dropMouseButtons(Qt::NoButton),
          dropModifiers(Qt::NoModifier) {}

    eDestinations dest;
    QString name;
    int id;
    int totalSize;
    int currentSize;
    QString srcUrl;
    QString originalSrcUrl;
    QString contentTypeHeader;
    bool modal;
    QPointF pos;
    QSize size;
    bool isBackground;

    QPoint dropPoint;
    Qt::DropActions dropActions;
    Qt::MouseButtons dropMouseButtons;
    Qt::KeyboardModifiers dropModifiers;
};

class DownloadHttpFile : public HttpGet {
    Q_OBJECT
public:
    DownloadHttpFile(int fileId, QObject *parent = 0);

signals:
    void downloadProgress(int id, qint64 current, qint64 total);
    void downloadFinished(int id, bool pSuccess, QUrl sourceUrl, QUrl contentUrl, QString pContentTypeHeader,
                          QByteArray pData, QPointF pPos, QSize pSize, bool isBackground);
    void downloadError(int id);

public slots:
    // 由 DownloadManager 持有并负责终结；取消与收尾都走这里。
    void cancel();

private slots:
    void onDownloadFinished(bool pSuccess, QUrl sourceUrl, QString pContentTypeHeader, QByteArray pData,
                            QPointF pPos, QSize pSize, bool isBackground);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private:
    int mId;
    bool mCancelled;
};

/**
 * 把文档内的媒体文件从来源位置复制进文档包。复制本身是阻塞 I/O，放在工作线程；
 * 但文档路径、选中文档等 GUI 线程状态必须在主线程解析（见 prepare()）。
 * 对象自持生命周期：线程结束后自行销毁。
 */
class AsyncLocalFileDownloader : public QThread {
    Q_OBJECT
public:
    AsyncLocalFileDownloader(sDownloadFileDesc desc, QObject *parent = 0);

    AsyncLocalFileDownloader *download();
    void run();
    void abort();

signals:
    void signal_asyncCopyFinished(int id, bool pSuccess, QUrl sourceUrl, QUrl contentUrl,
                                  QString pContentTypeHeader, QByteArray pData, QPointF pPos, QSize pSize,
                                  bool isBackground);

private:
    // 只能在 GUI 线程调用：解析来源、取出当前文档、准备好目标路径。
    bool prepare();

    sDownloadFileDesc mDesc;
    QAtomicInt mAborting;
    QString mFrom;
    QString mTo;
};

class DownloadManager : public QObject {
    Q_OBJECT
public:
    DownloadManager(QObject *parent = 0, const char *name = "DownloadManager");
    ~DownloadManager();
    static DownloadManager *downloadManager();
    int addFileToDownload(sDownloadFileDesc desc);
    QVector<sDownloadFileDesc> currentDownloads();
    QVector<sDownloadFileDesc> pendingDownloads();
    void cancelDownloads();
    void cancelDownload(int id);

    static void destroy();

signals:
    void fileAddedToDownload();
    void downloadUpdated(int id, qint64 crnt, qint64 total);
    void downloadFinished(int id);
    void downloadFinished(bool pSuccess, int id, QUrl sourceUrl, QString pContentTypeHeader,
                          QByteArray pData);
    void downloadFinished(bool pSuccess, sDownloadFileDesc desc, QByteArray pData);
    void downloadModalFinished();
    void addDownloadedFileToBoard(bool pSuccess, QUrl sourceUrl, QUrl contentUrl, QString pContentTypeHeader,
                                  QByteArray pData, QPointF pPos, QSize pSize, bool isBackground);
    void addDownloadedFileToLibrary(bool pSuccess, QUrl sourceUrl, QString pContentTypeHeader,
                                    QByteArray pData, QString pTitle);
    void cancelAllDownloads();
    void allDownloadsFinished();

private slots:
    void onUpdateDownloadLists();
    void onDownloadProgress(int id, qint64 received, qint64 total);
    void onDownloadFinished(int id, bool pSuccess, QUrl sourceUrl, QUrl contentUrl,
                            QString pContentTypeHeader, QByteArray pData, QPointF pPos, QSize pSize,
                            bool isBackground);
    void onDownloadError(int id);

private:
    void init();
    void updateDownloadOrder();
    void updateFileCurrentSize(int id, qint64 received = -1, qint64 total = -1);
    void startFileDownload(sDownloadFileDesc desc);
    void checkIfModalRemains();
    void finishDownloads(bool cancel = false);
    void releaseDownload(int id);

    QVector<sDownloadFileDesc> mCrntDL;
    QVector<sDownloadFileDesc> mPendingDL;
    int mLastID;
    QVector<int> mDLAvailability;
    QMap<int, QObject *> mDownloads;
};
