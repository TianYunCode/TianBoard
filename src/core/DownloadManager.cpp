#include "core/DownloadManager.h"
#include "core/Application.h"
#include "core/PersistenceManager.h"
#include "ui/MainWindow.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/BoardPaletteManager.h"
#include "model/DocumentProxy.h"
#include "utils/FileSystemUtils.h"


AsyncLocalFileDownloader::AsyncLocalFileDownloader(sDownloadFileDesc desc, QObject *parent)
    : QThread(parent), mDesc(desc), mAborting(0) {}

AsyncLocalFileDownloader *AsyncLocalFileDownloader::download() {
    if (!prepare())
        return nullptr;

    connect(this, &QThread::finished, this, &QObject::deleteLater);
    start();
    return this;
}

bool AsyncLocalFileDownloader::prepare() {
    QString src = mDesc.srcUrl;
    if (src.startsWith("file://"))
        src = QUrl(src).toLocalFile();
    else
        src = QUrl::fromLocalFile(src).toLocalFile();

    QString mimeType = FileSystemUtils::mimeTypeFromFileName(src);
    const int position = mimeType.indexOf(";");
    if (position != -1)
        mimeType = mimeType.left(position);

    const MimeType::Enum itemMimeType = FileSystemUtils::mimeTypeFromString(mimeType);

    QString destDirectory;
    if (MimeType::Video == itemMimeType)
        destDirectory = PersistenceManager::videoDirectory;
    else if (MimeType::Audio == itemMimeType)
        destDirectory = PersistenceManager::audioDirectory;

    mDesc.srcUrl = src;
    if (mDesc.originalSrcUrl.isEmpty())
        mDesc.originalSrcUrl = src;

    mFrom = src;
    mTo = PersistenceManager::persistenceManager()->documentFilePath(
        Application::boardController->selectedDocument(), destDirectory, QUuid::createUuid(),
        QFileInfo(src).suffix());

    return !mTo.isEmpty() && QFile::exists(mFrom);
}

void AsyncLocalFileDownloader::run() {
    // 这里只做阻塞式文件复制；任何文档与界面状态都已在 prepare() 中于主线程取好。
    bool success = false;
    if (!mAborting.loadAcquire()) {
        QFile source(mFrom);
        success = source.copy(mTo);
    }

    if (mAborting.loadAcquire() && QFile::exists(mTo))
        QFile::remove(mTo);

    emit signal_asyncCopyFinished(mDesc.id, success, QUrl::fromLocalFile(mTo),
                                  QUrl::fromLocalFile(mDesc.originalSrcUrl), "", QByteArray(), mDesc.pos,
                                  mDesc.size, mDesc.isBackground);
}

void AsyncLocalFileDownloader::abort() {
    mAborting.storeRelease(1);
}

/** The unique instance of the download manager */
static DownloadManager *pInstance = NULL;

/**
 * \brief Constructor
 * @param parent as the parent widget
 * @param name as the object name
 */
DownloadManager::DownloadManager(QObject *parent, const char *name) : QObject(parent) {
    setObjectName(name);
    init();
}

/**
 * \brief Destructor
 */
DownloadManager::~DownloadManager() {
    // 本地复制线程是自己释放的，父对象析构前必须等它们真正结束，
    // 否则会出现 “QThread: Destroyed while thread is still running”。
    // 这里刻意不碰全局单例：析构发生在 Application 拆除的最后阶段。
    const QList<AsyncLocalFileDownloader *> threads = findChildren<AsyncLocalFileDownloader *>();
    for (AsyncLocalFileDownloader *thread : threads) {
        thread->abort();
        thread->wait();
    }
}

/**
 * \brief Get the download manager
 * @return a pointer on the download manager
 */
DownloadManager *DownloadManager::downloadManager() {
    if (NULL == pInstance) {
        pInstance = new DownloadManager();
    }
    return pInstance;
}

void DownloadManager::destroy() {
    if (pInstance) {
        delete pInstance;
    }
    pInstance = NULL;
}

/**
 * \brief Add a file to the download list
 * @param desc as the given file description
 */
