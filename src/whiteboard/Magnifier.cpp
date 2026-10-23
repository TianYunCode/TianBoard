#include <QtWidgets>
#include "whiteboard/Magnifier.h"

#include "whiteboard/BoardController.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/BoardView.h"

struct Magnifier::Private
{
    MagnifierParams params;

    QPoint mMousePressPos;
    qreal  mMousePressDelta{0};
    bool   mShouldMoveWidget{false};
    bool   mShouldResizeWidget{false};

    int      m_iButtonInterval{0};
    QPixmap *sClosePixmap{nullptr};
    QRect    sClosePixmapButtonRect;
    QPixmap *sIncreasePixmap{nullptr};
    QRect    sIncreasePixmapButtonRect;
    QPixmap *sDecreasePixmap{nullptr};
    QRect    sDecreasePixmapButtonRect;
    QPixmap *sChangeModePixmap{nullptr};
    QRect    sChangeModePixmapButtonRect;
    QPixmap *mResizeItem{nullptr};
    QRect    mResizeItemButtonRect;

    bool    isCusrsorAlreadyStored{false};
    QCursor mOldCursor;
    QCursor mResizeCursor;

    Magnifier::DrawingMode mDrawingMode{Magnifier::circular};

    QTimer mRefreshTimer;
    bool   m_isInteractive{false};

    QPoint updPointGrab;
    QPoint updPointMove;

    QPixmap pMap;
    QBitmap bmpMask;
    QPen    borderPen{Qt::darkGray};

    QWidget *gView{nullptr};
    QWidget *mView{nullptr};
};

Magnifier::Magnifier(QWidget *parent, bool isInteractive)
    : QWidget(parent, parent ? Qt::Widget : Qt::Tool | (Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint)),
      d(new Private)
{
    d->isCusrsorAlreadyStored = false;
    setMouseTracking(true);

    //--------------------------------------------------//

    // 用旋转过的 resize 图标拼出斜向缩放手柄光标，热点放在图片中心。
    QPixmap    pix(":/images/cursors/resize.png");
    QTransform tr;
    tr.rotate(45);
    d->mResizeCursor               = QCursor(pix.transformed(tr, Qt::SmoothTransformation), pix.width() / 2, pix.height() / 2);

    //--------------------------------------------------//

    d->params.sizePercentFromScene = 20;
    d->m_isInteractive             = isInteractive;
    d->sClosePixmap                = new QPixmap(":/images/close.svg");
    d->sIncreasePixmap             = new QPixmap(":/images/increase.svg");
    d->sDecreasePixmap             = new QPixmap(":/images/decrease.svg");
    d->mResizeItem                 = new QPixmap(":/images/resize.svg");
    d->sChangeModePixmap           = new QPixmap();

    qDebug() << "sClosePixmap" << d->sClosePixmap->size() << Qt::endl
             << "sIncreasePixmap" << d->sIncreasePixmap->size() << Qt::endl
             << "sDecreasePixmap" << d->sDecreasePixmap->size() << Qt::endl
             << "mResizeItem" << d->mResizeItem->size() << Qt::endl;

    setDrawingMode(Settings::settings()->magnifierDrawingMode->get().toInt());

    if (parent)
    {
        setAttribute(Qt::WA_NoMousePropagation);
    }

    connect(&d->mRefreshTimer, &QTimer::timeout, this, &Magnifier::slot_refresh);
}

Magnifier::~Magnifier()
{
    if (d->sClosePixmap)
    {
        delete d->sClosePixmap;
        d->sClosePixmap = NULL;
    }

    if (d->sIncreasePixmap)
    {
        delete d->sIncreasePixmap;
        d->sIncreasePixmap = NULL;
    }

    if (d->sDecreasePixmap)
    {
        delete d->sDecreasePixmap;
        d->sDecreasePixmap = NULL;
    }

    if (d->sChangeModePixmap)
    {
        delete d->sChangeModePixmap;
        d->sChangeModePixmap = NULL;
    }
}

