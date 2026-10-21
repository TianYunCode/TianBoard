/**
 * @file WebController.h
 * @brief 声明 WebController：网页模式的入口控制器，创建浏览器窗口、工具面板并转接屏幕捕获。
 */

#pragma once

#include <QtWidgets>
#include <QtWebEngineWidgets>
#include <QWebEnginePage>
#include <QWebEngineView>

#include "core/IWebController.h"

class BrowserWindow;
class Application;
class MainWindow;
class WebToolsPalette;

/**
 * @brief 网页控制器：web 域与主窗口/看板之间的协调者。
 *
 * 首次进入网页模式时才创建 BrowserWindow，并把它加入主窗口的网页容器；同时设置镜像源、
 * 工具面板和“在副屏显示”动作。它不直接操作页面内容，只把捕获结果通过 imageCaptured
 * 交给 ApplicationController，把当前网页通过 activeWebPageChanged 通知看板。这两个信号住在
 * core/IWebController.h 那份契约上：core 与 recording 因此不必认识本类（拆环见
 * docs/architecture.md 第 7 步）。
 */
class WebController : public IWebController
{
    Q_OBJECT

public:
    /**
     * @brief 构造控制器，初始没有浏览器窗口。
     * @param mainWindow 主窗口。
     */
    WebController(MainWindow *mainWindow);

    /** @brief 析构。 */
    virtual ~WebController();

    /** @brief 退出前按白名单清 Cookie（自动清除开着时才动手）。 */
    void closing() override;

    /**
     * @brief 按 Web/CookiePolicy 装 Cookie 过滤器，并把 WebEngine 的 Cookie 信号镜像到
     *        应用的 Cookie 罐（顺带 loadAllCookies 让盘上的历史 Cookie 也进镜像）。
     * @note 构造时调用一次；设置页改了策略会再调一次，选完立即生效不用重启。重复调用
     *       只换过滤器，镜像连线不会重复安装。
     */
    void installCookiePolicy() override;

    /**
     * @brief 按当前设置统计：本次会话镜像到的 Cookie 里有多少个在退出时会被清掉。
     * @note 白名单空着时全都算进去——那种情况下走的是“一次性清空整个 Cookie 罐”，
     *       不是逐条删除，数字只是给用户看的预期。
     */
    int cookiesMarkedForDeletion() const override;

    /** @brief 按窗口宽度调整工具栏动作的显隐。 */
    void adaptToolBar() override;

    /** @brief 调整标签栏在顶部还是底部。 */
    void showTabAtTop(bool topAtTop) override;

    /**
     * @brief 加载 URL：外部浏览器模式下交给系统浏览器，否则在新标签页打开。
     * @note 内部浏览器分支假定 show() 已经创建了浏览器窗口。
     */
    void loadUrl(const QUrl &url) override;

    /**
     * @brief 在网页模式中新建标签页。
     * @return 新建页的 QWebEngineView；没有浏览器窗口时可能为空。
     */
    QWebEngineView *createNewTab();

    /** @brief 进入网页模式（懒创建浏览器窗口）。 */
    void show() override;

protected:
    /** @brief 创建网页工具面板并连接捕获/镜像/工具动作。 */
    void setupPalettes();
    /** @brief 抓取鼠标所在屏幕的截图，供自定义截图使用。 */
    QPixmap getScreenPixmap();

public slots:
    /** @brief 显示器布局变化时更新“在副屏显示”动作的可见性。 */
    void screenLayoutChanged() override;

    /** @brief 活动视图变化时更新镜像源与当前页。 */
    void setSourceWidget(QWidget *pWidget);
    /** @brief 打开自定义截图窗口并把结果发出去。 */
    void customCapture();
    /** @brief 开启/关闭屏幕镜像。 */
    void toogleMirroring(bool checked);

    /** @brief 返回网页模式的中央控件。 */
    QWidget *controlView() override
    {
        return mBrowserWidget;
    }

    /** @brief 对当前网页执行复制。 */
    void copy() override;
    /** @brief 对当前网页执行粘贴。 */
    void paste() override;
    /** @brief 对当前网页执行剪切。 */
    void cut() override;
    /** @brief 显示/隐藏网页工具面板。 */
    void triggerWebTools(bool checked);

private slots:
    /** @brief 切换标签页后更新当前网页并复位捕获按钮。 */
    void activePageChanged();
    /** @brief 开/关网页元素捕获。 */
    void toggleWebTrap(bool checked);

private:
    /** @brief 懒创建浏览器窗口并切换到网页模式。 */
    void webBrowserInstance();

    /** @brief 退出时执行清 Cookie：白名单空着就整罐清空，否则逐条删掉不匹配的。 */
    void clearCookiesOnExit();

    /** @brief 主窗口。 */
    MainWindow *mMainWindow;

    /** @brief 当前浏览器窗口，关闭后由 destroyed 信号置空。 */
    BrowserWindow *mCurrentWebBrowser;

    /** @brief 当前用于镜像的网页视图控件。 */
    QWidget *mBrowserWidget;

    /** @brief 网页工具面板。 */
    WebToolsPalette *mToolsCurrentPalette;

    /** @brief Cookie 镜像连线是否已经装上；改策略会重调 installCookiePolicy，连线不能重复。 */
    bool mCookieMirrorInstalled{false};
};
