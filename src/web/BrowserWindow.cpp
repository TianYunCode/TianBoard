#include "web/BrowserWindow.h"
#include <QWebEngineProfile>
#include <QRegExp>

#include <QtWidgets>
#include <QtWebEngineWidgets>
#include <QWebEngineSettings>
#include <QWebEngineHistory>
#include <QWebEngineHistoryItem>
#include "core/VirtualDesktop.h"

#include "core/Settings.h"
#include "core/Setting.h"
#include "core/Application.h"

#include "core/NetworkAccessManager.h"
#include "core/AutoSaver.h"

#include "ui/MainWindow.h"



#include "web/ChaseWidget.h"
#include "web/WebDownloadManager.h"
#include "web/TabWidget.h"
#include "web/ToolBarSearch.h"
#include "web/WebView.h"


WebDownloadManager *BrowserWindow::sWebDownloadManager = 0;

struct BrowserWindow::Private
{
    QToolBar *mWebToolBar{nullptr};
    //ToolbarSearch *mSearchToolBar;
    ChaseWidget *mChaseWidget{nullptr};
    TabWidget *mTabWidget{nullptr};

    //QAction *mSearchAction;

    QString mLastSearch;

    MainWindow* mMainWindow{nullptr};

    QMenu *mHistoryBackMenu{nullptr};
    QMenu *mHistoryForwardMenu{nullptr};
};

BrowserWindow::BrowserWindow(QWidget *parent, MainWindow* mainWindow)
        : QWidget(parent)
        , d(new Private)
{	
    //, mSearchToolBar(0)
    d->mTabWidget = new TabWidget(this);
    //, mSearchAction(0)
    d->mMainWindow = mainWindow;

    QWebEngineSettings *defaultSettings = QWebEngineProfile::defaultProfile()->settings();
    defaultSettings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    defaultSettings->setAttribute(QWebEngineSettings::PluginsEnabled, true);

    setupMenu();
    setupToolBar();


    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    layout->addWidget(d->mTabWidget);

    this->setLayout(layout);

    connect(d->mTabWidget, &TabWidget::loadPage, this, &BrowserWindow::loadPage);


    connect(d->mTabWidget, &TabWidget::setCurrentTitle, this, &BrowserWindow::slotUpdateWindowTitle);

    connect(d->mTabWidget, &TabWidget::loadProgress, this, &BrowserWindow::slotLoadProgress);


    connect(d->mTabWidget, &TabWidget::loadFinished, this, [this] (bool) { emit activeViewPageChanged(); });

    connect(d->mTabWidget, &TabWidget::geometryChangeRequested, this, &BrowserWindow::geometryChangeRequested);

    slotUpdateWindowTitle();


    d->mTabWidget->newTab();

    connect(d->mTabWidget, &QTabWidget::currentChanged, this, &BrowserWindow::tabCurrentChanged);

}


BrowserWindow::~BrowserWindow()
{
    if(d->mTabWidget){
        delete d->mTabWidget;
        d->mTabWidget = NULL;
    }
}

WebDownloadManager *BrowserWindow::downloadManager()
{
    if (!sWebDownloadManager) {
        sWebDownloadManager = new WebDownloadManager(Application::mainWindow);
    }
    return sWebDownloadManager;
}

QSize BrowserWindow::sizeHint() const
{
    QRect desktopRect = VirtualDesktop::instance()->screenGeometry(Application::controlScreenIndex());
    QSize size = desktopRect.size() * qreal(0.9);
    return size;
}

void BrowserWindow::setupMenu()
{
    new QShortcut(QKeySequence(Qt::Key_F6), this, this, &BrowserWindow::slotSwapFocus);
}

