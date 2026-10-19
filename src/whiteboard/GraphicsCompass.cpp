#include "whiteboard/GraphicsCompass.h"
#include "whiteboard/GraphicsScene.h"
#include "core/Application.h"
#include "core/Settings.h"
#include "ui/Resources.h"
#include "whiteboard/GraphicsScene.h"

#include "whiteboard/BoardController.h"
#include "whiteboard/DrawingController.h"

const QRect GraphicsCompass::sDefaultRect = QRect(0, -20, 300, 36);
const QColor GraphicsCompass::sLightBackgroundMiddleFillColor = QColor(0x72, 0x72, 0x72, sFillTransparency);
const QColor GraphicsCompass::sLightBackgroundEdgeFillColor = QColor(0xc3, 0xc3, 0xc3, sFillTransparency);
const QColor GraphicsCompass::sLightBackgroundDrawColor = QColor(0x33, 0x33, 0x33, sDrawTransparency);
const QColor GraphicsCompass::sDarkBackgroundMiddleFillColor = QColor(0xb5, 0xb5, 0xb5, sFillTransparency);
const QColor GraphicsCompass::sDarkBackgroundEdgeFillColor = QColor(0xdd, 0xdd, 0xdd, sFillTransparency);
const QColor GraphicsCompass::sDarkBackgroundDrawColor = QColor(0xff, 0xff, 0xff, sDrawTransparency);

const int GraphicsCompass::sMinRadius = GraphicsCompass::sNeedleLength + GraphicsCompass::sNeedleBaseLength
        + 24 + GraphicsCompass::sDefaultRect.height() + 24 + GraphicsCompass::sPencilBaseLength
        + GraphicsCompass::sPencilLength;

GraphicsCompass::GraphicsCompass()
    : QGraphicsRectItem()
    , mResizing(false)
    , mRotating(false)
    , mDrawing(false)
    , mShowButtons(false)
    , mSpanAngleInDegrees(0)
    , mDrewCircle(false)
    , mCloseSvgItem(0)
    , mResizeSvgItem(0)
    , mAntiScaleRatio(1.0)
    , mDrewCenterCross(false)
{
    setRect(sDefaultRect);

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    setAcceptHoverEvents(true);

    mCloseSvgItem = new QGraphicsSvgItem(":/images/closeTool.svg", this);
    mCloseSvgItem->setVisible(false);
    mCloseSvgItem->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Control));

    mResizeSvgItem = new QGraphicsSvgItem(":/images/resizeCompass.svg", this);
    mResizeSvgItem->setVisible(false);
    mResizeSvgItem->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Control));

    updateResizeCursor();
    updateDrawCursor();

    unsetCursor();

    setData(GraphicsItemData::itemLayerType, QVariant(itemLayerType::CppTool)); //Necessary to set if we want z value to be assigned correctly
    setFlag(QGraphicsItem::ItemIsSelectable, false);

    connect(Application::boardController, &BoardController::penColorChanged, this, &GraphicsCompass::penColorChanged);
    connect(DrawingController::drawingController(), &DrawingController::lineWidthIndexChanged, this,
            [this](int) { lineWidthChanged(); });
}

GraphicsCompass::~GraphicsCompass()
{
    // NOOP
}

Item* GraphicsCompass::deepCopy() const
{
   GraphicsCompass* copy = new GraphicsCompass();

    copyItemParameters(copy);

   return copy;
}

void GraphicsCompass::copyItemParameters(Item *copy) const
{
    GraphicsCompass *cp = dynamic_cast<GraphicsCompass*>(copy);
    if (cp)
    {
        cp->setPos(this->pos());
        cp->setRect(this->rect());
        cp->setTransform(this->transform());
    }
}

