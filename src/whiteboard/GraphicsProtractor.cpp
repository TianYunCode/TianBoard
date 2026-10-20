#include "whiteboard/GraphicsProtractor.h"
#include "core/Application.h"
#include "ui/Resources.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/DrawingController.h"
#include "whiteboard/AbstractDrawRuler.h"

#include <QtWidgets/QGraphicsView>

const QRectF GraphicsProtractor::sDefaultRect = QRectF(-250, -250, 500, 500);
const qreal  GraphicsProtractor::minRadius    = 70;

GraphicsProtractor::GraphicsProtractor()
    : QGraphicsEllipseItem(sDefaultRect), mCurrentTool(None), mShowButtons(false), mCurrentAngle(0), mSpan(180), mStartAngle(0), mScaleFactor(1),
      mResetSvgItem(0), mResizeSvgItem(0), mMarkerSvgItem(0)
{
    sFillTransparency = 127;
    sDrawTransparency = 192;
    create(*this);

    setCacheMode(QGraphicsItem::DeviceCoordinateCache);

    setStartAngle(0);
    setSpanAngle(180 * 16);

    mResetSvgItem = new QGraphicsSvgItem(":/images/resetTool.svg", this);
    mResetSvgItem->setVisible(false);
    mResetSvgItem->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Control));

    mResizeSvgItem = new QGraphicsSvgItem(":/images/resizeTool.svg", this);
    mResizeSvgItem->setVisible(false);
    mResizeSvgItem->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Control));

    mMarkerSvgItem = new QGraphicsSvgItem(":/images/angleMarker.svg", this);
    mMarkerSvgItem->setVisible(false);
    mMarkerSvgItem->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Tool));

    mRotateSvgItem = new QGraphicsSvgItem(":/images/rotateTool.svg", this);
    mRotateSvgItem->setVisible(false);
    mRotateSvgItem->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Control));

    setData(GraphicsItemData::itemLayerType,
        QVariant(itemLayerType::CppTool)); // 必须设置：场景按图层类型分配 z 值
    setFlag(QGraphicsItem::ItemIsSelectable, false);

    mCloseSvgItem->setPos(closeButtonRect().topLeft());
    mResetSvgItem->setPos(resetButtonRect().topLeft());
    mResizeSvgItem->setPos(resizeButtonRect().topLeft());
    mMarkerSvgItem->setPos(markerButtonRect().topLeft());
    mRotateSvgItem->setPos(rotateButtonRect().topLeft());
}

void GraphicsProtractor::paint(QPainter *painter, const QStyleOptionGraphicsItem *styleOption, QWidget *widget)
{
    painter->save();

    Q_UNUSED(styleOption);
    Q_UNUSED(widget);

    QPen pen_(drawColor());
    pen_.setWidth(0); // 线宽恒为 1 像素，不随缩放变化
    painter->setPen(pen_);

    painter->setFont(QFont("Arial", 11));
    painter->setBrush(fillBrush());
    painter->drawPie(
        QRectF(rect().center().x() - radius(), rect().center().y() - radius(), 2 * radius(), 2 * radius()), mStartAngle * 16, mSpan * 16);
    paintGraduations(painter);
    paintButtons(painter);
    paintAngleMarker(painter);

    painter->restore();
}

QVariant GraphicsProtractor::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == QGraphicsItem::ItemSceneChange)
    {
        mCloseSvgItem->setParentItem(this);
        mResizeSvgItem->setParentItem(this);
        mResetSvgItem->setParentItem(this);
        mRotateSvgItem->setParentItem(this);
        mMarkerSvgItem->setParentItem(this);
    }

    return QGraphicsEllipseItem::itemChange(change, value);
}

QRectF GraphicsProtractor::boundingRect() const
{
    QPointF center  = rect().center();
    qreal   centerX = center.x();
    qreal   centerY = center.y();

    QRectF bounds   = resizeButtonRect().adjusted(centerX, centerY, centerX, centerY);
    bounds          = bounds.united(closeButtonRect().adjusted(centerX, centerY, centerX, centerY));
    bounds          = bounds.united(resetButtonRect().adjusted(centerX, centerY, centerX, centerY));

    QTransform t;
    t.translate(centerX, centerY);
    t.rotate(-mStartAngle);
    t.translate(-centerX, -centerY);
    bounds = t.mapRect(bounds);

    bounds = bounds.united(QGraphicsEllipseItem::boundingRect());

    return bounds;
}

