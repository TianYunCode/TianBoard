#include "whiteboard/GraphicsAxes.h"

#include "utils/GeometryUtils.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/DrawingController.h"
#include "whiteboard/GraphicsScene.h"

const QRect GraphicsAxes::sDefaultRect               = QRect(-200, -200, 400, 400);

const QColor GraphicsAxes::sLightBackgroundDrawColor = QColor(0x33, 0x33, 0x33, sDrawTransparency);
const QColor GraphicsAxes::sDarkBackgroundDrawColor  = QColor(0xff, 0xff, 0xff, sDrawTransparency);

GraphicsAxes::GraphicsAxes()
    : QGraphicsPolygonItem(), mResizing(false), mResizeDirection(Left), mShowButtons(false), mShowNumbers(true), mAntiScaleRatio(1.0), mUnitLength(1),
      mCloseSvgItem(0), mMoveToolSvgItem(0), mNumbersSvgItem(0)
{
    setRect(sDefaultRect);

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setAcceptHoverEvents(true);

    mCloseSvgItem = new QGraphicsSvgItem(":/images/closeTool.svg", this);
    mCloseSvgItem->setVisible(false);
    mCloseSvgItem->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Control));

    mMoveToolSvgItem = new QGraphicsSvgItem(":/images/moveTool.svg", this);
    mMoveToolSvgItem->setVisible(false);
    mMoveToolSvgItem->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Control));

    mNumbersSvgItem = new QGraphicsSvgItem(":/images/numbersTool.svg", this);
    mNumbersSvgItem->setVisible(false);
    mNumbersSvgItem->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Control));

    setData(GraphicsItemData::itemLayerType,
        QVariant(itemLayerType::CppTool)); // 必须设好图层类型，Z 值才会被分到正确的层

    // 把手与按钮是按屏幕像素反缩放出来的，视图缩放一变就要重算本体形状。
    connect(BoardController::instance(), &BoardController::zoomChanged, this,
        [this](qreal)
        {
            setRect(mBounds);
        });

    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsFocusable, true); // 需要焦点才能收到方向键
    updateResizeCursor();
}

GraphicsAxes::~GraphicsAxes()
{
}

Item *GraphicsAxes::deepCopy() const
{
    GraphicsAxes *copy = new GraphicsAxes();
    copyItemParameters(copy);
    return copy;
}

void GraphicsAxes::copyItemParameters(Item *copy) const
{
    GraphicsAxes *cp = dynamic_cast<GraphicsAxes *>(copy);
    if (cp)
    {
        cp->setPos(pos());
        cp->setRect(mBounds);
        cp->setTransform(transform());
        cp->setShowNumbers(mShowNumbers);
    }
}

void GraphicsAxes::setRect(qreal x, qreal y, qreal w, qreal h)
{
    mBounds           = QRectF(x, y, w, h);

    mAntiScaleRatio   = 1 / (BoardController::instance()->systemScaleFactor() * BoardController::instance()->currentZoom());
    const qreal range = sItemWidth * mAntiScaleRatio;

    // 本体是一个十字形：只把四臂留出可点区域，中心留空，免得整块板面都被它抢去命中。
    QPolygonF polygon;
    polygon << QPointF(-range, -range) << QPointF(-range, mBounds.top()) << QPointF(range, mBounds.top()) << QPointF(range, -range)
            << QPointF(mBounds.right(), -range) << QPointF(mBounds.right(), range) << QPointF(range, range) << QPointF(range, mBounds.bottom())
            << QPointF(-range, mBounds.bottom()) << QPointF(-range, range) << QPointF(mBounds.left(), range) << QPointF(mBounds.left(), -range);
    setPolygon(polygon);
}

void GraphicsAxes::setRect(const QRectF &rect)
{
    setRect(rect.x(), rect.y(), rect.width(), rect.height());
}

QRectF GraphicsAxes::bounds() const
{
    return mBounds;
}

void GraphicsAxes::setShowNumbers(bool showNumbers)
{
    mShowNumbers = showNumbers;
}

bool GraphicsAxes::showNumbers() const
{
    return mShowNumbers;
}

