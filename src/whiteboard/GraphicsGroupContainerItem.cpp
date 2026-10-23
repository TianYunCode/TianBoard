#include "whiteboard/GraphicsGroupContainerItem.h"

#include <QtWidgets>

#include "whiteboard/GraphicsMediaItem.h"
#include "whiteboard/GraphicsTextItem.h"
#include "whiteboard/GraphicsItemDelegate.h"
#include "whiteboard/GraphicsGroupContainerItemDelegate.h"
#include "whiteboard/GraphicsScene.h"

GraphicsGroupContainerItem::GraphicsGroupContainerItem(QGraphicsItem *parent) : QGraphicsItem(parent), mCurrentItem(NULL)
{
    setData(GraphicsItemData::ItemLayerType, ItemLayerType::Object);

    setDelegate(new GraphicsGroupContainerItemDelegate(this, 0));

    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);

    GraphicsGroupContainerItem::setAcceptHoverEvents(true);

    setUuid(QUuid::createUuid());

    // 同时写入小写键，场景据图层类型分配 z 值，缺少该项会导致层级排序不正确
    setData(GraphicsItemData::itemLayerType, QVariant(itemLayerType::ObjectItem));
}

GraphicsGroupContainerItem::~GraphicsGroupContainerItem()
{
}

void GraphicsGroupContainerItem::addToGroup(QGraphicsItem *item)
{
    if (!item)
    {
        qWarning("GraphicsGroupContainerItem::addToGroup: 加不了空图元");
        return;
    }
    if (item == this)
    {
        qWarning("GraphicsGroupContainerItem::addToGroup: 不能把组合加进它自己");
        return;
    }

    // 组合的旋转/翻转/锁定能力取所有子项的交集：已有子项时只能收紧，不能放宽
    if (childItems().count())
    {
        if (GraphicsItem::isFlippable(this) && !GraphicsItem::isFlippable(item))
        {
            Delegate()->setFrameFlag(GF_FLIPPABLE_ALL_AXIS, false);
        }
        if (GraphicsItem::isRotatable(this) && !GraphicsItem::isRotatable(item))
        {
            Delegate()->setFrameFlag(GF_REVOLVABLE, false);
        }
        if (!GraphicsItem::isLocked(this) && GraphicsItem::isLocked(item))
        {
            Delegate()->setLocked(true);
        }
    }
    else
    {
        Delegate()->setFrameFlag(GF_FLIPPABLE_ALL_AXIS, GraphicsItem::isFlippable(item));
        Delegate()->setFrameFlag(GF_REVOLVABLE, GraphicsItem::isRotatable(item));
        Delegate()->setLocked(GraphicsItem::isLocked(item));
    }

    // 加入控制类子项后，组合整体也归入控制图层
    if (item->data(GraphicsItemData::ItemLayerType) == ItemLayerType::Control)
    {
        setData(GraphicsItemData::ItemLayerType, item->data(GraphicsItemData::ItemLayerType));
    }

    // 先求出子项到组合的变换，再改变父子关系，保证视觉位置不变
    bool       ok;
    QTransform itemTransform = item->itemTransform(this, &ok);

    if (!ok)
    {
        qWarning("GraphicsGroupContainerItem::addToGroup: 求不出图元到"
                 "组合坐标系的有效变换");
        return;
    }

    // 子项不再单独响应选择与拖动，统一由组合的选择框接管
    item->setSelected(false);
    item->setFlag(QGraphicsItem::ItemIsSelectable, false);
    item->setFlag(QGraphicsItem::ItemIsMovable, false);
    item->setFlag(QGraphicsItem::ItemIsFocusable, true);

    QTransform newItemTransform(itemTransform);
    item->setPos(mapFromItem(item, 0, 0));

    if (item->scene())
    {
        item->scene()->removeItem(item);
    }

    // 从待删除列表中移除，避免场景随后把刚加入组合的项删掉
    if (corescene())
    {
        corescene()->removeItemFromDeletion(item);
    }
    item->setParentItem(this);

    // pos 已经承载了平移，变换里要把这部分减掉，否则会重复平移
    if (!item->pos().isNull())
    {
        newItemTransform *= QTransform::fromTranslate(-item->x(), -item->y());
    }

    // 再把旋转、缩放、变换列表、变换原点等已经被 setPos/setRotation 等表达的属性剥离
    QPointF                     origin = item->transformOriginPoint();
    QMatrix4x4                  m;
    QList<QGraphicsTransform *> transformList = item->transformations();
    for (int i = 0; i < transformList.size(); ++i)
    {
        transformList.at(i)->applyTo(&m);
    }
    newItemTransform *= m.toTransform().inverted();
    newItemTransform.translate(origin.x(), origin.y());
    newItemTransform.rotate(-item->rotation());
    newItemTransform.scale(1 / item->scale(), 1 / item->scale());
    newItemTransform.translate(-origin.x(), -origin.y());

    // ### 开销较大，或可借助 dirtySceneTransform 标志位优化

    item->setTransform(newItemTransform);
    prepareGeometryChange();
    // 组合自身不参与绘制，包围盒靠累加子项的包围盒维持
    itemsBoundingRect |= itemTransform.mapRect(item->boundingRect() | item->childrenBoundingRect());
    update();
}
void GraphicsGroupContainerItem::removeFromGroup(QGraphicsItem *item)
{
    if (!item)
    {
        qDebug() << "传进来的图元是空指针，无法移出组合";
        return;
    }

    // 先登记到待删除列表，移出组合的过程中场景不会立即回收它
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
    // 只有媒体项需要额外的工具栏收尾，其他子项无需处理
    if (mCurrentItem && (mCurrentItem->type() == GraphicsMediaItem::Type || mCurrentItem->type() == GraphicsVideoItem::Type ||
                            mCurrentItem->type() == GraphicsAudioItem::Type))
    {
        dynamic_cast<GraphicsMediaItem *>(mCurrentItem)->Delegate()->getToolBarItem()->hide();

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

    // 组合本身无内容，绘制交给委托（画选择框）
    Delegate()->postpaint(painter, option, widget);
}

CoreGraphicsScene *GraphicsGroupContainerItem::corescene()
{
    CoreGraphicsScene *castScene = dynamic_cast<CoreGraphicsScene *>(QGraphicsItem::scene());

    return castScene;
}

GraphicsGroupContainerItem *GraphicsGroupContainerItem::deepCopyNoChildDuplication() const
{
    GraphicsGroupContainerItem *copy = new GraphicsGroupContainerItem();

    // 子项尚未复制，保留原 UUID 以便调用方把副本与原组合对应起来
    copy->setUuid(this->uuid());

    copyItemParameters(copy);

    return copy;
}

GraphicsGroupContainerItem *GraphicsGroupContainerItem::deepCopy() const
{
    GraphicsGroupContainerItem *copy = new GraphicsGroupContainerItem();

    copy->setUuid(this->uuid());

    foreach (QGraphicsItem *it, childItems())
    {
        Item *childAsItem = dynamic_cast<Item *>(it);
        if (childAsItem)
        {
            QGraphicsItem *cloneItem = dynamic_cast<QGraphicsItem *>(childAsItem->deepCopy());
            copy->addToGroup(cloneItem);
        }
    }
    copyItemParameters(copy);

    return copy;
}

void GraphicsGroupContainerItem::copyItemParameters(Item *copy) const
{
    GraphicsGroupContainerItem *cp = dynamic_cast<GraphicsGroupContainerItem *>(copy);
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
    // 同时存一份到图形项数据，场景按 UUID 查找项时可直接读取，无需转换类型
    setData(GraphicsItemData::ItemUuid, QVariant(pUuid));
}

void GraphicsGroupContainerItem::destroy(bool canUndo)
{
    // 先逐个解除父子关系并恢复交互标志，再删除空组合
    foreach (QGraphicsItem *item, childItems())
    {
        pRemoveFromGroup(item);
        item->setFlag(QGraphicsItem::ItemIsSelectable, true);
        item->setFlag(QGraphicsItem::ItemIsFocusable, true);
    }

    remove(canUndo);
}

void GraphicsGroupContainerItem::clearSource()
{
    foreach (QGraphicsItem *child, childItems())
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
    if (Delegate()->mousePressEvent(event))
    {
        // 有意为空：委托已消费该事件，无需再走基类逻辑
    }
    else
    {
        // 委托未接管时按普通图形项处理，并确保组合被选中
        QGraphicsItem::mousePressEvent(event);
        setSelected(true);
    }
}

void GraphicsGroupContainerItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate()->mouseMoveEvent(event))
    {
        // 有意为空：委托已消费该事件，无需再走基类逻辑
    }
    else
    {
        QGraphicsItem::mouseMoveEvent(event);
    }
}

void GraphicsGroupContainerItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mouseReleaseEvent(event);
}

QVariant GraphicsGroupContainerItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    QVariant newValue = Delegate()->itemChange(change, value);

    // 组合移动或变化后，子项的选择框要跟着重新摆放
    foreach (QGraphicsItem *child, childItems())
    {
        GraphicsItem *item = dynamic_cast<GraphicsItem *>(child);
        if (item)
        {
            item->Delegate()->positionHandles();
        }
    }

    // 选中状态变化时释放媒体子项的交互状态
    if (QGraphicsItem::ItemSelectedChange == change)
    {
        deselectCurrentItem();
    }

    return QGraphicsItem::itemChange(change, newValue);
}

void GraphicsGroupContainerItem::pRemoveFromGroup(QGraphicsItem *item)
{
    if (!item)
    {
        qWarning("QGraphicsItemGroup::removeFromGroup: 移不出空图元");
        return;
    }

    QGraphicsItem *newParent = parentItem();

    // 移出后重新计算剩余子项的公共能力，允许恢复旋转/翻转
    if (childItems().count())
    {
        if (!GraphicsItem::isFlippable(item) || !GraphicsItem::isRotatable(item))
        {
            bool flippableNow = true;
            bool rotatableNow = true;
            bool lockedNow    = false;

            foreach (QGraphicsItem *child, childItems())
            {
                // 此刻 setParentItem 还没发生，childItems() 里仍含被移出的那个 item；
                // 这里算的是"剩余子项"的公共能力，不排掉它就永远把离开的子项算在内，
                // 结果能力只会越算越紧、再也不会恢复。
                if (child == item)
                {
                    continue;
                }
                if (!GraphicsItem::isFlippable(child))
                {
                    flippableNow = false;
                }
                if (!GraphicsItem::isRotatable(child))
                {
                    rotatableNow = false;
                }
                if (GraphicsItem::isLocked(child))
                {
                    lockedNow = true;
                }

                if (!rotatableNow && !flippableNow && lockedNow)
                {
                    break;
                }
            }
            Delegate()->setFrameFlag(GF_FLIPPABLE_ALL_AXIS, flippableNow);
            Delegate()->setFrameFlag(GF_REVOLVABLE, rotatableNow);
            Delegate()->setLocked(lockedNow);
        }
    }

    // 先求子项到新父项的变换，再换父项并用地图像素位置保持视觉位置
    bool       ok;
    QTransform itemTransform;
    if (newParent)
    {
        itemTransform = item->itemTransform(newParent, &ok);
    }
    else
    {
        itemTransform = item->sceneTransform();
    }

    QPointF oldPos = item->mapToItem(newParent, 0, 0);
    item->setParentItem(newParent);
    item->setPos(oldPos);

    GraphicsScene *Scene = dynamic_cast<GraphicsScene *>(item->scene());
    if (Scene)
    {
        Scene->addItem(item);
    }

    // 从新变换的平移分量中扣除位置
    if (!item->pos().isNull())
    {
        itemTransform *= QTransform::fromTranslate(-item->x(), -item->y());
    }

    // 剥离 itemTransform()/sceneTransform() 附带的额外变换属性
    // （旋转、缩放、变换列表、变换原点）
    QPointF                     origin = item->transformOriginPoint();
    QMatrix4x4                  m;
    QList<QGraphicsTransform *> transformList = item->transformations();
    for (int i = 0; i < transformList.size(); ++i)
    {
        transformList.at(i)->applyTo(&m);
    }
    itemTransform *= m.toTransform().inverted();
    itemTransform.translate(origin.x(), origin.y());
    itemTransform.rotate(-item->rotation());
    itemTransform.scale(1 / item->scale(), 1 / item->scale());
    itemTransform.translate(-origin.x(), -origin.y());

    // ### 开销较大，或可借助 dirtySceneTransform 标志位优化

    item->setTransform(itemTransform);

    // ### 开销不小，不过 removeFromGroup() 很少被调用
    // 移出后组合不再需要累加维护包围盒，直接按剩余子项重算
    prepareGeometryChange();
    itemsBoundingRect = childrenBoundingRect();

    item->setFlag(ItemIsMovable, true);
}
