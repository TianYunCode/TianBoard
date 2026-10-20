/**
 * @file WebTrapWebView.h
 * @brief 声明 WebTrapWebView：带“网页元素捕获”模式的 WebEngine 视图及其鼠标事件遮罩。
 */

#pragma once

#include <QtWidgets>
#include <QtWebEngineWidgets>
#include <QWebEngineView>

#include <memory>

/**
 * @brief 可捕获网页元素的 WebEngine 视图：捕获模式下高亮并拾取鼠标下的内容。
 *
 * 是 WebView 的基类，只负责捕获模式与元素高亮绘制；页面加载、下载等由子类处理。
 * 捕获模式的命中测试尚未在当前 Qt6 移植中恢复，因此 highliteElementAtPos 只会重置状态，
 * trapElementAtPos 为空实现；WebController 也据此禁用了捕获动作。
 */
class WebTrapWebView : public QWebEngineView
{
    Q_OBJECT;

public:
    /** @brief 构造视图并监听页面加载结束。 */
    WebTrapWebView(QWidget *parent = 0);
    /** @brief 析构：释放捕获模式下的事件遮罩。 */
    virtual ~WebTrapWebView();

    /**
     * @brief 开启/关闭捕获模式。
     * @note 切换时会清空已记录的 DOM 矩形并重绘。
     */
    void setIsTrapping(bool pIsTrapping);

    /** @brief 返回是否处于捕获模式。 */
    bool isTrapping();

    /**
     * @brief 高亮鼠标位置的网页元素。
     * @note 当前实现只重置内容类型，不执行命中测试。
     */
    void highliteElementAtPos(const QPoint &pos);

    /**
     * @brief 捕获鼠标位置的网页元素。
     * @note 当前为空实现，保留接口供后续恢复。
     */
    void trapElementAtPos(const QPoint &pos);

signals:
    /**
     * @brief 捕获到网页中的图片位图。
     * @param pixmap 位图。
     * @param pageMode 是否整页截图。
     */
    void pixmapCaptured(const QPixmap &pixmap, bool pageMode);
    /**
     * @brief 捕获到 object/embed 对象。
     * @param pUrl 对象源地址。
     * @param pMimeType 对象类型。
     * @param pWidth 对象宽度。
     * @param pHeight 对象高度。
     */
    void objectCaptured(const QUrl &pUrl, const QString &pMimeType, int pWidth, int pHeight);
    /** @brief 捕获到可嵌入代码片段（如输入框中的 embed 代码）。 */
    void embedCodeCaptured(const QString &embedCode);

    /**
     * @brief 捕获到由 CSS 选择器定位的网页元素。
     * @param pUrl 页面地址。
     * @param query 元素选择器。
     */
    void webElementCaptured(const QUrl &pUrl, const QString &query);

private slots:
    /** @brief 页面加载结束槽，当前无操作。 */
    void viewLoadFinished(bool ok);

protected:
    /** @brief 捕获模式下吞掉按下事件，避免传给网页。 */
    virtual void mousePressEvent(QMouseEvent *event);
    /** @brief 捕获模式下高亮鼠标下的元素。 */
    virtual void mouseMoveEvent(QMouseEvent *event);
    /** @brief 捕获模式下尝试捕获元素。 */
    virtual void mouseReleaseEvent(QMouseEvent *event);
    /** @brief 视图隐藏时自动退出捕获模式。 */
    virtual void hideEvent(QHideEvent *event);

    /** @brief 先让 WebEngine 绘制页面，再叠加元素高亮框。 */
    virtual void paintEvent(QPaintEvent *event);

    /** @brief 尝试读取鼠标位置输入框中的可嵌入代码。 */
    QString potentialEmbedCodeAtPos(const QPoint &pos);

    /** @brief 捕获内容的类型分类。 */
    enum WebContentType
    {
        Unknown = 0,   ///< 未知
        Image,         ///< 图片
        Input,         ///< 输入框中的代码
        ObjectOrEmbed, ///< object/embed 对象
        ElementByQuery ///< 通过选择器定位的元素
    };

private:
    /** @brief 私有实现。 */
    struct Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief 捕获模式下的透明遮罩窗口：替 WebEngine 接收鼠标事件并转发给目标视图。
 *
 * 当前捕获模式的遮罩创建逻辑尚未启用，本类仅在后续恢复捕获时使用。
 */
class WebTrapMouseEventMask : public QWidget
{
public:
    /**
     * @brief 构造遮罩并绑定目标视图。
     */
    WebTrapMouseEventMask(WebTrapWebView *pWebView);
    /** @brief 析构。 */
    virtual ~WebTrapMouseEventMask();

protected:
    /** @brief 捕获模式下吞掉按下事件。 */
    virtual void mousePressEvent(QMouseEvent *event);
    /** @brief 捕获模式下转发移动事件以高亮元素。 */
    virtual void mouseMoveEvent(QMouseEvent *event);
    /** @brief 捕获模式下转发释放事件以捕获元素。 */
    virtual void mouseReleaseEvent(QMouseEvent *event);
    /** @brief 绘制半透明遮罩。 */
    virtual void paintEvent(QPaintEvent *);

private:
    /** @brief 私有实现。 */
    struct Private;
    std::unique_ptr<Private> d;
};
