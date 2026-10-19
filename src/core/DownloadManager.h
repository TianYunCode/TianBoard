#pragma once

#include <QObject>
#include <QString>
#include <QVector>
#include <QMutex>
#include <QDropEvent>

#include "core/DownloadThread.h"

#include "core/HttpGet.h"

#define SIMULTANEOUS_DOWNLOAD 2 

struct sDownloadFileDesc
{
    enum eDestinations {
        board 
        , library
        , graphicsWidget
    };

    sDownloadFileDesc() :
        dest(board)
      , id(0)
      , totalSize(0)
      , currentSize(0)
      , modal(false)
      , isBackground(false)
      , dropActions(Qt::IgnoreAction)
      , dropMouseButtons(Qt::NoButton)
      , dropModifiers(Qt::NoModifier)
    {}

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

class DownloadHttpFile : public HttpGet
{
    Q_OBJECT
public:
    DownloadHttpFile(int fileId, QObject* parent=0);
    ~DownloadHttpFile();

signals:
    void downloadProgress(int id, qint64 current,qint64 total);
    void downloadFinished(int id, bool pSuccess, QUrl sourceUrl, QUrl contentUrl, QString pContentTypeHeader, QByteArray pData, QPointF pPos, QSize pSize, bool isBackground);
    void downloadError(int id);

private slots:
    void onDownloadFinished(bool pSuccess, QUrl sourceUrl, QString pContentTypeHeader, QByteArray pData, QPointF pPos, QSize pSize, bool isBackground);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

private:
    int mId;
};

class AsyncLocalFileDownloader : public QThread
{
    Q_OBJECT
public:
    AsyncLocalFileDownloader(sDownloadFileDesc desc, QObject *parent = 0);

    AsyncLocalFileDownloader *download();    
    void run();
    void abort();

signals:
    void finished(QString srcUrl, QString resUrl);
    void signal_asyncCopyFinished(int id, bool pSuccess, QUrl sourceUrl, QUrl contentUrl, QString pContentTypeHeader, QByteArray pData, QPointF pPos, QSize pSize, bool isBackground);

private:
    sDownloadFileDesc mDesc;
    bool m_bAborting;
    QString mFrom;
    QString mTo;
};

class DownloadManager : public QObject
{
    Q_OBJECT
public:
    DownloadManager(QObject* parent=0, const char* name="DownloadManager");
    ~DownloadManager();
    static DownloadManager* downloadManager();
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
    void downloadFinished(bool pSuccess, int id, QUrl sourceUrl, QString pContentTypeHeader, QByteArray pData);
    void downloadFinished(bool pSuccess, sDownloadFileDesc desc, QByteArray pData);
    void downloadModalFinished();
    void addDownloadedFileToBoard(bool pSuccess, QUrl sourceUrl, QUrl contentUrl, QString pContentTypeHeader, QByteArray pData, QPointF pPos, QSize pSize, bool isBackground);
    void addDownloadedFileToLibrary(bool pSuccess, QUrl sourceUrl, QString pContentTypeHeader, QByteArray pData, QString pTitle);
    void cancelAllDownloads();
    void allDownloadsFinished();

private slots:
    void onUpdateDownloadLists();
    void onDownloadProgress(int id, qint64 received, qint64 total);
    void onDownloadFinished(int id, bool pSuccess, QUrl sourceUrl, QUrl contentUrl, QString pContentTypeHeader, QByteArray pData, QPointF pPos, QSize pSize, bool isBackground);
    void onDownloadError(int id);

private:
    void init();
    void updateDownloadOrder();
    void updateFileCurrentSize(int id, qint64 received=-1, qint64 total=-1);
    void startFileDownload(sDownloadFileDesc desc);
    void checkIfModalRemains();
    void finishDownloads(bool cancel=false);

    QVector<sDownloadFileDesc> mCrntDL;
    QVector<sDownloadFileDesc> mPendingDL;
    QMutex mMutex;
    int mLastID;
    QVector<int> mDLAvailability;
    QMap<int, QObject*> mDownloads;
};

