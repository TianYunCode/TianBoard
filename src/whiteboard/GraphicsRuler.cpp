#include <QPixmap>

#include "whiteboard/GraphicsRuler.h"
#include "whiteboard/GraphicsScene.h"
#include "core/Application.h"
#include "ui/Resources.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/DrawingController.h"


const QRect GraphicsRuler::sDefaultRect = QRect(0, 0, 800, 96);


GraphicsRuler::GraphicsRuler() : QGraphicsRectItem(), mResizing(false), mRotating(false) {
    setRect(sDefaultRect);

    mResizeSvgItem = new QGraphicsSvgItem(":/images/resizeRuler.svg", this);
    mResizeSvgItem->setVisible(false);
    mResizeSvgItem->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Control));

    mRotateSvgItem = new QGraphicsSvgItem(":/images/rotateTool.svg", this);
    mRotateSvgItem->setVisible(false);
    mRotateSvgItem->setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Control));

    create(*this);

    setData(GraphicsItemData::itemLayerType,
            QVariant(itemLayerType::CppTool)); //Necessary to set if we want z value to be assigned correctly

    setFlag(QGraphicsItem::ItemIsSelectable, false);
    updateResizeCursor();
}

void GraphicsRuler::updateResizeCursor() {
    QPixmap pix(":/images/cursors/resize.png");
    QTransform itemTransform = sceneTransform();
    QRectF itemRect = boundingRect();
    QPointF topLeft = itemTransform.map(itemRect.topLeft());
    QPointF topRight = itemTransform.map(itemRect.topRight());
    QLineF topLine(topLeft, topRight);
    qreal angle = topLine.angle();
    QTransform tr;
    tr.rotate(-angle);
    QCursor resizeCursor =
        QCursor(pix.transformed(tr, Qt::SmoothTransformation), pix.width() / 2, pix.height() / 2);
    mResizeCursor = resizeCursor;
}


GraphicsRuler::~GraphicsRuler() {
    // NOOP
}

Item *GraphicsRuler::deepCopy() const {
    GraphicsRuler *copy = new GraphicsRuler();

    copyItemParameters(copy);

    return copy;
}

void GraphicsRuler::copyItemParameters(Item *copy) const {
    GraphicsRuler *cp = dynamic_cast<GraphicsRuler *>(copy);
    if (cp) {
        cp->setPos(this->pos());
        cp->setRect(this->rect());
        cp->setTransform(this->transform());
    }
}

void GraphicsRuler::paint(QPainter *painter, const QStyleOptionGraphicsItem *styleOption, QWidget *widget) {
    Q_UNUSED(styleOption);
    Q_UNUSED(widget);

    AbstractDrawRuler::paint();

    QTransform antiScaleTransform2;
    qreal ratio = mAntiScaleRatio > 1.0 ? mAntiScaleRatio : 1.0;
    antiScaleTransform2.scale(ratio, 1.0);

    mResizeSvgItem->setTransform(antiScaleTransform2);
    mResizeSvgItem->setPos(resizeButtonRect().topLeft());

    mRotateSvgItem->setTransform(antiScaleTransform2);
    mRotateSvgItem->setPos(rotateButtonRect().topLeft());


    painter->setPen(drawColor());
    painter->setBrush(edgeFillColor());
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->drawRoundedRect(rect(), sRoundingRadius, sRoundingRadius);
    fillBackground(painter);
    paintGraduations(painter);
    if (mRotating)
        paintRotationCenter(painter);
}


QVariant GraphicsRuler::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == QGraphicsItem::ItemVisibleHasChanged) {
        mCloseSvgItem->setParentItem(this);
        mResizeSvgItem->setParentItem(this);
        mRotateSvgItem->setParentItem(this);
    }

    return QGraphicsRectItem::itemChange(change, value);
}

void GraphicsRuler::fillBackground(QPainter *painter) {
    QRectF rect1(rect().topLeft(), QSizeF(rect().width(), rect().height() / 4));
    QLinearGradient linearGradient1(rect1.topLeft(), rect1.bottomLeft());
    linearGradient1.setColorAt(0, edgeFillColor());
    linearGradient1.setColorAt(1, middleFillColor());
    painter->fillRect(rect1, linearGradient1);

    QRectF rect2(rect1.bottomLeft(), QSizeF(rect().width(), rect().height() / 2));
    painter->fillRect(rect2, middleFillColor());

    QRectF rect3(rect2.bottomLeft(), rect1.size());
    QLinearGradient linearGradient3(rect3.topLeft(), rect3.bottomLeft());
    linearGradient3.setColorAt(0, middleFillColor());
    linearGradient3.setColorAt(1, edgeFillColor());
    painter->fillRect(rect3, linearGradient3);
}

