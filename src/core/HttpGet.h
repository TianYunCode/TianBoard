#pragma once

#include <QtCore>
#include <QtNetwork>
#include <QDropEvent>

class HttpGet : public QObject
{
    Q_OBJECT

public:
    HttpGet(QObject* parent = 0);
    virtual ~HttpGet();

    QNetworkReply* get(QUrl pUrl, QPointF pPoint = QPointF(0, 0), QSize pSize = QSize(0, 0), bool isBackground = false);
    // 中止当前请求并释放 reply；reply 只在请求对象自己的线程里被删除。
    void cancelRequest();
//        QNetworkReply* get(const sDownloadFileDesc &downlinfo);

signals:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(bool pSuccess, QUrl sourceUrl, QString pContentTypeHeader
            , QByteArray pData, QPointF pPos, QSize pSize, bool isBackground);
//        void downloadFinished(bool pSuccess, QUrl sourceUrl, QString pContentTypeHeader, QByteArray pData
//                              , sDownloadFileDesc downlInfo);

private slots:
    void readyRead();
    void requestFinished();
    void downloadProgressed(qint64 bytesReceived, qint64 bytesTotal);

private:
    QByteArray mDownloadedBytes;
    QNetworkReply* mReply;
    QPointF mPos;
    QSize mSize;

    bool mIsBackground;
    int mRequestID;
    int mRedirectionCount;
    bool mIsSelfAborting;
//        sDownloadFileDesc mDownloadInfo;
};