QPainterPath GraphicsProtractor::shape() const
{
    QPainterPath path = QGraphicsEllipseItem::shape();
    QPainterPath buttonPath;
    QRectF       markerRect = markerButtonRect();

    QPointF center          = rect().center();
    qreal   centerX         = center.x();
    qreal   centerY         = center.y();

    buttonPath.addRect(resizeButtonRect().adjusted(centerX, centerY, centerX, centerY));
    if (!resizeButtonRect().contains(markerRect))
    {
        buttonPath.addRect(markerRect.adjusted(
            centerX - markerRect.left() * 2 - markerRect.width(), centerY, centerX - markerRect.left() * 2 - markerRect.width(), centerY));
        buttonPath.addRect(markerRect.adjusted(centerX, centerY, centerX, centerY));
    }
    buttonPath.addRect(closeButtonRect().adjusted(centerX, centerY, centerX, centerY));
    buttonPath.addRect(resetButtonRect().adjusted(centerX, centerY, centerX, centerY));
    QTransform t;
    t.translate(centerX, centerY);
    t.rotate(-mStartAngle);
    t.translate(-centerX, -centerY);
    buttonPath = t.map(buttonPath);
    buttonPath = buttonPath.subtracted(path);
    path.addPath(buttonPath);

    return path;
}

void GraphicsProtractor::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    mPreviousMousePos = event->pos();
    mCurrentTool      = toolFromPos(event->pos());
    mShowButtons      = mCurrentTool == Reset || mCurrentTool == Close;

    if (mCurrentTool == None || mCurrentTool == Move)
    {
        QGraphicsEllipseItem::mousePressEvent(event);
    }
    else
    {
        event->accept();
    }
}

void GraphicsProtractor::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF currentPoint = event->pos();
    QLineF  startLine(rect().center(), mPreviousMousePos);
    QLineF  currentLine(rect().center(), currentPoint);
    qreal   angle       = startLine.angleTo(currentLine);
    qreal   scaleFactor = currentLine.length() / startLine.length();

    switch (mCurrentTool)
    {
        case Rotate:
            prepareGeometryChange();
            mStartAngle = mStartAngle + angle;
            setStartAngle(mStartAngle * 16);
            mPreviousMousePos = currentPoint;

            break;

        case Resize:
            if (radius() * mScaleFactor * scaleFactor > minRadius)
            {
                prepareGeometryChange();
                setTransform(QTransform::fromTranslate(rect().center().x(), rect().center().y()), true);
                setTransform(QTransform::fromScale(scaleFactor, scaleFactor), true);
                setTransform(QTransform::fromTranslate(-rect().center().x(), -rect().center().y()), true);
                mScaleFactor *= scaleFactor;
            }
            break;

        case MoveMarker:

            mCurrentAngle += angle;
            if ((int)mCurrentAngle % 360 > 270)
            {
                mCurrentAngle = 0;
            }
            else if ((int)mCurrentAngle % 360 >= 180)
            {
                mCurrentAngle = 180;
            }

            mPreviousMousePos = currentPoint;
            update();
            break;

        case Move:
            QGraphicsEllipseItem::mouseMoveEvent(event);
            break;

        default:
            break;
    }

    if (mCurrentTool != Move)
    {
        event->accept();
    }
}

void GraphicsProtractor::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    switch (mCurrentTool)
    {
        case Reset:
            setStartAngle(0);
            mStartAngle = 0;
            break;

        case Close:
            hide();
            break;

        case MoveMarker:
            update();
            break;

        case Resize:
            update();
            break;

        default:
            QGraphicsEllipseItem::mouseReleaseEvent(event);
            break;
    }

    if (mCurrentTool != Move)
    {
        event->accept();
    }

    if (scene())
    {
        scene()->setModified(true);
    }

    if (!mShowButtons)
    {
        mShowButtons = true;
        update();
    }

    mCurrentTool = None;
}