void Magnifier::setSize(qreal percentFromScene)
{
    if (d->gView == NULL || d->mView == NULL)
    {
        return;
    }

    // 百分比是相对画布宽度定义的，换算出镜面在当前视图下的像素尺寸。
    d->params.sizePercentFromScene = percentFromScene;
    QSize sceneSize                = d->mView->size();
    qreal size                     = d->params.sizePercentFromScene * sceneSize.width() / 100;

    QRect currGeom                 = geometry();
    if (circular == d->mDrawingMode)
    {
        if (currGeom.width() == currGeom.height())
        {
            QPoint newPos = d->mView->mapFromGlobal(d->updPointMove);
            setGeometry(newPos.x() - size / 2, newPos.y() - size / 2, size, size);
        }
        else
        {
            setGeometry(0, 0, size, size);
        }
    }
    else if (rectangular == d->mDrawingMode)
    {
        QPoint newPos = d->mView->mapFromGlobal(d->updPointMove);
        setGeometry(newPos.x() - size / 2, newPos.y() - size / 2 / 3, size, size / 3);
    }

    calculateButtonsPositions();
    createMask();
}

void Magnifier::createMask()
{
    if (d->gView == NULL || d->mView == NULL)
    {
        return;
    }

    // 掩码按画布百分比换算出的尺寸绘制，边缘留出一个按钮图标的宽度。
    QSize sceneSize = d->mView->size();
    qreal isize     = d->params.sizePercentFromScene * sceneSize.width() / 100;

    QImage mask_img(width(), height(), QImage::Format_Mono);
    mask_img.fill(0xff);
    QPainter mask_ptr(&mask_img);
    mask_ptr.setBrush(QBrush(QColor(0, 0, 0)));

    if (circular == d->mDrawingMode)
    {
        mask_ptr.drawEllipse(QPointF(isize / 2, isize / 2), isize / 2 - d->sClosePixmap->width(), isize / 2 - d->sClosePixmap->width());
    }
    else if (rectangular == d->mDrawingMode)
    {
        mask_ptr.drawRoundedRect(QRect(d->sClosePixmap->width(), d->sClosePixmap->width(), size().width() - 2 * d->sClosePixmap->width(),
                                     size().height() - 2 * d->sClosePixmap->width()),
            d->sClosePixmap->width() / 2, d->sClosePixmap->width() / 2);
    }

    d->bmpMask = QBitmap::fromImage(mask_img);

    d->pMap    = QPixmap(width(), height());
    d->pMap.fill(Qt::transparent);
    d->pMap.setMask(d->bmpMask);
}

void Magnifier::setZoom(qreal zoom)
{
    d->params.zoom = zoom;
}

qreal Magnifier::zoom() const
{
    return d->params.zoom;
}

