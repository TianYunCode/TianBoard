#include "web/WebTrapWebView.h"

#include <QtWidgets>
#include <QtWebEngineWidgets>
#include <QWebEnginePage>

#include "web/WebKitUtils.h"

#include "core/Application.h"
#include "core/ApplicationController.h"
#include "whiteboard/BoardController.h"

struct WebTrapWebView::Private
{
    QRect                          mDomElementRect;
    QRect                          mWebViewElementRect;
    QString                        mElementQuery;
    WebTrapWebView::WebContentType mCurrentContentType{WebTrapWebView::Unknown};

    bool mIsTrapping{false};

    QWidget *mTrapingWidget{nullptr};
};

struct WebTrapMouseEventMask::Private
{
    WebTrapWebView *mTrappedWebView{nullptr};
};

WebTrapWebView::WebTrapWebView(QWidget *parent) : QWebEngineView(parent), d(new Private)
{
    connect(this, &QWebEngineView::loadFinished, this, &WebTrapWebView::viewLoadFinished);
}

WebTrapWebView::~WebTrapWebView()
{
    delete d->mTrapingWidget;
}

bool WebTrapWebView::isTrapping()
{
    return d->mIsTrapping;
}

void WebTrapWebView::setIsTrapping(bool pIsTrapping)
{
    d->mIsTrapping         = pIsTrapping;

    d->mDomElementRect     = QRect();
    d->mCurrentContentType = Unknown;
    // 抓取状态只体现在本控件自身的重绘上，不再额外创建置顶的鼠标遮罩窗口，
    // 因为 QWebEngineView 的合成窗口会盖在普通 QWidget 之上，遮罩拿不到鼠标事件。

    update();
}

void WebTrapWebView::hideEvent(QHideEvent *event)
{
    setIsTrapping(false);
    QWebEngineView::hideEvent(event);
}

void WebTrapWebView::highliteElementAtPos(const QPoint &pos)
{
    d->mCurrentContentType = Unknown;

    if (page() && page()->webChannel())
    {
        // 这里原本依赖 QWebFrame 的同步命中测试（hitTestContent）拿到元素包围盒，
        // 再据此生成 id 选择器或 nth-child 路径选择器。QWebEngine 已删除 QWebFrame，
        // 命中测试只能异步走 JavaScript，故本函数暂只把内容类型复位为 Unknown：
        // mDomElementRect 保持无效，paintEvent 的高亮分支因此不会绘制。
    }
}

QString WebTrapWebView::potentialEmbedCodeAtPos(const QPoint &pos)
{
    QString scr     = QString("document.elementFromPoint(%1, %2).tagName").arg(pos.x()).arg(pos.y());

    // QWebEngine 的 runJavaScript 是异步回调，拿不到同步返回值，tagName 只能留空，
    // 于是下面的分支恒不成立、函数恒返回空串。scr 与 valueScript 是原本要执行的脚本，
    // 保留在此便于日后改成异步回调实现。
    QString tagName = "";

    if ((tagName == "input") || (tagName == "textarea"))
    {
        QString valueScript;

        if (tagName == "input")
        {
            valueScript = QString("document.elementFromPoint(%1, %2).value").arg(pos.x()).arg(pos.y());
        }
        else if (tagName == "textarea")
        {
            valueScript = QString("document.elementFromPoint(%1, %2).innerHTML").arg(pos.x()).arg(pos.y());
        }

        QString value = "";

        if (value.length() > 0 && value.contains("width") && value.contains("height"))
        {
            return value;
        }
    }

    return "";
}

void WebTrapWebView::trapElementAtPos(const QPoint &pos)
{
    // 抓取逻辑整体依赖 QWebFrame 的同步命中测试与 evaluateJavaScript 同步取值，
    // 迁到 QWebEngine 后已无对应 API：本函数现为空实现，pos 不再被使用，鼠标释放时
    // 不再产生任何 pixmapCaptured / objectCaptured / embedCodeCaptured /
    // webElementCaptured 信号。
}

void WebTrapWebView::mouseMoveEvent(QMouseEvent *event)
{
    if (d->mIsTrapping)
    {
        highliteElementAtPos(event->pos());
    }
    else
    {
        QWebEngineView::mouseMoveEvent(event);
    }
}

void WebTrapWebView::mousePressEvent(QMouseEvent *event)
{
    if (d->mIsTrapping)
    {
        // 空实现：抓取态下按下事件必须就地吞掉，只调 accept() 不足以阻止它被转发给网页。
    }
    else
    {
        // 空实现：非抓取态同样不向基类转发，两个分支都为空，按下事件在本层被全部吞掉。
    }
}

void WebTrapWebView::mouseReleaseEvent(QMouseEvent *event)
{
    if (d->mIsTrapping)
    {
        trapElementAtPos(event->pos());
    }
    else
    {
        QWebEngineView::mouseReleaseEvent(event);
    }
}

void WebTrapWebView::paintEvent(QPaintEvent *event)
{
    QWebEngineView::paintEvent(event);

    if (d->mIsTrapping && d->mDomElementRect.isValid() && page())
    {
        QPainter p(this);

        QPen pen;

        if (d->mCurrentContentType != Unknown)
        {
            pen = QPen(QColor(0, 255, 0, 127));
        }
        else
        {
            pen = QPen(QColor(255, 0, 0, 127));
        }

        pen.setJoinStyle(Qt::RoundJoin);
        pen.setWidth(8);
        p.setPen(pen);

        // QWebEngine 没有帧滚动位置可查，只能按未滚动处理；命中框因此只在页面顶部
        // 滚动位置为 0 时才与元素真正对齐。
        QPoint scrollPosition;
        QRect  webViewPosition = d->mDomElementRect;
        webViewPosition.translate(-scrollPosition);

        d->mWebViewElementRect = webViewPosition.adjusted(-4, -4, 4, 4);

        p.drawRect(d->mWebViewElementRect);
    }
}

void WebTrapWebView::viewLoadFinished(bool ok)
{
    Q_UNUSED(ok);
}

WebTrapMouseEventMask::WebTrapMouseEventMask(WebTrapWebView *pWebView) : d(new Private)
{
    d->mTrappedWebView = pWebView;
    setMouseTracking(true);
}

WebTrapMouseEventMask::~WebTrapMouseEventMask()
{
    // 空实现：本控件没有自持资源，子对象由 Qt 父子关系回收。
}

void WebTrapMouseEventMask::mousePressEvent(QMouseEvent *event)
{
    if (d->mTrappedWebView && d->mTrappedWebView->isTrapping())
    {
        // 空实现：抓取态下按下事件就地吞掉，不让它落到被盖住的网页上。
    }
    else
    {
        QWidget::mousePressEvent(event);
    }
}

void WebTrapMouseEventMask::mouseMoveEvent(QMouseEvent *event)
{
    if (d->mTrappedWebView && d->mTrappedWebView->isTrapping())
    {
        d->mTrappedWebView->highliteElementAtPos(event->pos());
    }
    else
    {
        QWidget::mouseMoveEvent(event);
    }
}

void WebTrapMouseEventMask::mouseReleaseEvent(QMouseEvent *event)
{
    if (d->mTrappedWebView && d->mTrappedWebView->isTrapping())
    {
        d->mTrappedWebView->trapElementAtPos(event->pos());
    }
    else
    {
        QWidget::mouseReleaseEvent(event);
    }
}

void WebTrapMouseEventMask::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(127, 127, 127, 15));

    painter.drawRect(0, 0, width(), height());
}
