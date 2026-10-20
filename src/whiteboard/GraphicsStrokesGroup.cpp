#include "whiteboard/GraphicsStrokesGroup.h"
#include "whiteboard/GraphicsStroke.h"

#include "whiteboard/GraphicsPolygonItem.h"

GraphicsStrokesGroup::GraphicsStrokesGroup(QGraphicsItem *parent) : QGraphicsItemGroup(parent), GraphicsItem()
{
    setDelegate(new GraphicsItemDelegate(this, 0, GF_COMMON | GF_RESPECT_RATIO | GF_REVOLVABLE | GF_FLIPPABLE_ALL_AXIS));

    setData(GraphicsItemData::ItemLayerType, ItemLayerType::Object);

    setUuid(QUuid::createUuid());
    setData(GraphicsItemData::itemLayerType,
        QVariant(itemLayerType::ObjectItem)); // 必须设好图层类型，Z 值才会被分到正确的层
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);

    mDebugText       = NULL;
    debugTextEnabled = false; // 调试开关：置真会按笔迹的 Z 层上色显示，方便核对分层有没有错乱
}

GraphicsStrokesGroup::~GraphicsStrokesGroup()
{
}

void GraphicsStrokesGroup::setUuid(const QUuid &pUuid)
{
    Item::setUuid(pUuid);
    // UUID 同时存进图元数据：场景按 UUID 找回图元，不必遍历整棵树。
    setData(GraphicsItemData::ItemUuid, QVariant(pUuid));
}

void GraphicsStrokesGroup::setColor(const QColor &color, colorType pColorType)
{
    // TODO: 组的统一管理还没做，现在只能按类型强转子图元；
    // GraphicsStroke 该不该退役，取决于那套机制长什么样
    foreach (QGraphicsItem *item, childItems())
    {
        if (item->type() == GraphicsPolygonItem::Type)
        {
            GraphicsPolygonItem *curPolygon = static_cast<GraphicsPolygonItem *>(item);

            switch (pColorType)
            {
                case currentColor:
                    curPolygon->setColor(color);
                    break;
                case colorOnLightBackground:
                    curPolygon->setColorOnLightBackground(color);
                    break;
                case colorOnDarkBackground:
                    curPolygon->setColorOnDarkBackground(color);
                    break;
            }
        }
    }

    if (mDebugText)
    {
        mDebugText->setBrush(QBrush(color));
    }
}

QColor GraphicsStrokesGroup::color(colorType pColorType) const
{
    QColor result;

    foreach (QGraphicsItem *item, childItems())
    {
        if (item->type() == GraphicsPolygonItem::Type)
        {
            GraphicsPolygonItem *curPolygon = static_cast<GraphicsPolygonItem *>(item);

            switch (pColorType)
            {
                case currentColor:
                    result = curPolygon->color();
                    break;
                case colorOnLightBackground:
                    result = curPolygon->colorOnLightBackground();
                    break;
                case colorOnDarkBackground:
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
    if (!isLocked(this))
    {
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

Item *GraphicsStrokesGroup::deepCopy() const
{
    QTransform groupTransform  = transform();
    QPointF    groupPos        = pos();

    GraphicsStrokesGroup *copy = new GraphicsStrokesGroup();
    copyItemParameters(copy);
    copy->resetTransform();
    copy->setPos(0, 0);

    const_cast<GraphicsStrokesGroup *>(this)->resetTransform();
    const_cast<GraphicsStrokesGroup *>(this)->setPos(0, 0);

    QList<QGraphicsItem *> chl = childItems();

    GraphicsStroke *newStroke  = new GraphicsStroke;

    foreach (QGraphicsItem *child, chl)
    {
        GraphicsPolygonItem *polygon = dynamic_cast<GraphicsPolygonItem *>(child);

        if (polygon)
        {
            GraphicsPolygonItem *polygonCopy = dynamic_cast<GraphicsPolygonItem *>(polygon->deepCopy());
            if (polygonCopy)
            {
                QGraphicsItem *pItem = dynamic_cast<QGraphicsItem *>(polygonCopy);
                copy->addToGroup(pItem);
                polygonCopy->setStrokesGroup(copy);
                polygonCopy->setStroke(newStroke);
            }
        }
    }
    const_cast<GraphicsStrokesGroup *>(this)->setTransform(groupTransform);
    const_cast<GraphicsStrokesGroup *>(this)->setPos(groupPos);
    copy->setTransform(groupTransform);
    copy->setPos(groupPos);

    return copy;
}

void GraphicsStrokesGroup::copyItemParameters(Item *copy) const
{
    QGraphicsItem *cp = dynamic_cast<QGraphicsItem *>(copy);
    if (NULL != cp)
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
    // 从不画 Qt 自己的选框：选中效果由委托边框画，这里把选中状态摘掉
    QStyleOptionGraphicsItem styleOption  = QStyleOptionGraphicsItem(*option);
    QStyle::State            svState      = option->state;
    styleOption.state                    &= ~QStyle::State_Selected;
    QGraphicsItemGroup::paint(painter, &styleOption, widget);
    // 状态位原样还回去，别让摘掉的选中态泄漏给委托的后绘
    styleOption.state |= svState;

    Delegate()->postpaint(painter, &styleOption, widget);
}

QVariant GraphicsStrokesGroup::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (debugTextEnabled && change == ItemZValueChange)
    {
        double newZ               = qvariant_cast<double>(value);

        GraphicsPolygonItem *poly = NULL;
        if (childItems().size() > 2)
        {
            poly = dynamic_cast<GraphicsPolygonItem *>(childItems()[1]);
        }

        if (poly)
        {
            if (!mDebugText)
            {
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
        foreach (QGraphicsItem *item, childItems())
        {
            path.addPath(item->shape());
        }
    }

    return path;
}