void GraphicsCompass::paint(QPainter *painter, const QStyleOptionGraphicsItem *styleOption, QWidget *widget)
{
    Q_UNUSED(styleOption);
    Q_UNUSED(widget);

    painter->setBrush(edgeFillColor());


    mAntiScaleRatio = 1 / (Application::boardController->systemScaleFactor() * Application::boardController->currentZoom());
    QTransform antiScaleTransform;
    antiScaleTransform.scale(mAntiScaleRatio, mAntiScaleRatio);

    mCloseSvgItem->setTransform(antiScaleTransform);
    mCloseSvgItem->setPos(
        closeButtonRect().center().x() - mCloseSvgItem->boundingRect().width() * mAntiScaleRatio / 2,
        closeButtonRect().center().y() - mCloseSvgItem->boundingRect().height() * mAntiScaleRatio / 2);

    mResizeSvgItem->setTransform(antiScaleTransform);
    mResizeSvgItem->setPos(
        resizeButtonRect().center().x() - mResizeSvgItem->boundingRect().width() * mAntiScaleRatio / 2,
        resizeButtonRect().center().y() - mResizeSvgItem->boundingRect().height() * mAntiScaleRatio / 2);

    painter->setPen(drawColor());
    painter->drawRoundedRect(hingeRect(), sCornerRadius, sCornerRadius);
    painter->fillPath(hingeShape(), middleFillColor());

    painter->fillPath(needleShape(), middleFillColor());
    painter->drawPath(needleShape());
    painter->fillPath(needleBaseShape(), middleFillColor());
    painter->drawPath(needleBaseShape());

    QLinearGradient needleArmLinearGradient(
        QPointF(rect().left() + sNeedleLength + sNeedleBaseLength, rect().center().y()),
        QPointF(hingeRect().left(), rect().center().y()));
    needleArmLinearGradient.setColorAt(0, edgeFillColor());
    needleArmLinearGradient.setColorAt(1, middleFillColor());
    painter->fillPath(needleArmShape(), needleArmLinearGradient);
    painter->drawPath(needleArmShape());

    QRectF hingeGripRect(rect().center().x() - 16, rect().center().y() - 16, 32, 32);
    painter->drawEllipse(hingeGripRect);
    if (mShowButtons)
        paintAngleDisplay(painter);

    QLinearGradient pencilArmLinearGradient(
        QPointF(hingeRect().right(), rect().center().y()),
        QPointF(rect().right() - sPencilLength - sPencilBaseLength, rect().center().y()));
    pencilArmLinearGradient.setColorAt(0, middleFillColor());
    pencilArmLinearGradient.setColorAt(1, edgeFillColor());
    painter->fillPath(pencilArmShape(), pencilArmLinearGradient);
    painter->drawPath(pencilArmShape());

    if (scene()->isDarkBackground())
        painter->fillPath(pencilShape(), Application::boardController->penColorOnDarkBackground());
    else
        painter->fillPath(pencilShape(), Application::boardController->penColorOnLightBackground());

    painter->fillPath(pencilBaseShape(), middleFillColor());
    painter->drawPath(pencilBaseShape());

    if (mResizing || mRotating || mDrawing || (mShowButtons && rect().width() > sDisplayRadiusOnPencilArmMinLength))
        paintRadiusDisplay(painter);
}


QVariant GraphicsCompass::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == QGraphicsItem::ItemSceneChange)
    {
        mCloseSvgItem->setParentItem(this);
        mResizeSvgItem->setParentItem(this);
    }

    return QGraphicsRectItem::itemChange(change, value);
}

void GraphicsCompass::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (DrawingController::drawingController ()->stylusTool() != StylusTool::Selector &&
        DrawingController::drawingController ()->stylusTool() != StylusTool::Play)
        return;

    bool closing = false;

    if (resizeButtonRect().contains(event->pos()))
    {
        mResizing = true;
        mRotating = false;
        event->accept();
        qDebug() << "resizing";
    }
    else if (hingeRect().contains(event->pos()))
    {
        mRotating = true;
        mResizing = false;
        event->accept();
        qDebug() << "hinge";
    }
    else if (!closeButtonRect().contains(event->pos()))
    {
        qDebug() << "the rest";

        mDrawing = event->pos().x() > rect().right() - sPencilLength - sPencilBaseLength;
        if (mDrawing)
        {
            qDebug() << "drawing";
            mSpanAngleInDegrees = 0;
            mSceneArcStartPoint = mapToScene(pencilPosition());
            scene()->initStroke();
            scene()->moveTo(mSceneArcStartPoint);
        }
        QGraphicsRectItem::mousePressEvent(event);
    }
    else
        closing = true;

    mResizeSvgItem->setVisible(mShowButtons && mResizing);
    mCloseSvgItem->setVisible(mShowButtons && closing);
}

