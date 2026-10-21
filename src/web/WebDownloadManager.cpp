#include "web/WebDownloadManager.h"
#include <QWebEngineProfile>

#include <QtWidgets>
#include <QtWebEngineWidgets>
#include <QWebEngineSettings>
#include <QFileDialog>

#include "web/EditTableView.h"
#include "web/SqueezeLabel.h"

#include "utils/AutoSaver.h"
#include "core/NetworkAccessManager.h"
#include "utils/FileSystemUtils.h"

#include <math.h>

struct DownloadItem::Private
{
    QHBoxLayout  *horizontalLayout{nullptr};
    QLabel       *fileIcon{nullptr};
    QVBoxLayout  *verticalLayout_2{nullptr};
    SqueezeLabel *fileNameLabel{nullptr};
    QProgressBar *progressBar{nullptr};
    SqueezeLabel *downloadInfoLabel{nullptr};
    QVBoxLayout  *verticalLayout{nullptr};
    QPushButton  *tryAgainButton{nullptr};
    QPushButton  *stopButton{nullptr};
    QPushButton  *openButton{nullptr};
};

DownloadItem::DownloadItem(QNetworkReply *reply, bool requestFileName, QWidget *parent, QString customDownloadPath)
    : QWidget(parent), m_reply(reply), mRequestFileName(requestFileName), mBytesReceived(0), mCustomDownloadPath(customDownloadPath), d(new Private)
{
    if (objectName().isEmpty())
    {
        setObjectName("DownloadItem");
    }
    resize(423, 118);
    setWindowTitle(tr("Form"));

    d->horizontalLayout = new QHBoxLayout(this);
    d->horizontalLayout->setContentsMargins(0, 0, 0, 0);
    d->horizontalLayout->setObjectName("horizontalLayout");

    d->fileIcon = new QLabel(this);
    d->fileIcon->setObjectName("fileIcon");
    d->fileIcon->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
    d->fileIcon->setText(QString::fromUtf8("Ico"));
    d->horizontalLayout->addWidget(d->fileIcon);

    d->verticalLayout_2 = new QVBoxLayout();
    d->verticalLayout_2->setObjectName("verticalLayout_2");

    d->fileNameLabel = new SqueezeLabel(this);
    d->fileNameLabel->setObjectName("fileNameLabel");
    d->fileNameLabel->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
    d->fileNameLabel->setText(tr("Filename"));
    d->verticalLayout_2->addWidget(d->fileNameLabel);

    d->progressBar = new QProgressBar(this);
    d->progressBar->setObjectName("progressBar");
    d->progressBar->setValue(0);
    d->verticalLayout_2->addWidget(d->progressBar);

    d->downloadInfoLabel = new SqueezeLabel(this);
    d->downloadInfoLabel->setObjectName("downloadInfoLabel");
    d->downloadInfoLabel->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred));
    d->downloadInfoLabel->setText(QString());
    d->verticalLayout_2->addWidget(d->downloadInfoLabel);

    d->horizontalLayout->addLayout(d->verticalLayout_2);

    d->verticalLayout = new QVBoxLayout();
    d->verticalLayout->setObjectName("verticalLayout");
    d->verticalLayout->addItem(new QSpacerItem(17, 1, QSizePolicy::Minimum, QSizePolicy::Expanding));

    d->tryAgainButton = new QPushButton(this);
    d->tryAgainButton->setObjectName("tryAgainButton");
    d->tryAgainButton->setEnabled(false);
    d->tryAgainButton->setText(tr("Try Again"));
    d->verticalLayout->addWidget(d->tryAgainButton);

    d->stopButton = new QPushButton(this);
    d->stopButton->setObjectName("stopButton");
    d->stopButton->setText(tr("Stop"));
    d->verticalLayout->addWidget(d->stopButton);

    d->openButton = new QPushButton(this);
    d->openButton->setObjectName("openButton");
    d->openButton->setText(tr("Open"));
    d->verticalLayout->addWidget(d->openButton);

    d->verticalLayout->addItem(new QSpacerItem(17, 5, QSizePolicy::Minimum, QSizePolicy::Expanding));

    d->horizontalLayout->addLayout(d->verticalLayout);

    QPalette p = d->downloadInfoLabel->palette();
    p.setColor(QPalette::Text, Qt::darkGray);
    d->downloadInfoLabel->setPalette(p);
    d->progressBar->setMaximum(0);
    d->tryAgainButton->hide();
    connect(d->stopButton, &QAbstractButton::clicked, this,
        [this]
        {
            stop();
        });
    connect(d->openButton, &QAbstractButton::clicked, this,
        [this]
        {
            open();
        });
    connect(d->tryAgainButton, &QAbstractButton::clicked, this,
        [this]
        {
            tryAgain();
        });

    init();
}

