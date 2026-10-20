/**
 * @file WebView.h
 * @brief 声明 WebPage 与 WebView：网页标签页使用的页面对象与浏览器视图。
 */

#pragma once

#include <QtWidgets>
#include <QElapsedTimer>
#include <QtWebEngineWidgets>

#include "web/WebTrapWebView.h"

class BrowserWindow;
class QWebEngineDownloadRequest;


/**
 * @brief 网页标签页的页面对象：处理导航策略、控制台输出与新窗口创建。
 *
 * 由 WebView 创建并 setPage。Ctrl+点击或中键点击链接时改在本窗口的新标签页打开，
 * mailto 链接交给系统邮件程序。WebView 在鼠标事件里把修饰键与按键记录到本对象，
 * 供 acceptNavigationRequest 判断。
 */
class WebPage : public QWebEnginePage {
    Q_OBJECT;

public:
    /** @brief 构造页面并清空鼠标状态。 */
    WebPage(QObject *parent = 0);
    /** @brief 向上遍历父对象查找所属浏览器窗口，找不到返回空。 */
    BrowserWindow *mainWindow();

    /** @brief 把 JavaScript 控制台消息转发到 qDebug。 */
    void javaScriptConsoleMessage(QWebEnginePage::JavaScriptConsoleMessageLevel level, const QString &message,
                                  int lineNumber, const QString &sourceID) override;

protected:
    /**
     * @brief 导航请求策略：mailto 交给系统，Ctrl/中键点击链接改为新标签页。
     * @return 是否允许在当前页面导航。
     */
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override;
    /** @brief JavaScript 请求新窗口时新建标签页并返回其页面。 */
    QWebEnginePage *createWindow(QWebEnginePage::WebWindowType type);

private:
    friend class WebView;

    /** @brief 触发导航时的键盘修饰键。 */
    Qt::KeyboardModifiers mKeyboardModifiers;
    /** @brief 触发导航时的鼠标按键。 */
    Qt::MouseButtons mPressedButtons;
    /** @brief 是否要在新标签页打开链接（历史字段）。 */
    bool mOpenInNewTab;
};

/**
 * @brief 浏览器视图：一个网页标签页的主体，负责加载、进度跟踪与下载请求转交。
 *
 * 继承 WebTrapWebView 获得捕获模式能力。下载不交给 WebEngine 落盘，而是转给
 * BrowserWindow 的下载管理器；PDF 请求先询问用户是下载还是并入当前文档。
 */
class WebView : public WebTrapWebView {
    Q_OBJECT

public:
    /** @brief 构造视图，创建 WebPage 并绑定加载与下载信号。 */
    WebView(QWidget *parent = 0);
    /** @brief 返回本视图的页面对象。 */
    WebPage *webPage() const { return mPage; }

    /** @brief 加载 URL，同时记录为初始地址以便页面为空时回退显示。 */
    void load(const QUrl &url);
    /** @brief 返回当前 URL；页面尚未就绪时返回初始地址。 */
    QUrl url() const;

    /** @brief 返回当前加载进度（0-100）。 */
    inline int progress() const { return mProgress; }

protected:
    /** @brief 记录修饰键与按键后交给基类，供新标签页导航判断。 */
    void mousePressEvent(QMouseEvent *event);
    /** @brief 中键点击且事件未被接受时，用剪贴板中的 URL 导航。 */
    void mouseReleaseEvent(QMouseEvent *event);
    /** @brief 右键菜单，当前直接交给基类。 */
    void contextMenuEvent(QContextMenuEvent *event);
    /** @brief Ctrl+滚轮拦截（缩放暂未实现），其余交给基类。 */
    void wheelEvent(QWheelEvent *event);

private slots:
    /** @brief 记录加载进度。 */
    void setProgress(int progress);
    /** @brief 加载结束时打印耗时并复位进度。 */
    void loadFinished(bool ok);
    /** @brief 加载开始时启动计时器。 */
    void loadStarted();

    /** @brief 处理下载请求：PDF 先询问，其余交给下载管理器。 */
    void downloadRequested(QWebEngineDownloadRequest *item);
    /** @brief 触发“在新窗口打开链接”的页面动作。 */
    void openLinkInNewTab();

private:
    /** @brief 初始加载地址，作为 url() 的回退值。 */
    QUrl mInitialUrl;
    /** @brief 当前加载进度。 */
    int mProgress;
    /** @brief 本视图使用的页面对象。 */
    WebPage *mPage;
    /** @brief 加载计时器。 */
    QElapsedTimer mLoadStartTime;
};
