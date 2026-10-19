#include <QtWidgets>
#include <QRegExp>
#include "core/VirtualDesktop.h"
#include <QDomDocument>
#include <QWebEnginePage>

#include "platform/PlatformUtils.h"

#include "web/WebController.h"
#include "web/OEmbedParser.h"

#include "web/BrowserWindow.h"
#include "web/WebView.h"
#include "web/WebDownloadManager.h"
#include "web/TabWidget.h"

//#include "network/ServerXMLHttpRequest.h"
#include "core/NetworkAccessManager.h"

#include "ui/WidgetMirror.h"
#include "ui/MainWindow.h"
#include "ui/WebToolsPalette.h"
#include "platform/KeyboardPalette.h"

#include "core/Settings.h"
#include "core/Setting.h"
#include "core/Application.h"
#include "core/ApplicationController.h"
#include "core/DisplayManager.h"

#include "whiteboard/BoardController.h"
#include "whiteboard/DrawingController.h"

#include "whiteboard/GraphicsScene.h"

#include "platform/CustomCaptureWindow.h"
#include "whiteboard/BoardPaletteManager.h"


WebController::WebController(MainWindow* mainWindow)
    : QObject(mainWindow->centralWidget())
    , mMainWindow(mainWindow)
    , mCurrentWebBrowser(0)
    , mBrowserWidget(0)
    , mToolsCurrentPalette(0)
    , mToolsPalettePositionned(false)
    , mDownloadViewIsVisible(false)
{
    connect(&mOEmbedParser, SIGNAL(oembedParsed(QVector<sOEmbedContent>)), this, SLOT(onOEmbedParsed(QVector<sOEmbedContent>)));

    initialiazemOEmbedProviders();

 }


WebController::~WebController()
{
    // NOOP
}

void WebController::initialiazemOEmbedProviders()
{
    mOEmbedProviders << "baidu.com";
}

void WebController::webBrowserInstance()
{
    QString webHomePage = Settings::settings()->webHomePage->get().toString();
    QUrl currentUrl = BrowserWindow::guessUrlFromString(webHomePage);

    if (Settings::settings()->webUseExternalBrowser->get().toBool())
    {
        QDesktopServices::openUrl(currentUrl);
    }
    else
    {
        if (!mCurrentWebBrowser)
        {
            mCurrentWebBrowser = new BrowserWindow(mMainWindow->centralWidget(), mMainWindow);

            mMainWindow->addWebWidget(mCurrentWebBrowser);

            connect(mCurrentWebBrowser, SIGNAL(activeViewChange(QWidget*)), this, SLOT(setSourceWidget(QWidget*)));

            BrowserWindow::downloadManager()->setParent(mCurrentWebBrowser, Qt::Tool);

            Application::app()->insertSpaceToToolbarBeforeAction(mMainWindow->ui().webToolBar, mMainWindow->ui().actionBoard, 32);
            Application::app()->decorateActionMenu(mMainWindow->ui().actionMenu);

            bool showAddBookmarkButtons = Settings::settings()->webShowAddBookmarkButton->get().toBool();
            mMainWindow->ui().actionBookmarks->setVisible(showAddBookmarkButtons);
            mMainWindow->ui().actionAddBookmark->setVisible(showAddBookmarkButtons);

            showTabAtTop(Settings::settings()->appToolBarPositionedAtTop->get().toBool());

            adaptToolBar();

            connect(mCurrentWebBrowser, SIGNAL(activeViewPageChanged()), this, SLOT(activePageChanged()));

            mCurrentWebBrowser->loadUrl(currentUrl);

            mCurrentWebBrowser->tabWidget()->tabBar()->show();
            mCurrentWebBrowser->tabWidget()->lineEdits()->show();
        }

        Application::applicationController->setMirrorSourceWidget(mCurrentWebBrowser->paintWidget());
        mMainWindow->switchToWebWidget();

        setupPalettes();
        screenLayoutChanged();

        bool mirroring = Settings::settings()->webShowPageImmediatelyOnMirroredScreen->get().toBool();
        Application::mainWindow->ui().actionWebShowHideOnDisplay->setChecked(mirroring);
        mToolsCurrentPalette->show();
    }

    if (mDownloadViewIsVisible)
        BrowserWindow::downloadManager()->show();
}

