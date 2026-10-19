#include "web/BrowserWindow.h"
#include "web/WebDownloadManager.h"
#include "web/TabWidget.h"
#include "web/WebView.h"
#include "core/Application.h"
#include "core/ApplicationController.h"
#include "whiteboard/BoardController.h"
#include "settings/Settings.h"

#include "core/NetworkAccessManager.h"

#include <QtWidgets>
#include <QtWebEngineWidgets>
#include <QWebEngineProfile>
#include <QWebEngineDownloadRequest>
#include <QMessageBox>
#include <QWebChannel>


WebPage::WebPage(QObject *parent)
    : QWebEnginePage(parent)
    , mKeyboardModifiers(Qt::NoModifier)
    , mPressedButtons(Qt::NoButton)
    , mOpenInNewTab(false)
{
    //setNetworkAccessManager(NetworkAccessManager::defaultAccessManager());
}

void WebPage::javaScriptConsoleMessage(QWebEnginePage::JavaScriptConsoleMessageLevel level,
                                          const QString &message, int lineNumber,
                                          const QString &sourceID)
{
    Q_UNUSED(level);
    qDebug("JavaScript> %s (%s:%d)", qPrintable(message), qPrintable(sourceID), lineNumber);
}

BrowserWindow *WebPage::mainWindow()
{
    QObject *w = this->parent();
    while (w)
    {
        if (BrowserWindow *mw = qobject_cast<BrowserWindow*>(w))
            return mw;

        w = w->parent();
    }

    return 0;
}

bool WebPage::acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame)
{
    Q_UNUSED(isMainFrame);

    // Qt6 没有 QWebPage::unsupportedContent，mailto 只能在这里拦下来交给系统邮件程序。
    if (url.scheme().compare("mailto", Qt::CaseInsensitive) == 0)
    {
        QDesktopServices::openUrl(url);
        return false;
    }

    if (type == QWebEnginePage::NavigationTypeLinkClicked
        && (mKeyboardModifiers & Qt::ControlModifier
            || mPressedButtons == Qt::MiddleButton))
    {
        WebView *webView;

        bool selectNewTab = (mKeyboardModifiers & Qt::ShiftModifier);
        webView = mainWindow()->tabWidget()->newTab(selectNewTab);

        webView->load(url);
        mKeyboardModifiers = Qt::NoModifier;
        mPressedButtons = Qt::NoButton;

        return false;
    }

	return true;
}


QWebEnginePage *WebPage::createWindow(QWebEnginePage::WebWindowType type)
{
    Q_UNUSED(type);

    return mainWindow()->tabWidget()->newTab()->page();
}



WebView::WebView(QWidget* parent)
    : WebTrapWebView(parent)
    , mProgress(0)
    , mPage(new WebPage(this))
{

    setPage(mPage);

	//QWebEngineView::setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

    connect(this, &QWebEngineView::loadProgress,
            this, &WebView::setProgress);

    connect(this, &QWebEngineView::loadFinished,
            this, &WebView::loadFinished);

    connect(this, &QWebEngineView::loadStarted,
            this, &WebView::loadStarted);

    // 所有标签共用默认 profile，所以必须按 page 过滤，否则一次下载会触发每个标签页。
    if (QWebEngineProfile *prof = page()->profile())
    {
        connect(prof, &QWebEngineProfile::downloadRequested,
                this, [this](QWebEngineDownloadRequest *item) {
                    if (item && item->page() == mPage)
                        downloadRequested(item);
                });
    }
}


void WebView::contextMenuEvent(QContextMenuEvent *event)
{
    //QWebHitTestResult r = page()->mainFrame()->hitTestContent(event->pos());

    //if (!r.linkUrl().isEmpty())
    //{
    //    QMenu menu(this);
    //    menu.addAction(pageAction(QWebEnginePage::OpenLinkInNewWindow));
    //    menu.addAction(tr("Open in New Tab"), this, SLOT(openLinkInNewTab()));
    //    menu.addSeparator();
    //    menu.addAction(pageAction(QWebEnginePage::DownloadLinkToDisk));
    //    // Add link to bookmarks...
    //    menu.addSeparator();
    //    menu.addAction(pageAction(QWebEnginePage::CopyLinkToClipboard));
    //    if (page()->settings()->testAttribute(QWebEngineSettings::DeveloperExtrasEnabled))
    //        menu.addAction(pageAction(QWebEnginePage::InspectElement));
    //    menu.exec(mapToGlobal(event->pos()));
    //    return;
    //}
    WebTrapWebView::contextMenuEvent(event);
}


