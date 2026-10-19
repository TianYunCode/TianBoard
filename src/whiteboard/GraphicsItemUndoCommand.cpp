#include "whiteboard/GraphicsItemUndoCommand.h"

#include <QtWidgets>

#include "whiteboard/GraphicsScene.h"

#include "core/Application.h"

#include "whiteboard/BoardController.h"

#include "whiteboard/GraphicsGroupContainerItem.h"
#include "whiteboard/GraphicsPolygonItem.h"

GraphicsItemUndoCommand::GraphicsItemUndoCommand(GraphicsScene *pScene,
                                                 const QSet<QGraphicsItem *> &pRemovedItems,
                                                 const QSet<QGraphicsItem *> &pAddedItems,
                                                 const GroupDataTable &groupsMap)
    : UndoCommand(),
      mScene(pScene),
      mRemovedItems(pRemovedItems - pAddedItems),
      mAddedItems(pAddedItems - pRemovedItems),
      mExcludedFromGroup(groupsMap) {
    mFirstRedo = true;

    QSetIterator<QGraphicsItem *> itAdded(mAddedItems);
    while (itAdded.hasNext()) {
        Application::boardController->freezeWebWidget(itAdded.next(), true);
    }

    QSetIterator<QGraphicsItem *> itRemoved(mRemovedItems);
    while (itRemoved.hasNext()) {
        Application::boardController->freezeWebWidget(itRemoved.next(), false);
    }
}

GraphicsItemUndoCommand::GraphicsItemUndoCommand(GraphicsScene *pScene, QGraphicsItem *pRemovedItem,
                                                 QGraphicsItem *pAddedItem)
    : UndoCommand(), mScene(pScene) {
    if (pRemovedItem) {
        mRemovedItems.insert(pRemovedItem);
    }

    if (pAddedItem) {
        mAddedItems.insert(pAddedItem);
    }

    mFirstRedo = true;
}

GraphicsItemUndoCommand::~GraphicsItemUndoCommand() {
    //NOOP
}

void GraphicsItemUndoCommand::undo() {
    if (!mScene) {
        return;
    }

    QSetIterator<QGraphicsItem *> itAdded(mAddedItems);
    while (itAdded.hasNext()) {
        QGraphicsItem *item = itAdded.next();

        Application::boardController->freezeWebWidget(item, true);
        item->setSelected(false);

        QTransform t;
        bool bApplyTransform = false;
        GraphicsPolygonItem *polygonItem = qgraphicsitem_cast<GraphicsPolygonItem *>(item);
        if (polygonItem) {
            if (polygonItem->strokesGroup() && polygonItem->strokesGroup()->parentItem() &&
                GraphicsGroupContainerItem::Type == polygonItem->strokesGroup()->parentItem()->type()) {
                bApplyTransform = true;
                t = polygonItem->sceneTransform();
            } else if (polygonItem->strokesGroup())
                polygonItem->resetTransform();

            polygonItem->strokesGroup()->removeFromGroup(polygonItem);
        }
        mScene->removeItem(item);

        if (bApplyTransform)
            polygonItem->setTransform(t);
    }

    QSetIterator<QGraphicsItem *> itRemoved(mRemovedItems);
    while (itRemoved.hasNext()) {
        QGraphicsItem *item = itRemoved.next();
        if (item) {
            if (ItemLayerType::FixedBackground == item->data(GraphicsItemData::ItemLayerType))
                mScene->setAsBackgroundObject(item);
            else
                mScene->addItem(item);

            if (GraphicsPolygonItem::Type == item->type()) {
                GraphicsPolygonItem *polygonItem = qgraphicsitem_cast<GraphicsPolygonItem *>(item);
                if (polygonItem) {
                    mScene->removeItem(polygonItem);
                    mScene->removeItemFromDeletion(polygonItem);
                    polygonItem->strokesGroup()->addToGroup(polygonItem);
                }
            }

            Application::boardController->freezeWebWidget(item, false);
        }
    }

    QMultiMapIterator<GraphicsGroupContainerItem *, QUuid> curMapElement(mExcludedFromGroup);
    GraphicsGroupContainerItem *nextGroup = NULL;
    GraphicsGroupContainerItem *previousGroupItem = NULL;
    bool groupChanged = false;

    while (curMapElement.hasNext()) {
        curMapElement.next();

        groupChanged = previousGroupItem != curMapElement.key();
        //trying to find the group on the scene;
        if (!nextGroup || groupChanged) {
            GraphicsGroupContainerItem *groupCandidate = curMapElement.key();
            if (groupCandidate) {
                nextGroup = groupCandidate;
                if (!mScene->items().contains(nextGroup)) {
                    mScene->addItem(nextGroup);
                }
                nextGroup->setVisible(true);
            }
        }

        QGraphicsItem *groupedItem = mScene->itemForUuid(curMapElement.value());
        if (groupedItem) {
            nextGroup->addToGroup(groupedItem);
        }

        previousGroupItem = curMapElement.key();
        GraphicsItem::Delegate(nextGroup)->update();
    }

    // force refresh, QT is a bit lazy and take a lot of time (nb item ^2 ?) to trigger repaint
    mScene->update(mScene->sceneRect());
    mScene->updateSelectionFrame();
}