void WebController::show()
{
    webBrowserInstance();
}

void WebController::setSourceWidget(QWidget* pWidget)
{
    mBrowserWidget = pWidget;
    Application::applicationController->setMirrorSourceWidget(pWidget);
}

void WebController::activePageChanged()
{
    if (mCurrentWebBrowser && mCurrentWebBrowser->currentTabWebView())
    {
        mMainWindow->ui().actionWebTrap->setChecked(false);

        QUrl latestUrl = mCurrentWebBrowser->currentTabWebView()->url();

        // TODO : Uncomment the next line to continue the youtube button bugfix
        //Application::mainWindow->ui().actionWebOEmbed->setEnabled(hasEmbeddedContent());
        // And remove this line once the previous one is uncommented
        Application::mainWindow->ui().actionWebOEmbed->setEnabled(isOEmbedable(latestUrl));
        Application::mainWindow->ui().actionEduMedia->setEnabled(isEduMedia(latestUrl));

        emit activeWebPageChanged(mCurrentWebBrowser->currentTabWebView());
    }
}

bool WebController::hasEmbeddedContent()
{
    bool bHasContent = false;
    if(mCurrentWebBrowser){
		QString html = "";// mCurrentWebBrowser->currentTabWebView()->webPage()->mainFrame()->toHtml();

        // search the presence of "+oembed"
        QString query = "\\+oembed([^>]*)>";
        QRegExp exp(query);
        exp.indexIn(html);
        QStringList results = exp.capturedTexts();
        if(2 <= results.size() && "" != results.at(1)){
            // An embedded content has been found, no need to check the other ones
            bHasContent = true;
        }else{
            QList<QUrl> contentUrls;
            lookForEmbedContent(&html, "embed", "src", &contentUrls);
            lookForEmbedContent(&html, "video", "src", &contentUrls);
            lookForEmbedContent(&html, "object", "data", &contentUrls);

            // TODO: check the hidden iFrame

            if(!contentUrls.empty()){
                bHasContent = true;
            }
        }
    }

    return bHasContent;
}

QPixmap WebController::captureCurrentPage()
{
    QPixmap pix;

  /*  if (mCurrentWebBrowser
            && mCurrentWebBrowser->currentTabWebView()
            && mCurrentWebBrowser->currentTabWebView()->page()
            && mCurrentWebBrowser->currentTabWebView()->page()->mainFrame())
    {
		QWebEnginePage* frame = mCurrentWebBrowser->currentTabWebView()->page()->mainFrame();
        QSize size = frame->contentsSize();

        qDebug() << size;

        QVariant top = frame->evaluateJavaScript("document.getElementsByTagName('body')[0].clientTop");
        QVariant left = frame->evaluateJavaScript("document.getElementsByTagName('body')[0].clientLeft");
        QVariant width = frame->evaluateJavaScript("document.getElementsByTagName('body')[0].clientWidth");
        QVariant height = frame->evaluateJavaScript("document.getElementsByTagName('body')[0].clientHeight");

        QSize vieportSize = mCurrentWebBrowser->currentTabWebView()->page()->viewportSize();
        mCurrentWebBrowser->currentTabWebView()->page()->setViewportSize(frame->contentsSize());
        pix = QPixmap(frame->geometry().width(), frame->geometry().height());

        {
            QPainter p(&pix);
            frame->render(&p);
        }

        if (left.isValid() && top.isValid() && width.isValid() && height.isValid())
        {
            bool okLeft, okTop, okWidth, okHeight;

            int iLeft = left.toInt(&okLeft) * frame->zoomFactor();
            int iTop = top.toInt(&okTop) * frame->zoomFactor();
            int iWidth = width.toInt(&okWidth) * frame->zoomFactor();
            int iHeight = height.toInt(&okHeight) * frame->zoomFactor();

            if(okLeft && okTop && okWidth && okHeight)
            {
                pix = pix.copy(iLeft, iTop, iWidth, iHeight);
            }
        }


        mCurrentWebBrowser->currentTabWebView()->page()->setViewportSize(vieportSize);
    }*/

    return pix;
}