DownloadItem::~DownloadItem() = default;

void DownloadItem::init()
{
    if (!m_reply)
    {
        return;
    }

    // 绑定（或重试后重新绑定）网络回复：下载项的整个状态机都由这些信号驱动。
    m_url = m_reply->url();

    qDebug() << "DownloadItem::init() with url:" << m_url;

    m_reply->setParent(this);
    connect(m_reply, &QNetworkReply::readyRead, this, &DownloadItem::downloadReadyRead);
    connect(m_reply, &QNetworkReply::errorOccurred, this, &DownloadItem::error);
    connect(m_reply, &QNetworkReply::downloadProgress, this, &DownloadItem::downloadProgress);
    connect(m_reply, &QNetworkReply::metaDataChanged, this, &DownloadItem::metaDataChanged);
    connect(m_reply, &QNetworkReply::finished, this, &DownloadItem::finished);

    // 重试复用时要把上一轮的进度与提示清干净。
    d->downloadInfoLabel->clear();
    d->progressBar->setValue(0);
    getFileName();

    // 计时从请求绑定开始，用于估算平均速度与剩余时间。
    mDownloadTime.start();

    if (m_reply->error() != QNetworkReply::NoError)
    {
        error(m_reply->error());
        finished();
    }
}

void DownloadItem::getFileName()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("downloadmanager"));
    QString defaultLocation =
        !mCustomDownloadPath.isEmpty() ? mCustomDownloadPath : QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString downloadDirectory = settings.value(QLatin1String("downloadDirectory"), defaultLocation).toString();
    if (!downloadDirectory.isEmpty())
    {
        downloadDirectory += QLatin1Char('/');
    }

    QString defaultFileName = saveFileName(downloadDirectory);

    QString fileName        = defaultFileName;
    if (mRequestFileName)
    {
        fileName = QFileDialog::getSaveFileName(this, tr("Save File"), defaultFileName);
        if (fileName.isEmpty())
        {
            m_reply->close();
            d->fileNameLabel->setText(tr("Download canceled: %1").arg(QFileInfo(defaultFileName).fileName()));
            return;
        }
    }

    m_output.setFileName(fileName);
    d->fileNameLabel->setText(QFileInfo(m_output.fileName()).fileName());
    if (mRequestFileName)
    {
        downloadReadyRead();
    }
}

QString DownloadItem::saveFileName(const QString &directory) const
{
    // 文件名直接取自 URL 的最后一段；服务器在 Content-Disposition 中给出的名字没有解析。
    QString   path = m_url.path();
    QFileInfo info(path);
    QString   baseName = info.completeBaseName();
    QString   endName  = info.suffix();

    if (baseName.isEmpty())
    {
        baseName = QLatin1String("unnamed_download");
        qDebug() << "DownloadManager:: downloading unknown file:" << m_url;
    }
    QString name = directory + baseName + QLatin1Char('.') + endName;
    if (QFile::exists(name))
    {
        // 已存在同名文件时不覆盖，追加 -1、-2 … 直到找到空位。
        int i = 1;
        do
        {
            name = directory + baseName + QLatin1Char('-') + QString::number(i++) + QLatin1Char('.') + endName;
        } while (QFile::exists(name));
    }
    return name;
}

