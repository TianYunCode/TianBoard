#include "core/HttpGet.h"

#include <QtNetwork>

#include "core/NetworkAccessManager.h"
#include "core/DownloadManager.h"


HttpGet::HttpGet(QObject *parent)
    : QObject(parent), mReply(0), mIsBackground(false), mRedirectionCount(0), mIsSelfAborting(false) {
    // 成员都在初始化列表里置好，构造体内无额外动作。
}


HttpGet::~HttpGet() {
    cancelRequest();
}

void HttpGet::cancelRequest() {
    if (!mReply)
        return;

    mIsSelfAborting = true;

    QNetworkReply *reply = mReply;
    mReply = 0;
    disconnect(reply, nullptr, this, nullptr);

    reply->abort();
    reply->deleteLater();
}

QNetworkReply *HttpGet::get(QUrl pUrl, QPointF pPos, QSize pSize, bool isBackground) {
    mPos = pPos;
    mSize = pSize;
    mIsBackground = isBackground;

    cancelRequest();
    // cancelRequest() 会把 mIsSelfAborting 置真（用来忽略旧 reply 的 finished）；
    // 这里必须在新请求发出之后复位，否则重定向/第二次 get() 的完成信号会被永久吞掉。
    mIsSelfAborting = false;

    NetworkAccessManager *nam = NetworkAccessManager::defaultAccessManager();
    mReply = nam->get(QNetworkRequest(pUrl));

    mDownloadedBytes.clear();

    connect(mReply, &QNetworkReply::finished, this, &HttpGet::requestFinished);
    connect(mReply, &QNetworkReply::readyRead, this, &HttpGet::readyRead);
    connect(mReply, &QNetworkReply::downloadProgress, this, &HttpGet::downloadProgressed);

    return mReply;
}

void HttpGet::readyRead() {
    if (mReply)
        mDownloadedBytes += mReply->readAll();
}


void HttpGet::requestFinished() {
    if (!mReply || mIsSelfAborting) {
        return;
    }

    if (mReply->error() != QNetworkReply::NoError) {
        qWarning() << mReply->url().toString() << "get finished with error : " << mReply->error();

        mDownloadedBytes.clear();

        mRedirectionCount = 0;

        emit downloadFinished(false, mReply->url(), mReply->errorString(), mDownloadedBytes, mPos, mSize,
                              mIsBackground);
    } else {
        qDebug() << mReply->url().toString() << "http get finished ...";

        if (mReply->header(QNetworkRequest::LocationHeader).isValid() && mRedirectionCount < 10) {
            mRedirectionCount++;
            get(mReply->header(QNetworkRequest::LocationHeader).toUrl(), mPos, mSize, mIsBackground);

            return;
        }

        mRedirectionCount = 0;

        emit downloadFinished(true, mReply->url(),
                              mReply->header(QNetworkRequest::ContentTypeHeader).toString(), mDownloadedBytes,
                              mPos, mSize, mIsBackground);
    }
}

void HttpGet::downloadProgressed(qint64 bytesReceived, qint64 bytesTotal) {
    // bytesTotal 为 -1 表示响应没给出总长度，此时进度无从计算，直接不上报。
    if (-1 != bytesTotal) {
        emit downloadProgress(bytesReceived, bytesTotal);
    }
}