void WebController::setupPalettes()
{
    if(!mToolsCurrentPalette)
    {
        mToolsCurrentPalette = new WebToolsPalette(Application::mainWindow);
        Application::boardController->paletteManager()->setCurrentWebToolsPalette(mToolsCurrentPalette);
#ifndef Q_OS_WIN
        if (PlatformUtils::hasVirtualKeyboard() && Application::boardController->paletteManager()->mKeyboardPalette)
            connect(Application::boardController->paletteManager()->mKeyboardPalette, SIGNAL(closed()),
                    Application::boardController->paletteManager()->mKeyboardPalette, SLOT(onDeactivated()));
#endif

        connect(mMainWindow->ui().actionWebCustomCapture, SIGNAL(triggered()), this, SLOT(customCapture()));
        connect(mMainWindow->ui().actionWebWindowCapture, SIGNAL(triggered()), this, SLOT(captureWindow()));
        connect(mMainWindow->ui().actionWebOEmbed, SIGNAL(triggered()), this, SLOT(captureoEmbed()));
        connect(mMainWindow->ui().actionEduMedia, SIGNAL(triggered()), this, SLOT(captureEduMedia()));

        connect(mMainWindow->ui().actionWebShowHideOnDisplay, SIGNAL(toggled(bool)), this, SLOT(toogleMirroring(bool)));
        connect(mMainWindow->ui().actionWebTrap, SIGNAL(toggled(bool)), this, SLOT(toggleWebTrap(bool)));

        mToolsCurrentPalette->hide();
        mToolsCurrentPalette->adjustSizeAndPosition();

        if (controlView()){
            int left = controlView()->width() - 20 - mToolsCurrentPalette->width();
            int top = (controlView()->height() - mToolsCurrentPalette->height()) / 2;
            mToolsCurrentPalette->setCustomPosition(true);
            mToolsCurrentPalette->move(left, top);
        }
        
		connect(mMainWindow->ui().actionWebTools, SIGNAL(triggered(bool)), this, SLOT(triggerWebTools(bool)));
		mMainWindow->ui().actionWebTools->trigger();
    }
}



void WebController::toggleWebTrap(bool checked)
{
    if (mCurrentWebBrowser && mCurrentWebBrowser->currentTabWebView())
        mCurrentWebBrowser->currentTabWebView()->setIsTrapping(checked);
}


void WebController::captureWindow()
{
    QPixmap webPagePixmap = captureCurrentPage();

    if (!webPagePixmap.isNull())
        emit imageCaptured(webPagePixmap, true, mCurrentWebBrowser->currentTabWebView()->url());
}


void WebController::customCapture()
{
    mToolsCurrentPalette->setVisible(false);
    qApp->processEvents();

    CustomCaptureWindow customCaptureWindow(mCurrentWebBrowser);

    customCaptureWindow.show();

    if (customCaptureWindow.execute(getScreenPixmap()) == QDialog::Accepted)
    {
        QPixmap selectedPixmap = customCaptureWindow.getSelectedPixmap();
        emit imageCaptured(selectedPixmap, false, mCurrentWebBrowser->currentTabWebView()->url());
    }

    mToolsCurrentPalette->setVisible(true);
}


void WebController::toogleMirroring(bool checked)
{
    Application::applicationController->mirroringEnabled(checked);
}


QPixmap WebController::getScreenPixmap()
{
    VirtualDesktop *desktop = VirtualDesktop::instance();
    // we capture the screen in which the mouse is.
    const QPoint cursorPos = QCursor::pos();
    const QRect primaryScreenRect = desktop->screenGeometry(cursorPos);

    return VirtualDesktop::grabScreen(QGuiApplication::screenAt(cursorPos), primaryScreenRect);
}