void DownloadItem::stop()
{
    // 状态机：先切到“已停止/可重试”的界面，再中止网络回复；
    // 中止会触发 finished，后者只负责收尾，不改变可重试状态。
    setUpdatesEnabled(false);
    d->stopButton->setEnabled(false);
    d->stopButton->hide();
    d->tryAgainButton->setEnabled(true);
    d->tryAgainButton->show();
    setUpdatesEnabled(true);
    m_reply->abort();
}

void DownloadItem::open()
{
    QUrl url = QUrl::fromLocalFile(m_output.fileName());
    QDesktopServices::openUrl(url);
}

void DownloadItem::tryAgain()
{
    if (!d->tryAgainButton->isEnabled())
    {
        return;
    }

    // 重试即重新开一次请求：先恢复“下载中”的按钮状态，再丢弃旧回复与残留文件，
    // 否则第二次写入会拼接到上次的半截数据后面。
    d->tryAgainButton->setEnabled(false);
    d->tryAgainButton->setVisible(false);
    d->stopButton->setEnabled(true);
    d->stopButton->setVisible(true);
    d->progressBar->setVisible(true);

    QNetworkReply *r = NetworkAccessManager::defaultAccessManager()->get(QNetworkRequest(m_url));
    if (m_reply)
    {
        m_reply->deleteLater();
    }
    if (m_output.exists())
    {
        m_output.remove();
    }
    m_reply = r;
    init();
    emit statusChanged();
}

void DownloadItem::downloadReadyRead()
{
    if (mRequestFileName && m_output.fileName().isEmpty())
    {
        return;
    }

    if (!m_output.isOpen())
    {
        // 输出文件延迟到第一块数据到达时才打开；非“另存为”模式在这里补一次文件名解析，
        // 防止同名文件在等待期间被别人创建。
        if (!mRequestFileName)
        {
            getFileName();
        }

        if (!m_output.open(QIODevice::WriteOnly))
        {
            d->downloadInfoLabel->setText(tr("Error opening saved file: %1").arg(m_output.errorString()));
            d->stopButton->click();
            emit statusChanged();
            return;
        }

        emit statusChanged();
    }

    if (-1 == m_output.write(m_reply->readAll()))
    {
        d->downloadInfoLabel->setText(tr("Error saving: %1").arg(m_output.errorString()));
        d->stopButton->click();
    }
}

void DownloadItem::error(QNetworkReply::NetworkError)
{
    qDebug() << "DownloadItem::error" << m_reply->errorString() << m_url;
    d->downloadInfoLabel->setText(tr("Network Error: %1").arg(m_reply->errorString()));
    d->tryAgainButton->setEnabled(true);
    d->tryAgainButton->setVisible(true);
}

void DownloadItem::metaDataChanged()
{
    qDebug() << "DownloadItem::metaDataChanged: not handled.";
}

void DownloadItem::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    mBytesReceived = bytesReceived;
    if (bytesTotal == -1)
    {
        d->progressBar->setValue(0);
        d->progressBar->setMaximum(0);
    }
    else
    {
        d->progressBar->setValue(bytesReceived);
        d->progressBar->setMaximum(bytesTotal);
    }
    updateInfoLabel();
}

