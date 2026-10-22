#include "whiteboard/GraphicsItemUndoCommand.h"

#include <QtWidgets>

#include "whiteboard/GraphicsScene.h"

#include "whiteboard/BoardController.h"

#include "whiteboard/GraphicsGroupContainerItem.h"
#include "whiteboard/GraphicsPolygonItem.h"

GraphicsItemUndoCommand::GraphicsItemUndoCommand(
    GraphicsScene *pScene, const QSet<QGraphicsItem *> &pRemovedItems, const QSet<QGraphicsItem *> &pAddedItems, const GroupDataTable &groupsMap)
    : UndoCommand(), mScene(pScene), mRemovedItems(pRemovedItems - pAddedItems), mAddedItems(pAddedItems - pRemovedItems),
      mExcludedFromGroup(groupsMap)
{
    mFirstRedo = true;

    // 新增项先冻结、移除项解冻：避免内嵌网页控件在场景增删过程中继续渲染
    QSetIterator<QGraphicsItem *> itAdded(mAddedItems);
    while (itAdded.hasNext())
    {
        BoardController::instance()->freezeWebWidget(itAdded.next(), true);
    }

    QSetIterator<QGraphicsItem *> itRemoved(mRemovedItems);
    while (itRemoved.hasNext())
    {
        BoardController::instance()->freezeWebWidget(itRemoved.next(), false);
    }
}

GraphicsItemUndoCommand::GraphicsItemUndoCommand(GraphicsScene *pScene, QGraphicsItem *pRemovedItem, QGraphicsItem *pAddedItem)
    : UndoCommand(), mScene(pScene)
{
    // 单项版本：任一参数可为空，对应只做删除或只做新增
    if (pRemovedItem)
    {
        mRemovedItems.insert(pRemovedItem);
    }

    if (pAddedItem)
    {
        mAddedItems.insert(pAddedItem);
    }

    mFirstRedo = true;
}

GraphicsItemUndoCommand::~GraphicsItemUndoCommand()
{
}

void GraphicsItemUndoCommand::undo()
{
    if (!mScene)
    {
        return;
    }

    // 1) 撤销“新增”：把当时加入场景的项再移出
    QSetIterator<QGraphicsItem *> itAdded(mAddedItems);
    while (itAdded.hasNext())
    {
        QGraphicsItem *item = itAdded.next();

        BoardController::instance()->freezeWebWidget(item, true);
        item->setSelected(false);

        // 笔迹多边形可能挂在组合下，移出场景前要记住其场景变换并脱离组合，
        // 否则重新加入时会被组合的变换重复应用
        QTransform           t;
        bool                 bApplyTransform = false;
        GraphicsPolygonItem *polygonItem     = qgraphicsitem_cast<GraphicsPolygonItem *>(item);
        if (polygonItem)
        {
            if (polygonItem->strokesGroup() && polygonItem->strokesGroup()->parentItem() &&
                GraphicsGroupContainerItem::Type == polygonItem->strokesGroup()->parentItem()->type())
            {
                bApplyTransform = true;
                t               = polygonItem->sceneTransform();
            }
            else if (polygonItem->strokesGroup())
            {
                polygonItem->resetTransform();
            }

            polygonItem->strokesGroup()->removeFromGroup(polygonItem);
        }
        mScene->removeItem(item);

        if (bApplyTransform)
        {
            polygonItem->setTransform(t);
        }
    }

    // 2) 撤销“删除”：把当时移出的项放回场景
    QSetIterator<QGraphicsItem *> itRemoved(mRemovedItems);
    while (itRemoved.hasNext())
    {
        QGraphicsItem *item = itRemoved.next();
        if (item)
        {
            // 背景项不能走普通 addItem，否则会丢失固定背景标记
            if (ItemLayerType::FixedBackground == item->data(GraphicsItemData::ItemLayerType))
            {
                mScene->setAsBackgroundObject(item);
            }
            else
            {
                mScene->addItem(item);
            }

            // 笔迹多边形重新插回场景后，再挂回其笔迹组
            if (GraphicsPolygonItem::Type == item->type())
            {
                GraphicsPolygonItem *polygonItem = qgraphicsitem_cast<GraphicsPolygonItem *>(item);
                if (polygonItem)
                {
                    mScene->removeItem(polygonItem);
                    mScene->removeItemFromDeletion(polygonItem);
                    polygonItem->strokesGroup()->addToGroup(polygonItem);
                }
            }

            BoardController::instance()->freezeWebWidget(item, false);
        }
    }

    // 3) 恢复组合归属：按 UUID 找到项并重新加入原组合
    QMultiMapIterator<GraphicsGroupContainerItem *, QUuid> curMapElement(mExcludedFromGroup);
    GraphicsGroupContainerItem                            *nextGroup         = NULL;
    GraphicsGroupContainerItem                            *previousGroupItem = NULL;
    bool                                                   groupChanged      = false;

    while (curMapElement.hasNext())
    {
        curMapElement.next();

        groupChanged = previousGroupItem != curMapElement.key();
        // 同一个组合的条目连续排列，只在组合变化时切换当前目标
        if (!nextGroup || groupChanged)
        {
            GraphicsGroupContainerItem *groupCandidate = curMapElement.key();
            if (groupCandidate)
            {
                nextGroup = groupCandidate;
                // 组合本身也可能已被移出场景，需要先放回
                if (!mScene->items().contains(nextGroup))
                {
                    mScene->addItem(nextGroup);
                }
                nextGroup->setVisible(true);
            }
        }

        QGraphicsItem *groupedItem = mScene->itemForUuid(curMapElement.value());
        if (groupedItem)
        {
            nextGroup->addToGroup(groupedItem);
        }

        previousGroupItem = curMapElement.key();
        GraphicsItem::Delegate(nextGroup)->update();
    }

    // 主动刷新：场景的增量重绘在大批量项变化时可能延迟
    mScene->update(mScene->sceneRect());
    mScene->updateSelectionFrame();
}

