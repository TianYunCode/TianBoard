#include "core/DownloadManager.h"
#include "model/PackageLayout.hpp"
#include "core/Application.h"
#include "core/PersistenceManager.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/BoardPaletteManager.h"
#include "model/DocumentProxy.h"
#include "utils/FileSystemUtils.h"

AsyncLocalFileDownloader::AsyncLocalFileDownloader(sDownloadFileDesc desc, QObject *parent) : QThread(parent), mDesc(desc), mAborting(0)
{
}

AsyncLocalFileDownloader *AsyncLocalFileDownloader::download()
{
    if (!prepare())
    {
        return nullptr;
    }

    connect(this, &QThread::finished, this, &QObject::deleteLater);
    start();
    return this;
}

bool AsyncLocalFileDownloader::prepare()
{
    QString src = mDesc.srcUrl;
    if (src.startsWith("file://"))
    {
        src = QUrl(src).toLocalFile();
    }
    else
    {
        src = QUrl::fromLocalFile(src).toLocalFile();
    }

    QString   mimeType = FileSystemUtils::mimeTypeFromFileName(src);
    const int position = mimeType.indexOf(";");
    if (position != -1)
    {
        mimeType = mimeType.left(position);
    }

    const MimeType::Enum itemMimeType = FileSystemUtils::mimeTypeFromString(mimeType);

    QString destDirectory;
    if (MimeType::Video == itemMimeType)
    {
        destDirectory = PackageLayout::videoDirectory;
    }
    else if (MimeType::Audio == itemMimeType)
    {
        destDirectory = PackageLayout::audioDirectory;
    }

    mDesc.srcUrl = src;
    if (mDesc.originalSrcUrl.isEmpty())
    {
        mDesc.originalSrcUrl = src;
    }

    mFrom = src;
    mTo   = PersistenceManager::persistenceManager()->documentFilePath(
        Application::boardController->selectedDocument(), destDirectory, QUuid::createUuid(), QFileInfo(src).suffix());

    return !mTo.isEmpty() && QFile::exists(mFrom);
}

void AsyncLocalFileDownloader::run()
{
    // 这里只做阻塞式文件复制；任何文档与界面状态都已在 prepare() 中于主线程取好。
    bool success = false;
    if (!mAborting.loadAcquire())
    {
        QFile source(mFrom);
        success = source.copy(mTo);
    }

    if (mAborting.loadAcquire() && QFile::exists(mTo))
    {
        QFile::remove(mTo);
    }

    emit signal_asyncCopyFinished(mDesc.id, success, QUrl::fromLocalFile(mTo), QUrl::fromLocalFile(mDesc.originalSrcUrl), "", QByteArray(), mDesc.pos,
        mDesc.size, mDesc.isBackground);
}

void AsyncLocalFileDownloader::abort()
{
    mAborting.storeRelease(1);
}

static DownloadManager *pInstance = NULL;

DownloadManager::DownloadManager(QObject *parent, const char *name) : QObject(parent)
{
    setObjectName(name);
    init();
}

DownloadManager::~DownloadManager()
{
    // 本地复制线程是自己释放的，父对象析构前必须等它们真正结束，
    // 否则会出现 “QThread: Destroyed while thread is still running”。
    // 这里刻意不碰全局单例：析构发生在 Application 拆除的最后阶段。
    const QList<AsyncLocalFileDownloader *> threads = findChildren<AsyncLocalFileDownloader *>();
    for (AsyncLocalFileDownloader *thread : threads)
    {
        thread->abort();
        thread->wait();
    }
}

DownloadManager *DownloadManager::downloadManager()
{
    if (NULL == pInstance)
    {
        pInstance = new DownloadManager();
    }
    return pInstance;
}

void DownloadManager::destroy()
{
    if (pInstance)
    {
        delete pInstance;
    }
    pInstance = NULL;
}

