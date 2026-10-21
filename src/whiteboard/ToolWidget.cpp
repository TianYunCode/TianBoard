#include <QtWidgets>
#include <QWebEngineView>

#include "whiteboard/ToolWidget.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/BoardView.h"
#include "core/Application.h"
#include "settings/Settings.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsWidgetItem.h"
#include "platform/PlatformUtils.h"
#include "utils/FileSystemUtils.h"

// 关闭/解除固定图标被所有浮动窗口共用，只加载一次。
static QPixmap *sClosePixmap = 0;
static QPixmap *sUnpinPixmap = 0;

struct ToolWidget::Private
{
    QWebEngineView     *mWebView{nullptr};
    GraphicsWidgetItem *mToolWidget{nullptr};

    QPoint mMousePressPos;
    bool   mShouldMoveWidget{false};
    int    mContentMargin{0};
    int    mFrameWidth{0};
};

ToolWidget::ToolWidget(const QUrl &pUrl, QWidget *pParent) : QWidget(pParent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint), d(new Private)
{
    int widgetType = GraphicsWidgetItem::widgetType(pUrl);
    if (widgetType == WidgetType::Apple)
    {
        d->mToolWidget = new GraphicsAppleWidgetItem(pUrl);
    }
    else if (widgetType == WidgetType::Web)
    {
        d->mToolWidget = new GraphicsWebWidgetItem(pUrl);
    }
    else
    {
        qDebug() << "ToolWidget::ToolWidget: Unknown widget Type";
    }

    initialize();
}

ToolWidget::ToolWidget(GraphicsWidgetItem *pWidget, QWidget *pParent)
    : QWidget(pParent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint), d(new Private)
{
    d->mToolWidget = pWidget;

    initialize();
}

ToolWidget::~ToolWidget()
{
    // 网页视图与按钮都是本窗口的子对象，由 Qt 的对象树回收。
}

void ToolWidget::initialize()
{
    if (!sClosePixmap)
    {
        sClosePixmap = new QPixmap(":/images/close.svg");
    }

    if (!sUnpinPixmap)
    {
        sUnpinPixmap = new QPixmap(":/images/unpin.svg");
    }

    GraphicsScene *wscene = dynamic_cast<GraphicsScene *>(d->mToolWidget->scene());
    if (wscene)
    {
        // 对象从场景移交给了这个窗口，先取消删除标记再摘出场景，避免场景析构时误删它。
        wscene->removeItemFromDeletion(d->mToolWidget);
        wscene->removeItem(d->mToolWidget);
    }

    d->mWebView = new QWebEngineView(this);

    d->mWebView->installEventFilter(this);

    d->mFrameWidth    = Settings::settings()->objectFrameWidth;
    // 边距取半个图标宽加上边框宽度，让标题条上的按钮完整落在灰色区域内。
    d->mContentMargin = sClosePixmap->width() / 2 + d->mFrameWidth;
    setLayout(new QVBoxLayout());
    layout()->setContentsMargins(d->mContentMargin, d->mContentMargin, d->mContentMargin, d->mContentMargin);
    layout()->addWidget(d->mWebView);

    setFixedSize(d->mToolWidget->boundingRect().width() + d->mContentMargin * 2, d->mToolWidget->boundingRect().height() + d->mContentMargin * 2);

    // 小部件的 JS 桥在 Qt6 没有 addToJavaScriptWindowObject 的等价物（要换成 QWebChannel
    // 并改页面脚本），所以这里只加载页面，不再生成无人接收的 API 对象。
    d->mWebView->load(d->mToolWidget->mainHtml());

    d->mWebView->setAcceptDrops(false);
    d->mWebView->setAttribute(Qt::WA_OpaquePaintEvent, false);
}

bool ToolWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (d->mShouldMoveWidget && obj == d->mWebView && event->type() == QEvent::MouseMove)
    {
        QMouseEvent *mouseMoveEvent = static_cast<QMouseEvent *>(event);

        if (mouseMoveEvent->buttons() & Qt::LeftButton)
        {
            move(pos() - d->mMousePressPos + d->mWebView->mapTo(this, mouseMoveEvent->pos()));

            event->accept();
            return true;
        }
    }

    // 其余事件交回默认处理
    return QObject::eventFilter(obj, event);
}

void ToolWidget::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);

    // 浏览器若已消费按下事件就不能拖动窗口，否则会把网页里的点击变成拖窗。
    d->mShouldMoveWidget = !event->isAccepted() && (event->buttons() & Qt::LeftButton);
    d->mMousePressPos    = event->pos();
    event->accept();
    update();
}

void ToolWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (d->mShouldMoveWidget && (event->buttons() & Qt::LeftButton))
    {
        move(pos() - d->mMousePressPos + event->pos());
        event->accept();
    }
    QWidget::mouseMoveEvent(event);
}

void ToolWidget::mouseReleaseEvent(QMouseEvent *event)
{
    d->mShouldMoveWidget = false;

    if (event->pos().x() >= 0 && event->pos().x() < sClosePixmap->width() && event->pos().y() >= 0 && event->pos().y() < sClosePixmap->height())
    {
        Application::boardController->removeTool(this);
        event->accept();
    }
    else if (d->mToolWidget->canBeContent() && event->pos().x() >= d->mContentMargin &&
             event->pos().x() < d->mContentMargin + sUnpinPixmap->width() && event->pos().y() >= 0 && event->pos().y() < sUnpinPixmap->height())
    {
        Application::boardController->moveToolWidgetToScene(this);
        event->accept();
    }
    else
    {
        // 不要向上冒泡：松开后对象可能已被移除，父部件不再处理该事件。
        QWidget::mouseReleaseEvent(event);
    }
}

void ToolWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    // 无论窗口是否处于激活状态都绘制标题条，保证按钮在网页上方始终可见。
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(127, 127, 127, 127));

        painter.drawRoundedRect(QRectF(sClosePixmap->width() / 2, sClosePixmap->height() / 2, width() - sClosePixmap->width(), d->mFrameWidth),
            d->mFrameWidth / 2, d->mFrameWidth / 2);

        painter.drawPixmap(0, 0, *sClosePixmap);

        if (d->mToolWidget->canBeContent())
        {
            painter.drawPixmap(d->mContentMargin, 0, *sUnpinPixmap);
        }
    }
}

GraphicsWidgetItem *ToolWidget::toolWidget() const
{
    return d->mToolWidget;
}

QPoint ToolWidget::naturalCenter() const
{
    if (d->mWebView)
    {
        return d->mWebView->geometry().center();
    }
    else
    {
        return QPoint(0, 0);
    }
}

void ToolWidget::remove()
{
    d->mToolWidget = NULL;
    hide();
    deleteLater();
}

void ToolWidget::centerOn(const QPoint &pos)
{
    QWidget::move(pos - QPoint(width() / 2, height() / 2));
}