void GraphicsAxes::paint(QPainter *painter, const QStyleOptionGraphicsItem *styleOption, QWidget *widget)
{
    Q_UNUSED(styleOption);
    Q_UNUSED(widget);

    mAntiScaleRatio = 1 / (BoardController::instance()->systemScaleFactor() * BoardController::instance()->currentZoom());

    QTransform antiScaleTransform;
    antiScaleTransform.scale(mAntiScaleRatio, mAntiScaleRatio);

    mCloseSvgItem->setTransform(antiScaleTransform);
    mCloseSvgItem->setPos(closeButtonRect().topLeft());

    mNumbersSvgItem->setTransform(antiScaleTransform);
    mNumbersSvgItem->setPos(numbersButtonRect().topLeft());

    mMoveToolSvgItem->setVisible(hasFocus());
    if (hasFocus())
    {
        mMoveToolSvgItem->setTransform(antiScaleTransform);
        mMoveToolSvgItem->setPos(moveToolButtonRect().topLeft());
    }

    QPen pen(drawColor());
    pen.setWidthF(2);
    painter->setPen(pen);
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->drawLine(xAxis());
    painter->drawLine(yAxis());

    // 箭头由轴端两条斜线构成，不用填充多边形，缩放时不会糊成一团。
    QPointF tip = xAxis().p2();
    painter->drawLine(tip.x(), tip.y(), tip.x() - sArrowLength, tip.y() + sArrowWidth);
    painter->drawLine(tip.x(), tip.y(), tip.x() - sArrowLength, tip.y() - sArrowWidth);

    tip = yAxis().p2();
    painter->drawLine(tip.x(), tip.y(), tip.x() + sArrowWidth, tip.y() + sArrowLength);
    painter->drawLine(tip.x(), tip.y(), tip.x() - sArrowWidth, tip.y() + sArrowLength);

    pen.setWidthF(1);
    painter->setPen(pen);
    paintGraduations(painter);
}

QVariant GraphicsAxes::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == QGraphicsItem::ItemVisibleHasChanged)
    {
        mCloseSvgItem->setParentItem(this);
        mNumbersSvgItem->setParentItem(this);
    }

    return QGraphicsPolygonItem::itemChange(change, value);
}

void GraphicsAxes::paintGraduations(QPainter *painter)
{
    painter->save();
    painter->setFont(font());
    QFontMetricsF fontMetrics(painter->font());

    // 一个刻度就是一份页面网格：网格显示与否都不影响这里的取值，刻度与页面格线因此总对齐。
    mUnitLength              = scene()->backgroundGridSize();

    // 单位太窄时数字会叠在一起，只标 1 和 5 的倍数。
    const qreal numbersWidth = fontMetrics.boundingRect(QStringLiteral("-00")).width();
    const bool  allNumbers   = (numbersWidth <= (mUnitLength - 5));

    const int fromX          = static_cast<int>((xAxis().x1() + sMargin) / mUnitLength);
    const int toX            = static_cast<int>((xAxis().x2() - sMargin) / mUnitLength);

    for (int unit = fromX; unit <= toX; unit++)
    {
        const bool  prominent   = qAbs(unit) == 1 || qAbs(unit) % 5 == 0;
        const qreal graduationX = mUnitLength * unit;
        const qreal halfTick    = GeometryUtils::millimeterGraduationHeight;

        painter->drawLine(QLineF(graduationX, halfTick, graduationX, -halfTick));

        if (!mShowNumbers || (!allNumbers && !prominent) || unit == 0)
        {
            continue;
        }

        const QString text       = QString::number(unit);
        const qreal   textWidth  = fontMetrics.horizontalAdvance(text);
        const qreal   textHeight = fontMetrics.tightBoundingRect(text).height();

        if (graduationX + textWidth / 2 >= xAxis().x2())
        {
            continue;
        }

        const QRectF textRect(graduationX - textWidth / 2, textHeight - 5, textWidth, textHeight);
        // 只画完整落在范围内的数字，越界的一半会被当成乱码读。
        if (mBounds.contains(textRect))
        {
            painter->drawText(textRect, Qt::AlignVCenter, text);
        }
    }

    const int fromY = static_cast<int>((-yAxis().y1() + sMargin) / mUnitLength);
    const int toY   = static_cast<int>((-yAxis().y2() - sMargin) / mUnitLength);

    for (int unit = fromY; unit <= toY; unit++)
    {
        const bool  prominent   = qAbs(unit) == 1 || qAbs(unit) % 5 == 0;
        const qreal graduationY = -mUnitLength * unit;
        const qreal halfTick    = GeometryUtils::millimeterGraduationHeight;

        painter->drawLine(QLineF(halfTick, graduationY, -halfTick, graduationY));

        if (!mShowNumbers || (!allNumbers && !prominent) || unit == 0)
        {
            continue;
        }

        const QString text       = QString::number(unit);
        const qreal   textWidth  = fontMetrics.horizontalAdvance(text);
        const qreal   textHeight = fontMetrics.tightBoundingRect(text).height();
        const QRectF  textRect(-textWidth - 10, graduationY - textHeight / 2, textWidth, textHeight);

        if (mBounds.contains(textRect))
        {
            painter->drawText(textRect, Qt::AlignVCenter, text);
        }
    }

    painter->restore();
}