void WebController::screenLayoutChanged()
{
    bool hasDisplay = (Application::applicationController &&
                       Application::applicationController->displayManager() &&
                       Application::applicationController->displayManager()->hasDisplay());

    Application::mainWindow->ui().actionWebShowHideOnDisplay->setVisible(hasDisplay);
}


void WebController::closing()
{
    //NOOP
}


void WebController::adaptToolBar()
{
    bool highResolution = mMainWindow->width() > 1024;

    mMainWindow->ui().actionWebReload->setVisible(highResolution);
    mMainWindow->ui().actionStopLoading->setVisible(highResolution);

    //if(mCurrentWebBrowser )
    //    mCurrentWebBrowser->adaptToolBar(highResolution);

}


void WebController::showTabAtTop(bool attop)
{
    if (mCurrentWebBrowser)
        mCurrentWebBrowser->showTabAtTop(attop);
}

void WebController::captureoEmbed()
{
    if ( mCurrentWebBrowser && mCurrentWebBrowser->currentTabWebView()){
        // TODO : Uncomment the next lines to continue the youtube button bugfix
        //    getEmbeddableContent();

        // And comment from here

        QWebEngineView* webView = mCurrentWebBrowser->currentTabWebView();
        QUrl currentUrl = webView->url();

        if (isOEmbedable(currentUrl))
        {
            GraphicsWebWidgetItem * widget = Application::boardController->activeScene()->addOEmbed(currentUrl);

            if(widget)
            {
                Application::applicationController->showBoard();
                DrawingController::drawingController()->setStylusTool(StylusTool::Selector);
            }
        }
        // --> Until here
    }
}

void WebController::lookForEmbedContent(QString* pHtml, QString tag, QString attribute, QList<QUrl> *pList)
{
    if(NULL != pHtml && NULL != pList){
        QVector<QString> urlsFound;
        // Check for <embed> content
        QRegExp exp(QString("<%0(.*)").arg(tag));
        exp.indexIn(*pHtml);
        QStringList strl = exp.capturedTexts();
        if(2 <= strl.size() && strl.at(1) != ""){
            // Here we call this regular expression:
            // src\s?=\s?['"]([^'"]*)['"]
            // It says: give me all characters that are after src=" (or src = ")
            QRegExp src(QString("%0\\s?=\\s?['\"]([^'\"]*)['\"]").arg(attribute));
            for(int i=1; i<strl.size(); i++){
                src.indexIn(strl.at(i));
                QStringList urls = src.capturedTexts();
                if(2 <= urls.size() && urls.at(1) != "" && !urlsFound.contains(urls.at(1))){
                    urlsFound << urls.at(1);
                    pList->append(QUrl(urls.at(1)));
                }
            }
        }
    }
}

void WebController::checkForOEmbed(QString *pHtml)
{
    mOEmbedParser.parse(*pHtml);
}

void WebController::getEmbeddableContent()
{
    // Get the source code of the page
    if(mCurrentWebBrowser){
        //QNetworkAccessManager* pNam = mCurrentWebBrowser->currentTabWebView()->webPage()->networkAccessManager();
        //if(NULL != pNam){
        //    QString html = mCurrentWebBrowser->currentTabWebView()->webPage()->mainFrame()->toHtml();
        //    mOEmbedParser.setNetworkAccessManager(pNam);

        //    // First, we have to check if there is some oembed content
        //    checkForOEmbed(&html);

        //    // Note: The other contents will be verified once the oembed ones have been checked
        //}
    }
}