int DownloadManager::addFileToDownload(sDownloadFileDesc desc) {
    // Set the ID for this download
    desc.id = mLastID;
    mLastID++;

    // Add the file to the pending download list
    mPendingDL.append(desc);

    // If the download is modal, show the download dialog
    if (desc.modal) {
        // Update the download order (priority to modal files)
        updateDownloadOrder();
        Application::mainWindow->showDownloadWidget();
    }
    Application::boardController->paletteManager()->startDownloads();

    emit fileAddedToDownload();

    onUpdateDownloadLists();

    return desc.id;
}

/**
 * \brief Initialize the download manager
 */
void DownloadManager::init() {
    mCrntDL.clear();
    mPendingDL.clear();
    mDownloads.clear();
    mLastID = 1;
    mDLAvailability.clear();
    for (int i = 0; i < SIMULTANEOUS_DOWNLOAD; i++) {
        mDLAvailability.append(-1);
    }
}

/**
 * \brief Update the download order. The modal downloads will be put in priority.
 */
void DownloadManager::updateDownloadOrder() {
    QVector<sDownloadFileDesc> modalFiles;
    QVector<sDownloadFileDesc> nonModalfiles;

    for (int i = 0; i < mPendingDL.size(); i++) {
        sDownloadFileDesc crnt = mPendingDL.at(i);
        if (crnt.modal) {
            modalFiles.append(crnt);
        } else {
            nonModalfiles.append(crnt);
        }
    }

    mPendingDL = modalFiles + nonModalfiles;
}

/**
 * \brief Update the download list. If a current download is finished, we take a
 *  file from the pending download list and add it to the download list.
 */
void DownloadManager::onUpdateDownloadLists() {
    for (int i = 0; i < SIMULTANEOUS_DOWNLOAD; i++) {
        if (mPendingDL.empty()) {
            // If we fall here that means that there is no pending download
            break;
        }
        if (-1 == mDLAvailability.at(i)) {
            // Pending downloads exist and a download 'slot' is available
            // Let's move the first pending download to the current download
            // list and fill the slot
            sDownloadFileDesc desc = mPendingDL.at(0);
            mCrntDL.append(desc);
            mPendingDL.remove(0);
            mDLAvailability.remove(i);
            mDLAvailability.insert(i, desc.id);

            // Start the download of this file
            startFileDownload(desc);
        }
    }
}

/**
 * \brief Get the list of the current downloads
 * @return a QVector of current downloads
 */
QVector<sDownloadFileDesc> DownloadManager::currentDownloads() {
    return mCrntDL;
}

/**
 * \brief Get the list of the pending downloads
 * @return a QVector of pending downloads
 */
QVector<sDownloadFileDesc> DownloadManager::pendingDownloads() {
    return mPendingDL;
}

/**
 * \brief Update the file transfer information
 * @param desc as the current downloaded file description
 */
void DownloadManager::onDownloadProgress(int id, qint64 received, qint64 total) {
    updateFileCurrentSize(id, received, total);
}

/**
 * \brief Called when the download of the given file is finished
 * @param desc as the current downloaded file description
 */

void DownloadManager::onDownloadFinished(int id, bool pSuccess, QUrl sourceUrl, QUrl contentUrl,
                                         QString pContentTypeHeader, QByteArray pData, QPointF pPos,
                                         QSize pSize, bool isBackground) {
    //    Temporary data for dnd do not delete it please
    Q_UNUSED(pPos)
    Q_UNUSED(pSize)
    Q_UNUSED(isBackground)

    for (int i = 0; i < mCrntDL.size(); i++) {
        sDownloadFileDesc desc = mCrntDL.at(i);
        if (id == desc.id) {
            if (desc.dest == sDownloadFileDesc::graphicsWidget) {
                desc.contentTypeHeader = pContentTypeHeader;
                emit downloadFinished(pSuccess, desc, pData);

            } else if (desc.dest == sDownloadFileDesc::board) {
                // The downloaded file is modal so we must put it on the board
                emit addDownloadedFileToBoard(pSuccess, sourceUrl, contentUrl, pContentTypeHeader, pData,
                                              pPos, pSize, isBackground);
            } else {
                emit addDownloadedFileToLibrary(pSuccess, sourceUrl, pContentTypeHeader, pData, desc.name);
            }

            break;
        }
    }

    // Then do this
    updateFileCurrentSize(id);
}