int DownloadManager::addFileToDownload(sDownloadFileDesc desc)
{
    // 编号由管理器统一分配，调用方传入的值会被覆盖。
    desc.id = mLastID;
    mLastID++;

    mPendingDL.append(desc);

    if (desc.modal)
    {
        // 模态任务排在队首；下载窗口自己从 fileAddedToDownload 之后查有没有模态任务决定显示。
        updateDownloadOrder();
    }
    Application::boardController->paletteManager()->startDownloads();

    emit fileAddedToDownload();

    onUpdateDownloadLists();

    return desc.id;
}

void DownloadManager::init()
{
    mCrntDL.clear();
    mPendingDL.clear();
    mDownloads.clear();
    mLastID = 1;
    mDLAvailability.clear();
    for (int i = 0; i < SIMULTANEOUS_DOWNLOAD; i++)
    {
        mDLAvailability.append(-1);
    }
}

void DownloadManager::updateDownloadOrder()
{
    // 模态任务排到非模态任务之前，保证等待框对应的下载优先完成。
    QVector<sDownloadFileDesc> modalFiles;
    QVector<sDownloadFileDesc> nonModalfiles;

    for (int i = 0; i < mPendingDL.size(); i++)
    {
        sDownloadFileDesc crnt = mPendingDL.at(i);
        if (crnt.modal)
        {
            modalFiles.append(crnt);
        }
        else
        {
            nonModalfiles.append(crnt);
        }
    }

    mPendingDL = modalFiles + nonModalfiles;
}

void DownloadManager::onUpdateDownloadLists()
{
    // 并发槽位以 id 记录，-1 表示空闲；有空位就把等待队列头部移入进行中。
    for (int i = 0; i < SIMULTANEOUS_DOWNLOAD; i++)
    {
        if (mPendingDL.empty())
        {
            break;
        }
        if (-1 == mDLAvailability.at(i))
        {
            sDownloadFileDesc desc = mPendingDL.at(0);
            mCrntDL.append(desc);
            mPendingDL.remove(0);
            mDLAvailability.remove(i);
            mDLAvailability.insert(i, desc.id);

            startFileDownload(desc);
        }
    }
}

QVector<sDownloadFileDesc> DownloadManager::currentDownloads()
{
    return mCrntDL;
}

QVector<sDownloadFileDesc> DownloadManager::pendingDownloads()
{
    return mPendingDL;
}

void DownloadManager::onDownloadProgress(int id, qint64 received, qint64 total)
{
    updateFileCurrentSize(id, received, total);
}

void DownloadManager::onDownloadFinished(int id, bool pSuccess, QUrl sourceUrl, QUrl contentUrl, QString pContentTypeHeader, QByteArray pData,
    QPointF pPos, QSize pSize, bool isBackground)
{
    // 拖放位置与尺寸只对画板生效，这里先不拆参，由下游信号原样带走。
    Q_UNUSED(pPos)
    Q_UNUSED(pSize)
    Q_UNUSED(isBackground)

    for (int i = 0; i < mCrntDL.size(); i++)
    {
        sDownloadFileDesc desc = mCrntDL.at(i);
        if (id == desc.id)
        {
            if (desc.dest == sDownloadFileDesc::graphicsWidget)
            {
                desc.contentTypeHeader = pContentTypeHeader;
                emit downloadFinished(pSuccess, desc, pData);
            }
            else if (desc.dest == sDownloadFileDesc::board)
            {
                // 画板模式下由画板控制器决定落点与插入方式。
                emit addDownloadedFileToBoard(pSuccess, sourceUrl, contentUrl, pContentTypeHeader, pData, pPos, pSize, isBackground);
            }
            else
            {
                emit addDownloadedFileToLibrary(pSuccess, sourceUrl, pContentTypeHeader, pData, desc.name);
            }

            break;
        }
    }

    updateFileCurrentSize(id);
}