void GraphicsRuler::paintGraduations(QPainter *painter) {
    painter->save();
    painter->setFont(font());
    QFontMetricsF fontMetrics(painter->font());

    // Update the width of one "centimeter" to correspond to the width of the background grid (whether it is displayed or not)
    sPixelsPerCentimeter = Application::boardController->activeScene()->backgroundGridSize();

    qreal pixelsPerMillimeter = sPixelsPerCentimeter / 10.0;
    int rulerLengthInMillimeters = (rect().width() - sLeftEdgeMargin - sRoundingRadius) / pixelsPerMillimeter;

    // When a "centimeter" is too narrow, we only display every 5th number, and every 5th millimeter mark
    double numbersWidth = fontMetrics.horizontalAdvance("00");
    bool shouldDisplayAllNumbers = (numbersWidth <= (sPixelsPerCentimeter - 5));

    for (int millimeters(0); millimeters < rulerLengthInMillimeters; millimeters++) {
        double graduationX = rotationCenter().x() + pixelsPerMillimeter * millimeters;
        double graduationHeight = 0;

        if (millimeters % GeometryUtils::millimetersPerCentimeter == 0)
            graduationHeight = GeometryUtils::centimeterGraduationHeight;

        else if (millimeters % GeometryUtils::millimetersPerHalfCentimeter == 0)
            graduationHeight = GeometryUtils::halfCentimeterGraduationHeight;

        else
            graduationHeight = GeometryUtils::millimeterGraduationHeight;


        if (shouldDisplayAllNumbers || millimeters % GeometryUtils::millimetersPerHalfCentimeter == 0) {
            painter->drawLine(QLineF(graduationX, rotationCenter().y(), graduationX,
                                     rotationCenter().y() + graduationHeight));
            painter->drawLine(QLineF(graduationX, rotationCenter().y() + rect().height(), graduationX,
                                     rotationCenter().y() + rect().height() - graduationHeight));
        }


        if ((shouldDisplayAllNumbers && millimeters % GeometryUtils::millimetersPerCentimeter == 0) ||
            millimeters % (GeometryUtils::millimetersPerCentimeter * 5) == 0) {
            QString text = QString("%1").arg((int)(millimeters / GeometryUtils::millimetersPerCentimeter));

            if (graduationX + fontMetrics.horizontalAdvance(text) / 2 < rect().right()) {
                qreal textWidth = fontMetrics.horizontalAdvance(text);
                qreal textHeight = fontMetrics.tightBoundingRect(text).height() + 5;
                painter->drawText(QRectF(graduationX - textWidth / 2,
                                         rect().top() + 5 + GeometryUtils::centimeterGraduationHeight,
                                         textWidth, textHeight),
                                  Qt::AlignVCenter, text);
                painter->drawText(
                    QRectF(graduationX - textWidth / 2,
                           rect().bottom() - 5 - GeometryUtils::centimeterGraduationHeight - textHeight,
                           textWidth, textHeight),
                    Qt::AlignVCenter, text);
            }
        }
    }


    painter->restore();
}

void GraphicsRuler::paintRotationCenter(QPainter *painter) {
    painter->drawArc(rotationCenter().x() - sRotationRadius, rotationCenter().y() - sRotationRadius,
                     2 * sRotationRadius, 2 * sRotationRadius, 270 * sDegreeToQtAngleUnit,
                     90 * sDegreeToQtAngleUnit);
}

void GraphicsRuler::rotateAroundCenter(qreal angle) {
    QTransform transform;
    transform.translate(rotationCenter().x(), rotationCenter().y());
    transform.rotate(angle);
    transform.translate(-rotationCenter().x(), -rotationCenter().y());
    setTransform(transform, true);
}

QPointF GraphicsRuler::rotationCenter() const {
    return QPointF(rect().x() + sLeftEdgeMargin, rect().y());
}


