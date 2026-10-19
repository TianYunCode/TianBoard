#include "core/HttpGet.h"

#include <QtNetwork>

#include "core/NetworkAccessManager.h"
#include "core/DownloadManager.h"


sDownloadFileDesc desc;

HttpGet::HttpGet(QObject* parent)
    : QObject(parent)
    , mReply(0)
    , mIsBackground(false)
    , mRedirectionCount(0)
    , mIsSelfAborting(false)
{
    // NOOP
}


HttpGet::~HttpGet()
{
        if (mReply)
    {
        mIsSelfAborting = true;
        mReply->abort();
                delete mReply;
    }
}

QNetworkReply* HttpGet::get(QUrl pUrl, QPointF pPos, QSize pSize, bool isBackground)
{
    mPos = pPos;
    mSize = pSize;
    mIsBackground = isBackground;

    if (mReply)
        delete mReply;

    NetworkAccessManager * nam = NetworkAccessManager::defaultAccessManager();
    mReply = nam->get(QNetworkRequest(pUrl)); //mReply deleted by this destructor

    mDownloadedBytes.clear();

    connect(mReply, &QNetworkReply::finished, this, &HttpGet::requestFinished);
    connect(mReply, &QNetworkReply::readyRead, this, &HttpGet::readyRead);
    connect(mReply, &QNetworkReply::downloadProgress, this, &HttpGet::downloadProgressed);

    return mReply;
}

void HttpGet::readyRead()
{
        if (mReply)
                mDownloadedBytes += mReply->readAll();
}


void HttpGet::requestFinished()
{
    if (!mReply || mIsSelfAborting)
    {
        return;
    }

    if (mReply->error() != QNetworkReply::NoError)
    {
        qWarning() << mReply->url().toString() << "get finished with error : " << mReply->error();

        mDownloadedBytes.clear();

        mRedirectionCount = 0;

        emit downloadFinished(false, mReply->url(), mReply->errorString(), mDownloadedBytes, mPos, mSize, mIsBackground);
    }
    else
    {

        qDebug() << mReply->url().toString() << "http get finished ...";

        if (mReply->header(QNetworkRequest::LocationHeader).isValid() && mRedirectionCount < 10)
        {
            mRedirectionCount++;
            get(mReply->header(QNetworkRequest::LocationHeader).toUrl(), mPos, mSize, mIsBackground);

            return;
        }

        mRedirectionCount = 0;

        emit downloadFinished(true, mReply->url(), mReply->header(QNetworkRequest::ContentTypeHeader).toString(),
                        mDownloadedBytes, mPos, mSize, mIsBackground);
    }

}

void HttpGet::downloadProgressed(qint64 bytesReceived, qint64 bytesTotal)
{
//    qDebug() << "received: " << bytesReceived << ", / " << bytesTotal << " bytes";
    if (-1 != bytesTotal)
    {
        emit downloadProgress(bytesReceived, bytesTotal);
    }
}

