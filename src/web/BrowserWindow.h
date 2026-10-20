/**
 * @file BrowserWindow.h
 * @brief 声明浏览器窗口 BrowserWindow：承载标签页、网页工具栏与下载管理器入口。
 */

#pragma once

#include <QtWidgets>
#include <QtWebEngineWidgets>

#include <memory>

class ChaseWidget;
class TabWidget;
class ToolbarSearch;
class WebView;
class MainWindow;
class WebDownloadManager;

/**
 * @brief 浏览器窗口：网页模式的顶层容器，组合 TabWidget、工具栏部件与前进/后退历史菜单。
 *
 * 位于 web 域的窗口层，由 WebController 在首次进入网页模式时创建，并 addWebWidget 到主窗口。
 * 关闭时 closeEvent 走 deleteLater，WebController 通过 destroyed 信号清空自己的裸指针。
 * BrowserWindow 同时是 WebPage::mainWindow() 的查找终点，也是全局下载管理器
 * （静态 sWebDownloadManager）的宿主。
 */
class BrowserWindow : public QWidget
{
    Q_OBJECT;

public:
    /**
     * @brief 构造浏览器窗口：设置 WebEngine 全局属性、工具栏并创建首个标签页。
     * @param parent 父控件。
     * @param mainWindow 主窗口，工具栏动作与设置都取自它。
     */
    BrowserWindow(QWidget *parent = 0, MainWindow *mainWindow = 0);

    /**
     * @brief 析构：若静态下载管理器挂在窗口下则先清空静态指针，再删除标签页。
     */
    ~BrowserWindow();

    /** @brief 建议窗口尺寸为主屏工作区的 90%。 */
    QSize sizeHint() const;

public:
    /**
     * @brief 把用户输入的文本猜测成 URL。
     *
     * 依次尝试：带协议头的地址、本地文件、自动补 http:// 的主机名，最后交给 QUrl 的
     * 宽容解析模式。
     */
    static QUrl guessUrlFromString(const QString &url);

    /** @brief 返回内部标签页容器。 */
    TabWidget *tabWidget() const;

    /** @brief 返回当前标签页的 WebView，没有则返回空。 */
    WebView *currentTabWebView() const;

    /**
     * @brief 返回全局下载管理器，首次调用时创建。
     *
     * 管理器以主窗口为父对象，因此整个进程共用同一个下载对话框。
     */
    static WebDownloadManager *downloadManager();

public slots:
    /**
     * @brief 把用户输入的字符串解析成 URL 后加载。
     */
    void loadPage(const QString &url);

    /** @brief 加载设置中的主页。 */
    void slotHome();

    /**
     * @brief 在当前标签页加载 URL，并同步地址栏文本。
     */
    void loadUrl(const QUrl &url);

    /**
     * @brief 新开标签页并加载 URL。
     */
    void loadUrlInNewTab(const QUrl &url);

    /**
     * @brief 新建标签页。
     * @return 新建的 WebView；首个标签页可能只创建占位控件而返回空。
     */
    WebView *createNewTab();

    /**
     * @brief 返回用于屏幕镜像的当前 WebView。
     */
    WebView *paintWidget();

    /**
     * @brief 标签页切换时同步活动视图信号。
     */
    void tabCurrentChanged(int);

    /**
     * @brief 设置标签栏在顶部还是底部。
     */
    void showTabAtTop(bool attop);

    /** @brief 后退历史菜单弹出前填充历史条目。 */
    void aboutToShowBackMenu();
    /** @brief 前进历史菜单弹出前填充历史条目。 */
    void aboutToShowForwardMenu();

    /** @brief 处理历史菜单项：跳转或清空历史。 */
    void openActionUrl(QAction *action);

signals:
    /** @brief 当前页面加载状态或标题变化，镜像视图需要刷新。 */
    void activeViewPageChanged();
    /** @brief 活动视图控件变化。 */
    void activeViewChange(QWidget *);
    /** @brief 屏幕镜像开关变化。 */
    void mirroringEnabled(bool checked);

protected:
    /** @brief 关闭窗口时接受事件并延迟销毁。 */
    void closeEvent(QCloseEvent *event);

private slots:

    /** @brief 按加载进度启停转轮动画。 */
    void slotLoadProgress(int);
    /** @brief 状态栏文本更新，当前未使用。 */
    void slotUpdateStatusbar(const QString &string);
    /** @brief 窗口标题更新，当前未使用。 */
    void slotUpdateWindowTitle(const QString &title = QString());

    /** @brief 把当前页面另存为（走下载流程）。 */
    void slotFileSaveAs();

    /** @brief 放大当前页面。 */
    void slotViewZoomIn();
    /** @brief 缩小当前页面。 */
    void slotViewZoomOut();
    /** @brief 恢复 100% 缩放。 */
    void slotViewResetZoom();
    /** @brief 切换“仅缩放文字”，当前实现落到 AutoLoadIconsForPage 属性上。 */
    void slotViewZoomTextOnly(bool enable);

    /** @brief F6 快捷键在地址栏与页面之间切换焦点。 */
    void slotSwapFocus();

    /** @brief 页面请求改变窗口几何时应用之。 */
    void geometryChangeRequested(const QRect &geometry);

private:
    /** @brief 进程级共享的下载管理器。 */
    static WebDownloadManager *sWebDownloadManager;

    /** @brief 注册 F6 快捷键等窗口级菜单动作。 */
    void setupMenu();
    /** @brief 组装网页工具栏、历史菜单与转轮部件。 */
    void setupToolBar();
    /**
     * @brief 给工具栏按钮挂上历史下拉菜单。
     * @note 按 QToolButton 的 MenuButtonPopup 模式，让按钮主体保持“后退一步”。
     */
    void attachHistoryMenu(QAction *action, QMenu *menu);
    /** @brief 更新状态栏动作文本的可见性。 */
    void updateStatusbarActionText(bool visible);

    /** @brief 私有实现，隔离内部部件细节。 */
    struct Private;
    std::unique_ptr<Private> d;
};
