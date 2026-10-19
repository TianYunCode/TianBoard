#include "whiteboard/GraphicsGroupContainerItem.h"

#include <QtWidgets>

#include "whiteboard/GraphicsMediaItem.h"
#include "whiteboard/GraphicsTextItem.h"
#include "whiteboard/GraphicsItemDelegate.h"
#include "whiteboard/GraphicsGroupContainerItemDelegate.h"
#include "whiteboard/GraphicsScene.h"


GraphicsGroupContainerItem::GraphicsGroupContainerItem(QGraphicsItem *parent)
    : QGraphicsItem(parent)
    , mCurrentItem(NULL)
{
    setData(GraphicsItemData::ItemLayerType, ItemLayerType::Object);

    setDelegate(new GraphicsGroupContainerItemDelegate(this, 0));

    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);

    GraphicsGroupContainerItem::setAcceptHoverEvents(true);

    setUuid(QUuid::createUuid());

    setData(GraphicsItemData::itemLayerType, QVariant(itemLayerType::ObjectItem)); //Necessary to set if we want z value to be assigned correctly
}

GraphicsGroupContainerItem::~GraphicsGroupContainerItem()
{
}

void GraphicsGroupContainerItem::addToGroup(QGraphicsItem *item)
{
    if (!item) {
        qWarning("GraphicsGroupContainerItem::addToGroup: cannot add null item");
        return;
    }
    if (item == this) {
        qWarning("GraphicsGroupContainerItem::addToGroup: cannot add a group to itself");
        return;
    }

    //Check if group is allready rotatable or flippable
    if (childItems().count()) {
        if (GraphicsItem::isFlippable(this) && !GraphicsItem::isFlippable(item)) {
            Delegate()->setFrameFlag(GF_FLIPPABLE_ALL_AXIS, false);
        }
        if (GraphicsItem::isRotatable(this) && !GraphicsItem::isRotatable(item)) {
            Delegate()->setFrameFlag(GF_REVOLVABLE, false);
        }
        if (!GraphicsItem::isLocked(this) && GraphicsItem::isLocked(item)) {
            Delegate()->setLocked(true);
        }
    }
    else {
        Delegate()->setFrameFlag(GF_FLIPPABLE_ALL_AXIS, GraphicsItem::isFlippable(item));
        Delegate()->setFrameFlag(GF_REVOLVABLE, GraphicsItem::isRotatable(item));
        Delegate()->setLocked(GraphicsItem::isLocked(item));
    }        

    if (item->data(GraphicsItemData::ItemLayerType) == ItemLayerType::Control)
        setData(GraphicsItemData::ItemLayerType, item->data(GraphicsItemData::ItemLayerType));

    // COMBINE
    bool ok;
    QTransform itemTransform = item->itemTransform(this, &ok);

    if (!ok) {
        qWarning("GraphicsGroupContainerItem::addToGroup: could not find a valid transformation from item to group coordinates");
        return;
    }

    //setting item flags to given item
    item->setSelected(false);
    item->setFlag(QGraphicsItem::ItemIsSelectable, false);
    item->setFlag( QGraphicsItem::ItemIsMovable, false);
    item->setFlag(QGraphicsItem::ItemIsFocusable, true);

    QTransform newItemTransform(itemTransform);
    item->setPos(mapFromItem(item, 0, 0));

    if (item->scene()) {
        item->scene()->removeItem(item);
    }

    if (corescene())
        corescene()->removeItemFromDeletion(item);
    item->setParentItem(this);

    // removing position from translation component of the new transform
    if (!item->pos().isNull())
        newItemTransform *= QTransform::fromTranslate(-item->x(), -item->y());

    // removing additional transformations properties applied with itemTransform()
    QPointF origin = item->transformOriginPoint();
    QMatrix4x4 m;
    QList<QGraphicsTransform*> transformList = item->transformations();
    for (int i = 0; i < transformList.size(); ++i)
        transformList.at(i)->applyTo(&m);
    newItemTransform *= m.toTransform().inverted();
    newItemTransform.translate(origin.x(), origin.y());
    newItemTransform.rotate(-item->rotation());
    newItemTransform.scale(1/item->scale(), 1/item->scale());
    newItemTransform.translate(-origin.x(), -origin.y());

    // ### Expensive, we could maybe use dirtySceneTransform bit for optimization

    item->setTransform(newItemTransform);
    //    item->d_func()->setIsMemberOfGroup(true);
    prepareGeometryChange();
    itemsBoundingRect |= itemTransform.mapRect(item->boundingRect() | item->childrenBoundingRect());
    update();
}
void GraphicsGroupContainerItem::removeFromGroup(QGraphicsItem *item)
{
    if (!item) {
        qDebug() << "can't specify the item because of the null pointer";
        return;
    }

    CoreGraphicsScene *groupScene = corescene();
    if (groupScene)
    {
        groupScene->addItemToDeletion(item);
    }

    pRemoveFromGroup(item);

    item->setFlags(ItemIsSelectable | ItemIsFocusable);

}

