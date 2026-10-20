#include "web/BrowserWindow.h"
#include <QWebEngineProfile>
#include <QRegExp>

#include <QtWidgets>
#include <QtWebEngineWidgets>
#include <QWebEngineSettings>
#include <QWebEngineHistory>
#include <QWebEngineHistoryItem>
#include "core/VirtualDesktop.h"

#include "settings/Settings.h"
#include "settings/Setting.h"
#include "core/Application.h"

#include "core/NetworkAccessManager.h"
#include "core/AutoSaver.h"

#include "ui/MainWindow.h"

#include "web/ChaseWidget.h"
#include "web/WebDownloadManager.h"
#include "web/TabWidget.h"
#include "web/WebView.h"

WebDownloadManager *BrowserWindow::sWebDownloadManager = 0;

struct BrowserWindow::Private
{
    QToolBar    *mWebToolBar{nullptr};
    ChaseWidget *mChaseWidget{nullptr};
    TabWidget   *mTabWidget{nullptr};

    MainWindow *mMainWindow{nullptr};

    QMenu *mHistoryBackMenu{nullptr};
    QMenu *mHistoryForwardMenu{nullptr};
};

BrowserWindow::BrowserWindow(QWidget *parent, MainWindow *mainWindow) : QWidget(parent), d(new Private)
{
    d->mTabWidget                       = new TabWidget(this);
    d->mMainWindow                      = mainWindow;

    // 所有标签页共用默认 profile，属性只需设置一次，之后新建的页面都会继承。
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

    connect(d->mTabWidget, &TabWidget::loadFinished, this,
        [this](bool)
        {
            emit activeViewPageChanged();
        });

    connect(d->mTabWidget, &TabWidget::geometryChangeRequested, this, &BrowserWindow::geometryChangeRequested);

    slotUpdateWindowTitle();

    // 先创建首个标签页，再连接 currentChanged：初始化期间切换标签不需要刷新
    // 尚未完成布局的工具栏与状态栏。
    d->mTabWidget->newTab();

    connect(d->mTabWidget, &QTabWidget::currentChanged, this, &BrowserWindow::tabCurrentChanged);
}

BrowserWindow::~BrowserWindow()
{
    // 下载管理器在 webBrowserInstance() 里被 setParent 到本窗口，会随窗口一起销毁；
    // 静态指针不清空的话，下一次 downloadManager() 会返回悬空对象。
    if (sWebDownloadManager && sWebDownloadManager->parent() == this)
    {
        sWebDownloadManager = nullptr;
    }

    if (d->mTabWidget)
    {
        delete d->mTabWidget;
        d->mTabWidget = NULL;
    }
}

WebDownloadManager *BrowserWindow::downloadManager()
{
    if (!sWebDownloadManager)
    {
        sWebDownloadManager = new WebDownloadManager(Application::mainWindow);
    }
    return sWebDownloadManager;
}

QSize BrowserWindow::sizeHint() const
{
    QRect desktopRect = VirtualDesktop::instance()->screenGeometry(Application::controlScreenIndex());
    QSize size        = desktopRect.size() * qreal(0.9);
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
    attachHistoryMenu(d->mMainWindow->ui().actionWebBack, d->mHistoryBackMenu);

    d->mHistoryForwardMenu = new QMenu(this);
    connect(d->mHistoryForwardMenu, &QMenu::aboutToShow, this, &BrowserWindow::aboutToShowForwardMenu);
    connect(d->mHistoryForwardMenu, &QMenu::triggered, this, &BrowserWindow::openActionUrl);
    attachHistoryMenu(d->mMainWindow->ui().actionWebForward, d->mHistoryForwardMenu);

    d->mWebToolBar->insertWidget(d->mMainWindow->ui().actionWebBigger, d->mTabWidget->lineEditStack());

    d->mChaseWidget = new ChaseWidget(this);
    d->mWebToolBar->insertWidget(d->mMainWindow->ui().actionWebBigger, d->mChaseWidget);
    d->mWebToolBar->insertSeparator(d->mMainWindow->ui().actionWebBigger);

    connect(d->mMainWindow->ui().actionHome, &QAction::triggered, this,
        [this]
        {
            slotHome();
        });

    connect(d->mMainWindow->ui().actionWebBigger, &QAction::triggered, this,
        [this]
        {
            slotViewZoomIn();
        });
    connect(d->mMainWindow->ui().actionWebSmaller, &QAction::triggered, this,
        [this]
        {
            slotViewZoomOut();
        });

    d->mWebToolBar->show();
}

// 历史菜单挂在后退/前进按钮上：按钮本体仍是"后退一步"，旁边的小箭头展开历史列表。
void BrowserWindow::attachHistoryMenu(QAction *action, QMenu *menu)
{
    const QList<QObject *> associated = action->associatedObjects();
    for (QObject *object : associated)
    {
        QToolButton *button = qobject_cast<QToolButton *>(object);
        if (button && !button->menu())
        {
            button->setMenu(menu);
            button->setPopupMode(QToolButton::MenuButtonPopup);
        }
    }
}