void GraphicsItemUndoCommand::redo()
{
    // 撤销框架在 push 命令时会立即调用一次 redo，而此前的操作已经生效，
    // 因此首次调用只清除标志，不重复执行
    if (!mFirstRedo)
    {
        if (!mScene)
        {
            return;
        }

        // 1) 先按组合归属把项从组合中移出，再执行场景层面的增删
        QMultiMapIterator<GraphicsGroupContainerItem *, QUuid> curMapElement(mExcludedFromGroup);
        GraphicsGroupContainerItem                            *nextGroup         = NULL;
        GraphicsGroupContainerItem                            *previousGroupItem = NULL;
        bool                                                   groupChanged      = false;

        while (curMapElement.hasNext())
        {
            curMapElement.next();

            groupChanged = previousGroupItem != curMapElement.key();
            if (!nextGroup || groupChanged)
            {
                GraphicsGroupContainerItem *groupCandidate = curMapElement.key();
                if (groupCandidate)
                {
                    nextGroup = groupCandidate;
                }
            }
            QGraphicsItem *groupedItem = mScene->itemForUuid(curMapElement.value());
            if (groupedItem)
            {
                // 组合只剩最后一个成员时直接解散，避免留下空组合
                if (nextGroup->childItems().count() == 1)
                {
                    nextGroup->destroy(false);
                    break;
                }
                nextGroup->removeFromGroup(groupedItem);
            }

            previousGroupItem = curMapElement.key();
            GraphicsItem::Delegate(nextGroup)->update();
        }

        // 2) 重做“删除”
        QSetIterator<QGraphicsItem *> itRemoved(mRemovedItems);
        while (itRemoved.hasNext())
        {
            QGraphicsItem *item = itRemoved.next();
            item->setSelected(false);

            QTransform           t;
            bool                 bApplyTransform = false;
            GraphicsPolygonItem *polygonItem     = qgraphicsitem_cast<GraphicsPolygonItem *>(item);

            if (polygonItem)
            {
                if (polygonItem->strokesGroup() && polygonItem->strokesGroup()->parentItem() &&
                    GraphicsGroupContainerItem::Type == polygonItem->strokesGroup()->parentItem()->type())
                {
                    bApplyTransform = true;
                    t               = polygonItem->sceneTransform();
                }
                else if (polygonItem->strokesGroup())
                {
                    polygonItem->resetTransform();
                }

                polygonItem->strokesGroup()->removeFromGroup(polygonItem);
            }
            mScene->removeItem(item);

            if (bApplyTransform)
            {
                item->setTransform(t);
            }

            BoardController::instance()->freezeWebWidget(item, true);
        }

        // 3) 重做“新增”
        QSetIterator<QGraphicsItem *> itAdded(mAddedItems);
        while (itAdded.hasNext())
        {
            QGraphicsItem *item = itAdded.next();
            if (item)
            {
                BoardController::instance()->freezeWebWidget(item, false);

                if (ItemLayerType::FixedBackground == item->data(GraphicsItemData::ItemLayerType))
                {
                    mScene->setAsBackgroundObject(item);
                }
                else
                {
                    mScene->addItem(item);
                }

                GraphicsPolygonItem *polygonItem = qgraphicsitem_cast<GraphicsPolygonItem *>(item);
                if (polygonItem)
                {
                    mScene->removeItem(polygonItem);
                    mScene->removeItemFromDeletion(polygonItem);
                    polygonItem->strokesGroup()->addToGroup(polygonItem);
                }
            }
        }

        // 主动刷新：场景的增量重绘在大批量项变化时可能延迟
        mScene->update(mScene->sceneRect());
    }
    else
    {
        mFirstRedo = false;
    }
}