QRectF GraphicsRuler::resizeButtonRect() const {
    QPixmap resizePixmap(":/images/resizeRuler.svg");
    QSizeF resizeRectSize(resizePixmap.rect().width(), rect().height());

    qreal ratio = mAntiScaleRatio > 1.0 ? mAntiScaleRatio : 1.0;
    QPointF resizeRectTopLeft(rect().width() - resizeRectSize.width() * ratio, 0);

    QRectF resizeRect(resizeRectTopLeft, resizeRectSize);
    resizeRect.translate(rect().topLeft());

    return resizeRect;
}

QRectF GraphicsRuler::closeButtonRect() const {
    QPixmap closePixmap(":/images/closeTool.svg");

    QSizeF closeRectSize(closePixmap.width() * mAntiScaleRatio, closePixmap.height() * mAntiScaleRatio);

    QPointF closeRectCenter(rect().left() + sLeftEdgeMargin + sPixelsPerCentimeter / 2, rect().center().y());

    QPointF closeRectTopLeft(closeRectCenter.x() - closeRectSize.width() / 2,
                             closeRectCenter.y() - closeRectSize.height() / 2);

    return QRectF(closeRectTopLeft, closeRectSize);
}

QRectF GraphicsRuler::rotateButtonRect() const {
    QPixmap rotatePixmap(":/images/closeTool.svg");

    QSizeF rotateRectSize(rotatePixmap.width() * mAntiScaleRatio, rotatePixmap.height() * mAntiScaleRatio);

    int centimeters =
        (int)(rect().width() - sLeftEdgeMargin - resizeButtonRect().width()) / (sPixelsPerCentimeter);
    QPointF rotateRectCenter(rect().left() + sLeftEdgeMargin + (centimeters - 0.5) * sPixelsPerCentimeter,
                             rect().center().y());

    QPointF rotateRectTopLeft(rotateRectCenter.x() - rotateRectSize.width() / 2,
                              rotateRectCenter.y() - rotateRectSize.height() / 2);

    return QRectF(rotateRectTopLeft, rotateRectSize);
}

void GraphicsRuler::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    StylusTool::Enum currentTool = (StylusTool::Enum)DrawingController::drawingController()->stylusTool();

    if (currentTool == StylusTool::Selector || currentTool == StylusTool::Play) {
        mCloseSvgItem->setVisible(mShowButtons);
        mResizeSvgItem->setVisible(mShowButtons);
        mRotateSvgItem->setVisible(mShowButtons);
        if (resizeButtonRect().contains(event->pos()))
            setCursor(resizeCursor());
        else if (closeButtonRect().contains(event->pos()))
            setCursor(closeCursor());
        else if (rotateButtonRect().contains(event->pos()))
            setCursor(rotateCursor());
        else
            setCursor(moveCursor());

        event->accept();
    } else if (DrawingController::drawingController()->isDrawingTool()) {
        event->accept();
    }
}

void GraphicsRuler::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (event->pos().x() > resizeButtonRect().left()) {
        mResizing = true;
        event->accept();
    } else if (rotateButtonRect().contains(event->pos())) {
        mRotating = true;
        event->accept();
    } else {
        mResizeSvgItem->setVisible(false);
        mRotateSvgItem->setVisible(false);
        QGraphicsItem::mousePressEvent(event);
    }
    mResizeSvgItem->setVisible(mShowButtons && mResizing);
    mRotateSvgItem->setVisible(mShowButtons && mRotating);
    mCloseSvgItem->setVisible(false);
}

void GraphicsRuler::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (!mResizing && !mRotating) {
        QGraphicsItem::mouseMoveEvent(event);
    } else {
        if (mResizing) {
            QPointF delta = event->pos() - event->lastPos();
            if (rect().width() + delta.x() < sMinLength)
                delta.setX(sMinLength - rect().width());

            if (rect().width() + delta.x() > sMaxLength)
                delta.setX(sMaxLength - rect().width());

            setRect(QRectF(rect().topLeft(), QSizeF(rect().width() + delta.x(), rect().height())));
        } else {
            QLineF currentLine(rotationCenter(), event->pos());
            QLineF lastLine(rotationCenter(), event->lastPos());
            rotateAroundCenter(currentLine.angleTo(lastLine));
        }

        event->accept();
    }
}