void BrowserWindow::setupToolBar()
{
    d->mWebToolBar = d->mMainWindow->ui().webToolBar;

    d->mTabWidget->addWebAction(d->mMainWindow->ui().actionWebBack, QWebEnginePage::Back);
    d->mTabWidget->addWebAction(d->mMainWindow->ui().actionWebForward, QWebEnginePage::Forward);
    d->mTabWidget->addWebAction(d->mMainWindow->ui().actionWebReload, QWebEnginePage::Reload);
    d->mTabWidget->addWebAction(d->mMainWindow->ui().actionStopLoading, QWebEnginePage::Stop);

    d->mHistoryBackMenu = new QMenu(this);
    connect(d->mHistoryBackMenu, &QMenu::aboutToShow, this, &BrowserWindow::aboutToShowBackMenu);
    connect(d->mHistoryBackMenu, &QMenu::triggered, this, &BrowserWindow::openActionUrl);

    //foreach (QWidget* menuWidget,  mMainWindow->ui().actionWebBack->associatedWidgets())
    //{
    //    QToolButton *tb = qobject_cast<QToolButton*>(menuWidget);

    //    if (tb && !tb->menu())
    //    {
    //        tb->setMenu(mHistoryBackMenu);
    //       // tb->setStyleSheet("QToolButton::menu-indicator { subcontrol-position: bottom left; padding-left: 8px;}");
    //    }
    //}

    d->mHistoryForwardMenu = new QMenu(this);
    connect(d->mHistoryForwardMenu, &QMenu::aboutToShow, this, &BrowserWindow::aboutToShowForwardMenu);
    connect(d->mHistoryForwardMenu, &QMenu::triggered, this, &BrowserWindow::openActionUrl);

    //foreach (QWidget* menuWidget,  mMainWindow->ui().actionWebForward->associatedWidgets())
    //{
    //    QToolButton *tb = qobject_cast<QToolButton*>(menuWidget);

    //    if (tb && !tb->menu())
    //    {
    //        tb->setMenu(mHistoryForwardMenu);
    //        //tb->setStyleSheet("QToolButton { padding-right: 8px; }");
    //    }
    //}

    d->mWebToolBar->insertWidget(d->mMainWindow->ui().actionWebBigger, d->mTabWidget->lineEditStack());

    //mSearchToolBar = new ToolbarSearch(mWebToolBar);

    //mSearchAction = mWebToolBar->insertWidget(mMainWindow->ui().actionWebBigger, mSearchToolBar);

    //connect(mSearchToolBar, SIGNAL(search(const QUrl&)), SLOT(loadUrl(const QUrl&)));

    d->mChaseWidget = new ChaseWidget(this);
    d->mWebToolBar->insertWidget(d->mMainWindow->ui().actionWebBigger, d->mChaseWidget);
    d->mWebToolBar->insertSeparator(d->mMainWindow->ui().actionWebBigger);

    connect(d->mMainWindow->ui().actionHome, &QAction::triggered, this, [this] { slotHome(); });

    connect(d->mMainWindow->ui().actionWebBigger, &QAction::triggered, this, [this] { slotViewZoomIn(); });
    connect(d->mMainWindow->ui().actionWebSmaller, &QAction::triggered, this, [this] { slotViewZoomOut(); });

    d->mWebToolBar->show();
}


//void BrowserWindow::adaptToolBar(bool wideRes)
//{
//    if (mSearchAction)
//    {
//        mSearchAction->setVisible(wideRes);
//    }
//}