void GraphicsAxes::keyPressEvent(QKeyEvent *event)
{
    QGraphicsItem::keyPressEvent(event);
    switch (event->key())
    {
        case Qt::Key_Up:
            moveBy(0, -1);
            event->accept();
            break;
        case Qt::Key_Down:
            moveBy(0, 1);
            event->accept();
            break;
        case Qt::Key_Left:
            moveBy(-1, 0);
            event->accept();
            break;
        case Qt::Key_Right:
            moveBy(1, 0);
            event->accept();
            break;
        default:
            break;
    }
}

void GraphicsAxes::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (resizeLeftRect().contains(event->pos()))
    {
        mResizeDirection = Left;
        mResizing        = true;
        event->accept();
    }
    else if (resizeRightRect().contains(event->pos()))
    {
        mResizeDirection = Right;
        mResizing        = true;
        event->accept();
    }
    else if (resizeTopRect().contains(event->pos()))
    {
        mResizeDirection = Top;
        mResizing        = true;
        event->accept();
    }
    else if (resizeBottomRect().contains(event->pos()))
    {
        mResizeDirection = Bottom;
        mResizing        = true;
        event->accept();
    }
    else
    {
        mResizing = false;
        QGraphicsItem::mousePressEvent(event);
    }

    mCloseSvgItem->setVisible(false);
    mNumbersSvgItem->setVisible(false);
}

void GraphicsAxes::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!mResizing)
    {
        QGraphicsItem::mouseMoveEvent(event);
        return;
    }

    QPointF delta = event->pos() - event->lastPos();
    prepareGeometryChange();

    switch (mResizeDirection)
    {
        case Left:
        {
            delta.setY(0);
            if (delta.x() > -mBounds.left() - sMinLength)
            {
                delta.setX(-mBounds.left() - sMinLength);
            }
            setRect(mBounds.x() + delta.x(), mBounds.y(), mBounds.width() - delta.x(), mBounds.height());
            break;
        }
        case Right:
        {
            if (-delta.x() > mBounds.right() - sMinLength)
            {
                delta.setX(sMinLength - mBounds.right());
            }
            setRect(mBounds.x(), mBounds.y(), mBounds.width() + delta.x(), mBounds.height());
            break;
        }
        case Top:
        {
            delta.setX(0);
            if (delta.y() > -mBounds.top() - sMinLength)
            {
                delta.setY(-mBounds.top() - sMinLength);
            }
            setRect(mBounds.x(), mBounds.y() + delta.y(), mBounds.width(), mBounds.height() - delta.y());
            break;
        }
        case Bottom:
        {
            if (-delta.y() > mBounds.bottom() - sMinLength)
            {
                delta.setY(sMinLength - mBounds.bottom());
            }
            setRect(mBounds.x(), mBounds.y(), mBounds.width(), mBounds.height() + delta.y());
            break;
        }
    }

    event->accept();
}

void GraphicsAxes::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (mResizing)
    {
        mResizing = false;
        event->accept();
    }
    else if (closeButtonRect().contains(event->pos()))
    {
        hide();
        event->accept();
    }
    else if (numbersButtonRect().contains(event->pos()))
    {
        mShowNumbers = !mShowNumbers;
        update(boundingRect());
        event->accept();
    }
    else
    {
        QGraphicsItem::mouseReleaseEvent(event);
    }

    if (scene())
    {
        scene()->setModified(true);
    }
}

void GraphicsAxes::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    const StylusTool::Enum currentTool = static_cast<StylusTool::Enum>(DrawingController::drawingController()->stylusTool());

    if (currentTool == StylusTool::Selector || currentTool == StylusTool::Play)
    {
        mCloseSvgItem->setParentItem(this);
        mNumbersSvgItem->setParentItem(this);

        mShowButtons = true;
        mCloseSvgItem->setVisible(mShowButtons);
        mNumbersSvgItem->setVisible(mShowButtons);
        selectCursor(event);
        event->accept();
        update();
    }
}

void GraphicsAxes::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    const StylusTool::Enum currentTool = static_cast<StylusTool::Enum>(DrawingController::drawingController()->stylusTool());

    if (currentTool == StylusTool::Selector || currentTool == StylusTool::Play)
    {
        mCloseSvgItem->setVisible(mShowButtons);
        mNumbersSvgItem->setVisible(mShowButtons);
        selectCursor(event);
        event->accept();
    }
}

void GraphicsAxes::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    mShowButtons = false;
    setCursor(Qt::ArrowCursor);
    mCloseSvgItem->setVisible(mShowButtons);
    mNumbersSvgItem->setVisible(mShowButtons);
    event->accept();
    update();
}

void GraphicsAxes::updateResizeCursor()
{
    QPixmap    pix(QStringLiteral(":/images/cursors/resize.png"));
    QTransform tr;
    tr.rotate(-90);
    mResizeCursorH = QCursor(pix, pix.width() / 2, pix.height() / 2);
    mResizeCursorV = QCursor(pix.transformed(tr, Qt::SmoothTransformation), pix.width() / 2, pix.height() / 2);
}