/**
 * \brief Update the description of the given current downloaded file
 * @param desc as the current downloaded file description
 */
void DownloadManager::updateFileCurrentSize(int id, qint64 received, qint64 total) {
    for (int i = 0; i < mCrntDL.size(); i++) {
        if (mCrntDL.at(i).id != id)
            continue;

        if (received >= 0 && total >= 0) {
            // -------------------------------------
            // [=============== x% ====            ]
            // -------------------------------------
            sDownloadFileDesc desc = mCrntDL.at(i);
            desc.currentSize = received;
            desc.totalSize = total;
            mCrntDL[i] = desc;

            emit downloadUpdated(id, received, total);
            return;
        }

        // -------------------------------------
        // [=============== 100% ==============]
        // -------------------------------------
        // received and total are negative. That means that the download is finished
        mCrntDL[i].currentSize = mCrntDL.at(i).totalSize;

        // Remove the finished file from the current download list
        mCrntDL.remove(i);

        // Here we don't forget to remove the reply related to the finished download
        releaseDownload(id);

        // Free the download slot used by the finished file
        for (int j = 0; j < mDLAvailability.size(); j++) {
            if (id == mDLAvailability.at(j)) {
                mDLAvailability[j] = -1;
                break;
            }
        }

        const bool allDone = mCrntDL.isEmpty() && mPendingDL.isEmpty();

        // Here we check if some modal downloads remain
        checkIfModalRemains();

        // Then we start the next pending download. 走队列而不是直接递归调用：
        // 同步失败的下载会在同一栈里层层回调进来。
        if (!allDone)
            QMetaObject::invokeMethod(this, &DownloadManager::onUpdateDownloadLists, Qt::QueuedConnection);

        emit downloadFinished(id);

        // Verify if all downloads are finished
        if (allDone) {
            finishDownloads();
        }

        return;
    }
}

/**
 * \brief Start the download of a file
 * @param desc as the given file description
 */
void DownloadManager::startFileDownload(sDownloadFileDesc desc) {
    if (desc.srcUrl.startsWith("file://") || desc.srcUrl.startsWith("/")) {
        AsyncLocalFileDownloader *cpHelper = new AsyncLocalFileDownloader(desc, this);
        connect(cpHelper, &AsyncLocalFileDownloader::signal_asyncCopyFinished, this,
                &DownloadManager::onDownloadFinished);

        if (cpHelper->download())
            mDownloads[desc.id] = cpHelper;
        else
            delete cpHelper;
    } else {
        DownloadHttpFile *http = new DownloadHttpFile(desc.id, this);
        connect(http, &DownloadHttpFile::downloadProgress, this, &DownloadManager::onDownloadProgress);
        connect(http, &DownloadHttpFile::downloadFinished, this, &DownloadManager::onDownloadFinished);
        connect(http, &DownloadHttpFile::downloadError, this, &DownloadManager::onDownloadError);

        //the desc.srcUrl is encoded. So we have to decode it before.
        QUrl url = QUrl::fromEncoded(desc.srcUrl.toUtf8());
        http->get(url, desc.pos, desc.size, desc.isBackground);

        // 记录句柄本身而不是 reply：reply 由 HttpGet 持有并负责释放。
        mDownloads[desc.id] = http;
    }
}

void DownloadManager::releaseDownload(int id) {
    QObject *handle = mDownloads.take(id);
    if (!handle)
        return;

    // 本地复制线程在 run() 结束后自行 deleteLater；HTTP 句柄则由这里回收。
    if (!qobject_cast<AsyncLocalFileDownloader *>(handle))
        handle->deleteLater();
}

/**
 * \brief Verify if modal downloads remains and notify everyone if it is not the case.
 */
void DownloadManager::checkIfModalRemains() {
    bool bModal = false;
    for (int i = 0; i < mCrntDL.size(); i++) {
        if (mCrntDL.at(i).modal) {
            bModal = true;
            break;
        }
    }

    if (!bModal) {
        for (int j = 0; j < mPendingDL.size(); j++) {
            if (mPendingDL.at(j).modal) {
                bModal = true;
                break;
            }
        }
    }

    if (bModal || (mCrntDL.empty() && mPendingDL.empty())) {
        // Close the modal window
        Application::mainWindow->hideDownloadWidget();

        // Notify that no modal downloads are pending
        emit downloadModalFinished();
    }
}

