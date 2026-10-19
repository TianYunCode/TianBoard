#pragma once

#include <QtWidgets>
#include <QElapsedTimer>
#include <QtWebEngineWidgets>

#include "web/WebTrapWebView.h"

class BrowserWindow;
class QWebEngineDownloadRequest;

class WebPage : public QWebEnginePage
{
    Q_OBJECT;

public:
    WebPage(QObject *parent = 0);
    BrowserWindow *mainWindow();

    void javaScriptConsoleMessage(QWebEnginePage::JavaScriptConsoleMessageLevel level,
                                  const QString &message, int lineNumber,
                                  const QString &sourceID) override;

protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override;
	QWebEnginePage *createWindow(QWebEnginePage::WebWindowType type);

    //bool supportsExtension(Extension extension) const {
    //    if (extension == QWebEnginePage::ErrorPageExtension)
    //    {
    //        return true;
    //    }
    //    return false;
    //}

    //bool extension(Extension extension, const ExtensionOption *option = 0, ExtensionReturn *output = 0)
    //{
    //    if (extension != QWebEnginePage::ErrorPageExtension)
    //        return false;

    //    ErrorPageExtensionOption *errorOption = (ErrorPageExtensionOption*) option;
    //    qDebug() << "Error loading " << qPrintable(errorOption->url.toString());
    //    if(errorOption->domain == QWebEnginePage::QtNetwork)
    //        qDebug() << "Network error (" << errorOption->error << "): ";
    //    else if(errorOption->domain == QWebEnginePage::Http)
    //        qDebug() << "HTTP error (" << errorOption->error << "): ";
    //    else if(errorOption->domain == QWebEnginePage::WebKit)
    //        qDebug() << "WebKit error (" << errorOption->error << "): ";

    //    qDebug() << qPrintable(errorOption->errorString);

    //    return false;
    //}

signals:
	void loadingUrl(const QUrl &url);

private slots:
    // Qt6 WebEngine 没有 QWebPage::unsupportedContent，本槽目前没有调用方；
    // 想在浏览器标签里恢复 PDF/`.widget` 内联处理，需要改接 QWebEngineProfile::downloadRequested。
    void handleUnsupportedContent(QNetworkReply *reply);

private:
    friend class WebView;

    // set the webview mousepressedevent
    Qt::KeyboardModifiers mKeyboardModifiers;
    Qt::MouseButtons mPressedButtons;
    bool mOpenInNewTab;
    QUrl mLoadingUrl;
};

class WebView : public WebTrapWebView
{
    Q_OBJECT

public:
    WebView(QWidget *parent = 0);
    WebPage *webPage() const { return mPage; }

    void load(const QUrl &url);
    QUrl url() const;

    QString lastStatusBarText() const;
    inline int progress() const { return mProgress; }

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);
    void wheelEvent(QWheelEvent *event);

private slots:
    void setProgress(int progress);
    void loadFinished(bool ok);
    void loadStarted();

    void setStatusBarText(const QString &string);

    void downloadRequested(QWebEngineDownloadRequest *item);
    void openLinkInNewTab();

private:
    QString mLastStatusBarText;
    QUrl mInitialUrl;
    int mProgress;
    WebPage *mPage;
    QElapsedTimer mLoadStartTime;
};

