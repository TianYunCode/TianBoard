#pragma once

#include <QtNetwork>
#include <QtWidgets>
#include <QFileIconProvider>

#include <memory>

class DownloadItem : public QWidget
{
    Q_OBJECT

public:
    DownloadItem(QNetworkReply *reply = 0, bool requestFileName = false, QWidget *parent = 0, QString customDownloadPath = QString());
    ~DownloadItem() override;
    bool downloading() const;
    bool downloadedSuccessfully() const;

    // The transfer is still running: its "Stop" button is enabled.
    bool isActive() const;
    // The transfer failed or was stopped: "Try Again" is offered.
    bool retryAvailable() const;

    void setFileName(const QString &fileName);
    void setFileIconPixmap(const QPixmap &pixmap);
    void setDownloadCompleted(bool completed);
    QString infoText() const;

    QUrl m_url;

    QFile m_output;
    QNetworkReply *m_reply;
signals:
void statusChanged();

private slots:
    void stop();
    void tryAgain();
    void open();

    void downloadReadyRead();
    void error(QNetworkReply::NetworkError code);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void metaDataChanged();
    void finished();

private:
    void getFileName();
    void init();
    void updateInfoLabel();
    QString dataString(int size) const;

    QString saveFileName(const QString &directory) const;

    bool mRequestFileName;
    qint64 mBytesReceived;
    QElapsedTimer mDownloadTime;
    QString mCustomDownloadPath;

    struct Private;
    std::unique_ptr<Private> d;

};

class AutoSaver;
class DownloadModel;

class WebDownloadManager : public QDialog
{
    Q_OBJECT
    Q_PROPERTY(RemovePolicy removePolicy READ removePolicy WRITE setRemovePolicy)
    Q_ENUMS(RemovePolicy)

public:
    enum RemovePolicy {
        Never,
        Exit,
        SuccessFullDownload
    };

    WebDownloadManager(QWidget *parent = 0);
    ~WebDownloadManager();
    int activeDownloads() const;

    RemovePolicy removePolicy() const;
    void setRemovePolicy(RemovePolicy policy);

public slots:
    void download(const QNetworkRequest &request, bool requestFileName = false);
    inline void download(const QUrl &url, bool requestFileName = false)
        { download(QNetworkRequest(url), requestFileName); }
    void handleUnsupportedContent(QNetworkReply *reply, bool requestFileName = false, QString customDownloadPath = QString());
    void cleanup();

private slots:
    void save() const;
    void updateRow();
    void processDownloadedContent(QNetworkReply *reply, bool requestFileName, QString customDownloadPath = QString());

private:
    void addItem(DownloadItem *item);
    void updateItemCount();
    void load();

    AutoSaver *mAutoSaver;
    DownloadModel *mModel;
    QNetworkAccessManager *mManager;
    QFileIconProvider *mIconProvider;
    QList<DownloadItem*> mDownloads;
    RemovePolicy m_RemovePolicy;
    friend class DownloadModel;

    bool mIsLoading;

    struct Private;
    std::unique_ptr<Private> d;
};

class DownloadModel : public QAbstractListModel
{
    friend class WebDownloadManager;
    Q_OBJECT

public:
    DownloadModel(WebDownloadManager *downloadManager, QObject *parent = 0);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());

private:
    WebDownloadManager *mDownloadManager;

};


