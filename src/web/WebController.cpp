#include <QDebug>
#include <QtWidgets>
#include "platform/VirtualDesktop.h"
#include <QWebEngineCookieStore>
#include <QWebEnginePage>
#include <QWebEngineProfile>

#include "platform/PlatformUtils.h"

#include "web/WebController.h"

#include "web/BrowserWindow.h"
#include "web/WebView.h"
#include "web/WebDownloadManager.h"
#include "web/TabWidget.h"

#include "ui/WidgetMirror.h"
#include "ui/MainWindow.h"
#include "ui/WebToolsPalette.h"

#include "settings/Settings.h"
#include "settings/Setting.h"
#include "core/Application.h"
#include "core/ApplicationController.h"
#include "core/CookieJar.h"
#include "core/DisplayManager.h"
#include "core/NetworkAccessManager.h"
#include "utils/CookieRules.h"

#include "whiteboard/BoardController.h"
#include "whiteboard/DrawingController.h"

#include "ui/CustomCaptureWindow.h"
#include "whiteboard/BoardPaletteManager.h"

namespace
{
    /**
     * @brief 取回应用共享的 Cookie 罐。
     * @return 不是 CookieJar 时返回空指针——NetworkAccessManager 构造时装的就是它，
     *         正常跑起来不会为空。
     */
    CookieJar *appCookieJar()
    {
        return qobject_cast<CookieJar *>(NetworkAccessManager::defaultAccessManager()->cookieJar());
    }
} // namespace

WebController::WebController(MainWindow *mainWindow)
    : IWebController(mainWindow->centralWidget()), mMainWindow(mainWindow), mCurrentWebBrowser(0), mBrowserWidget(0), mToolsCurrentPalette(0)
{
    installCookiePolicy();
}

WebController::~WebController()
{
    // 浏览器窗口与工具面板都由主窗口的控件树持有，随父对象销毁，这里无需手动释放。
}