void Magnifier::calculateButtonsPositions()
{
    qDebug() << "放大镜当前控件尺寸:" << size();

    d->m_iButtonInterval           = 5;
    d->mResizeItemButtonRect       = QRect(size().width() - 1.5 * d->mResizeItem->width() - d->m_iButtonInterval,
        size().height() - 1.5 * d->mResizeItem->height() - d->m_iButtonInterval, d->mResizeItem->width(), d->mResizeItem->height());
    d->sClosePixmapButtonRect      = QRect(d->mResizeItemButtonRect.x() - d->sChangeModePixmap->width() - 3 * d->m_iButtonInterval,
        size().height() - d->sChangeModePixmap->height(), d->sChangeModePixmap->width(), d->sChangeModePixmap->height());
    d->sChangeModePixmapButtonRect = QRect(d->sClosePixmapButtonRect.x() - d->sChangeModePixmap->width() - d->m_iButtonInterval,
        size().height() - d->sChangeModePixmap->height(), d->sChangeModePixmap->width(), d->sChangeModePixmap->height());
    d->sDecreasePixmapButtonRect   = QRect(d->sChangeModePixmapButtonRect.x() - d->sChangeModePixmap->width() - d->m_iButtonInterval,
        size().height() - d->sDecreasePixmap->height(), d->sChangeModePixmap->width(), d->sDecreasePixmap->height());
    d->sIncreasePixmapButtonRect   = QRect(d->sDecreasePixmapButtonRect.x() - d->sChangeModePixmap->width() - d->m_iButtonInterval,
        size().height() - d->sIncreasePixmap->height(), d->sIncreasePixmap->width(), d->sIncreasePixmap->height());

    qDebug() << "mResizeItemButtonRect" << d->mResizeItemButtonRect << Qt::endl
             << "sClosePixmapButtonRect" << d->sClosePixmapButtonRect << Qt::endl
             << "sChangeModePixmapButtonRect" << d->sChangeModePixmapButtonRect << Qt::endl
             << "sDecreasePixmapButtonRect" << d->sDecreasePixmapButtonRect << Qt::endl
             << "sIncreasePixmapButtonRect" << d->sIncreasePixmapButtonRect << Qt::endl;
}

void Magnifier::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    if (d->m_isInteractive)
    {
        painter.setBrush(QColor(127, 127, 127, 127));
        painter.drawRoundedRect(QRectF(size().width() / 2, size().height() / 2, (size().width() - d->sClosePixmap->width()) / 2,
                                    (size().height() - d->sClosePixmap->width()) / 2),
            15, 15);
    }

    painter.setBrush(QColor(190, 190, 190, 255));
    if (circular == d->mDrawingMode)
    {
        painter.drawEllipse(QPoint(size().width() / 2, size().height() / 2), (size().width() - d->sClosePixmap->width()) / 2,
            (size().height() - d->sClosePixmap->height()) / 2);
    }
    else if (rectangular == d->mDrawingMode)
    {
        QRect r = QRect(d->sClosePixmap->width() / 2, d->sClosePixmap->width() / 2, size().width() - d->sClosePixmap->width(),
            size().height() - d->sClosePixmap->width());
        painter.drawRoundedRect(r, d->sClosePixmap->width() / 2, d->sClosePixmap->width() / 2);
    }

    painter.drawPixmap(0, 0, d->pMap);

    if (d->m_isInteractive)
    {
        painter.setBrush(QColor(190, 190, 190, 255));
        painter.drawPixmap(d->sClosePixmapButtonRect.topLeft(), *d->sClosePixmap);
        painter.drawPixmap(d->sIncreasePixmapButtonRect.topLeft(), *d->sIncreasePixmap);
        painter.drawPixmap(d->sDecreasePixmapButtonRect.topLeft(), *d->sDecreasePixmap);
        painter.drawPixmap(d->sChangeModePixmapButtonRect.topLeft(), *d->sChangeModePixmap);
        painter.drawPixmap(d->mResizeItemButtonRect.topLeft(), *d->mResizeItem);
    }
}

void Magnifier::mousePressEvent(QMouseEvent *event)
{
    if (d->m_isInteractive)
    {
        QWidget::mousePressEvent(event);

        if (event->pos().x() >= size().width() - d->mResizeItem->width() - 14 && event->pos().x() < size().width() - 14 &&
            event->pos().y() >= size().height() - d->mResizeItem->height() - 14 && event->pos().y() < size().height() - -14)
        {
            d->mShouldResizeWidget = true;
        }
        else
        {
            d->mShouldMoveWidget = !event->isAccepted() && (event->buttons() & Qt::LeftButton);
        }

        d->mMousePressPos   = event->pos();
        d->mMousePressDelta = (qreal)d->updPointGrab.x() + (qreal)size().width() / 2 - (qreal)event->globalPosition().toPoint().x();

        event->accept();

        update();
    }
    else
    {
        event->ignore();
    }
}