void GraphicsRuler::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (mResizing) {
        mResizing = false;
        event->accept();
    } else if (mRotating) {
        mRotating = false;
        updateResizeCursor();
        update(QRectF(rotationCenter(), QSizeF(sRotationRadius, sRotationRadius)));
        event->accept();
    } else if (closeButtonRect().contains(event->pos())) {
        hide();
        event->accept();
    } else {
        QGraphicsItem::mouseReleaseEvent(event);
    }

    if (scene())
        scene()->setModified(true);
}

void GraphicsRuler::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
    StylusTool::Enum currentTool = (StylusTool::Enum)DrawingController::drawingController()->stylusTool();

    if (currentTool == StylusTool::Selector || currentTool == StylusTool::Play) {
        mCloseSvgItem->setParentItem(this);
        mResizeSvgItem->setParentItem(this);
        mRotateSvgItem->setParentItem(this);

        mShowButtons = true;
        mCloseSvgItem->setVisible(mShowButtons);
        mResizeSvgItem->setVisible(mShowButtons);
        mRotateSvgItem->setVisible(mShowButtons);
        if (event->pos().x() >= resizeButtonRect().left()) {
            setCursor(resizeCursor());
        } else {
            if (closeButtonRect().contains(event->pos()))
                setCursor(closeCursor());
            else if (rotateButtonRect().contains(event->pos()))
                setCursor(rotateCursor());
            else
                setCursor(moveCursor());
        }
        event->accept();
        update();
    } else if (DrawingController::drawingController()->isDrawingTool()) {
        setCursor(drawRulerLineCursor());
        DrawingController::drawingController()->mActiveRuler = this;
        event->accept();
    }
}

void GraphicsRuler::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
    mShowButtons = false;
    setCursor(Qt::ArrowCursor);
    mCloseSvgItem->setVisible(mShowButtons);
    mResizeSvgItem->setVisible(mShowButtons);
    mRotateSvgItem->setVisible(mShowButtons);
    DrawingController::drawingController()->mActiveRuler = NULL;
    event->accept();
    update();
}


GraphicsScene *GraphicsRuler::scene() const {
    return static_cast<GraphicsScene *>(QGraphicsRectItem::scene());
}

void GraphicsRuler::StartLine(const QPointF &scenePos, qreal width) {
    Q_UNUSED(width);

    QPointF itemPos = mapFromScene(scenePos);

    mStrokeWidth = DrawingController::drawingController()->currentToolWidth();

    qreal y;

    if (itemPos.y() > rect().y() + rect().height() / 2) {
        drawLineDirection = 0;
        y = rect().y() + rect().height() + mStrokeWidth / 2;
    } else {
        drawLineDirection = 1;
        y = rect().y() - mStrokeWidth / 2;
    }

    if (itemPos.x() < rect().x() + sLeftEdgeMargin)
        itemPos.setX(rect().x() + sLeftEdgeMargin);
    if (itemPos.x() > rect().x() + rect().width() - sLeftEdgeMargin)
        itemPos.setX(rect().x() + rect().width() - sLeftEdgeMargin);

    itemPos.setY(y);
    itemPos = mapToScene(itemPos);

    scene()->moveTo(itemPos);
    scene()->drawLineTo(itemPos, mStrokeWidth, true);
}

void GraphicsRuler::DrawLine(const QPointF &scenePos, qreal width) {
    Q_UNUSED(width);
    QPointF itemPos = mapFromScene(scenePos);

    qreal y;
    if (drawLineDirection == 0) {
        y = rect().y() + rect().height() + mStrokeWidth / 2;
    } else {
        y = rect().y() - mStrokeWidth / 2;
    }
    if (itemPos.x() < rect().x() + sLeftEdgeMargin)
        itemPos.setX(rect().x() + sLeftEdgeMargin);
    if (itemPos.x() > rect().x() + rect().width() - sLeftEdgeMargin)
        itemPos.setX(rect().x() + rect().width() - sLeftEdgeMargin);

    itemPos.setY(y);
    itemPos = mapToScene(itemPos);

    // We have to use "pointed" line for marker tool
    scene()->drawLineTo(itemPos, mStrokeWidth,
                        DrawingController::drawingController()->stylusTool() != StylusTool::Marker);
}

void GraphicsRuler::EndLine() {
    // We never come to this place
    scene()->inputDeviceRelease();
}