void GraphicsProtractor::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    if (DrawingController::drawingController()->stylusTool() != StylusTool::Selector &&
        DrawingController::drawingController()->stylusTool() != StylusTool::Play)
    {
        return;
    }

    if (!mShowButtons)
    {
        mShowButtons = true;

        mCloseSvgItem->setParentItem(this);
        mResizeSvgItem->setParentItem(this);
        mResetSvgItem->setParentItem(this);
        mRotateSvgItem->setParentItem(this);
        mMarkerSvgItem->setParentItem(this);

        update();
    }

    event->accept();
}

void GraphicsProtractor::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    mShowButtons = false;
    unsetCursor();
    update();
    event->accept();
}

void GraphicsProtractor::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    if (DrawingController::drawingController()->stylusTool() != StylusTool::Selector)
    {
        return;
    }

    Tool currentTool = toolFromPos(event->pos());

    if (currentTool == Move)
    {
        setCursor(Qt::SizeAllCursor);
    }
    else if (currentTool == Rotate)
    {
        setCursor(rotateCursor());
    }
    else
    {
        setCursor(Qt::ArrowCursor);
    }

    event->accept();
}

qreal GraphicsProtractor::antiScale() const
{
    return 1 / (mScaleFactor * Application::boardController->systemScaleFactor() * Application::boardController->currentZoom());
}

QRectF GraphicsProtractor::resetButtonRect() const
{
    qreal refWidth = buttonSizeReference().width();
    qreal width    = mResetSvgItem->boundingRect().width();
    qreal height   = mResetSvgItem->boundingRect().height();

    return QRectF(-refWidth * 7, -height / 2.0, width, height);

    // 注意：这些硬编码的位置值导致无法对按钮调用 setScale()
    // 而不影响它们在工具上的相对位置。
}

QRectF GraphicsProtractor::closeButtonRect() const
{
    qreal refWidth = buttonSizeReference().width();
    qreal width    = mCloseSvgItem->boundingRect().width();
    qreal height   = mCloseSvgItem->boundingRect().height();

    return QRectF(-refWidth * 9, -height / 2.0, width, height);
}

QRectF GraphicsProtractor::resizeButtonRect() const
{
    qreal refWidth = buttonSizeReference().width();
    qreal width    = mResizeSvgItem->boundingRect().width();
    qreal height   = mResizeSvgItem->boundingRect().height();

    return QRectF(refWidth * 8, -height / 2.0, width, height);
}

QRectF GraphicsProtractor::rotateButtonRect() const
{
    qreal refWidth = buttonSizeReference().width();
    qreal width    = mRotateSvgItem->boundingRect().width();
    qreal height   = mRotateSvgItem->boundingRect().height();

    return QRectF(refWidth * 5.5, -refWidth * 5, width, height);
}
QRectF GraphicsProtractor::markerButtonRect() const
{
    qreal width  = mMarkerSvgItem->boundingRect().width();
    qreal height = mMarkerSvgItem->boundingRect().height();

    return QRectF(radius() + 3, -height / 2, width, height);
}

