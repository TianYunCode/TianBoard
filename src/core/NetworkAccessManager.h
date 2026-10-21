/**
 * @file NetworkAccessManager.h
 * @brief 声明应用共享的网络访问管理器，统一代理配置、缓存与认证处理。
 */

#pragma once

#include <QtNetwork>

#include <functional>

/**
 * @brief 全局 QNetworkAccessManager：配置磁盘缓存与代理，并处理需要凭据的认证请求。
 *
 * 详细说明：进程级单例，首次调用 defaultAccessManager() 时创建，父对象为 qApp；
 * 所有网络模块应通过它取 reply，以共享连接与缓存。线程约束：GUI 线程专用，
 * 认证对话框直接以活动窗口为父窗口弹出。
 */
class NetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT

public:
    /// 返回进程内唯一实例，首次调用时创建。
    static NetworkAccessManager *defaultAccessManager();

    /**
     * @brief 向用户征询站点凭据的钩子签名：填好 user/password 并回答是否记住。
     * @return 用户确认返回 true；取消返回 false（这一次认证就放弃）。
     */
    using CredentialPrompt = std::function<bool(QWidget *parent, const QUrl &url, QString &user, QString &password, bool &remember)>;

    /**
     * @brief 装上凭据征询钩子，实现是 ui/ProxyDialog::prompt()，由装配点装入（app/main.cpp）。
     * @note 服务层不认识对话框：没装上时收到认证请求只会告警一句并放弃，不会崩。
     */
    static void installCredentialPrompt(CredentialPrompt prompt);

    /// 构造管理器，加载代理设置并建立磁盘缓存。
    NetworkAccessManager(QObject *parent = 0);

    /// 发起 GET 请求；当前实现只做转发。
    virtual QNetworkReply *get(const QNetworkRequest &request);

protected:
    /// 为每个请求开启 HTTP 流水线，然后交给基类创建 reply。
    virtual QNetworkReply *createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData = 0);

private:
    QList<QString> sslTrustedHostList;

    static NetworkAccessManager *sNetworkAccessManager;

    volatile int mProxyAuthenticationCount;

private slots:
    /// 站点要求认证时弹窗询问用户名与密码。
    void authenticationRequired(QNetworkReply *reply, QAuthenticator *auth);
    /// 代理要求认证时静默填入已保存的代理凭据，连续失败三次后提示。
    void proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *auth);
    /**
     * @brief 处理 SSL 证书错误。
     * @note Qt6 已移除 QNetworkAccessManager 的 sslErrors 信号，当前未接入，
     * 保留实现以便将来通过 QNetworkReply 的忽略错误接口重新启用。
     */
    void sslErrors(QNetworkReply *reply, const QList<QSslError> &error);
};