void GraphicsCompass::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (DrawingController::drawingController ()->stylusTool() != StylusTool::Selector &&
        DrawingController::drawingController ()->stylusTool() != StylusTool::Play)
        return;

    if (!mResizing && !mRotating && !mDrawing)
    {
        QGraphicsRectItem::mouseMoveEvent(event);
        mDrewCenterCross = false;
    }
    else
    {
        if (mResizing)
        {
            QPointF delta = event->pos() - event->lastPos();
            if (rect().width() + delta.x() < sMinRadius)
                delta.setX(sMinRadius - rect().width());
            setRect(QRectF(rect().topLeft(), QSizeF(rect().width() + delta.x(), rect().height())));
        }
        else
        {
            QLineF currentLine(needlePosition(), event->pos());
            QLineF lastLine(needlePosition(), event->lastPos());
            qreal deltaAngle = currentLine.angleTo(lastLine);
            if (deltaAngle > 180)
                deltaAngle -= 360;
            else if (deltaAngle < -180)
                deltaAngle += 360;
            rotateAroundNeedle(deltaAngle);

            if (mDrawing)
            {
                mSpanAngleInDegrees += deltaAngle;
                if (mSpanAngleInDegrees >= 1080)
                    mSpanAngleInDegrees -= 360;
                else if (mSpanAngleInDegrees < -1080)
                    mSpanAngleInDegrees += 360;
                drawArc();
            }
        }
        event->accept();
    }
}

void GraphicsCompass::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (DrawingController::drawingController ()->stylusTool() != StylusTool::Selector &&
        DrawingController::drawingController ()->stylusTool() != StylusTool::Play)
        return;

    if (mResizing)
    {
        event->accept();
    }
    else if (mRotating)
    {
        updateResizeCursor();
        updateDrawCursor();
        event->accept();
    }
    else if (mDrawing)
    {
        updateResizeCursor();
        updateDrawCursor();
        mDrewCenterCross = false;
        event->accept();
    }
    else if (closeButtonRect().contains(event->pos()))
    {
        hide();
        event->accept();
    }
    else
    {
        QGraphicsRectItem::mouseReleaseEvent(event);
    }

    mRotating = false;
    mResizing = false;
    mDrawing = false;

    if (scene())
        scene()->setModified(true);
}

void GraphicsCompass::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    if (DrawingController::drawingController ()->stylusTool() != StylusTool::Selector &&
        DrawingController::drawingController ()->stylusTool() != StylusTool::Play)
        return;

    mOuterCursor = cursor();
    mShowButtons = shape().contains(event->pos());

    mCloseSvgItem->setParentItem(this);
    mResizeSvgItem->setParentItem(this);

    mCloseSvgItem->setVisible(mShowButtons);
    if (mShowButtons)
    {
        if (hingeRect().contains(event->pos()))
            setCursor(rotateCursor());
        else if (event->pos().x() > rect().right() - sPencilLength - sPencilBaseLength)
            setCursor(drawCursor());
        else if (resizeButtonRect().contains(event->pos()))
            setCursor(resizeCursor());
        else if (closeButtonRect().contains(event->pos()))
            setCursor(closeCursor());
        else
            setCursor(moveCursor());
    }
    event->accept();
    update();
}

void GraphicsCompass::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    if (DrawingController::drawingController ()->stylusTool() != StylusTool::Selector &&
        DrawingController::drawingController ()->stylusTool() != StylusTool::Play)
        return;

    mShowButtons = false;
    mCloseSvgItem->setVisible(mShowButtons);
    mResizeSvgItem->setVisible(mShowButtons);
    unsetCursor();
    event->accept();
    update();
}

