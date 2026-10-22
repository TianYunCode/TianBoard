#include "core/NetworkAccessManager.h"
#include "utils/UserNotices.h"
#include "core/CookieJar.h"
#include <QElapsedTimer>

#include <QtWidgets>
#include <QtNetwork>
#include <QMessageBox>
#include <QPushButton>

#include "core/ApplicationController.h"
#include "platform/CredentialStore.h"
#include "settings/Settings.h"

NetworkAccessManager *NetworkAccessManager::sNetworkAccessManager = 0;

NetworkAccessManager *NetworkAccessManager::defaultAccessManager()
{
    if (!sNetworkAccessManager)
    {
        sNetworkAccessManager = new NetworkAccessManager(qApp);
    }
    return sNetworkAccessManager;
}

NetworkAccessManager::NetworkAccessManager(QObject *parent) : QNetworkAccessManager(parent), mProxyAuthenticationCount(0)
{
    connect(this, &QNetworkAccessManager::authenticationRequired, this, &NetworkAccessManager::authenticationRequired);
    connect(this, &QNetworkAccessManager::proxyAuthenticationRequired, this, &NetworkAccessManager::proxyAuthenticationRequired);

    // Qt6 删掉了 QNetworkAccessManager::sslErrors 信号，且 6.9 的公开 API 里没有任何
    // 等价入口能枚举握手失败的证书错误（QNetworkReply::sslErrorList() 不存在），
    // 所以这里的「首次遇到自签证书时询问是否信任」流程无法照搬。
    // sslErrors() 槽保留下来，等将来改用 QNetworkReply::ignoreSslErrors + 自定义
    // QNetworkRequest 配置时再接上。

    QNetworkProxy *proxy = Settings::settings()->httpProxy();

    if (proxy)
    {
        setProxy(*proxy);
    }
    else
    {
        QNetworkProxyFactory::setUseSystemConfiguration(true);
    }

    // 用可枚举的罐子：web/WebController 要把内嵌浏览器的 Cookie 同步进来，
    // 退出时才能按域白名单逐条删除（QWebEngineCookieStore 自己列不出清单）。
    setCookieJar(new CookieJar(this));

    QNetworkDiskCache *diskCache = new QNetworkDiskCache(this);
    QString            location  = Settings::userDataDirectory() + "/web-cache";
    diskCache->setCacheDirectory(location);
    setCache(diskCache);
}

QNetworkReply *NetworkAccessManager::createRequest(Operation op, const QNetworkRequest &req, QIODevice *outgoingData)
{
    QNetworkRequest request = req; // 拷一份再改，不动调用方传进来的那个请求
    // 临时做法：这里对所有请求一律打开 HTTP 管线化，没有按网页视图给出的管线化标志
    // 区分。等真正接上那套标志后，再改成按请求决定。
    request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);

    QNetworkReply *reply = QNetworkAccessManager::createRequest(op, request, outgoingData);

    return reply;
}

QNetworkReply *NetworkAccessManager::get(const QNetworkRequest &request)
{
    QElapsedTimer loadStartTime;
    loadStartTime.start();
    QNetworkReply *networkReply = QNetworkAccessManager::get(request);
    return networkReply;
}

namespace
{
    NetworkAccessManager::CredentialPrompt gCredentialPrompt;
} // namespace

void NetworkAccessManager::installCredentialPrompt(CredentialPrompt prompt)
{
    gCredentialPrompt = std::move(prompt);
}

void NetworkAccessManager::authenticationRequired(QNetworkReply *reply, QAuthenticator *auth)
{
    if (!gCredentialPrompt)
    {
        qWarning() << "没有装上凭据征询钩子，站点要求的认证只能放弃：" << reply->url();
        return;
    }

    // 用凭据管理器里那对预填：代理是静默用的同一套，用户在这里改了也就一起改。
    const ProxyCredentials stored   = CredentialStore::proxyCredentials();
    QString                user     = stored.userName;
    QString                password = stored.password;
    bool                   remember = false;

    if (!gCredentialPrompt(QApplication::activeWindow(), reply->url(), user, password, remember))
    {
        return;
    }

    if (auth)
    {
        auth->setUser(user);
        auth->setPassword(password);
    }

    if (remember)
    {
        CredentialStore::setProxyCredentials(user, password);
    }
}

void NetworkAccessManager::proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *auth)
{
    Q_UNUSED(proxy);

    const ProxyCredentials stored = CredentialStore::proxyCredentials();

    if (!stored.isEmpty())
    {
        auth->setUser(stored.userName);
        auth->setPassword(stored.password);
    }

    mProxyAuthenticationCount++;

    if (mProxyAuthenticationCount == 3)
    {
        UserNotices::presentStatusMessage(tr("Failed to log to Proxy"));
        disconnect(this, &QNetworkAccessManager::proxyAuthenticationRequired, this, &NetworkAccessManager::proxyAuthenticationRequired);
    }

    return;
}

void NetworkAccessManager::sslErrors(QNetworkReply *reply, const QList<QSslError> &error)
{
    // 先看这个主机（含端口）是否已被用户信任过，信任过就不再弹窗打断
    QString replyHost = reply->url().host() + ":" + QString::number(reply->url().port());
    if (!sslTrustedHostList.contains(replyHost))
    {
        QWidget *mainWindow = QApplication::activeWindow();

        QStringList errorStrings;
        for (int i = 0; i < error.count(); ++i)
        {
            errorStrings += error.at(i).errorString();
        }

        QString errors = errorStrings.join(QLatin1String("\n"));

        QMessageBox messageBox;
        messageBox.setParent(mainWindow);
        messageBox.setWindowFlags(Qt::Dialog);
        messageBox.setWindowTitle(QCoreApplication::applicationName());
        messageBox.setText(tr("SSL Errors:\n\n%1\n\n%2\n\n"
                              "Do you want to ignore these errors for this host?")
                .arg(reply->url().toString())
                .arg(errors));
        QPushButton *yesButton = messageBox.addButton(tr("Yes"), QMessageBox::YesRole);
        messageBox.addButton(tr("No"), QMessageBox::NoRole);
        messageBox.setIcon(QMessageBox::Question);
        messageBox.exec();

        if (messageBox.clickedButton() == yesButton)
        {
            reply->ignoreSslErrors();
            sslTrustedHostList.append(replyHost);
        }
    }
}
