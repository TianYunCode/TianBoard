/**
 * @file IWebController.h
 * @brief 网页控制器的对外契约：主窗口装配与录制/镜像需要的就是这几件事。
 */

#pragma once

#include <QtCore>
#include <QtGui>

class QUrl;

/**
 * @brief 网页控制器接口，实现是 web/WebController，由装配点造好装上
 *        （`Application::installWebControllerFactory`，见 app/main.cpp）。
 *
 * 详细说明：接口住在 core，因为用它的是 core 的 `Application`（生命周期）与
 * `ApplicationController`（模式切换）；web 反过来 include 本文件，于是 core 不再认识 web，
 * `core` ↔ `web` 那组环消失（拆环手法见 docs/architecture.md 第 7 步）。recording 也靠这份
 * 契约拿"当前活动网页"与镜像源，因此它不再需要 include web 的实现类。
 * activeWebPageChanged 在实现里发的是 WebView*，这里声明成 QWidget*：_emit_ 端隐式向上转型
 * 即可，接收端只需要一个能当镜像源/捕获源的窗口。
 * 线程约束：只在 GUI 线程使用。
 */
class IWebController : public QObject
{
    Q_OBJECT

public:
    explicit IWebController(QObject *parent = nullptr) : QObject(parent)
    {
    }
    ~IWebController() override                   = default;

    /// 进入网页模式（实现里懒创建浏览器窗口）。
    virtual void show()                          = 0;

    /// 打开一个 URL：外部浏览器模式下交给系统浏览器，否则在新标签页打开。
    virtual void loadUrl(const QUrl &url)        = 0;

    /// 按窗口宽度调整网页工具栏动作的显隐。
    virtual void adaptToolBar()                  = 0;

    /// 调整标签栏在顶部还是底部。
    virtual void showTabAtTop(bool topAtTop)     = 0;

    /// 对当前网页执行剪切。
    virtual void cut()                           = 0;
    /// 对当前网页执行复制。
    virtual void copy()                          = 0;
    /// 对当前网页执行粘贴。
    virtual void paste()                         = 0;

    /// 网页模式的中央控件，供屏幕镜像与录制选捕获源。
    virtual QWidget *controlView()               = 0;

    /// 退出前按白名单清 Cookie（"离开时清除"开着时才动手）。
    virtual void closing()                       = 0;

    /// 按 Web/CookiePolicy 装 Cookie 过滤器并维护镜像罐；重复调用只换过滤器。
    virtual void installCookiePolicy()           = 0;

    /// 按当前设置统计退出时会被清掉的 Cookie 条数，供设置页显示预期。
    virtual int cookiesMarkedForDeletion() const = 0;

signals:
    /// 完成一次屏幕/网页元素捕获；pageMode 表示是否整页，source 是来源 URL。
    void imageCaptured(const QPixmap &pixmap, bool pageMode, const QUrl &source);

    /// 当前活动网页变化：镜像视图与录制捕获源要跟着换。
    void activeWebPageChanged(QWidget *webView);

public slots:
    /// 显示器布局变化时更新"在副屏显示"动作的可见性。
    virtual void screenLayoutChanged() = 0;
};