void GraphicsCompass::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    if (DrawingController::drawingController ()->stylusTool() != StylusTool::Selector &&
        DrawingController::drawingController ()->stylusTool() != StylusTool::Play)
        return;

    mShowButtons = shape().contains(event->pos());
    mCloseSvgItem->setVisible(mShowButtons);
    mResizeSvgItem->setVisible(mShowButtons);
    if (mShowButtons)
    {
        if (hingeRect().contains(event->pos()))
            setCursor(rotateCursor());
        else if (event->pos().x() > rect().right() - sPencilLength - sPencilBaseLength)
            setCursor(drawCursor());
        else if (resizeButtonRect().contains(event->pos()))
            setCursor(resizeCursor());
        else if (closeButtonRect().contains(event->pos()))
            setCursor(closeCursor());
        else
            setCursor(moveCursor());
    }
    else
    {
        setCursor(mOuterCursor);
    }
    event->accept();
    update();
}

void GraphicsCompass::paintAngleDisplay(QPainter *painter)
{
    qreal angle = angleInDegrees();

    qreal angleValue = mDrawing ? - mSpanAngleInDegrees : angle;
    QString angleText = QString("%1").arg(angleValue, 0, 'f', 1) + "°";

    painter->save();
    painter->setFont(font());
        QFontMetricsF fm(painter->font());
    painter->translate(hingeRect().center());
    painter->rotate(angle);
        painter->drawText(
        QRectF(
            - fm.horizontalAdvance(angleText) / 2,
            - fm.height() / 2,
            fm.horizontalAdvance(angleText),
            fm.height()),
        Qt::AlignTop,
        angleText);
    painter->restore();
}

void GraphicsCompass::paintRadiusDisplay(QPainter *painter)
{
    double pixelsPerCentimeter = Application::boardController->activeScene()->backgroundGridSize();

    qreal radiusInCentimeters = rect().width() / pixelsPerCentimeter;
    QString format = rect().width() >= sDisplayRadiusUnitMinLength ? "%1 cm" : "%1";
    QString radiusText = QString(format).arg(radiusInCentimeters, 0, 'f', 1);

    bool onPencilArm = rect().width() > sDisplayRadiusOnPencilArmMinLength;

    painter->save();
    painter->setFont(font());
    QFontMetricsF fm(painter->font());
    QPointF textCenter;

    if (onPencilArm)
        textCenter = QPointF(rect().right() - sPencilBaseLength - sPencilLength - fm.horizontalAdvance(radiusText) / 2 - 24 - 8, rect().center().y());
    else
        textCenter = QPointF((rect().left() + sNeedleLength + sNeedleBaseLength + hingeRect().left()) / 2, rect().center().y());

    painter->translate(textCenter);
    qreal angle = angleInDegrees();
    if (angle > 180)
        angle -= 360;
    else if (angle < -180)
        angle += 360;
    if (angle <= -90 || angle > 90)
        painter->rotate(180);
    painter->drawText(
        QRectF(
            - fm.horizontalAdvance(radiusText) / 2,
            - rect().height() / 2,
            fm.horizontalAdvance(radiusText),
            rect().height()),
        Qt::AlignVCenter,
        radiusText);
    painter->restore();
}

QCursor GraphicsCompass::moveCursor() const
{
    return Qt::SizeAllCursor;
}

QCursor GraphicsCompass::resizeCursor() const
{
    return mResizeCursor;
}

QCursor GraphicsCompass::rotateCursor() const
{
    return Resources::resources()->rotateCursor;
}

QCursor GraphicsCompass::closeCursor() const
{
    return Qt::ArrowCursor;
}

QCursor GraphicsCompass::drawCursor() const
{
    return mDrawCursor;
}

QRectF GraphicsCompass::hingeRect() const
{
    QRectF rotationRect(rect().width() / 2 - rect().height() / 2, 0, rect().height(), rect().height());
    rotationRect.translate(rect().topLeft());
    return rotationRect;
}

QRectF GraphicsCompass::closeButtonRect() const
{
    QPixmap closePixmap(":/images/closeTool.svg");

    QSizeF closeRectSize(
        closePixmap.width() * mAntiScaleRatio,
        closePixmap.height() * mAntiScaleRatio);

    QPointF closeRectTopLeft(
        sNeedleLength + sNeedleBaseLength + 4,
        (rect().height() - closeRectSize.height()) / 2);

    QRectF closeRect(closeRectTopLeft, closeRectSize);
    closeRect.translate(rect().topLeft());

    return closeRect;
}