void WebController::webBrowserInstance()
{
    QString webHomePage = Settings::settings()->webHomePage->get().toString();
    QUrl    currentUrl  = BrowserWindow::guessUrlFromString(webHomePage);

    if (Settings::settings()->webUseExternalBrowser->get().toBool())
    {
        QDesktopServices::openUrl(currentUrl);
    }
    else
    {
        // 浏览器窗口懒创建：首次进入网页模式才构建，之后复用同一个实例。
        if (!mCurrentWebBrowser)
        {
            mCurrentWebBrowser = new BrowserWindow(mMainWindow->centralWidget(), mMainWindow);

            // closeEvent 里是 deleteLater()，窗口没了 mCurrentWebBrowser/mBrowserWidget
            // 必须跟着置空，否则下次进入网页模式会把悬空指针当活对象用。
            connect(mCurrentWebBrowser, &QObject::destroyed, this,
                [this]
                {
                    mCurrentWebBrowser = nullptr;
                    mBrowserWidget     = nullptr;
                });

            mMainWindow->addWebWidget(mCurrentWebBrowser);

            connect(mCurrentWebBrowser, &BrowserWindow::activeViewChange, this, &WebController::setSourceWidget);

            BrowserWindow::downloadManager()->setParent(mCurrentWebBrowser, Qt::Tool);

            Application::app()->insertSpaceToToolbarBeforeAction(mMainWindow->ui().webToolBar, mMainWindow->ui().actionBoard, 32);
            Application::app()->decorateActionMenu(mMainWindow->ui().actionMenu);

            showTabAtTop(Settings::settings()->appToolBarPositionedAtTop->get().toBool());

            adaptToolBar();

            connect(mCurrentWebBrowser, &BrowserWindow::activeViewPageChanged, this, &WebController::activePageChanged);

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
}

void WebController::show()
{
    webBrowserInstance();
}

void WebController::setSourceWidget(QWidget *pWidget)
{
    mBrowserWidget = pWidget;
    Application::applicationController->setMirrorSourceWidget(pWidget);
}

void WebController::activePageChanged()
{
    if (mCurrentWebBrowser && mCurrentWebBrowser->currentTabWebView())
    {
        mMainWindow->ui().actionWebTrap->setChecked(false);

        emit activeWebPageChanged(mCurrentWebBrowser->currentTabWebView());
    }
}

void WebController::setupPalettes()
{
    if (!mToolsCurrentPalette)
    {
        mToolsCurrentPalette = new WebToolsPalette(Application::mainWindow);
        Application::boardController->paletteManager()->setCurrentWebToolsPalette(mToolsCurrentPalette);

        connect(mMainWindow->ui().actionWebCustomCapture, &QAction::triggered, this,
            [this]
            {
                customCapture();
            });

        connect(mMainWindow->ui().actionWebShowHideOnDisplay, &QAction::toggled, this, &WebController::toogleMirroring);
        // 网页元素捕获在 Qt6 移植中尚未恢复（命中测试依赖旧 WebKit 的 DOM 接口），
        // 先禁用，避免用户点亮一个只会改变光标、什么都不捕获的按钮。
        mMainWindow->ui().actionWebTrap->setEnabled(false);
        connect(mMainWindow->ui().actionWebTrap, &QAction::toggled, this, &WebController::toggleWebTrap);

        mToolsCurrentPalette->hide();
        mToolsCurrentPalette->adjustSizeAndPosition();

        if (controlView())
        {
            int left = controlView()->width() - 20 - mToolsCurrentPalette->width();
            int top  = (controlView()->height() - mToolsCurrentPalette->height()) / 2;
            mToolsCurrentPalette->setCustomPosition(true);
            mToolsCurrentPalette->move(left, top);
        }

        connect(mMainWindow->ui().actionWebTools, &QAction::triggered, this, &WebController::triggerWebTools);
        mMainWindow->ui().actionWebTools->trigger();
    }
}

void WebController::toggleWebTrap(bool checked)
{
    if (mCurrentWebBrowser && mCurrentWebBrowser->currentTabWebView())
    {
        mCurrentWebBrowser->currentTabWebView()->setIsTrapping(checked);
    }
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
        emit    imageCaptured(selectedPixmap, false, mCurrentWebBrowser->currentTabWebView()->url());
    }

    mToolsCurrentPalette->setVisible(true);
}

void WebController::toogleMirroring(bool checked)
{
    Application::applicationController->mirroringEnabled(checked);
}

QPixmap WebController::getScreenPixmap()
{
    VirtualDesktop *desktop        = VirtualDesktop::instance();
    // 抓取鼠标当前所在的那块屏幕，而不是固定抓主屏。
    const QPoint cursorPos         = QCursor::pos();
    const QRect  primaryScreenRect = desktop->screenGeometry(cursorPos);

    return VirtualDesktop::grabScreen(QGuiApplication::screenAt(cursorPos), primaryScreenRect);
}

void WebController::screenLayoutChanged()
{
    bool hasDisplay = (Application::applicationController && Application::applicationController->displayManager() &&
                       Application::applicationController->displayManager()->hasDisplay());

    Application::mainWindow->ui().actionWebShowHideOnDisplay->setVisible(hasDisplay);
}

void WebController::closing()
{
    // 浏览器窗口由主窗口统一销毁；这里只有一件事：按白名单清 Cookie。
    // 放在 closing 而不是析构里，是因为此刻 QtWebEngine 的浏览器线程还活着——
    // 等到 ~WebController 已经被父对象链拖到退出末尾，那时候删 Cookie 是不保证的。
    clearCookiesOnExit();
}

void WebController::installCookiePolicy()
{
    QWebEngineCookieStore *cookieStore = QWebEngineProfile::defaultProfile()->cookieStore();

    const CookieRules::Policy policy   = CookieRules::policyFromName(Settings::settings()->webCookiePolicy->get().toString());

    cookieStore->setCookieFilter(
        [policy](const QWebEngineCookieStore::FilterRequest &request)
        {
            switch (policy)
            {
                case CookieRules::Policy::DenyAll:
                    return false;
                case CookieRules::Policy::DenyThirdParty:
                    return !request.thirdParty;
                case CookieRules::Policy::AcceptAll:
                    return true;
            }

            return false;
        });

    if (mCookieMirrorInstalled)
    {
        return;
    }

    CookieJar *jar = appCookieJar();

    if (!jar)
    {
        qWarning() << "WebController: 应用 Cookie 罐不是 CookieJar，Cookie 清理功能不可用";
        return;
    }

    // 镜像：内核每收/删一条就同步进应用自己的罐子，顺带让应用发出的网络请求与内嵌浏览器
    // 共用同一份会话。context 用 jar，界面销毁后连接自动断开。
    connect(cookieStore, &QWebEngineCookieStore::cookieAdded, jar,
        [jar](const QNetworkCookie &cookie)
        {
            jar->insertCookie(cookie);
        });
    connect(cookieStore, &QWebEngineCookieStore::cookieRemoved, jar,
        [jar](const QNetworkCookie &cookie)
        {
            jar->deleteCookie(cookie);
        });
    mCookieMirrorInstalled = true;

    // 盘上的历史 Cookie 不会自己冒出来：loadAllCookies 让内核把每一条都发一遍 cookieAdded，
    // 镜像里才有清单可逐条判断。浏览器进程要到第一个页面才真起来，所以这次枚举不保证拿全，
    // 界面上的数字按“至少这么多”报（见 PreferencesController::cookieCountChanged）。
    cookieStore->loadAllCookies();
}

int WebController::cookiesMarkedForDeletion() const
{
    if (!Settings::settings()->webCookieAutoDelete->get().toBool())
    {
        return 0;
    }

    const CookieJar *jar = appCookieJar();

    if (!jar)
    {
        return 0;
    }

    const QStringList           keepDomains = Settings::settings()->webCookieKeepDomains->get().toStringList();
    const QList<QNetworkCookie> cookies     = jar->storedCookies();

    if (keepDomains.isEmpty())
    {
        return cookies.size();
    }

    int marked = 0;

    for (const QNetworkCookie &cookie : cookies)
    {
        if (!CookieRules::isKeptDomain(cookie.domain(), keepDomains))
        {
            ++marked;
        }
    }

    return marked;
}

void WebController::clearCookiesOnExit()
{
    if (!Settings::settings()->webCookieAutoDelete->get().toBool())
    {
        return;
    }

    QWebEngineCookieStore *cookieStore = QWebEngineProfile::defaultProfile()->cookieStore();
    const QStringList      keepDomains = Settings::settings()->webCookieKeepDomains->get().toStringList();

    if (keepDomains.isEmpty())
    {
        // 没有白名单就交给内核整罐清空：连历史 Cookie 一起，比按镜像逐条删更彻底。
        cookieStore->deleteAllCookies();
        return;
    }

    CookieJar *jar = appCookieJar();

    if (!jar)
    {
        return;
    }

    // 先拷一份：deleteCookie 会回调 cookieRemoved，进而改罐子里的条目。
    const QList<QNetworkCookie> cookies = jar->storedCookies();
    int                         deleted = 0;

    for (const QNetworkCookie &cookie : cookies)
    {
        if (!CookieRules::isKeptDomain(cookie.domain(), keepDomains))
        {
            cookieStore->deleteCookie(cookie);
            ++deleted;
        }
    }

    qDebug() << "WebController: 退出清理 Cookie，删掉" << deleted << "个，保留" << keepDomains;
}

void WebController::adaptToolBar()
{
    bool highResolution = mMainWindow->width() > 1024;

    mMainWindow->ui().actionWebReload->setVisible(highResolution);
    mMainWindow->ui().actionStopLoading->setVisible(highResolution);
}

void WebController::showTabAtTop(bool attop)
{
    if (mCurrentWebBrowser)
    {
        mCurrentWebBrowser->showTabAtTop(attop);
    }
}

void WebController::loadUrl(const QUrl &url)
{
    Application::applicationController->showInternet();
    if (Settings::settings()->webUseExternalBrowser->get().toBool())
    {

        QDesktopServices::openUrl(url);
    }
    else
    {
        mCurrentWebBrowser->loadUrlInNewTab(url);
    }
}

QWebEngineView *WebController::createNewTab()
{
    if (mCurrentWebBrowser)
    {
        Application::applicationController->showInternet();
    }

    return mCurrentWebBrowser->createNewTab();
}

void WebController::copy()
{
    if (mCurrentWebBrowser && mCurrentWebBrowser->currentTabWebView())
    {
        QWebEngineView *webView = mCurrentWebBrowser->currentTabWebView();
        QAction        *act     = webView->pageAction(QWebEnginePage::Copy);
        if (act)
        {
            act->trigger();
        }
    }
}

void WebController::paste()
{
    if (mCurrentWebBrowser && mCurrentWebBrowser->currentTabWebView())
    {
        QWebEngineView *webView = mCurrentWebBrowser->currentTabWebView();
        QAction        *act     = webView->pageAction(QWebEnginePage::Paste);
        if (act)
        {
            act->trigger();
        }
    }
}

void WebController::cut()
{
    if (mCurrentWebBrowser && mCurrentWebBrowser->currentTabWebView())
    {
        QWebEngineView *webView = mCurrentWebBrowser->currentTabWebView();
        QAction        *act     = webView->pageAction(QWebEnginePage::Cut);
        if (act)
        {
            act->trigger();
        }
    }
}

void WebController::triggerWebTools(bool checked)
{
    mToolsCurrentPalette->setVisible(checked);
}