/**
 * \brief Cancel all downloads
 */
void DownloadManager::cancelDownloads() {
    // Stop the current downloads
    QMap<int, QObject *>::iterator it = mDownloads.begin();
    for (; it != mDownloads.end(); it++) {
        if (AsyncLocalFileDownloader *localDownload = qobject_cast<AsyncLocalFileDownloader *>(it.value()))
            localDownload->abort();
        else if (DownloadHttpFile *httpDownload = qobject_cast<DownloadHttpFile *>(it.value()))
            httpDownload->cancel();
    }

    // Clear all the lists
    init();

    checkIfModalRemains();

    finishDownloads(true);
}

void DownloadManager::onDownloadError(int id) {
    // 失败也必须走一遍收尾，否则这个下载会永久占住一个并发槽位，队列就此卡死。
    onDownloadFinished(id, false, QUrl(), QUrl(), QString(), QByteArray(), QPointF(), QSize(), false);
}

void DownloadManager::finishDownloads(bool cancel) {
    Application::boardController->paletteManager()->stopDownloads();
    if (cancel) {
        emit cancelAllDownloads();
    } else {
        emit allDownloadsFinished();
    }
}

void DownloadManager::cancelDownload(int id) {
    // Stop the download. 两类句柄都会自行安排释放，这里只负责发出取消请求。
    QObject *handle = mDownloads.value(id);
    if (AsyncLocalFileDownloader *pLocalDownload = qobject_cast<AsyncLocalFileDownloader *>(handle)) {
        if (pLocalDownload->isRunning())
            pLocalDownload->abort();
    } else if (DownloadHttpFile *pHttpDownload = qobject_cast<DownloadHttpFile *>(handle)) {
        pHttpDownload->cancel();
    }

    mDownloads.remove(id);

    // Remove the canceled download from the download lists
    bool bFound = false;
    for (int i = 0; i < mCrntDL.size(); i++) {
        if (id == mCrntDL.at(i).id) {
            mCrntDL.remove(i);
            bFound = true;
            break;
        }
    }
    if (!bFound) {
        for (int j = 0; j < mPendingDL.size(); j++) {
            if (id == mPendingDL.at(j).id) {
                mPendingDL.remove(j);
                bFound = true;
                break;
            }
        }
    }

    // Free the download slot used by the finished file
    for (int h = 0; h < mDLAvailability.size(); h++) {
        if (id == mDLAvailability.at(h)) {
            mDLAvailability.remove(h);
            mDLAvailability.insert(h, -1);
            break;
        }
    }

    // Here we check if some modal downloads remain
    checkIfModalRemains();

    // Then we update the list of downloads
    onUpdateDownloadLists();

    // Verify if all downloads are finished
    if (mCrntDL.empty() && mPendingDL.empty()) {
        finishDownloads();
    }
}

// ------------------------------------------------------------------------------
DownloadHttpFile::DownloadHttpFile(int fileId, QObject *parent)
    : HttpGet(parent), mId(fileId), mCancelled(false) {
    connect(this, &HttpGet::downloadFinished, this, &DownloadHttpFile::onDownloadFinished);
    connect(this, &HttpGet::downloadProgress, this, &DownloadHttpFile::onDownloadProgress);
}

void DownloadHttpFile::cancel() {
    mCancelled = true;
    cancelRequest();
    deleteLater();
}

void DownloadHttpFile::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    emit downloadProgress(mId, bytesReceived, bytesTotal);
}

void DownloadHttpFile::onDownloadFinished(bool pSuccess, QUrl sourceUrl, QString pContentTypeHeader,
                                          QByteArray pData, QPointF pPos, QSize pSize, bool isBackground) {
    if (pSuccess) {
        // Notify the end of the download
        emit downloadFinished(mId, pSuccess, sourceUrl, sourceUrl, pContentTypeHeader, pData, pPos, pSize,
                              isBackground);
    } else {
        // Notify the fact that and error occured during the download
        emit downloadError(mId);
    }

    // 一次性的下载对象：转发完结果就自行回收，否则整个响应体一直挂在下载管理器下面。
    if (!mCancelled)
        deleteLater();
}