QRectF GraphicsCompass::resizeButtonRect() const
{
    QPixmap resizePixmap(":/images/resizeCompass.svg");

    QSizeF resizeRectSize(
        resizePixmap.width() * mAntiScaleRatio,
        resizePixmap.height() * mAntiScaleRatio);

    QPointF resizeRectTopLeft(
        rect().width() - sPencilLength - sPencilBaseLength - resizeRectSize.width() - 4,
        (rect().height() - resizeRectSize.height()) / 2);

    QRectF resizeRect(resizeRectTopLeft, resizeRectSize);
    resizeRect.translate(rect().topLeft());

    return resizeRect;
}

void GraphicsCompass::rotateAroundNeedle(qreal angle)
{
    QTransform transform;
    transform.translate(needlePosition().x(), needlePosition().y());
    transform.rotate(angle);
    transform.translate(- needlePosition().x(), - needlePosition().y());
    setTransform(transform, true);
}

void GraphicsCompass::drawArc()
{
    if (!mDrewCenterCross)
    {
        paintCenterCross();
        mDrewCenterCross = true;
        scene()->moveTo(mSceneArcStartPoint);
    }
    QPointF sceneNeedlePosition = mapToScene(needlePosition());
    qreal arcSpanAngle = mSpanAngleInDegrees;
    if (arcSpanAngle > 360)
        arcSpanAngle = 360;
    else if (arcSpanAngle < -360)
        arcSpanAngle = -360;
    if (!mDrewCircle || (-360 != arcSpanAngle && 360 != arcSpanAngle))
    {
        mDrewCircle = (-360 == arcSpanAngle || 360 == arcSpanAngle);
        scene()->drawArcTo(sceneNeedlePosition, arcSpanAngle);
    }
}

void GraphicsCompass::updateResizeCursor()
{
    QPixmap pix(":/images/cursors/resize.png");
    qreal angle = angleInDegrees();

    QTransform tr;
    tr.rotate(- angle);
    mResizeCursor = QCursor(pix.transformed(tr, Qt::SmoothTransformation), pix.width() / 2,  pix.height() / 2);
}

void GraphicsCompass::updateDrawCursor()
{
    QPixmap pix(":/images/cursors/drawCompass.png");
    qreal angle = angleInDegrees();

    QTransform tr;
    tr.rotate(- angle);
    mDrawCursor = QCursor(pix.transformed(tr, Qt::SmoothTransformation), pix.width() / 2,  pix.height() / 2);
}

void GraphicsCompass::paintCenterCross()
{
    QPointF needleCrossCenter = mapToScene(needlePosition());
    scene()->moveTo(QPointF(needleCrossCenter.x() - 5, needleCrossCenter.y()));
    scene()->drawLineTo(QPointF(needleCrossCenter.x() + 5, needleCrossCenter.y()), 1,
        DrawingController::drawingController()->stylusTool() == StylusTool::Line);
    scene()->moveTo(QPointF(needleCrossCenter.x(), needleCrossCenter.y() - 5));
    scene()->drawLineTo(QPointF(needleCrossCenter.x(), needleCrossCenter.y() + 5), 1,
        DrawingController::drawingController()->stylusTool() == StylusTool::Line);
}

QPointF GraphicsCompass::needlePosition() const
{
    return QPointF(rect().x(), rect().y() + rect().height() / 2);
}

QPointF GraphicsCompass::pencilPosition() const
{
    return QPointF(rect().right(), rect().center().y());
}

QPainterPath GraphicsCompass::shape() const
{
    QPainterPath path = needleShape();
    path = path.united(needleBaseShape());
    path = path.united(needleArmShape());
    path.addRect(hingeRect());
    path = path.united(pencilArmShape());
    path = path.united(pencilBaseShape());
    path = path.united(pencilShape());
    return path;
}

QPainterPath GraphicsCompass::needleShape() const
{
    QPainterPath path;
    path.moveTo(rect().left(), rect().center().y());
    path.lineTo(rect().left() + sNeedleLength, rect().center().y() - sNeedleWidth/2);
    path.lineTo(rect().left() + sNeedleLength, rect().center().y() + sNeedleWidth/2);
    path.closeSubpath();
    return path;
}