void DownloadItem::updateInfoLabel()
{
    // 错误提示由错误分支负责；正常下载时进度、速度与剩余时间都要继续刷新。
    if (m_reply->error() != QNetworkReply::NoError)
    {
        return;
    }

    qint64 bytesTotal                = d->progressBar->maximum();
    bool   running                   = !downloadedSuccessfully();

    // 重算速度、剩余时间与标签文案
    const qint64 elapsedMs           = mDownloadTime.elapsed();
    double       speed               = elapsedMs > 0 ? mBytesReceived * 1000.0 / elapsedMs : 0.0;
    double       timeRemaining       = ((double)(bytesTotal - mBytesReceived)) / speed;
    QString      timeRemainingString = tr("seconds");
    if (timeRemaining > 60)
    {
        timeRemaining       = timeRemaining / 60;
        timeRemainingString = tr("minutes");
    }
    timeRemaining = floor(timeRemaining);

    // 仍在下载时剩余时间不许显示 0，否则看着像已经卡死
    if (timeRemaining == 0)
    {
        timeRemaining = 1;
    }

    QString info;
    if (running)
    {
        QString remaining;
        if (bytesTotal != 0)
        {
            remaining = tr("- %4 %5 remaining").arg(timeRemaining).arg(timeRemainingString);
        }
        info = QString(tr("%1 of %2 (%3/sec) %4"))
                   .arg(dataString(mBytesReceived))
                   .arg(bytesTotal == 0 ? tr("?", "unknown file size") : dataString(bytesTotal))
                   .arg(dataString((int)speed))
                   .arg(remaining);
    }
    else
    {
        if (mBytesReceived == bytesTotal)
        {
            info = dataString(m_output.size());
        }
        else
        {
            info = tr("%1 of %2 - Stopped").arg(dataString(mBytesReceived)).arg(dataString(bytesTotal));
        }
    }
    d->downloadInfoLabel->setText(info);
}

QString DownloadItem::dataString(int size) const
{
    QString unit;
    if (size < 1024)
    {
        unit = tr("bytes");
    }
    else if (size < 1024 * 1024)
    {
        size /= 1024;
        unit  = tr("KB");
    }
    else
    {
        size /= 1024 * 1024;
        unit  = tr("MB");
    }
    return QString(QLatin1String("%1 %2")).arg(size).arg(unit);
}

bool DownloadItem::downloading() const
{
    return (d->progressBar->isVisible());
}

bool DownloadItem::downloadedSuccessfully() const
{
    return (d->stopButton->isHidden() && d->tryAgainButton->isHidden());
}

bool DownloadItem::isActive() const
{
    return d->stopButton->isEnabled();
}

bool DownloadItem::retryAvailable() const
{
    return d->tryAgainButton->isEnabled();
}

void DownloadItem::setFileName(const QString &fileName)
{
    d->fileNameLabel->setText(fileName);
}

void DownloadItem::setFileIconPixmap(const QPixmap &pixmap)
{
    d->fileIcon->setPixmap(pixmap);
}

// 把条目收进“下载结束”的样子：成功就什么都不留，被关程序打断则留下重试按钮和进度条。
void DownloadItem::setDownloadCompleted(bool completed)
{
    d->stopButton->setVisible(false);
    d->stopButton->setEnabled(false);
    d->tryAgainButton->setVisible(!completed);
    d->tryAgainButton->setEnabled(!completed);
    d->progressBar->setVisible(!completed);
}

QString DownloadItem::infoText() const
{
    return d->downloadInfoLabel->text();
}

void DownloadItem::finished()
{
    d->progressBar->hide();
    d->stopButton->setEnabled(false);
    d->stopButton->hide();
    m_output.close();
    updateInfoLabel();
    if (m_output.fileName().toLower().endsWith(".wgt") || m_output.fileName().toLower().endsWith(".wdgt"))
    {
        QString destPath = QFileInfo(m_output.fileName()).absolutePath() + "/Dir" + QFileInfo(m_output.fileName()).fileName();
        if (!QFileInfo(m_output.fileName()).isDir())
        {
            FileSystemUtils::expandZipToDir(QFile(m_output.fileName()), QDir(destPath));
            m_output.remove();
        }
    }
    emit statusChanged();
}

struct WebDownloadManager::Private
{
    QGridLayout   *gridLayout{nullptr};
    EditTableView *downloadsView{nullptr};
    QHBoxLayout   *horizontalLayout{nullptr};
    QPushButton   *cleanupButton{nullptr};
    QLabel        *itemCount{nullptr};
};

