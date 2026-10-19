#include <QtWidgets>
#include "core/VirtualDesktop.h"
#include <QWebEnginePage>

#include "platform/PlatformUtils.h"

#include "web/WebController.h"

#include "web/BrowserWindow.h"
#include "web/WebView.h"
#include "web/WebDownloadManager.h"
#include "web/TabWidget.h"

#include "ui/WidgetMirror.h"
#include "ui/MainWindow.h"
#include "ui/WebToolsPalette.h"
#include "platform/KeyboardPalette.h"

#include "settings/Settings.h"
#include "settings/Setting.h"
#include "core/Application.h"
#include "core/ApplicationController.h"
#include "core/DisplayManager.h"

#include "whiteboard/BoardController.h"
#include "whiteboard/DrawingController.h"

#include "platform/CustomCaptureWindow.h"
#include "whiteboard/BoardPaletteManager.h"


WebController::WebController(MainWindow *mainWindow)
    : QObject(mainWindow->centralWidget()),
      mMainWindow(mainWindow),
      mCurrentWebBrowser(0),
      mBrowserWidget(0),
      mToolsCurrentPalette(0) {}


WebController::~WebController() {
    // NOOP
}

void WebController::webBrowserInstance() {
    QString webHomePage = Settings::settings()->webHomePage->get().toString();
    QUrl currentUrl = BrowserWindow::guessUrlFromString(webHomePage);

    if (Settings::settings()->webUseExternalBrowser->get().toBool()) {
        QDesktopServices::openUrl(currentUrl);
    } else {
        if (!mCurrentWebBrowser) {
            mCurrentWebBrowser = new BrowserWindow(mMainWindow->centralWidget(), mMainWindow);

            mMainWindow->addWebWidget(mCurrentWebBrowser);

            connect(mCurrentWebBrowser, &BrowserWindow::activeViewChange, this,
                    &WebController::setSourceWidget);

            BrowserWindow::downloadManager()->setParent(mCurrentWebBrowser, Qt::Tool);

            Application::app()->insertSpaceToToolbarBeforeAction(mMainWindow->ui().webToolBar,
                                                                 mMainWindow->ui().actionBoard, 32);
            Application::app()->decorateActionMenu(mMainWindow->ui().actionMenu);

            showTabAtTop(Settings::settings()->appToolBarPositionedAtTop->get().toBool());

            adaptToolBar();

            connect(mCurrentWebBrowser, &BrowserWindow::activeViewPageChanged, this,
                    &WebController::activePageChanged);

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

void WebController::show() {
    webBrowserInstance();
}

void WebController::setSourceWidget(QWidget *pWidget) {
    mBrowserWidget = pWidget;
    Application::applicationController->setMirrorSourceWidget(pWidget);
}

void WebController::activePageChanged() {
    if (mCurrentWebBrowser && mCurrentWebBrowser->currentTabWebView()) {
        mMainWindow->ui().actionWebTrap->setChecked(false);

        emit activeWebPageChanged(mCurrentWebBrowser->currentTabWebView());
    }
}

void WebController::setupPalettes() {
    if (!mToolsCurrentPalette) {
        mToolsCurrentPalette = new WebToolsPalette(Application::mainWindow);
        Application::boardController->paletteManager()->setCurrentWebToolsPalette(mToolsCurrentPalette);
#ifndef Q_OS_WIN
        if (PlatformUtils::hasVirtualKeyboard() &&
            Application::boardController->paletteManager()->mKeyboardPalette)
            connect(Application::boardController->paletteManager()->mKeyboardPalette,
                    &KeyboardPalette::closed,
                    Application::boardController->paletteManager()->mKeyboardPalette,
                    &KeyboardPalette::onDeactivated);
#endif

        connect(mMainWindow->ui().actionWebCustomCapture, &QAction::triggered, this,
                [this] { customCapture(); });

        connect(mMainWindow->ui().actionWebShowHideOnDisplay, &QAction::toggled, this,
                &WebController::toogleMirroring);
        // 网页捕获在 Qt6 下是空的（trapElementAtPos 整个函数体还在注释里），先灰掉，
        // 别让用户点亮一个只会改变光标、什么都不捕获的按钮。恢复要重新实现命中测试。
        mMainWindow->ui().actionWebTrap->setEnabled(false);
        connect(mMainWindow->ui().actionWebTrap, &QAction::toggled, this, &WebController::toggleWebTrap);

        mToolsCurrentPalette->hide();
        mToolsCurrentPalette->adjustSizeAndPosition();

        if (controlView()) {
            int left = controlView()->width() - 20 - mToolsCurrentPalette->width();
            int top = (controlView()->height() - mToolsCurrentPalette->height()) / 2;
            mToolsCurrentPalette->setCustomPosition(true);
            mToolsCurrentPalette->move(left, top);
        }

        connect(mMainWindow->ui().actionWebTools, &QAction::triggered, this, &WebController::triggerWebTools);
        mMainWindow->ui().actionWebTools->trigger();
    }
}


void WebController::toggleWebTrap(bool checked) {
    if (mCurrentWebBrowser && mCurrentWebBrowser->currentTabWebView())
        mCurrentWebBrowser->currentTabWebView()->setIsTrapping(checked);
}


void WebController::customCapture() {
    mToolsCurrentPalette->setVisible(false);
    qApp->processEvents();

    CustomCaptureWindow customCaptureWindow(mCurrentWebBrowser);

    customCaptureWindow.show();

    if (customCaptureWindow.execute(getScreenPixmap()) == QDialog::Accepted) {
        QPixmap selectedPixmap = customCaptureWindow.getSelectedPixmap();
        emit imageCaptured(selectedPixmap, false, mCurrentWebBrowser->currentTabWebView()->url());
    }

    mToolsCurrentPalette->setVisible(true);
}


void WebController::toogleMirroring(bool checked) {
    Application::applicationController->mirroringEnabled(checked);
}


QPixmap WebController::getScreenPixmap() {
    VirtualDesktop *desktop = VirtualDesktop::instance();
    // we capture the screen in which the mouse is.
    const QPoint cursorPos = QCursor::pos();
    const QRect primaryScreenRect = desktop->screenGeometry(cursorPos);

    return VirtualDesktop::grabScreen(QGuiApplication::screenAt(cursorPos), primaryScreenRect);
}


void WebController::screenLayoutChanged() {
    bool hasDisplay =
        (Application::applicationController && Application::applicationController->displayManager() &&
         Application::applicationController->displayManager()->hasDisplay());

    Application::mainWindow->ui().actionWebShowHideOnDisplay->setVisible(hasDisplay);
}


void WebController::closing() {
    //NOOP
}


void WebController::adaptToolBar() {
    bool highResolution = mMainWindow->width() > 1024;

    mMainWindow->ui().actionWebReload->setVisible(highResolution);
    mMainWindow->ui().actionStopLoading->setVisible(highResolution);
}


void WebController::showTabAtTop(bool attop) {
    if (mCurrentWebBrowser)
        mCurrentWebBrowser->showTabAtTop(attop);
}

void WebController::loadUrl(const QUrl &url) {
    Application::applicationController->showInternet();
    if (Settings::settings()->webUseExternalBrowser->get().toBool())

        QDesktopServices::openUrl(url);
    else
        mCurrentWebBrowser->loadUrlInNewTab(url);
}


QWebEngineView *WebController::createNewTab() {
    if (mCurrentWebBrowser)
        Application::applicationController->showInternet();

    return mCurrentWebBrowser->createNewTab();
}


void WebController::copy() {
    if (mCurrentWebBrowser && mCurrentWebBrowser->currentTabWebView()) {
        QWebEngineView *webView = mCurrentWebBrowser->currentTabWebView();
        QAction *act = webView->pageAction(QWebEnginePage::Copy);
        if (act)
            act->trigger();
    }
}


void WebController::paste() {
    if (mCurrentWebBrowser && mCurrentWebBrowser->currentTabWebView()) {
        QWebEngineView *webView = mCurrentWebBrowser->currentTabWebView();
        QAction *act = webView->pageAction(QWebEnginePage::Paste);
        if (act)
            act->trigger();
    }
}


void WebController::cut() {
    if (mCurrentWebBrowser && mCurrentWebBrowser->currentTabWebView()) {
        QWebEngineView *webView = mCurrentWebBrowser->currentTabWebView();
        QAction *act = webView->pageAction(QWebEnginePage::Cut);
        if (act)
            act->trigger();
    }
}

void WebController::triggerWebTools(bool checked) {
    mToolsCurrentPalette->setVisible(checked);
}