void WebView::wheelEvent(QWheelEvent *event)
{
    if (QApplication::keyboardModifiers() & Qt::ControlModifier)
    {
        int numDegrees = event->angleDelta().y() / 8;
        int numSteps = numDegrees / 15;
        //setTextSizeMultiplier(textSizeMultiplier() + numSteps * 0.1);
        event->accept();
        return;
    }
    WebTrapWebView::wheelEvent(event);
}


void WebView::openLinkInNewTab()
{
    mPage->mOpenInNewTab = true;
    pageAction(QWebEnginePage::OpenLinkInNewWindow)->trigger();
}


void WebView::setProgress(int progress)
{
    //qDebug() << "loading progress" << progress << "% in" << mLoadStartTime.elapsed() << "ms";

    mProgress = progress;
}


void WebView::loadStarted()
{
    mLoadStartTime.start();
}


void WebView::loadFinished(bool ok)
{
    if (100 != mProgress)
    {
        qWarning() << "Received finished signal while progress is still:" << progress()
                   << "Url:" << url();
    }

    QString error;

    if (!ok)
        error = "with error";

    qDebug() << "page loaded in" << mLoadStartTime.elapsed() << "ms" << url().toString() << error;

    mProgress = 0;
}


void WebView::load(const QUrl &url)
{
    qDebug() << "loading " << url.toString();

    mInitialUrl = url;

    WebTrapWebView::load(url);
}



QUrl WebView::url() const
{
    QUrl url;
    try{
        url = QWebEngineView::url();
    } catch(...)
    {}

    if (!url.isEmpty())
       return url;

    return mInitialUrl;
}

void WebView::mousePressEvent(QMouseEvent *event)
{
    mPage->mPressedButtons = event->buttons();
    mPage->mKeyboardModifiers = event->modifiers();

    WebTrapWebView::mousePressEvent(event);
}

void WebView::mouseReleaseEvent(QMouseEvent *event)
{
    WebTrapWebView::mouseReleaseEvent(event);

    if (!event->isAccepted() && (mPage->mPressedButtons & Qt::MiddleButton))
    {
        QUrl url(QApplication::clipboard()->text(QClipboard::Selection));
        if (!url.isEmpty() && url.isValid() && !url.scheme().isEmpty())
        {
            setUrl(url);
        }
    }
}



void WebView::downloadRequested(QWebEngineDownloadRequest *item)
{
    if (!item)
        return;

    // Qt5 时代点击 PDF 走 unsupportedContent，在那里问“下载还是并入当前文档”；
    // Qt6 没有那个回调，PDF 统一变成下载请求，所以这个选择必须在入口补回来，
    // 否则教师只能把 PDF 存到磁盘，再手动导入。
    if (item->mimeType().compare(QLatin1String("application/pdf"), Qt::CaseInsensitive) == 0)
    {
        QMessageBox messageBox(this);
        messageBox.setText(tr("Download PDF Document: would you prefer to download the PDF file or add it to the current TianBoard document?"));

        messageBox.addButton(tr("Download"), QMessageBox::AcceptRole);
        QAbstractButton *addButton = messageBox.addButton(tr("Add to Current Document"), QMessageBox::AcceptRole);

        messageBox.exec();

        if (messageBox.clickedButton() == addButton)
        {
            Application::applicationController->showBoard();
            Application::boardController->downloadURL(item->url());
            item->cancel();
            return;
        }
    }

    // Qt6 的下载条目自带落盘能力，但本项目的下载管理器（含下载对话框 UI）是按
    // QNetworkRequest 重新抓取实现的，这里延用它，并放弃 WebEngine 自己的下载。
    BrowserWindow::downloadManager()->download(QNetworkRequest(item->url()));
    item->cancel();
}