// 下载管理器就是一个装着若干下载条目的对话框，只负责把文件落到磁盘上：
// 不做种子、不自动解包、不识别断点续传之外的任何花样。
WebDownloadManager::WebDownloadManager(QWidget *parent)
    : QDialog(parent), mAutoSaver(new AutoSaver(this)), mManager(NetworkAccessManager::defaultAccessManager()), mIconProvider(0),
      m_RemovePolicy(Never), d(new Private)
{
    if (objectName().isEmpty())
    {
        setObjectName("DownloadDialog");
    }
    resize(332, 252);
    setWindowTitle(tr("Downloads"));

    d->gridLayout = new QGridLayout(this);
    d->gridLayout->setSpacing(0);
    d->gridLayout->setContentsMargins(0, 0, 0, 0);
    d->gridLayout->setObjectName("gridLayout");

    d->downloadsView = new EditTableView(this);
    d->downloadsView->setObjectName("downloadsView");
    d->gridLayout->addWidget(d->downloadsView, 0, 0, 1, 3);

    d->horizontalLayout = new QHBoxLayout();
    d->horizontalLayout->setObjectName("horizontalLayout");

    d->cleanupButton = new QPushButton(this);
    d->cleanupButton->setObjectName("cleanupButton");
    d->cleanupButton->setEnabled(false);
    d->cleanupButton->setText(tr("Clean Up"));
    d->horizontalLayout->addWidget(d->cleanupButton);

    d->horizontalLayout->addItem(new QSpacerItem(58, 24, QSizePolicy::Expanding, QSizePolicy::Minimum));

    d->gridLayout->addLayout(d->horizontalLayout, 1, 0, 1, 1);

    d->itemCount = new QLabel(this);
    d->itemCount->setObjectName("itemCount");
    d->itemCount->setText(tr("0 Items"));
    d->gridLayout->addWidget(d->itemCount, 1, 1, 1, 1);

    d->gridLayout->addItem(new QSpacerItem(148, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), 1, 2, 1, 1);

    hide();

    d->downloadsView->setShowGrid(false);
    d->downloadsView->verticalHeader()->hide();
    d->downloadsView->horizontalHeader()->hide();
    d->downloadsView->setAlternatingRowColors(true);
    d->downloadsView->horizontalHeader()->setStretchLastSection(true);
    mModel = new DownloadModel(this);
    d->downloadsView->setModel(mModel);
    connect(d->cleanupButton, &QAbstractButton::clicked, this,
        [this]
        {
            cleanup();
        });
    load();
}

WebDownloadManager::~WebDownloadManager()
{
    mAutoSaver->changeOccurred();
    mAutoSaver->saveIfNeccessary();
    if (mIconProvider)
    {
        delete mIconProvider;
    }
}

int WebDownloadManager::activeDownloads() const
{
    int count = 0;
    for (int i = 0; i < mDownloads.count(); ++i)
    {
        if (mDownloads.at(i)->isActive())
        {
            ++count;
        }
    }
    return count;
}

void WebDownloadManager::download(const QNetworkRequest &request, bool requestFileName)
{
    if (request.url().isEmpty())
    {
        return;
    }
    processDownloadedContent(mManager->get(request), requestFileName);
}

void WebDownloadManager::handleUnsupportedContent(QNetworkReply *reply, bool requestFileName, QString customDownloadPath)
{
    QUrl originalUrl = reply->request().url();

    if (originalUrl.isEmpty())
    {
        return;
    }

    processDownloadedContent(mManager->get(QNetworkRequest(originalUrl)), requestFileName, customDownloadPath);
}

void WebDownloadManager::processDownloadedContent(QNetworkReply *reply, bool requestFileName, QString customDownloadPath)
{
    if (!reply || reply->url().isEmpty())
    {
        return;
    }
    QVariant header = reply->header(QNetworkRequest::ContentLengthHeader);

    bool ok;
    int  size = header.toInt(&ok);
    if (ok && size == 0)
    {
        return;
    }

    // 到这里内容类型不认识，先看看是不是个重定向

    qDebug() << "redirect" << reply->header(QNetworkRequest::LocationHeader);
    qDebug() << "ContentTypeHeader" << reply->header(QNetworkRequest::ContentTypeHeader);
    qDebug() << "DownloadManager::handleUnsupportedContent" << reply->url() << "requestFileName" << requestFileName;

    DownloadItem *item = new DownloadItem(reply, requestFileName, this, customDownloadPath);
    addItem(item);
}