QUrl BrowserWindow::guessUrlFromString(const QString &string)
{
    QString urlStr = string.trimmed();
    QRegExp test(QLatin1String("^[a-zA-Z]+\\:.*"));

    // 先看是否已经带协议头；带协议但没写域名后缀时补 .com，再由 QUrl 宽容解析。
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

    // 也可能是本地文件路径，存在就转成本地文件 URL。
    if (QFile::exists(urlStr))
    {
        QFileInfo info(urlStr);
        return QUrl::fromLocalFile(info.absoluteFilePath());
    }

    // 没有协议头时按“主机名”猜测，默认补 http://。
    if (!hasSchema)
    {
        QString schema  = "http";

        QString guessed = schema + "://" + urlStr;

        int dotCount    = guessed.count(".");

        if (dotCount == 0 && !urlStr.contains(".com"))
        {
            guessed += ".com";
        }

        QUrl url = QUrl::fromEncoded(guessed.toUtf8(), QUrl::TolerantMode);

        if (url.isValid())
        {
            return url;
        }
    }

    // 最后退回 QUrl 自己的宽容解析；仍没有协议时补 http://，避免地址栏输入被当成相对路径。
    QUrl url = QUrl::fromEncoded(string.toUtf8(), QUrl::TolerantMode);

    if (url.scheme().isEmpty())
    {
        url = QUrl::fromEncoded("http://" + string.toUtf8(), QUrl::TolerantMode);
    }

    return url;
}

void BrowserWindow::loadUrl(const QUrl &url)
{
    if (!currentTabWebView() || !url.isValid())
    {
        return;
    }

    d->mTabWidget->currentLineEdit()->setText(url.toString());
    d->mTabWidget->loadUrlInCurrentTab(url);
}

void BrowserWindow::loadUrlInNewTab(const QUrl &url)
{
    if (!url.isValid())
    {
        return;
    }

    d->mTabWidget->newTab();
    loadUrl(url);
}

WebView *BrowserWindow::createNewTab()
{
    return d->mTabWidget->newTab();
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
    // 关闭只做延迟销毁：WebEngine 页面还在处理事件，立即 delete 容易在回调里重入。
    event->accept();
    deleteLater();
}

void BrowserWindow::slotViewZoomIn()
{
    WebView *currentWebView = currentTabWebView();

    if (!currentWebView)
    {
        return;
    }

    currentWebView->setZoomFactor(currentWebView->zoomFactor() + 0.1);
}

void BrowserWindow::slotViewZoomOut()
{
    WebView *currentWebView = currentTabWebView();

    if (!currentWebView)
    {
        return;
    }

    currentWebView->setZoomFactor(currentWebView->zoomFactor() - 0.1);
}

void BrowserWindow::slotViewResetZoom()
{
    if (!currentTabWebView())
    {
        return;
    }
    currentTabWebView()->setZoomFactor(1.0);
}

void BrowserWindow::slotViewZoomTextOnly(bool enable)
{
    if (!currentTabWebView())
    {
        return;
    }
    currentTabWebView()->page()->profile()->settings()->setAttribute(QWebEngineSettings::AutoLoadIconsForPage, enable);
}

void BrowserWindow::slotHome()
{
    loadPage(Settings::settings()->webHomePage->get().toString());
}

void BrowserWindow::slotSwapFocus()
{
    if (currentTabWebView()->hasFocus())
    {
        d->mTabWidget->currentLineEdit()->setFocus();
    }
    else
    {
        currentTabWebView()->setFocus();
    }
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
    {
        d->mChaseWidget->setAnimated(progress > 0 && progress < 100);
    }
}

void BrowserWindow::geometryChangeRequested(const QRect &geometry)
{
    setGeometry(geometry);
}

void BrowserWindow::tabCurrentChanged(int index)
{
    QWidget *current = d->mTabWidget->widget(index);

    emit activeViewChange(current);
    emit activeViewPageChanged();
}

WebView *BrowserWindow::paintWidget()
{
    return d->mTabWidget->currentWebView();
}

void BrowserWindow::showTabAtTop(bool attop)
{
    if (attop)
    {
        d->mTabWidget->setTabPosition(QTabWidget::North);
    }
    else
    {
        d->mTabWidget->setTabPosition(QTabWidget::South);
    }
}

void BrowserWindow::aboutToShowBackMenu()
{
    d->mHistoryBackMenu->clear();
    if (!currentTabWebView())
    {
        return;
    }
    QWebEngineHistory *history = currentTabWebView()->history();

    int historyCount           = history->count();
    int historyLimit           = history->backItems(historyCount).count() - Settings::settings()->historyLimit->get().toReal();
    if (historyLimit < 0)
    {
        historyLimit = 0;
    }

    for (int i = history->backItems(historyCount).count() - 1; i >= historyLimit; --i)
    {
        QWebEngineHistoryItem item = history->backItems(historyCount).at(i);

        QAction *action            = new QAction(this);
        // 用负偏移表示后退几步，openActionUrl 据此调用 backItems(-offset)。
        action->setData(-1 * (historyCount - i - 1));

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
    {
        return;
    }
    QWebEngineHistory *history      = currentTabWebView()->history();
    int                historyCount = history->count();

    int historyLimit                = history->forwardItems(historyCount).count();
    if (historyLimit > Settings::settings()->historyLimit->get().toReal())
    {
        historyLimit = Settings::settings()->historyLimit->get().toReal();
    }

    for (int i = 0; i < historyLimit; ++i)
    {
        QWebEngineHistoryItem item = history->forwardItems(historyCount).at(i);

        QAction *action            = new QAction(this);
        // 用正偏移表示前进几步，openActionUrl 据此调用 forwardItems(count - offset + 1)。
        action->setData(historyCount - i);

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
    {
        history->goToItem(history->backItems(-1 * offset).first());
    }
    else if (offset > 0)
    {
        history->goToItem(history->forwardItems(history->count() - offset + 1).back());
    }
}