QPainterPath GraphicsCompass::needleBaseShape() const
{
    int smallHalfSide = sNeedleBaseWidth/2 - sCornerRadius;

    QPainterPath path;
    path.moveTo(rect().left() + sNeedleLength, rect().center().y() - smallHalfSide);
    path.arcTo(
        rect().left() + sNeedleLength,
        rect().center().y() - smallHalfSide - sCornerRadius,
        sCornerRadius*2, sCornerRadius*2,
        180, -90);
    path.lineTo(rect().left() + sNeedleLength + sNeedleBaseLength, rect().center().y() - sNeedleBaseWidth/2);
    path.lineTo(rect().left() + sNeedleLength + sNeedleBaseLength, rect().center().y() + sNeedleBaseWidth/2);
    path.lineTo(rect().left() + sNeedleLength + sCornerRadius, rect().center().y() + smallHalfSide + sCornerRadius);
    path.arcTo(
        rect().left() + sNeedleLength,
        rect().center().y() + smallHalfSide - sCornerRadius,
        sCornerRadius*2, sCornerRadius*2,
        -90, -90);
    path.closeSubpath();

    return path;
}

QPainterPath GraphicsCompass::needleArmShape() const
{
    int smallHalfSide = sNeedleArmLeftWidth/2 - sCornerRadius;

    QPainterPath path;
    path.moveTo(rect().left() + sNeedleLength + sNeedleBaseLength, rect().center().y() - smallHalfSide);
    path.arcTo(
        rect().left() + sNeedleLength + sNeedleBaseLength,
        rect().center().y() - sNeedleArmLeftWidth/2,
        sCornerRadius*2, sCornerRadius*2,
        180, -90);
    path.lineTo(hingeRect().left(), rect().center().y() - sNeedleArmRigthWidth/2);
    path.lineTo(hingeRect().left(), rect().center().y() + sNeedleArmRigthWidth/2);
    path.lineTo(rect().left() + sNeedleLength + sNeedleBaseLength + sCornerRadius, rect().center().y() + sNeedleArmLeftWidth/2);
    path.arcTo(
        rect().left() + sNeedleLength + sNeedleBaseLength,
        rect().center().y() + smallHalfSide - sCornerRadius,
        sCornerRadius*2, sCornerRadius*2,
        -90, -90);
    path.closeSubpath();
    return path;
}

QPainterPath GraphicsCompass::hingeShape() const
{
    QPainterPath path;
    path.moveTo(hingeRect().left() + sCornerRadius, hingeRect().top());
    path.lineTo(hingeRect().right() - sCornerRadius, hingeRect().top());
    path.arcTo(
        hingeRect().right() - sCornerRadius*2,
        hingeRect().top(),
        sCornerRadius*2, sCornerRadius*2,
        90, -90);
    path.lineTo(hingeRect().right(), hingeRect().bottom() - sCornerRadius);
    path.arcTo(
        hingeRect().right() - sCornerRadius*2,
        hingeRect().bottom() - sCornerRadius*2,
        sCornerRadius*2, sCornerRadius*2,
        0, -90);
    path.lineTo(hingeRect().left() + sCornerRadius, hingeRect().bottom());
    path.arcTo(
        hingeRect().left(),
        hingeRect().bottom() - sCornerRadius*2,
        sCornerRadius*2, sCornerRadius*2,
        -90, -90);
    path.lineTo(hingeRect().left(), hingeRect().top() + sCornerRadius);
    path.arcTo(
        hingeRect().left(),
        hingeRect().top(),
        sCornerRadius*2, sCornerRadius*2,
        -180, -90);
    path.closeSubpath();
    return path;
}

QPainterPath GraphicsCompass::pencilShape() const
{
    int penWidthIndex = Settings::settings()->penWidthIndex();
    int logicalCompassPencilWidth = penWidthIndex > 1 ? 8 : (penWidthIndex > 0 ? 4 : 2);
    QPainterPath path;
    path.moveTo(rect().right() - sPencilLength, rect().center().y() - logicalCompassPencilWidth / 2);
    path.lineTo(rect().right() - logicalCompassPencilWidth / 2, rect().center().y() - logicalCompassPencilWidth / 2);
    QRectF tipRect(rect().right() - logicalCompassPencilWidth, rect().center().y() - logicalCompassPencilWidth / 2, logicalCompassPencilWidth, logicalCompassPencilWidth);
    path.arcTo(tipRect, 90, -180);
    path.lineTo(rect().right() - sPencilLength, rect().center().y() + logicalCompassPencilWidth / 2);
    path.closeSubpath();
    return path;
}