void DownloadManager::updateFileCurrentSize(int id, qint64 received, qint64 total)
{
    for (int i = 0; i < mCrntDL.size(); i++)
    {
        if (mCrntDL.at(i).id != id)
        {
            continue;
        }

        if (received >= 0 && total >= 0)
        {
            // 进度更新：保留总量未知（-1）的情况给下面的收尾分支。
            sDownloadFileDesc desc = mCrntDL.at(i);
            desc.currentSize       = received;
            desc.totalSize         = total;
            mCrntDL[i]             = desc;

            emit downloadUpdated(id, received, total);
            return;
        }

        // received/total 为负表示下载已结束：把大小补成总量并移出进行中列表。
        mCrntDL[i].currentSize = mCrntDL.at(i).totalSize;

        mCrntDL.remove(i);

        releaseDownload(id);

        // 归还并发槽位。
        for (int j = 0; j < mDLAvailability.size(); j++)
        {
            if (id == mDLAvailability.at(j))
            {
                mDLAvailability[j] = -1;
                break;
            }
        }

        const bool allDone = mCrntDL.isEmpty() && mPendingDL.isEmpty();

        checkIfModalRemains();

        // 走队列而不是直接递归调用：同步失败的下载会在同一栈里层层回调进来。
        if (!allDone)
        {
            QMetaObject::invokeMethod(this, &DownloadManager::onUpdateDownloadLists, Qt::QueuedConnection);
        }

        emit downloadFinished(id);

        if (allDone)
        {
            finishDownloads();
        }

        return;
    }
}

void DownloadManager::startFileDownload(sDownloadFileDesc desc)
{
    // 本地文件走工作线程复制；网络资源走 HTTP 下载，两者共用同一套完成回调。
    if (desc.srcUrl.startsWith("file://") || desc.srcUrl.startsWith("/"))
    {
        AsyncLocalFileDownloader *cpHelper = new AsyncLocalFileDownloader(desc, this);
        connect(cpHelper, &AsyncLocalFileDownloader::signal_asyncCopyFinished, this, &DownloadManager::onDownloadFinished);

        if (cpHelper->download())
        {
            mDownloads[desc.id] = cpHelper;
        }
        else
        {
            delete cpHelper;
            // 下载根本没开始，不会再有 finished 信号；手动走收尾释放并发槽，
            // 否则这个 id 永远占着 mDLAvailability 的一个位置。
            onDownloadError(desc.id);
        }
    }
    else
    {
        DownloadHttpFile *http = new DownloadHttpFile(desc.id, this);
        connect(http, &DownloadHttpFile::downloadProgress, this, &DownloadManager::onDownloadProgress);
        connect(http, &DownloadHttpFile::downloadFinished, this, &DownloadManager::onDownloadFinished);
        connect(http, &DownloadHttpFile::downloadError, this, &DownloadManager::onDownloadError);

        // 传入的地址是编码过的，先解码再交给网络层。
        QUrl url = QUrl::fromEncoded(desc.srcUrl.toUtf8());
        http->get(url, desc.pos, desc.size, desc.isBackground);

        // 记录句柄本身而不是 reply：reply 由 HttpGet 持有并负责释放。
        mDownloads[desc.id] = http;
    }
}

void DownloadManager::releaseDownload(int id)
{
    QObject *handle = mDownloads.take(id);
    if (!handle)
    {
        return;
    }

    // 本地复制线程在 run() 结束后自行 deleteLater；HTTP 句柄则由这里回收。
    if (!qobject_cast<AsyncLocalFileDownloader *>(handle))
    {
        handle->deleteLater();
    }
}

bool DownloadManager::hasModalDownload() const
{
    for (const sDownloadFileDesc &desc : mCrntDL)
    {
        if (desc.modal)
        {
            return true;
        }
    }

    for (const sDownloadFileDesc &desc : mPendingDL)
    {
        if (desc.modal)
        {
            return true;
        }
    }

    return false;
}

void DownloadManager::checkIfModalRemains()
{
    if (!hasModalDownload())
    {
        // 只剩事实要广播：下载窗口连着这个信号自己隐藏（见 ui/DownloadWidget）。
        emit downloadModalFinished();
    }
}