void GraphicsProtractor::paintGraduations(QPainter *painter)
{
    painter->save();

    const int tenDegreeGraduationLength  = 22;
    const int fiveDegreeGraduationLength = 15;
    const int oneDegreeGraduationLength  = 7;

    QFont font1                          = painter->font();

    QFontMetricsF fm1(font1);

    // 内弧刻度数字用的字体
    QFont font2 = painter->font();
    font2.setPointSizeF(font1.pointSizeF() / 1.5);
    QFontMetricsF fm2(font2);

    qreal rad      = radius();

    QPointF center = rect().center();
    painter->drawArc(QRectF(center.x() - rad / 2, center.y() - rad / 2, rad, rad), mStartAngle * 16, mSpan * 16);

    for (int angle = 1; angle < mSpan; angle++)
    {
        int graduationLength =
            (0 == angle % 10) ? tenDegreeGraduationLength : ((0 == angle % 5) ? fiveDegreeGraduationLength : oneDegreeGraduationLength);
        qreal co = cos(((qreal)angle + mStartAngle) * PI / 180);
        qreal si = sin(((qreal)angle + mStartAngle) * PI / 180);
        if (0 == angle % 90)
        {
            painter->drawLine(QLineF(
                QPointF(center.x(), center.y()), QPointF(center.x() + co * tenDegreeGraduationLength, center.y() - si * tenDegreeGraduationLength)));
        }

        // 外弧刻度线
        painter->drawLine(QLineF(QPointF(center.x() + rad * co, center.y() - rad * si),
            QPointF(center.x() + (rad - graduationLength) * co, center.y() - (rad - graduationLength) * si)));
        // 内弧刻度线
        painter->drawLine(QLineF(QPointF(center.x() + rad / 2 * co, center.y() - rad / 2 * si),
            QPointF(center.x() + (rad / 2 + graduationLength) * co, center.y() - (rad / 2 + graduationLength) * si)));

        if (0 == angle % 10)
        {
            // 外弧刻度数字
            painter->setFont(font1);
            QString grad  = QString("%1").arg((int)(angle));
            QString grad2 = QString("%1").arg((int)(mSpan - angle));

            painter->drawText(QRectF(center.x() + (rad - graduationLength * 1.5) * co - fm1.horizontalAdvance(grad) / 2,
                                  center.y() - (rad - graduationLength * 1.5) * si - fm1.height() / 2, fm1.horizontalAdvance(grad), fm1.height()),
                Qt::AlignTop, grad);

            // 内弧刻度数字
            painter->setFont(font2);
            painter->drawText(
                QRectF(center.x() + (rad / 2 + graduationLength * 1.5) * co - fm2.horizontalAdvance(grad2) / 2,
                    center.y() - (rad / 2 + graduationLength * 1.5) * si - fm2.height() / 2, fm2.horizontalAdvance(grad2), fm2.height()),
                Qt::AlignTop, grad2);
            painter->setFont(font1);
        }
    }

    painter->restore();
}

void GraphicsProtractor::paintButtons(QPainter *painter)
{
    Q_UNUSED(painter);

    if (mShowButtons)
    {
        // 关闭、重置、缩放、旋转四个按钮绕量角器中心旋转，跟随工具一起转动。
        // 由于坐标是各 QGraphicsItem 的本地坐标，旋转中心相对按钮的位置
        // 就是按钮自身位置取反。

        mCloseSvgItem->setTransformOriginPoint(closeButtonRect().topLeft() * -1);
        mCloseSvgItem->setRotation(rotation() - mStartAngle);

        mResetSvgItem->setTransformOriginPoint(resetButtonRect().topLeft() * -1);
        mResetSvgItem->setRotation(rotation() - mStartAngle);

        mResizeSvgItem->setTransformOriginPoint(resizeButtonRect().topLeft() * -1);
        mResizeSvgItem->setRotation(rotation() - mStartAngle);

        mRotateSvgItem->setTransformOriginPoint(rotateButtonRect().topLeft() * -1);
        mRotateSvgItem->setRotation(rotation() - mStartAngle);
    }

    mMarkerSvgItem->setTransformOriginPoint(markerButtonRect().topLeft() * -1);
    mMarkerSvgItem->setRotation(rotation() - mStartAngle - mCurrentAngle);

    mCloseSvgItem->setVisible(mShowButtons);
    mResetSvgItem->setVisible(mShowButtons);
    mResizeSvgItem->setVisible(mShowButtons);
    mRotateSvgItem->setVisible(mShowButtons);
    mMarkerSvgItem->setVisible(true);
}