QCursor GraphicsAxes::resizeCursor() const
{
    return (mResizeDirection == Left || mResizeDirection == Right) ? mResizeCursorH : mResizeCursorV;
}

QCursor GraphicsAxes::closeCursor() const
{
    return Qt::ArrowCursor;
}

QCursor GraphicsAxes::moveCursor() const
{
    return Qt::SizeAllCursor;
}

void GraphicsAxes::selectCursor(QGraphicsSceneHoverEvent *event)
{
    if (resizeLeftRect().contains(event->pos()))
    {
        mResizeDirection = Left;
        setCursor(resizeCursor());
    }
    else if (resizeRightRect().contains(event->pos()))
    {
        mResizeDirection = Right;
        setCursor(resizeCursor());
    }
    else if (resizeTopRect().contains(event->pos()))
    {
        mResizeDirection = Top;
        setCursor(resizeCursor());
    }
    else if (resizeBottomRect().contains(event->pos()))
    {
        mResizeDirection = Bottom;
        setCursor(resizeCursor());
    }
    else if (closeButtonRect().contains(event->pos()) || numbersButtonRect().contains(event->pos()))
    {
        setCursor(closeCursor());
    }
    else
    {
        setCursor(moveCursor());
    }
}

QRectF GraphicsAxes::resizeLeftRect() const
{
    const qreal range = sItemWidth * mAntiScaleRatio;
    return QRectF(xAxis().x1(), xAxis().y1() - range, sArrowLength, 2 * range);
}

QRectF GraphicsAxes::resizeRightRect() const
{
    const qreal range = sItemWidth * mAntiScaleRatio;
    return QRectF(xAxis().x2() - sArrowLength, xAxis().y2() - range, sArrowLength, 2 * range);
}

QRectF GraphicsAxes::resizeBottomRect() const
{
    const qreal range = sItemWidth * mAntiScaleRatio;
    return QRectF(yAxis().x1() - range, yAxis().y1() - sArrowLength, 2 * range, sArrowLength);
}

QRectF GraphicsAxes::resizeTopRect() const
{
    const qreal range = sItemWidth * mAntiScaleRatio;
    return QRectF(yAxis().x2() - range, yAxis().y2(), 2 * range, sArrowLength);
}

QRectF GraphicsAxes::closeButtonRect() const
{
    const QPixmap closePixmap(QStringLiteral(":/images/closeTool.svg"));
    const QSizeF  closeRectSize(closePixmap.width() * mAntiScaleRatio, closePixmap.height() * mAntiScaleRatio);
    const QPointF closeRectTopLeft(-sItemWidth * mAntiScaleRatio, -sItemWidth * mAntiScaleRatio);
    return QRectF(closeRectTopLeft, closeRectSize);
}

QRectF GraphicsAxes::numbersButtonRect() const
{
    const QPixmap numbersPixmap(QStringLiteral(":/images/numbersTool.svg"));
    const QSizeF  numbersRectSize(numbersPixmap.width() * mAntiScaleRatio, numbersPixmap.height() * mAntiScaleRatio);
    const QPointF numbersRectTopLeft(sItemWidth * mAntiScaleRatio - numbersRectSize.width(), sItemWidth * mAntiScaleRatio - numbersRectSize.height());
    return QRectF(numbersRectTopLeft, numbersRectSize);
}

QRectF GraphicsAxes::moveToolButtonRect() const
{
    const QPixmap moveToolPixmap(QStringLiteral(":/images/moveTool.svg"));
    const QSizeF  moveToolRectSize(moveToolPixmap.width() * mAntiScaleRatio, moveToolPixmap.height() * mAntiScaleRatio);
    const QPointF moveToolRectTopLeft(sItemWidth * mAntiScaleRatio - mMoveToolSvgItem->boundingRect().width() - 5,
        -sItemWidth * mAntiScaleRatio + mMoveToolSvgItem->boundingRect().height() / 2);
    return QRectF(moveToolRectTopLeft, moveToolRectSize);
}

QLineF GraphicsAxes::xAxis() const
{
    return QLineF(mBounds.left(), 0, mBounds.right(), 0);
}

QLineF GraphicsAxes::yAxis() const
{
    return QLineF(0, mBounds.bottom(), 0, mBounds.top());
}

GraphicsScene *GraphicsAxes::scene() const
{
    return dynamic_cast<GraphicsScene *>(QGraphicsPolygonItem::scene());
}

QColor GraphicsAxes::drawColor() const
{
    return scene()->isDarkBackground() ? sDarkBackgroundDrawColor : sLightBackgroundDrawColor;
}

QFont GraphicsAxes::font() const
{
    QFont font(QStringLiteral("Arial"));
    font.setPixelSize(16);
    font.setBold(true);
    return font;
}