void GraphicsGroupContainerItem::deselectCurrentItem()
{
    if (mCurrentItem && (mCurrentItem->type() == GraphicsMediaItem::Type
                         || mCurrentItem->type() == GraphicsVideoItem::Type
                         || mCurrentItem->type() == GraphicsAudioItem::Type))
    {
        dynamic_cast<GraphicsMediaItem*>(mCurrentItem)->Delegate()->getToolBarItem()->hide();

        mCurrentItem->setSelected(false);
        mCurrentItem = NULL;
    }
}

QRectF GraphicsGroupContainerItem::boundingRect() const
{
    return itemsBoundingRect;
}

void GraphicsGroupContainerItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget);
    Q_UNUSED(painter);
    Q_UNUSED(option);

    Delegate()->postpaint(painter, option, widget);
}

CoreGraphicsScene *GraphicsGroupContainerItem::corescene()
{
    CoreGraphicsScene *castScene = dynamic_cast<CoreGraphicsScene*>(QGraphicsItem::scene());

    return castScene;
}

GraphicsGroupContainerItem *GraphicsGroupContainerItem::deepCopyNoChildDuplication() const
{
    GraphicsGroupContainerItem *copy = new GraphicsGroupContainerItem();

    copy->setUuid(this->uuid()); // this is OK for now as long as Widgets are imutable

    copyItemParameters(copy);

    return copy;
}


GraphicsGroupContainerItem *GraphicsGroupContainerItem::deepCopy() const
{
    GraphicsGroupContainerItem *copy = new GraphicsGroupContainerItem();

    copy->setUuid(this->uuid()); // this is OK for now as long as Widgets are imutable

    foreach (QGraphicsItem *it, childItems()) {
        Item *childAsItem = dynamic_cast<Item*>(it);
        if (childAsItem) {
            QGraphicsItem *cloneItem = dynamic_cast<QGraphicsItem*>(childAsItem->deepCopy());
            copy->addToGroup(cloneItem);
        }
    }
    copyItemParameters(copy);

    return copy;
}



void GraphicsGroupContainerItem::copyItemParameters(Item *copy) const
{
    GraphicsGroupContainerItem *cp = dynamic_cast<GraphicsGroupContainerItem*>(copy);
    if (cp)
    {
        cp->setPos(this->pos());
        cp->setTransform(this->transform());
        cp->setFlag(QGraphicsItem::ItemIsMovable, true);
        cp->setFlag(QGraphicsItem::ItemIsSelectable, true);
        cp->setData(GraphicsItemData::ItemLayerType, this->data(GraphicsItemData::ItemLayerType));
        cp->setData(GraphicsItemData::ItemLocked, this->data(GraphicsItemData::ItemLocked));
    }
}

void GraphicsGroupContainerItem::setUuid(const QUuid &pUuid)
{
    Item::setUuid(pUuid);
    setData(GraphicsItemData::ItemUuid, QVariant(pUuid)); //store item uuid inside the QGraphicsItem to fast operations with Items on the scene
}

void GraphicsGroupContainerItem::destroy(bool canUndo) {

    foreach (QGraphicsItem *item, childItems()) {
        pRemoveFromGroup(item);
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        item->setFlag(QGraphicsItem::ItemIsFocusable, true);
    }

    remove(canUndo);
}

