/**
 * @file HttpGet.h
 * @brief 声明基于 QNetworkAccessManager 的单次 GET 下载对象。
 */

#pragma once

#include <QtCore>
#include <QtNetwork>
#include <QDropEvent>

/**
 * @brief 一次 GET 请求：累积响应数据，处理重定向，并以信号报告结果。
 *
 * 详细说明：同一时间只持有一个 QNetworkReply，重复调用 get() 会先取消旧请求；
 * 由调用方（如 DownloadManager）创建并持有，本类不负责自身的销毁。
 * 线程约束：与所属线程的事件循环绑定，网络回调都在该线程触发。
 */
class HttpGet : public QObject {
    Q_OBJECT

public:
    HttpGet(QObject *parent = 0);
    virtual ~HttpGet();

    /**
     * @brief 发起 GET 请求。
     * @param pUrl 目标地址。
     * @param pPoint 请求携带的界面位置，随完成信号原样返回。
     * @param pSize 请求携带的尺寸，随完成信号原样返回。
     * @param isBackground true 表示后台下载，界面可不显示进度。
     * @return 新建的 reply；其所有权仍在本对象。
     * @note 会先中止上一个未完成的请求。
     */
    QNetworkReply *get(QUrl pUrl, QPointF pPoint = QPointF(0, 0), QSize pSize = QSize(0, 0),
                       bool isBackground = false);
    // 中止当前请求并释放 reply；reply 只在请求对象自己的线程里被删除。
    /// 中止当前请求；没有进行中的请求时为空操作。
    void cancelRequest();
    //        QNetworkReply* get(const sDownloadFileDesc &downlinfo);

signals:
    /// 下载进度（字节）。
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    /// 请求结束；pData 为完整响应体，contentTypeHeader 为响应头中的类型。
    void downloadFinished(bool pSuccess, QUrl sourceUrl, QString pContentTypeHeader, QByteArray pData,
                          QPointF pPos, QSize pSize, bool isBackground);
    //        void downloadFinished(bool pSuccess, QUrl sourceUrl, QString pContentTypeHeader, QByteArray pData
    //                              , sDownloadFileDesc downlInfo);

private slots:
    /// 把新到达的数据追加到缓冲区。
    void readyRead();
    /// 根据错误或重定向决定继续请求还是发出完成信号。
    void requestFinished();
    /// 转发进度信号，总量未知（-1）时忽略。
    void downloadProgressed(qint64 bytesReceived, qint64 bytesTotal);

private:
    QByteArray mDownloadedBytes;
    QNetworkReply *mReply;
    QPointF mPos;
    QSize mSize;

    bool mIsBackground;
    int mRequestID;
    int mRedirectionCount;
    bool mIsSelfAborting;
    //        sDownloadFileDesc mDownloadInfo;
};
