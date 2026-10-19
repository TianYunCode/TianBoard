#include <QDebug>

#include "whiteboard/GraphicsCache.h"

#include "core/Application.h"

#include "whiteboard/BoardController.h"
#include "whiteboard/BoardView.h"
#include "whiteboard/GraphicsScene.h"


QMap<GraphicsScene *, GraphicsCache *> GraphicsCache::sInstances;

GraphicsCache *GraphicsCache::instance(GraphicsScene *scene) {
    if (!sInstances.contains(scene))
        sInstances.insert(scene, new GraphicsCache(scene));
    return sInstances[scene];
}

GraphicsCache::GraphicsCache(GraphicsScene *scene)
    : QGraphicsRectItem(),
      mMaskColor(Qt::black),
      mMaskShape(eMaskShape_Circle),
      mShapeWidth(100),
      mDrawMask(false),
      mScene(scene) {
    // Get the board size and pass it to the shape
    QRect boardRect = Application::boardController->displayView()->rect();
    setRect(-15 * boardRect.width(), -15 * boardRect.height(), 30 * boardRect.width(),
            30 * boardRect.height());
    setData(Qt::UserRole, QVariant("Cache"));
    setData(GraphicsItemData::itemLayerType,
            QVariant(itemLayerType::Cache)); //Necessary to set if we want z value to be assigned correctly
}

GraphicsCache::~GraphicsCache() {
    sInstances.remove(mScene);
}

Item *GraphicsCache::deepCopy() const {
    GraphicsCache *copy = new GraphicsCache(mScene);

    copyItemParameters(copy);

    return copy;
}

void GraphicsCache::copyItemParameters(Item *copy) const {
    GraphicsCache *cp = dynamic_cast<GraphicsCache *>(copy);
    if (cp) {
        cp->setPos(this->pos());
        cp->setRect(this->rect());
        cp->setTransform(this->transform());
    }
}

QColor GraphicsCache::maskColor() {
    return mMaskColor;
}

void GraphicsCache::setMaskColor(QColor color) {
    mMaskColor = color;
    update();
}

eMaskShape GraphicsCache::maskshape() {
    return mMaskShape;
}

void GraphicsCache::setMaskShape(eMaskShape shape) {
    mMaskShape = shape;
    update();
}

void GraphicsCache::init() {
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
}

void GraphicsCache::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setBrush(mMaskColor);
    painter->setPen(mMaskColor);

    // Draw the hole
    QPainterPath path;
    path.addRect(rect());

    if (mDrawMask) {
        if (eMaskShape_Circle == mMaskShape) {
            path.addEllipse(mShapePos, mShapeWidth, mShapeWidth);
        } else if (eMaskShap_Rectangle == mMaskShape) {
            path.addRect(mShapePos.x() - mShapeWidth, mShapePos.y() - mShapeWidth, 2 * mShapeWidth,
                         2 * mShapeWidth);
        }
        path.setFillRule(Qt::OddEvenFill);
    }

    painter->drawPath(path);
}

void GraphicsCache::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    Q_UNUSED(event);
    mShapePos = event->pos();
    mDrawMask = true;

    // Note: if refresh issues occure, replace the following 3 lines by: update();
    update(updateRect(event->pos()));
    mOldShapeWidth = mShapeWidth;
    mOldShapePos = event->pos();
}

void GraphicsCache::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    mShapePos = event->pos();

    // Note: if refresh issues occure, replace the following 3 lines by: update();
    update(updateRect(event->pos()));
    mOldShapeWidth = mShapeWidth;
    mOldShapePos = event->pos();
}

void GraphicsCache::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    Q_UNUSED(event);
    mDrawMask = false;

    // Note: if refresh issues occure, replace the following 3 lines by: update();
    update(updateRect(event->pos()));
    mOldShapeWidth = mShapeWidth;
    mOldShapePos = event->pos();
}

int GraphicsCache::shapeWidth() {
    return mShapeWidth;
}

void GraphicsCache::setShapeWidth(int width) {
    mShapeWidth = width;
    update();
}

QRectF GraphicsCache::updateRect(QPointF currentPoint) {
    QRectF r;
    int x;
    int y;

    x = qMin(currentPoint.x() - mShapeWidth, mOldShapePos.x() - mOldShapeWidth);
    y = qMin(currentPoint.y() - mShapeWidth, mOldShapePos.y() - mOldShapeWidth);
    r = QRect(x, y, qAbs(currentPoint.x() - mOldShapePos.x()) + 2 * mShapeWidth,
              qAbs(currentPoint.y() - mOldShapePos.y()) + 2 * mShapeWidth);
    return r;
}