QUrl BrowserWindow::guessUrlFromString(const QString &string)
{
    QString urlStr = string.trimmed();
    QRegExp test(QLatin1String("^[a-zA-Z]+\\:.*"));

    // Check if it looks like a qualified URL. Try parsing it and see.
    bool hasSchema = test.exactMatch(urlStr);
    if (hasSchema)
    {
        int dotCount = urlStr.count(".");

        if (dotCount == 0 && !urlStr.contains(".com"))
        {
            urlStr += ".com";
        }

        QUrl url = QUrl::fromEncoded(urlStr.toUtf8(), QUrl::TolerantMode);
        if (url.isValid())
        {
            return url;
        }
    }

    // Might be a file.
    if (QFile::exists(urlStr))
    {
        QFileInfo info(urlStr);
        return QUrl::fromLocalFile(info.absoluteFilePath());
    }

    // Might be a shorturl - try to detect the schema.
    if (!hasSchema)
    {
        QString schema = "http";

        QString guessed = schema + "://" + urlStr;

        int dotCount = guessed.count(".");

        if (dotCount == 0 && !urlStr.contains(".com"))
        {
            guessed += ".com";
        }

        QUrl url = QUrl::fromEncoded(guessed.toUtf8(), QUrl::TolerantMode);

        if (url.isValid())
            return url;
    }

    // Fall back to QUrl's own tolerant parser.
    QUrl url = QUrl::fromEncoded(string.toUtf8(), QUrl::TolerantMode);

    // finally for cases where the user just types in a hostname add http
    if (url.scheme().isEmpty())
        url = QUrl::fromEncoded("http://" + string.toUtf8(), QUrl::TolerantMode);

    return url;
}

void BrowserWindow::loadUrl(const QUrl &url)
{
    if (!currentTabWebView() || !url.isValid())
        return;

    d->mTabWidget->currentLineEdit()->setText(url.toString());
    d->mTabWidget->loadUrlInCurrentTab(url);
}

void BrowserWindow::loadUrlInNewTab(const QUrl &url)
{
    if (!url.isValid())
        return;

    d->mTabWidget->newTab();
    loadUrl(url);
}

WebView* BrowserWindow::createNewTab()
{
    return d->mTabWidget->newTab();
}

void BrowserWindow::slotSelectLineEdit()
{
    d->mTabWidget->currentLineEdit()->selectAll();
    d->mTabWidget->currentLineEdit()->setFocus();
}

void BrowserWindow::slotFileSaveAs()
{
    downloadManager()->download(currentTabWebView()->url(), true);
}

void BrowserWindow::slotUpdateStatusbar(const QString &string)
{
    Q_UNUSED(string);
}

void BrowserWindow::slotUpdateWindowTitle(const QString &title)
{
    Q_UNUSED(title);
}

void BrowserWindow::closeEvent(QCloseEvent *event)
{
    event->accept();
    deleteLater();
}

void BrowserWindow::slotViewZoomIn()
{
    WebView *currentWebView = currentTabWebView();

    if (!currentWebView)
        return;

    currentWebView->setZoomFactor(currentWebView->zoomFactor() + 0.1);
}

void BrowserWindow::slotViewZoomOut()
{
    WebView *currentWebView = currentTabWebView();

    if (!currentWebView)
        return;

    currentWebView->setZoomFactor(currentWebView->zoomFactor() - 0.1);
}

void BrowserWindow::slotViewResetZoom()
{
    if (!currentTabWebView())
        return;
    currentTabWebView()->setZoomFactor(1.0);
}

void BrowserWindow::slotViewZoomTextOnly(bool enable)
{
    if (!currentTabWebView())
        return;
    currentTabWebView()->page()->profile()->settings()->setAttribute(QWebEngineSettings::AutoLoadIconsForPage, enable); //QWebSettings::ZoomTextOnly
}

void BrowserWindow::slotHome()
{
    loadPage(Settings::settings()->webHomePage->get().toString());
}

void BrowserWindow::slotWebSearch()
{
    //mSearchToolBar->lineEdit()->selectAll();
    //mSearchToolBar->lineEdit()->setFocus();
}

void BrowserWindow::slotToggleInspector(bool enable)
{
    Q_UNUSED(enable);
}

void BrowserWindow::slotSwapFocus()
{
    if (currentTabWebView()->hasFocus())
        d->mTabWidget->currentLineEdit()->setFocus();
    else
        currentTabWebView()->setFocus();
}