void WebDownloadManager::addItem(DownloadItem *item)
{
    connect(item, &DownloadItem::statusChanged, this, &WebDownloadManager::updateRow);
    int row = mDownloads.count();
    mModel->beginInsertRows(QModelIndex(), row, row);
    mDownloads.append(item);
    mModel->endInsertRows();
    updateItemCount();

    if (!mIsLoading)
    {
        show();
    }

    d->downloadsView->setIndexWidget(mModel->index(row, 0), item);

    if (!mIconProvider)
    {
        mIconProvider = new QFileIconProvider();
    }
    QIcon icon = mIconProvider->icon(QFileInfo(item->m_output.fileName()));
    if (icon.isNull())
    {
        icon = style()->standardIcon(QStyle::SP_FileIcon);
    }
    item->setFileIconPixmap(icon.pixmap(48, 48));
    d->downloadsView->setRowHeight(row, item->sizeHint().height());
}

void WebDownloadManager::updateRow()
{
    DownloadItem *item = qobject_cast<DownloadItem *>(sender());
    int           row  = mDownloads.indexOf(item);
    if (-1 == row)
    {
        return;
    }

    if (!mIconProvider)
    {
        mIconProvider = new QFileIconProvider();
    }

    QIcon icon = mIconProvider->icon(QFileInfo(item->m_output.fileName()));
    if (icon.isNull())
    {
        icon = style()->standardIcon(QStyle::SP_FileIcon);
    }

    item->setFileIconPixmap(icon.pixmap(48, 48));
    d->downloadsView->setRowHeight(row, item->minimumSizeHint().height());

    // 这里曾经把 JavascriptEnabled 当成 PrivateBrowsingEnabled 判断：JS 默认开启，
    // 于是任何下载一结束就被移出列表，removePolicy() 完全失效。
    // 本程序没有隐私浏览模式，直接按用户选的策略处理即可。
    bool remove = false;

    if (item->downloadedSuccessfully() && removePolicy() == WebDownloadManager::SuccessFullDownload)
    {
        remove = true;
    }

    if (remove)
    {
        mModel->removeRow(row);
    }

    d->cleanupButton->setEnabled(mDownloads.count() - activeDownloads() > 0);
}

WebDownloadManager::RemovePolicy WebDownloadManager::removePolicy() const
{
    return m_RemovePolicy;
}

void WebDownloadManager::setRemovePolicy(RemovePolicy policy)
{
    if (policy == m_RemovePolicy)
    {
        return;
    }

    m_RemovePolicy = policy;
    mAutoSaver->changeOccurred();
}

void WebDownloadManager::save() const
{
    QSettings settings;
    settings.beginGroup(QLatin1String("downloadmanager"));
    QMetaEnum removePolicyEnum = staticMetaObject.enumerator(staticMetaObject.indexOfEnumerator("RemovePolicy"));
    settings.setValue(QLatin1String("removeDownloadsPolicy"), QLatin1String(removePolicyEnum.valueToKey(m_RemovePolicy)));

    if (m_RemovePolicy == Exit)
    {
        return;
    }

    for (int i = 0; i < mDownloads.count(); ++i)
    {
        QString key = QString(QLatin1String("download_%1_")).arg(i);
        settings.setValue(key + QLatin1String("url"), mDownloads[i]->m_url);
        settings.setValue(key + QLatin1String("location"), QFileInfo(mDownloads[i]->m_output).filePath());
        settings.setValue(key + QLatin1String("done"), mDownloads[i]->downloadedSuccessfully());
    }

    int     i   = mDownloads.count();
    QString key = QString(QLatin1String("download_%1_")).arg(i);

    while (settings.contains(key + QLatin1String("url")))
    {
        settings.remove(key + QLatin1String("url"));
        settings.remove(key + QLatin1String("location"));
        settings.remove(key + QLatin1String("done"));
        key = QString(QLatin1String("download_%1_")).arg(++i);
    }
}