void GraphicsItemUndoCommand::redo() {
    // the Undo framework calls a redo while appending the undo command.
    // as we have already plotted the elements, we do not want to do it twice
    if (!mFirstRedo) {
        if (!mScene) {
            return;
        }

        QMultiMapIterator<GraphicsGroupContainerItem *, QUuid> curMapElement(mExcludedFromGroup);
        GraphicsGroupContainerItem *nextGroup = NULL;
        GraphicsGroupContainerItem *previousGroupItem = NULL;
        bool groupChanged = false;

        while (curMapElement.hasNext()) {
            curMapElement.next();

            groupChanged = previousGroupItem != curMapElement.key();
            //trying to find the group on the scene;
            if (!nextGroup || groupChanged) {
                GraphicsGroupContainerItem *groupCandidate = curMapElement.key();
                if (groupCandidate) {
                    nextGroup = groupCandidate;
                }
            }
            QGraphicsItem *groupedItem = mScene->itemForUuid(curMapElement.value());
            if (groupedItem) {
                if (nextGroup->childItems().count() == 1) {
                    nextGroup->destroy(false);
                    break;
                }
                nextGroup->removeFromGroup(groupedItem);
            }

            previousGroupItem = curMapElement.key();
            GraphicsItem::Delegate(nextGroup)->update();
        }

        QSetIterator<QGraphicsItem *> itRemoved(mRemovedItems);
        while (itRemoved.hasNext()) {
            QGraphicsItem *item = itRemoved.next();
            item->setSelected(false);

            QTransform t;
            bool bApplyTransform = false;
            GraphicsPolygonItem *polygonItem = qgraphicsitem_cast<GraphicsPolygonItem *>(item);

            if (polygonItem) {
                if (polygonItem->strokesGroup() && polygonItem->strokesGroup()->parentItem() &&
                    GraphicsGroupContainerItem::Type == polygonItem->strokesGroup()->parentItem()->type()) {
                    bApplyTransform = true;
                    t = polygonItem->sceneTransform();
                } else if (polygonItem->strokesGroup())
                    polygonItem->resetTransform();

                polygonItem->strokesGroup()->removeFromGroup(polygonItem);
            }
            mScene->removeItem(item);

            if (bApplyTransform)
                item->setTransform(t);

            Application::boardController->freezeWebWidget(item, true);
        }

        QSetIterator<QGraphicsItem *> itAdded(mAddedItems);
        while (itAdded.hasNext()) {
            QGraphicsItem *item = itAdded.next();
            if (item) {
                Application::boardController->freezeWebWidget(item, false);

                if (ItemLayerType::FixedBackground == item->data(GraphicsItemData::ItemLayerType))
                    mScene->setAsBackgroundObject(item);
                else
                    mScene->addItem(item);

                GraphicsPolygonItem *polygonItem = qgraphicsitem_cast<GraphicsPolygonItem *>(item);
                if (polygonItem) {
                    mScene->removeItem(polygonItem);
                    mScene->removeItemFromDeletion(polygonItem);
                    polygonItem->strokesGroup()->addToGroup(polygonItem);
                }
            }
        }

        // force refresh, QT is a bit lazy and take a lot of time (nb item ^2) to trigger repaint
        mScene->update(mScene->sceneRect());
    } else {
        mFirstRedo = false;
    }
}