void BrowserWindow::loadPage(const QString &page)
{
    QUrl url = guessUrlFromString(page);
    loadUrl(url);
}

TabWidget *BrowserWindow::tabWidget() const
{
    return d->mTabWidget;
}

WebView *BrowserWindow::currentTabWebView() const
{
    return d->mTabWidget->currentWebView();
}

void BrowserWindow::slotLoadProgress(int progress)
{
    if (d->mChaseWidget)
        d->mChaseWidget->setAnimated(progress > 0 && progress < 100);
}

void BrowserWindow::geometryChangeRequested(const QRect &geometry)
{
    setGeometry(geometry);
}

void BrowserWindow::tabCurrentChanged(int index)
{
    QWidget* current = d->mTabWidget->widget(index);

    emit activeViewChange(current);
    emit activeViewPageChanged();
}

WebView* BrowserWindow::paintWidget()
{
    return d->mTabWidget->currentWebView();
}

void BrowserWindow::showTabAtTop(bool attop)
{
    if (attop)
        d->mTabWidget->setTabPosition(QTabWidget::North);
    else
        d->mTabWidget->setTabPosition(QTabWidget::South);
}

void BrowserWindow::aboutToShowBackMenu()
{
    d->mHistoryBackMenu->clear();
    if (!currentTabWebView())
        return;
    QWebEngineHistory *history = currentTabWebView()->history();

    int historyCount = history->count();
    int historyLimit = history->backItems(historyCount).count() - Settings::settings()->historyLimit->get().toReal();
    if (historyLimit < 0)
        historyLimit = 0;

    for (int i = history->backItems(historyCount).count() - 1; i >= historyLimit; --i)
    {
		QWebEngineHistoryItem item = history->backItems(historyCount).at(i);

        QAction *action = new QAction(this);
        action->setData(-1*(historyCount-i-1));

        /*if (!QWebEngineSettings::iconForUrl(item.originalUrl()).isNull())
			action->setIcon(item.icon());*/
        action->setText(item.title().isEmpty() ? item.url().toString() : item.title());
        d->mHistoryBackMenu->addAction(action);
    }

    d->mHistoryBackMenu->addSeparator();

    QAction *action = new QAction(this);
    action->setData("clear");
    action->setText("Clear history");
    d->mHistoryBackMenu->addAction(action);

}

void BrowserWindow::aboutToShowForwardMenu()
{
    d->mHistoryForwardMenu->clear();
    if (!currentTabWebView())
        return;
    QWebEngineHistory *history = currentTabWebView()->history();
    int historyCount = history->count();

    int historyLimit = history->forwardItems(historyCount).count();
    if (historyLimit > Settings::settings()->historyLimit->get().toReal())
        historyLimit = Settings::settings()->historyLimit->get().toReal();

    for (int i = 0; i < historyLimit; ++i)
    {
		QWebEngineHistoryItem item = history->forwardItems(historyCount).at(i);

        QAction *action = new QAction(this);
        action->setData(historyCount-i);

		/*if (!QWebEngineSettings::iconForUrl(item.originalUrl()).isNull())
				 action->setIcon(item.icon());*/
        action->setText(item.title().isEmpty() ? item.url().toString() : item.title());
        d->mHistoryForwardMenu->addAction(action);
    }

    d->mHistoryForwardMenu->addSeparator();

    QAction *action = new QAction(this);
    action->setData("clear");
    action->setText("Clear history");
    d->mHistoryForwardMenu->addAction(action);
}

void BrowserWindow::openActionUrl(QAction *action)
{
    QWebEngineHistory *history = currentTabWebView()->history();

    if (action->data() == "clear")
    {
        history->clear();
        return;
    }

    int offset = action->data().toInt();
    if (offset < 0)
        history->goToItem(history->backItems(-1*offset).first());
    else if (offset > 0)
        history->goToItem(history->forwardItems(history->count() - offset + 1).back());
 }