void WebDownloadManager::load()
{
    mIsLoading = true;

    QSettings settings;
    settings.beginGroup(QLatin1String("downloadmanager"));

    QByteArray value            = settings.value(QLatin1String("removeDownloadsPolicy"), QLatin1String("Never")).toByteArray();
    QMetaEnum  removePolicyEnum = staticMetaObject.enumerator(staticMetaObject.indexOfEnumerator("RemovePolicy"));
    m_RemovePolicy              = removePolicyEnum.keyToValue(value) == -1 ? Never : static_cast<RemovePolicy>(removePolicyEnum.keyToValue(value));

    int     i                   = 0;
    QString key                 = QString(QLatin1String("download_%1_")).arg(i);

    while (settings.contains(key + QLatin1String("url")))
    {
        QUrl    url      = settings.value(key + QLatin1String("url")).toUrl();
        QString fileName = settings.value(key + QLatin1String("location")).toString();
        bool    done     = settings.value(key + QLatin1String("done"), true).toBool();
        if (!url.isEmpty() && !fileName.isEmpty())
        {
            DownloadItem *item = new DownloadItem(0, this);
            item->m_output.setFileName(fileName);
            item->setFileName(QFileInfo(item->m_output.fileName()).fileName());
            item->m_url = url;
            item->setDownloadCompleted(done);
            addItem(item);
        }
        key = QString(QLatin1String("download_%1_")).arg(++i);
    }

    d->cleanupButton->setEnabled(mDownloads.count() - activeDownloads() > 0);

    mIsLoading = false;
}

void WebDownloadManager::cleanup()
{
    if (mDownloads.isEmpty())
    {
        return;
    }

    mModel->removeRows(0, mDownloads.count());
    updateItemCount();

    if (mDownloads.isEmpty() && mIconProvider)
    {
        delete mIconProvider;
        mIconProvider = 0;
    }

    mAutoSaver->changeOccurred();
}

void WebDownloadManager::updateItemCount()
{
    int count = mDownloads.count();
    d->itemCount->setText(count == 1 ? tr("1 Download") : tr("%1 Downloads", "always >= 2").arg(count));
}

DownloadModel::DownloadModel(WebDownloadManager *downloadManager, QObject *parent) : QAbstractListModel(parent), mDownloadManager(downloadManager)
{
    // 刻意留空
}

QVariant DownloadModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= rowCount(index.parent()))
    {
        return QVariant();
    }

    if (role == Qt::ToolTipRole)
    {
        if (!mDownloadManager->mDownloads.at(index.row())->downloadedSuccessfully())
        {
            return mDownloadManager->mDownloads.at(index.row())->infoText();
        }
    }

    return QVariant();
}

int DownloadModel::rowCount(const QModelIndex &parent) const
{
    return (parent.isValid()) ? 0 : mDownloadManager->mDownloads.count();
}

bool DownloadModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid() || count <= 0)
    {
        return false;
    }

    // 调用方可能拿着 -1 或过期行号进来（空列表按 Delete）；越界时 at(i) 会读到缓冲区外。
    if (row < 0 || row + count > mDownloadManager->mDownloads.size())
    {
        return false;
    }

    int lastRow = row + count - 1;

    for (int i = lastRow; i >= row; --i)
    {
        if (mDownloadManager->mDownloads.at(i)->downloadedSuccessfully() || mDownloadManager->mDownloads.at(i)->retryAvailable())
        {
            beginRemoveRows(parent, i, i);
            mDownloadManager->mDownloads.takeAt(i)->deleteLater();
            endRemoveRows();
        }
    }

    mDownloadManager->mAutoSaver->changeOccurred();
    return true;
}