QPainterPath GraphicsCompass::pencilBaseShape() const
{
    QPainterPath path;
    path.moveTo(rect().right() - sPencilLength - sPencilBaseLength, rect().center().y() - sPencilBaseWidth/2);
    path.lineTo(rect().right() - sPencilLength - sCornerRadius, rect().center().y() - sPencilBaseWidth/2);
    path.arcTo(
        rect().right() - sPencilLength - sCornerRadius*2, rect().center().y() - sPencilBaseWidth/2,
        sCornerRadius*2, sCornerRadius*2,
        90, -90);
    path.lineTo(rect().right() - sPencilLength, rect().center().y() + sPencilBaseWidth/2 - sCornerRadius);
    path.arcTo(
        rect().right() - sPencilLength - sCornerRadius*2, rect().center().y() + sPencilBaseWidth/2 - sCornerRadius*2,
        sCornerRadius*2, sCornerRadius*2,
        0, -90);
    path.lineTo(rect().right() - sPencilLength - sPencilBaseLength, rect().center().y() + sPencilBaseWidth/2);
    path.closeSubpath();

    return path;
}

QPainterPath GraphicsCompass::pencilArmShape() const
{
    QPainterPath path;
    path.moveTo(hingeRect().right(), rect().center().y() - sPencilArmLeftWidth/2);
    path.lineTo(rect().right() - sPencilLength - sPencilBaseLength - sCornerRadius, rect().center().y() - sPencilArmRightWidth/2);
    path.arcTo(
        rect().right() - sPencilLength - sPencilBaseLength - sCornerRadius*2, rect().center().y() - sPencilArmRightWidth/2,
        sCornerRadius*2, sCornerRadius*2,
        90, -90);
    path.lineTo(rect().right() - sPencilLength - sPencilBaseLength, rect().center().y() + sPencilArmRightWidth/2 - sCornerRadius);
    path.arcTo(
        rect().right() - sPencilLength - sPencilBaseLength - sCornerRadius*2, rect().center().y() + sPencilArmRightWidth/2 - sCornerRadius*2,
        sCornerRadius*2, sCornerRadius*2,
        0, -90);
    path.lineTo(hingeRect().right(), rect().center().y() + sPencilArmLeftWidth/2);
    path.closeSubpath();
    return path;
}

GraphicsScene* GraphicsCompass::scene() const
{
    return static_cast<GraphicsScene*>(QGraphicsRectItem::scene());
}

QColor GraphicsCompass::drawColor() const
{
    return scene()->isDarkBackground() ? sDarkBackgroundDrawColor : sLightBackgroundDrawColor;
}

QColor GraphicsCompass::middleFillColor() const
{
    return scene()->isDarkBackground() ? sDarkBackgroundMiddleFillColor : sLightBackgroundMiddleFillColor;
}

QColor GraphicsCompass::edgeFillColor() const
{
    return scene()->isDarkBackground() ? sDarkBackgroundEdgeFillColor : sLightBackgroundEdgeFillColor;
}

QFont GraphicsCompass::font() const
{
    QFont font("Arial");
    font.setPixelSize(16);
    font.setBold(true);
    return font;
}

qreal GraphicsCompass::angleInDegrees() const
{
    QRectF itemRect = boundingRect();
    QTransform itemTransform = sceneTransform();
    QPointF topLeft = itemTransform.map(itemRect.topLeft());
    QPointF topRight = itemTransform.map(itemRect.topRight());
    QLineF topLine(topLeft, topRight);
    return topLine.angle();
}

void GraphicsCompass::penColorChanged()
{
    QRect pencilRect(rect().right() - sPencilLength, rect().top(), sPencilLength, rect().height());
    update(pencilRect);
}

void GraphicsCompass::lineWidthChanged()
{
    QRect pencilRect(rect().right() - sPencilLength, rect().top(), sPencilLength, rect().height());
    update(pencilRect);
}