void Magnifier::mouseMoveEvent(QMouseEvent *event)
{
    if (d->m_isInteractive)
    {
        if (d->mShouldMoveWidget && (event->buttons() & Qt::LeftButton))
        {
            move(pos() - d->mMousePressPos + event->pos());
            event->accept();

            QWidget::mouseMoveEvent(event);
            emit magnifierMoved_Signal(QPoint(this->pos().x() + size().width() / 2, this->pos().y() + size().height() / 2));
            return;
        }

        if (d->mShouldResizeWidget && (event->buttons() & Qt::LeftButton))
        {
            QPoint currGlobalPos = event->globalPosition().toPoint();
            qreal  cvW           = d->mView->width();

            qreal newXSize       = (currGlobalPos.x() + d->mMousePressDelta - d->updPointGrab.x()) * 2;
            qreal newPercentSize = newXSize * 100 / cvW;

            emit magnifierResized_Signal(newPercentSize);

            event->ignore();
            return;
        }

        if (d->mResizeItemButtonRect.contains(event->pos()) && d->isCusrsorAlreadyStored == false)
        {
            d->mOldCursor             = cursor();
            d->isCusrsorAlreadyStored = true;
            setCursor(d->mResizeCursor);
        }
    }
    else
    {
        event->ignore();
    }
}

void Magnifier::mouseReleaseEvent(QMouseEvent *event)
{
    if (d->m_isInteractive)
    {
        d->mShouldMoveWidget   = false;
        d->mShouldResizeWidget = false;

        if (d->sClosePixmapButtonRect.contains(event->pos()))
        {
            event->accept();
            emit magnifierClose_Signal();
        }
        else if (d->sIncreasePixmapButtonRect.contains(event->pos()))
        {
            event->accept();
            emit magnifierZoomIn_Signal();
        }
        else if (d->sDecreasePixmapButtonRect.contains(event->pos()))
        {
            event->accept();
            emit magnifierZoomOut_Signal();
        }
        else if (d->sChangeModePixmapButtonRect.contains(event->pos()))
        {
            event->accept();
            emit magnifierDrawingModeChange_Signal(static_cast<int>(d->mDrawingMode + 1) % modesCount);
        }
        else
        {
            // 不要向上冒泡：松开事件之后场景会通过 removeTool 删掉这个部件。
            QWidget::mouseReleaseEvent(event);
        }
    }
    else
    {
        event->ignore();
    }
}

void Magnifier::slot_refresh()
{
    if (!(d->updPointGrab.isNull()))
    {
        grabPoint(d->updPointGrab);
    }

    if (d->isCusrsorAlreadyStored)
    {
        QPoint globalCursorPos = QCursor::pos();
        QPoint cursorPos       = mapFromGlobal(globalCursorPos);
        if (cursorPos.x() < size().width() - d->mResizeItem->width() - 14 || cursorPos.x() > size().width() - 14 ||
            cursorPos.y() < size().height() - d->mResizeItem->height() - 14 || cursorPos.y() > size().height() - -14)
        {
            d->isCusrsorAlreadyStored = false;
            setCursor(d->mOldCursor);
        }
    }
}

void Magnifier::grabPoint()
{
    QTransform transM  = BoardController::instance()->controlView()->transform();
    QPointF    itemPos = d->gView->mapFromGlobal(d->updPointGrab);

    qreal zWidth       = width() / (d->params.zoom * transM.m11());
    qreal zWidthHalf   = zWidth / 2;
    qreal zHeight      = height() / (d->params.zoom * transM.m22());
    qreal zHeightHalf  = zHeight / 2;

    QPointF pfScLtF(BoardController::instance()->controlView()->mapToScene(QPoint(itemPos.x(), itemPos.y())));

    float x = pfScLtF.x() - zWidthHalf;
    float y = pfScLtF.y() - zHeightHalf;

    QPointF leftTop(x, y);
    QPointF rightBottom(x + zWidth, y + zHeight);
    QRectF  srcRect(leftTop, rightBottom);

    QPixmap  newPixMap(QSize(width(), height()));
    QPainter painter(&newPixMap);

    BoardController::instance()->activeScene()->render(&painter, QRectF(0, 0, width(), height()), srcRect);
    painter.end();

    d->pMap.fill(Qt::transparent);
    d->pMap = newPixMap.scaled(QSize(width(), height()));
    d->pMap.setMask(d->bmpMask);

    update();
}