void GraphicsGroupContainerItem::clearSource()
{
    foreach(QGraphicsItem *child, childItems())
    {
        GraphicsItem *item = dynamic_cast<GraphicsItem *>(child);
        if (item)
        {
            item->clearSource();
        }
    }
}

void GraphicsGroupContainerItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate()->mousePressEvent(event)) {
        //NOOP
    } else {

    QGraphicsItem::mousePressEvent(event);
        setSelected(true);
    }


}

void GraphicsGroupContainerItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate()->mouseMoveEvent(event)) {
        // NOOP;
    } else {
        QGraphicsItem::mouseMoveEvent(event);
    }

}

void GraphicsGroupContainerItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
//    mDelegate->mouseReleaseEvent(event);
    QGraphicsItem::mouseReleaseEvent(event);
}

QVariant GraphicsGroupContainerItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    QVariant newValue = Delegate()->itemChange(change, value);

    foreach(QGraphicsItem *child, childItems())
    {
        GraphicsItem *item = dynamic_cast<GraphicsItem*>(child);
        if (item)
        {
            item->Delegate()->positionHandles();
        }
    }

    if (QGraphicsItem::ItemSelectedChange == change)
    {
        deselectCurrentItem();
    }

    return QGraphicsItem::itemChange(change, newValue);
}

void GraphicsGroupContainerItem::pRemoveFromGroup(QGraphicsItem *item)
{
    if (!item) {
        qWarning("QGraphicsItemGroup::removeFromGroup: cannot remove null item");
        return;
    }

    QGraphicsItem *newParent = parentItem();

    if (childItems().count()) {
        if (!GraphicsItem::isFlippable(item) || !GraphicsItem::isRotatable(item)) {
            bool flippableNow = true;
            bool rotatableNow = true;
            bool lockedNow = false;

            foreach (QGraphicsItem *item, childItems()) {
                if (!GraphicsItem::isFlippable(item)) {
                    flippableNow = false;
                }
                if (!GraphicsItem::isRotatable(item)) {
                    rotatableNow = false;
                }
                if(GraphicsItem::isLocked(item))
                    lockedNow = true;

                if (!rotatableNow && !flippableNow && lockedNow) {
                    break;
                }

            }
            Delegate()->setFrameFlag(GF_FLIPPABLE_ALL_AXIS, flippableNow);
            Delegate()->setFrameFlag(GF_REVOLVABLE, rotatableNow);
            Delegate()->setLocked(lockedNow);
        }
    }

    // COMBINE
    bool ok;
    QTransform itemTransform;
    if (newParent)
        itemTransform = item->itemTransform(newParent, &ok);
    else
        itemTransform = item->sceneTransform();

    QPointF oldPos = item->mapToItem(newParent, 0, 0);
    item->setParentItem(newParent);
    item->setPos(oldPos);

    GraphicsScene *Scene = dynamic_cast<GraphicsScene *>(item->scene());
    if (Scene)
    {
        Scene->addItem(item);
    }

    // removing position from translation component of the new transform
    if (!item->pos().isNull())
        itemTransform *= QTransform::fromTranslate(-item->x(), -item->y());

    // removing additional transformations properties applied
    // with itemTransform() or sceneTransform()
    QPointF origin = item->transformOriginPoint();
    QMatrix4x4 m;
    QList<QGraphicsTransform*> transformList = item->transformations();
    for (int i = 0; i < transformList.size(); ++i)
        transformList.at(i)->applyTo(&m);
    itemTransform *= m.toTransform().inverted();
    itemTransform.translate(origin.x(), origin.y());
    itemTransform.rotate(-item->rotation());
    itemTransform.scale(1 / item->scale(), 1 / item->scale());
    itemTransform.translate(-origin.x(), -origin.y());

    // ### Expensive, we could maybe use dirtySceneTransform bit for optimization

    item->setTransform(itemTransform);
//    item->d_func()->setIsMemberOfGroup(item->group() != 0);

    // ### Quite expensive. But removeFromGroup() isn't called very often.
    prepareGeometryChange();
    itemsBoundingRect = childrenBoundingRect();

    item->setFlag(ItemIsMovable, true);
}