void DownloadManager::cancelDownloads()
{
    // 先请求中止，再清空队列；句柄自身负责延后释放。
    QMap<int, QObject *>::iterator it = mDownloads.begin();
    for (; it != mDownloads.end(); it++)
    {
        if (AsyncLocalFileDownloader *localDownload = qobject_cast<AsyncLocalFileDownloader *>(it.value()))
        {
            localDownload->abort();
        }
        else if (DownloadHttpFile *httpDownload = qobject_cast<DownloadHttpFile *>(it.value()))
        {
            httpDownload->cancel();
        }
    }

    init();

    checkIfModalRemains();

    finishDownloads(true);
}

void DownloadManager::onDownloadError(int id)
{
    // 失败也必须走一遍收尾，否则这个下载会永久占住一个并发槽位，队列就此卡死。
    onDownloadFinished(id, false, QUrl(), QUrl(), QString(), QByteArray(), QPointF(), QSize(), false);
}

void DownloadManager::finishDownloads(bool cancel)
{
    Application::boardController->paletteManager()->stopDownloads();
    if (cancel)
    {
        emit cancelAllDownloads();
    }
    else
    {
        emit allDownloadsFinished();
    }
}

void DownloadManager::cancelDownload(int id)
{
    // 两类句柄都会自行安排释放，这里只负责发出取消请求。
    QObject *handle = mDownloads.value(id);
    if (AsyncLocalFileDownloader *pLocalDownload = qobject_cast<AsyncLocalFileDownloader *>(handle))
    {
        if (pLocalDownload->isRunning())
        {
            pLocalDownload->abort();
        }
    }
    else if (DownloadHttpFile *pHttpDownload = qobject_cast<DownloadHttpFile *>(handle))
    {
        pHttpDownload->cancel();
    }

    mDownloads.remove(id);

    // 从进行中或等待列表里摘掉该任务。
    bool bFound = false;
    for (int i = 0; i < mCrntDL.size(); i++)
    {
        if (id == mCrntDL.at(i).id)
        {
            mCrntDL.remove(i);
            bFound = true;
            break;
        }
    }
    if (!bFound)
    {
        for (int j = 0; j < mPendingDL.size(); j++)
        {
            if (id == mPendingDL.at(j).id)
            {
                mPendingDL.remove(j);
                bFound = true;
                break;
            }
        }
    }

    // 归还并发槽位。
    for (int h = 0; h < mDLAvailability.size(); h++)
    {
        if (id == mDLAvailability.at(h))
        {
            mDLAvailability.remove(h);
            mDLAvailability.insert(h, -1);
            break;
        }
    }

    checkIfModalRemains();

    onUpdateDownloadLists();

    if (mCrntDL.empty() && mPendingDL.empty())
    {
        finishDownloads();
    }
}

DownloadHttpFile::DownloadHttpFile(int fileId, QObject *parent) : HttpGet(parent), mId(fileId), mCancelled(false)
{
    connect(this, &HttpGet::downloadFinished, this, &DownloadHttpFile::onDownloadFinished);
    connect(this, &HttpGet::downloadProgress, this, &DownloadHttpFile::onDownloadProgress);
}

void DownloadHttpFile::cancel()
{
    mCancelled = true;
    cancelRequest();
    deleteLater();
}

void DownloadHttpFile::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    emit downloadProgress(mId, bytesReceived, bytesTotal);
}

void DownloadHttpFile::onDownloadFinished(
    bool pSuccess, QUrl sourceUrl, QString pContentTypeHeader, QByteArray pData, QPointF pPos, QSize pSize, bool isBackground)
{
    if (pSuccess)
    {
        emit downloadFinished(mId, pSuccess, sourceUrl, sourceUrl, pContentTypeHeader, pData, pPos, pSize, isBackground);
    }
    else
    {
        emit downloadError(mId);
    }

    // 一次性的下载对象：转发完结果就自行回收，否则整个响应体一直挂在下载管理器下面。
    if (!mCancelled)
    {
        deleteLater();
    }
}
