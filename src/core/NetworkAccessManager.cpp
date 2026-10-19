#include "core/NetworkAccessManager.h"
#include <QElapsedTimer>

#include <QtWidgets>
#include <QtNetwork>
#include <QMessageBox>
#include <QPushButton>

#include "core/Application.h"
#include "core/ApplicationController.h"
#include "settings/Settings.h"

#include "ui/ProxyDialog.h"



NetworkAccessManager *NetworkAccessManager::sNetworkAccessManager = 0;

NetworkAccessManager *NetworkAccessManager::defaultAccessManager()
{
    if (!sNetworkAccessManager) {
        sNetworkAccessManager = new NetworkAccessManager(qApp);
    }
    return sNetworkAccessManager;
}

NetworkAccessManager::NetworkAccessManager(QObject *parent)
    : QNetworkAccessManager(parent)
      , mProxyAuthenticationCount(0)
{
    connect(this, &QNetworkAccessManager::authenticationRequired,
            this, &NetworkAccessManager::authenticationRequired);
    connect(this, &QNetworkAccessManager::proxyAuthenticationRequired,
            this, &NetworkAccessManager::proxyAuthenticationRequired);

    // Qt6 删掉了 QNetworkAccessManager::sslErrors 信号，且 6.9 的公开 API 里没有任何
    // 等价入口能枚举握手失败的证书错误（QNetworkReply::sslErrorList() 不存在），
    // 所以这里的「首次遇到自签证书时询问是否信任」流程无法照搬。
    // sslErrors() 槽保留下来，等将来改用 QNetworkReply::ignoreSslErrors + 自定义
    // QNetworkRequest 配置时再接上。

    QNetworkProxy* proxy = Settings::settings()->httpProxy();

    if (proxy)
    {
        setProxy(*proxy);
    }
    else
    {
        QNetworkProxyFactory::setUseSystemConfiguration(true);
    }

    QNetworkDiskCache *diskCache = new QNetworkDiskCache(this);
    QString location = Settings::userDataDirectory() + "/web-cache";
    diskCache->setCacheDirectory(location);
    setCache(diskCache);
}

QNetworkReply* NetworkAccessManager::createRequest(Operation op, const QNetworkRequest & req, QIODevice * outgoingData)
{
    QNetworkRequest request = req; // copy so we can modify
    // this is a temporary hack until we properly use the pipelining flags from QtWebkit
    // pipeline everything! :)
    request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);

    QNetworkReply* reply = QNetworkAccessManager::createRequest(op, request, outgoingData);

    return reply;
}

QNetworkReply *NetworkAccessManager::get(const QNetworkRequest &request)
{
    QElapsedTimer loadStartTime;
    loadStartTime.start();
    QNetworkReply *networkReply = QNetworkAccessManager::get(request);
    return networkReply;
}

void NetworkAccessManager::authenticationRequired(QNetworkReply *reply, QAuthenticator *auth)
{
    QWidget *mainWindow = QApplication::activeWindow();

    ProxyDialog dialog(mainWindow);
    dialog.setWindowFlags(Qt::Sheet);

    QString introMessage = tr("<qt>Enter username and password for \"%1\" at %2</qt>");
    introMessage = introMessage.arg((reply->url().toString()).toHtmlEscaped()).arg((reply->url().toString()).toHtmlEscaped());
    dialog.setIntroText(introMessage);

    // Pre-fill with the proxy credentials stored by the preferences dialog, the
    // same pair proxyAuthenticationRequired() uses silently.
    dialog.setUserName(Settings::settings()->proxyUsername());
    dialog.setPassword(Settings::settings()->proxyPassword());

    if (dialog.exec() == QDialog::Accepted)
    {
        if (auth)
        {
            auth->setUser(dialog.userName());
            auth->setPassword(dialog.password());
        }

        if (dialog.shouldSaveCredentials())
        {
            Settings::settings()->setProxyUsername(dialog.userName());
            Settings::settings()->setProxyPassword(dialog.password());
        }
    }

}

void NetworkAccessManager::proxyAuthenticationRequired(const QNetworkProxy &proxy, QAuthenticator *auth)
{
    Q_UNUSED(proxy);

    QString username = Settings::settings()->proxyUsername();
    QString password = Settings::settings()->proxyPassword();

    if (username.length() > 0 || password.length() > 0)
    {
        auth->setUser(username);
        auth->setPassword(password);
    }

    mProxyAuthenticationCount++;

    if (mProxyAuthenticationCount == 3)
    {
        Application::showMessage(tr("Failed to log to Proxy"));
        disconnect(this, &QNetworkAccessManager::proxyAuthenticationRequired,
                   this, &NetworkAccessManager::proxyAuthenticationRequired);

    }

    return;

}

void NetworkAccessManager::sslErrors(QNetworkReply *reply, const QList<QSslError> &error)
{
    // check if SSL certificate has been trusted already
    QString replyHost = reply->url().host() + ":" + QString::number(reply->url().port());
    if(!sslTrustedHostList.contains(replyHost))
    {
        QWidget *mainWindow = QApplication::activeWindow();

        QStringList errorStrings;
        for (int i = 0; i < error.count(); ++i)
            errorStrings += error.at(i).errorString();

        QString errors = errorStrings.join(QLatin1String("\n"));

        QMessageBox messageBox;
        messageBox.setParent(mainWindow);
        messageBox.setWindowFlags(Qt::Dialog);
        messageBox.setWindowTitle(QCoreApplication::applicationName());
        messageBox.setText(tr("SSL Errors:\n\n%1\n\n%2\n\n"
                              "Do you want to ignore these errors for this host?").arg(reply->url().toString()).arg(errors));
        QPushButton* yesButton = messageBox.addButton(tr("Yes"),QMessageBox::YesRole);
        messageBox.addButton(tr("No"),QMessageBox::NoRole);
        messageBox.setIcon(QMessageBox::Question);
        messageBox.exec();

        if(messageBox.clickedButton() == yesButton) {
            reply->ignoreSslErrors();
            sslTrustedHostList.append(replyHost);
        }
    }
}