void Magnifier::grabPoint(const QPoint &pGrab)
{
    QTransform transM = BoardController::instance()->controlView()->transform();
    d->updPointGrab   = pGrab;
    QPointF itemPos   = d->gView->mapFromGlobal(pGrab);

    // 视图缩放会同时影响取景范围，所以源矩形要除以“倍率 × 视图缩放”。
    qreal zWidth      = width() / (d->params.zoom * transM.m11());
    qreal zWidthHalf  = zWidth / 2;
    qreal zHeight     = height() / (d->params.zoom * transM.m22());
    qreal zHeightHalf = zHeight / 2;

    // 把镜面中心映射到场景坐标，再向四周扩出源矩形。
    QPointF pfScLtF(BoardController::instance()->controlView()->mapToScene(QPoint(itemPos.x(), itemPos.y())));

    float x = pfScLtF.x() - zWidthHalf;
    float y = pfScLtF.y() - zHeightHalf;

    QPointF leftTop(x, y);
    QPointF rightBottom(x + zWidth, y + zHeight);
    QRectF  srcRect(leftTop, rightBottom);

    QPixmap  newPixMap(QSize(width(), height()));
    QPainter painter(&newPixMap);

    // 只渲染源矩形，并把它铺满整块镜面画布，得到放大的效果。
    BoardController::instance()->activeScene()->render(&painter, QRectF(0, 0, width(), height()), srcRect);
    painter.end();

    d->pMap = newPixMap;
    d->pMap.setMask(d->bmpMask);

    update();
}

// 参数均为全局坐标：先抓图，再把镜面移到目标点。
void Magnifier::grabNMove(const QPoint &pGrab, const QPoint &pMove, bool needGrab, bool needMove)
{
    QPoint pointToGrab = pGrab;
    d->updPointGrab    = pointToGrab;
    d->updPointMove    = pMove;

    if (needGrab)
    {
        grabPoint(pointToGrab);
    }

    if (needMove)
    {
        QPoint movePos = d->mView->mapFromGlobal(pMove);
        move(movePos.x() - width() / 2, movePos.y() - height() / 2);
    }
}

void Magnifier::setGrabView(QWidget *view)
{
    d->gView = view;
    d->mRefreshTimer.setInterval(40);
    d->mRefreshTimer.start();
}

void Magnifier::setMoveView(QWidget *view)
{
    d->mView = view;
}

void Magnifier::setDrawingMode(int mode)
{
    d->mDrawingMode = static_cast<DrawingMode>(mode);

    // 切换按钮显示的是“将要切换到的形状”，因此图标名与当前形状正好相反。
    QString sMode;

    if (circular == d->mDrawingMode)
    {
        sMode = "roundeRrectangle";
        resize(width(), width());
    }

    if (rectangular == d->mDrawingMode)
    {
        sMode = "circle";
        resize(width(), height() / 3);

        if (d->mView)
        {
            qreal newPercentSize = size().width() / 3 * 100 / d->mView->width();
            emit  magnifierResized_Signal(newPercentSize);
        }
    }

    d->sChangeModePixmap->load(":/images/" + sMode + ".svg");

    calculateButtonsPositions();
    if (d->mView && d->gView)
    {
        BoardController::instance()->controlView()->scene()->moveMagnifier();
    }

    createMask();

    Settings::settings()->magnifierDrawingMode->set(mode);
}
