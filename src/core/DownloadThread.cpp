#include <QDebug>
#include <QNetworkProxy>
#include <QNetworkDiskCache>

#include "core/Settings.h"

#include "core/DownloadThread.h"


DownloadThread::DownloadThread(QObject *parent, const char *name):QThread(parent)
    , mbRun(false)
    ,mpReply(NULL)
{
    setObjectName(name);
}

DownloadThread::~DownloadThread()
{
    if(NULL != mpReply)
    {
        delete mpReply;
        mpReply = NULL;
    }
}

void DownloadThread::run()
{
    qDebug() << mUrl;
    // We start the download
    QNetworkAccessManager* pNam = new QNetworkAccessManager();

    mpReply = pNam->get(QNetworkRequest(QUrl(mUrl)));
    qDebug() << " -- Http GET reply ---------------------- ";
    qDebug() << mpReply->readAll();
    qDebug() << " ---------------------------------------- ";

    connect(mpReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(onDownloadProgress(qint64,qint64)));
    connect(mpReply, SIGNAL(finished()), this, SLOT(onDownloadFinished()));

    while(mbRun)
    {
        // Wait here until the end of the download
        sleep(100);
    }

    disconnect(mpReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(onDownloadProgress(qint64,qint64)));
    disconnect(mpReply, SIGNAL(finished()), this, SLOT(onDownloadFinished()));
    if(NULL != mpReply)
    {
        delete mpReply;
        mpReply = NULL;
    }
}

void DownloadThread::stopDownload()
{
    mbRun = false;
}

void DownloadThread::startDownload(int id, QString url)
{
    mID = id;
    mUrl = url;
    mbRun = true;
    start();
}

void DownloadThread::onDownloadProgress(qint64 received, qint64 total)
{
    qDebug() << received << " on " << total;
    emit downloadProgress(mID, received, total);
}

void DownloadThread::onDownloadFinished()
{
    emit downloadFinised(mID);
}
