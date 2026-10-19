#include "whiteboard/GraphicsStrokesGroup.h"
#include "whiteboard/GraphicsStroke.h"

#include "whiteboard/GraphicsPolygonItem.h"


GraphicsStrokesGroup::GraphicsStrokesGroup(QGraphicsItem *parent)
    :QGraphicsItemGroup(parent), GraphicsItem()
{
    setDelegate(new GraphicsItemDelegate(this, 0, GF_COMMON
                                           | GF_RESPECT_RATIO
                                           | GF_REVOLVABLE
                                           | GF_FLIPPABLE_ALL_AXIS));

    setData(GraphicsItemData::ItemLayerType, ItemLayerType::Object);

    setUuid(QUuid::createUuid());
    setData(GraphicsItemData::itemLayerType, QVariant(itemLayerType::ObjectItem)); //Necessary to set if we want z value to be assigned correctly
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);

    mDebugText = NULL;
    debugTextEnabled = false; // set to true to get a graphical display of strokes' Z-levels
}

GraphicsStrokesGroup::~GraphicsStrokesGroup()
{
}

void GraphicsStrokesGroup::setUuid(const QUuid &pUuid)
{
    Item::setUuid(pUuid);
    setData(GraphicsItemData::ItemUuid, QVariant(pUuid)); //store item uuid inside the QGraphicsItem to fast operations with Items on the scene
}

void GraphicsStrokesGroup::setColor(const QColor &color, colorType pColorType)
{
    //TODO Implement common mechanism of managing groups, drop GraphicsStroke if it's obsolete
    //Using casting for the moment
    foreach (QGraphicsItem *item, childItems()) {
        if (item->type() == GraphicsPolygonItem::Type) {
            GraphicsPolygonItem *curPolygon = static_cast<GraphicsPolygonItem *>(item);

            switch (pColorType) {
            case currentColor :
                curPolygon->setColor(color);
                break;
            case colorOnLightBackground :
                 curPolygon->setColorOnLightBackground(color);
                break;
            case colorOnDarkBackground :
                 curPolygon->setColorOnDarkBackground(color);
                break;
            }
        }
    }

    if (mDebugText)
        mDebugText->setBrush(QBrush(color));
}

QColor GraphicsStrokesGroup::color(colorType pColorType) const
{
    QColor result;

    foreach (QGraphicsItem *item, childItems()) {
        if (item->type() == GraphicsPolygonItem::Type) {
            GraphicsPolygonItem *curPolygon = static_cast<GraphicsPolygonItem *>(item);

            switch (pColorType) {
            case currentColor :
                result = curPolygon->color();
                break;
            case colorOnLightBackground :
                result = curPolygon->colorOnLightBackground();
                break;
            case colorOnDarkBackground :
                result = curPolygon->colorOnDarkBackground();
                break;
            }

        }
    }

    return result;
}

void GraphicsStrokesGroup::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Delegate()->startUndoStep();

    QGraphicsItemGroup::mousePressEvent(event);
    event->accept();

    setSelected(false);
}

void GraphicsStrokesGroup::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!isLocked(this)) {
        QGraphicsItemGroup::mouseMoveEvent(event);

        event->accept();
        setSelected(false);
    }
}

void GraphicsStrokesGroup::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Delegate()->commitUndoStep();
    event->accept();

    Delegate()->mouseReleaseEvent(event);
    QGraphicsItemGroup::mouseReleaseEvent(event);
}

Item* GraphicsStrokesGroup::deepCopy() const
{
    QTransform groupTransform = transform();
    QPointF groupPos = pos();

    GraphicsStrokesGroup* copy = new GraphicsStrokesGroup();
    copyItemParameters(copy);
    copy->resetTransform();
    copy->setPos(0,0);

    const_cast<GraphicsStrokesGroup*>(this)->resetTransform();
    const_cast<GraphicsStrokesGroup*>(this)->setPos(0,0);

    QList<QGraphicsItem*> chl = childItems();

    GraphicsStroke* newStroke = new GraphicsStroke;

    foreach(QGraphicsItem *child, chl)
    {
        GraphicsPolygonItem *polygon = dynamic_cast<GraphicsPolygonItem*>(child);

        if (polygon){
            GraphicsPolygonItem *polygonCopy = dynamic_cast<GraphicsPolygonItem*>(polygon->deepCopy());
            if (polygonCopy)
            {
                QGraphicsItem* pItem = dynamic_cast<QGraphicsItem*>(polygonCopy);
                copy->addToGroup(pItem);
                polygonCopy->setStrokesGroup(copy);
                polygonCopy->setStroke(newStroke);
            }
        }
    }
    const_cast<GraphicsStrokesGroup*>(this)->setTransform(groupTransform);
    const_cast<GraphicsStrokesGroup*>(this)->setPos(groupPos);
    copy->setTransform(groupTransform);
    copy->setPos(groupPos);

    return copy;
}

void GraphicsStrokesGroup::copyItemParameters(Item *copy) const
{
    QGraphicsItem *cp = dynamic_cast<QGraphicsItem*>(copy);
    if(NULL != cp)
    {
        cp->setTransform(transform());
        cp->setPos(pos());

        cp->setFlag(QGraphicsItem::ItemIsMovable, true);
        cp->setFlag(QGraphicsItem::ItemIsSelectable, true);
        cp->setData(GraphicsItemData::ItemLayerType, this->data(GraphicsItemData::ItemLayerType));
        cp->setData(GraphicsItemData::ItemLocked, this->data(GraphicsItemData::ItemLocked));
        cp->setZValue(this->zValue());
    }
}

void GraphicsStrokesGroup::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // Never draw the rubber band, we draw our custom selection with the DelegateFrame
    QStyleOptionGraphicsItem styleOption = QStyleOptionGraphicsItem(*option);
    QStyle::State svState = option->state;
    styleOption.state &= ~QStyle::State_Selected;
    QGraphicsItemGroup::paint(painter, &styleOption, widget);
    //Restoring state
    styleOption.state |= svState;

    Delegate()->postpaint(painter, &styleOption, widget);
}

QVariant GraphicsStrokesGroup::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (debugTextEnabled && change == ItemZValueChange) {
        double newZ = qvariant_cast<double>(value);

        GraphicsPolygonItem * poly = NULL;
        if (childItems().size() > 2)
            poly = dynamic_cast<GraphicsPolygonItem*>(childItems()[1]);

        if (poly) {
            if (!mDebugText) {
                mDebugText = new QGraphicsSimpleTextItem("None", this);
                mDebugText->setPos(poly->boundingRect().topLeft() + QPointF(10, 10));
                mDebugText->setBrush(QBrush(poly->color()));
            }
            mDebugText->setText(QString("Z: %1").arg(newZ));
        }
    }

    QVariant newValue = Delegate()->itemChange(change, value);
    return QGraphicsItemGroup::itemChange(change, newValue);
}

QPainterPath GraphicsStrokesGroup::shape() const
{
    QPainterPath path;

    if (isSelected())
    {
        path.addRect(boundingRect());
    }
    else
    {
        foreach(QGraphicsItem* item, childItems())
        {
            path.addPath(item->shape());
        }
    }

    return path;
}
