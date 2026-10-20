#include "whiteboard/GraphicsPolygonItem.h"

#include "utils/GeometryUtils.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsPolygonItem.h"
#include "whiteboard/GraphicsStroke.h"


GraphicsPolygonItem::GraphicsPolygonItem(QGraphicsItem *parent)
    : QGraphicsPolygonItem(parent),
      mHasAlpha(false),
      mOriginalWidth(-1),
      mIsNominalLine(false),
      mStroke(0),
      mpGroup(NULL) {
    // 刻意留空
    initialize();
}


GraphicsPolygonItem::GraphicsPolygonItem(const QPolygonF &polygon, QGraphicsItem *parent)
    : QGraphicsPolygonItem(polygon, parent),
      mOriginalWidth(-1),
      mIsNominalLine(false),
      mStroke(0),
      mpGroup(NULL) {
    // 刻意留空
    initialize();
}


GraphicsPolygonItem::GraphicsPolygonItem(const QLineF &pLine, qreal pWidth)
    : QGraphicsPolygonItem(GeometryUtils::lineToPolygon(pLine, pWidth)),
      mOriginalLine(pLine),
      mOriginalWidth(pWidth),
      mIsNominalLine(true),
      mStroke(0),
      mpGroup(NULL) {
    // 刻意留空
    initialize();
}

GraphicsPolygonItem::GraphicsPolygonItem(const QLineF &pLine, qreal pStartWidth, qreal pEndWidth)
    : QGraphicsPolygonItem(GeometryUtils::lineToPolygon(pLine, pStartWidth, pEndWidth)),
      mOriginalLine(pLine),
      mOriginalWidth(pEndWidth),
      mIsNominalLine(true),
      mStroke(0),
      mpGroup(NULL) {
    // 刻意留空
    initialize();
}


void GraphicsPolygonItem::initialize() {
    setData(GraphicsItemData::itemLayerType,
            QVariant(itemLayerType::DrawingItem)); // 必须设好图层类型，Z 值才会被分到正确的层
    setUuid(QUuid::createUuid());
}

void GraphicsPolygonItem::setUuid(const QUuid &pUuid) {
    Item::setUuid(pUuid);
    setData(GraphicsItemData::ItemUuid,
            QVariant(pUuid)); // UUID 存进图元数据，场景按 UUID 找图元，不必遍历整棵树
}

void GraphicsPolygonItem::clearStroke() {
    if (mStroke != NULL) {
        mStroke->remove(this);
        if (mStroke->polygons().empty())
            delete mStroke;
        mStroke = NULL;
    }
}

GraphicsPolygonItem::~GraphicsPolygonItem() {
    clearStroke();
}

void GraphicsPolygonItem::setStrokesGroup(GraphicsStrokesGroup *group) {
    mpGroup = group;
}

void GraphicsPolygonItem::setStroke(GraphicsStroke *stroke) {
    if (stroke) {
        clearStroke();

        mStroke = stroke;
        mStroke->addPolygon(this);
    }
}

GraphicsStroke *GraphicsPolygonItem::stroke() const {
    return mStroke;
}


void GraphicsPolygonItem::setColor(const QColor &pColor) {
    QGraphicsPolygonItem::setBrush(QBrush(pColor));
    setPen(Qt::NoPen);

    mHasAlpha = (pColor.alphaF() < 1.0);
}


QColor GraphicsPolygonItem::color() const {
    return QGraphicsPolygonItem::brush().color();
}


Item *GraphicsPolygonItem::deepCopy() const {
    GraphicsPolygonItem *copy = new GraphicsPolygonItem(polygon(), 0);
    copyItemParameters(copy);
    return copy;
}


void GraphicsPolygonItem::copyItemParameters(Item *copy) const {
    GraphicsPolygonItem *cp = dynamic_cast<GraphicsPolygonItem *>(copy);
    if (cp) {
        cp->mOriginalLine = this->mOriginalLine;
        cp->mOriginalWidth = this->mOriginalWidth;
        cp->mIsNominalLine = this->mIsNominalLine;

        cp->setTransform(transform());
        cp->setBrush(this->brush());
        cp->setPen(this->pen());
        cp->mHasAlpha = this->mHasAlpha;
        cp->setFillRule(this->fillRule());

        cp->setColorOnDarkBackground(this->colorOnDarkBackground());
        cp->setColorOnLightBackground(this->colorOnLightBackground());

        cp->setZValue(this->zValue());
        cp->setData(GraphicsItemData::ItemLayerType, this->data(GraphicsItemData::ItemLayerType));
    }
}

void GraphicsPolygonItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    if (mHasAlpha && scene() && scene()->isLightBackground())
        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);

    painter->setRenderHints(QPainter::Antialiasing);

    QGraphicsPolygonItem::paint(painter, option, widget);
}

GraphicsScene *GraphicsPolygonItem::scene() {
    return qobject_cast<GraphicsScene *>(QGraphicsPolygonItem::scene());
}
