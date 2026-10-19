#include <QtWidgets>
#include <QWebEngineView>
#include <QtWebChannel/QWebChannel>

#include "ui/ToolWidget.h"
#include "api/WebWidgetBoardAPI.h"
#include "api/WebWidgetAPI.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/BoardView.h"
#include "core/Application.h"
#include "core/Settings.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsWidgetItem.h"
#include "platform/PlatformUtils.h"
#include "utils/FileSystemUtils.h"


static QPixmap* sClosePixmap = 0;
static QPixmap* sUnpinPixmap = 0;

struct ToolWidget::Private
{
    QWebEngineView *mWebView{nullptr};
    GraphicsWidgetItem *mToolWidget{nullptr};

    QPoint mMousePressPos;
    bool mShouldMoveWidget{false};
    int mContentMargin{0};
    int mFrameWidth{0};
};


ToolWidget::ToolWidget(const QUrl& pUrl, QWidget *pParent)
    : QWidget(pParent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    , d(new Private)
{
    int widgetType = GraphicsWidgetItem::widgetType(pUrl);
    if (widgetType == WidgetType::Apple)
        d->mToolWidget = new GraphicsAppleWidgetItem(pUrl);
    else if (widgetType == WidgetType::Web)
        d->mToolWidget = new GraphicsWebWidgetItem(pUrl);
    else
        qDebug() << "ToolWidget::ToolWidget: Unknown widget Type";

    initialize();
}

ToolWidget::ToolWidget(GraphicsWidgetItem *pWidget, QWidget *pParent)
    : QWidget(pParent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    , d(new Private)
{
    d->mToolWidget = pWidget;

    initialize();
    javaScriptWindowObjectCleared();
}

ToolWidget::~ToolWidget()
{
    // NOOP
}

void ToolWidget::initialize()
{
    if (!sClosePixmap)
        sClosePixmap = new QPixmap(":/images/close.svg");

    if(!sUnpinPixmap)
        sUnpinPixmap = new QPixmap(":/images/unpin.svg");

    GraphicsScene *wscene = dynamic_cast<GraphicsScene *>(d->mToolWidget->scene());
    if (wscene)
    {
        wscene->removeItemFromDeletion(d->mToolWidget);
        wscene->removeItem(d->mToolWidget);
    }


    d->mWebView = new QWebEngineView(this);

    //QPalette palette = mWebView->page()->palette();
    //palette.setBrush(QPalette::Base, QBrush(Qt::transparent));
    //mWebView->page()->setPalette(palette);


    d->mWebView->installEventFilter(this);

    d->mFrameWidth = Settings::settings()->objectFrameWidth;
    d->mContentMargin = sClosePixmap->width() / 2 + d->mFrameWidth;
    setLayout(new QVBoxLayout());
    layout()->setContentsMargins(d->mContentMargin, d->mContentMargin, d->mContentMargin, d->mContentMargin);
    layout()->addWidget(d->mWebView);

    setFixedSize(d->mToolWidget->boundingRect().width() + d->mContentMargin * 2, d->mToolWidget->boundingRect().height() + d->mContentMargin * 2);

    //connect(mWebView->page()-> mainFrame(), &QWebChannel::javaScriptWindowObjectCleared,
    d->mWebView->load(d->mToolWidget->mainHtml());


    d->mWebView->setAcceptDrops(false);
    //mWebView->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    d->mWebView->setAttribute(Qt::WA_OpaquePaintEvent, false);


    connect(Application::boardController, SIGNAL(activeSceneChanged()), this, SLOT(javaScriptWindowObjectCleared()));
}


bool ToolWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (d->mShouldMoveWidget && obj == d->mWebView && event->type() == QEvent::MouseMove)
    {
        QMouseEvent *mouseMoveEvent = static_cast<QMouseEvent*>(event);

        if (mouseMoveEvent->buttons() & Qt::LeftButton)
        {
            move(pos() - d->mMousePressPos + d->mWebView->mapTo(this, mouseMoveEvent->pos()));

            event->accept();
            return true;
        }
    }

    // standard event processing
    return QObject::eventFilter(obj, event);
}

void ToolWidget::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);

    /* did webkit consume the mouse press ? */
    d->mShouldMoveWidget = !event->isAccepted() && (event->buttons() & Qt::LeftButton);
    d->mMousePressPos = event->pos();
    event->accept();
    update();
}

void ToolWidget::mouseMoveEvent(QMouseEvent *event)
{
    if(d->mShouldMoveWidget && (event->buttons() & Qt::LeftButton)) {
        move(pos() - d->mMousePressPos + event->pos());
        event->accept();
    }
    QWidget::mouseMoveEvent(event);
}

void ToolWidget::mouseReleaseEvent(QMouseEvent *event)
{
    d->mShouldMoveWidget = false;

    if (event->pos().x() >= 0 && event->pos().x() < sClosePixmap->width() && event->pos().y() >= 0 && event->pos().y() < sClosePixmap->height()) {
        Application::boardController->removeTool(this);
        event->accept();
    }
    else if (d->mToolWidget->canBeContent() && event->pos().x() >= d->mContentMargin && event->pos().x() < d->mContentMargin + sUnpinPixmap->width() && event->pos().y() >= 0 && event->pos().y() < sUnpinPixmap->height()) {
        Application::boardController->moveToolWidgetToScene(this);
        event->accept();
    }
    else
        QWidget::mouseReleaseEvent(event); /* don't propgate to parent, the widget is deleted in Application */

}

void ToolWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    //if (isActiveWindow())
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(127, 127, 127, 127));

        painter.drawRoundedRect(QRectF(sClosePixmap->width() / 2
                                     , sClosePixmap->height() / 2
                                     , width() - sClosePixmap->width()
                                     , d->mFrameWidth)
                                     , d->mFrameWidth / 2
                                     , d->mFrameWidth / 2);

        painter.drawPixmap(0, 0, *sClosePixmap);

        if (d->mToolWidget->canBeContent())
            painter.drawPixmap(d->mContentMargin, 0, *sUnpinPixmap);
    }
}

void ToolWidget::javaScriptWindowObjectCleared()
{
    WebWidgetBoardAPI *boardAPI = new WebWidgetBoardAPI(Application::boardController->activeScene(), d->mToolWidget);
	

    //GraphicsWebWidgetItem *graphicsWebWidgetItem = dynamic_cast<GraphicsWebWidgetItem*>(mToolWidget);
    //if (graphicsWebWidgetItem)
    //{
    //    WebWidgetAPI* widgetAPI = new WebWidgetAPI(graphicsWebWidgetItem);
    //    mWebView->page()->mainFrame()->addToJavaScriptWindowObject("widget", widgetAPI);
    //}
}

GraphicsWidgetItem* ToolWidget::toolWidget() const
{
    return d->mToolWidget;
}

QPoint ToolWidget::naturalCenter() const
{
    if (d->mWebView)
        return d->mWebView->geometry().center();
    else
        return QPoint(0, 0);
}

void ToolWidget::remove()
{
    d->mToolWidget = NULL;
    hide();
    deleteLater();
}

void ToolWidget::centerOn(const QPoint& pos)
{
    QWidget::move(pos - QPoint(width() / 2, height() / 2));
}