void GraphicsProtractor::paintAngleMarker(QPainter *painter)
{
    painter->save();
    painter->translate(rect().center());
    painter->rotate(-mStartAngle);
    painter->translate(-rect().center().x(), -rect().center().y());
    qreal co  = cos(mCurrentAngle * PI / 180);
    qreal si  = sin(mCurrentAngle * PI / 180);
    qreal rad = radius();

    painter->drawLine(QLineF(rect().center(), QPointF(rect().center().x() + (rad + 20) * co, rect().center().y() - (rad + 20) * si)));
    QPointF center = rect().center();
    painter->drawArc(
        QRectF(center.x() - rad / 8, center.y() - rad / 8, rad / 4, rad / 4), 0, (mCurrentAngle - (int)(mCurrentAngle / 360) * 360) * 16);
    painter->translate(rect().center());
    painter->rotate(-mCurrentAngle);
    painter->translate(-rect().center().x(), -rect().center().y());

    // 绘制角度文字（保持水平）

    // 撤销前面叠加的旋转变换
    painter->translate(rect().center());
    painter->rotate(mCurrentAngle);
    painter->rotate(mStartAngle);
    painter->translate(-rect().center().x(), -rect().center().y());

    qreal angle = mCurrentAngle - (int)(mCurrentAngle / 360) * 360;

    if (angle != 0)
    {
        QString ang   = QString("%1°").arg(angle, 0, 'f', 1);
        QFont   font2 = painter->font();
        font2.setBold(true);
        QFontMetricsF fm2(font2);
        painter->setFont(font2);
        if (angle < 50)
        {
            angle = 90;
        }
        else
        {
            angle = angle / 2;
        }

        co = cos((mStartAngle + angle) * PI / 180);
        si = sin((mStartAngle + angle) * PI / 180);
        painter->drawText(QRectF(rect().center().x() + (rad * 2.5 / 10) * co - fm2.horizontalAdvance(ang) / 2,
                              rect().center().y() - (rad * 2.5 / 10) * si - fm2.height() / 2, fm2.horizontalAdvance(ang), fm2.height()),
            Qt::AlignTop, ang);
    }

    painter->restore();
}

GraphicsProtractor::Tool GraphicsProtractor::toolFromPos(QPointF pos)
{
    pos = pos - rect().center();
    QLineF     line(QPointF(0, 0), pos);
    QTransform t;
    t.rotate(mStartAngle);
    QPointF p1 = t.map(pos);
    t.rotate(mCurrentAngle);
    QPointF p2 = t.map(pos);

    if (resizeButtonRect().contains(p1.x(), p1.y()))
    {
        return Resize;
    }

    else if (closeButtonRect().contains(p1))
    {
        return Close;
    }

    else if (resetButtonRect().contains(p1))
    {
        return Reset;
    }

    else if (rotateButtonRect().contains(p1))
    {
        return Rotate;
    }

    else if (markerButtonRect().contains(p2))
    {
        return MoveMarker;
    }

    else if (line.length() <= radius())
    {
        return Move;
    }

    else
    {
        return None;
    }
}

GraphicsScene *GraphicsProtractor::scene() const
{
    return static_cast<GraphicsScene *>(QGraphicsEllipseItem::scene());
}

QBrush GraphicsProtractor::fillBrush() const
{
    QColor fillColor        = edgeFillColor();
    QColor fillColorCenter  = middleFillColor();
    QColor transparentWhite = Qt::white;
    transparentWhite.setAlpha(scene()->isDarkBackground() ? sDrawTransparency : sFillTransparency);
    QRadialGradient radialGradient(rect().center(), radius(), rect().center());
    radialGradient.setColorAt(0, fillColorCenter);
    radialGradient.setColorAt(1, fillColor);
    return radialGradient;
}

Item *GraphicsProtractor::deepCopy() const
{
    GraphicsProtractor *copy = new GraphicsProtractor();

    copyItemParameters(copy);

    return copy;
}
void GraphicsProtractor::copyItemParameters(Item *copy) const
{
    GraphicsProtractor *cp = dynamic_cast<GraphicsProtractor *>(copy);
    if (cp)
    {
        cp->setPos(this->pos());
        cp->setRect(this->rect());
        cp->setTransform(this->transform());

        cp->mCurrentAngle = this->mCurrentAngle;
        cp->mSpan         = this->mSpan;
        cp->mStartAngle   = this->mStartAngle;
        cp->mScaleFactor  = this->mScaleFactor;
    }
}

void GraphicsProtractor::rotateAroundCenter(qreal angle)
{
    // 与其他工具对象保持一致：绕旋转中心转动
    QTransform transform;
    transform.translate(rotationCenter().x(), rotationCenter().y());
    transform.rotate(angle);
    transform.translate(-rotationCenter().x(), -rotationCenter().y());
    setTransform(transform, true);
}

QPointF GraphicsProtractor::rotationCenter() const
{
    return QPointF(rect().x(), rect().y());
}