void WebController::captureEduMedia()
{
    if (mCurrentWebBrowser && mCurrentWebBrowser->currentTabWebView())
    {
        QWebEngineView* webView = mCurrentWebBrowser->currentTabWebView();
        QUrl currentUrl = webView->url();

        if (isEduMedia(currentUrl))
        {
           /* QWebElementCollection objects = webView->page()->currentFrame()->findAllElements("object");

            foreach(QWebElement object, objects)
            {
                foreach(QWebElement param, object.findAll("param"))
                {
                    if(param.attribute("name") == "flashvars")
                    {
                        QString value = param.attribute("value");
                        QString midValue;
                        QString langValue;
                        QString hostValue;

                        QStringList flashVars = value.split("&");

                        foreach(QString flashVar, flashVars)
                        {
                            QStringList var = flashVar.split("=");

                            if (var.length() < 2)
                                break;

                            if (var.at(0) == "mid")
                                midValue = var.at(1);
                            else if (var.at(0) == "lang")
                                langValue = var.at(1);
                            else if (var.at(0) == "host")
                                hostValue = var.at(1);

                        }

                        if (midValue.length() > 0 && langValue.length() > 0 && hostValue.length() > 0)
                        {
                            QString swfUrl = "http://" + hostValue + "/" + langValue + "/fl/" + midValue;

                            Application::boardController->downloadURL(QUrl(swfUrl));

                            Application::applicationController->showBoard();
                            DrawingController::drawingController()->setStylusTool(StylusTool::Selector);

                            return;
                        }
                    }
                }
            }*/
        }
    }
    else
    {
        Application::showMessage("Cannot find any reference to eduMedia content");
    }
}

bool WebController::isOEmbedable(const QUrl& pUrl)
{
    QString urlAsString = pUrl.toString();

    foreach(QString provider, mOEmbedProviders)
    {
        if(urlAsString.contains(provider))
            return true;
    }

    return false;
}


bool WebController::isEduMedia(const QUrl& pUrl)
{
    QString urlAsString = pUrl.toString();

    if (urlAsString.contains("edumedia-sciences.com"))
    {
        return true;
    }

    return false;
}


void WebController::loadUrl(const QUrl& url)
{
    Application::applicationController->showInternet();
    if (Settings::settings()->webUseExternalBrowser->get().toBool())

        QDesktopServices::openUrl(url);
    else
        mCurrentWebBrowser->loadUrlInNewTab(url);


}


QWebEngineView* WebController::createNewTab()
{
    if (mCurrentWebBrowser)
        Application::applicationController->showInternet();

    return mCurrentWebBrowser->createNewTab();
}


void WebController::copy()
{
    if (mCurrentWebBrowser && mCurrentWebBrowser->currentTabWebView())
    {
        QWebEngineView* webView = mCurrentWebBrowser->currentTabWebView();
        QAction *act = webView->pageAction(QWebEnginePage::Copy);
        if(act)
            act->trigger();
    }
}


void WebController::paste()
{
    if (mCurrentWebBrowser && mCurrentWebBrowser->currentTabWebView())
    {
        QWebEngineView* webView = mCurrentWebBrowser->currentTabWebView();
        QAction *act = webView->pageAction(QWebEnginePage::Paste);
        if(act)
            act->trigger();
    }
}


void WebController::cut()
{
    if (mCurrentWebBrowser && mCurrentWebBrowser->currentTabWebView())
    {
        QWebEngineView* webView = mCurrentWebBrowser->currentTabWebView();
        QAction *act = webView->pageAction(QWebEnginePage::Cut);
        if(act)
            act->trigger();
    }
}

void WebController::onOEmbedParsed(QVector<sOEmbedContent> contents)
{
    QList<QUrl> urls;

    foreach(sOEmbedContent cnt, contents){
        urls << QUrl(cnt.url);
    }

    // TODO : Implement this
    //lookForEmbedContent(&html, "embed", "src", &urls);
    //lookForEmbedContent(&html, "video", "src", &contentUrls);
    //lookForEmbedContent(&html, "object", "data", &contentUrls);

    // TODO: check the hidden iFrame

    if(!urls.empty()){
        QUrl contentUrl;    // The selected content url

        if(1 == urls.size()){
            contentUrl = urls.at(0);
        }else{
            // TODO : Display a dialog box asking the user which content to get and set contentUrl to the selected content

        }

        GraphicsWebWidgetItem * widget = Application::boardController->activeScene()->addOEmbed(contentUrl);

        if(widget)
        {
            Application::applicationController->showBoard();
            DrawingController::drawingController()->setStylusTool(StylusTool::Selector);
        }
    }
}

void WebController::triggerWebTools(bool checked)
{
	mToolsCurrentPalette->setVisible(checked);

}