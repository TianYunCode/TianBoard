#include "utils/CoreGraphicsScene.h"

#include "whiteboard/GraphicsMediaItem.h"
#include "whiteboard/GraphicsGroupContainerItem.h"


CoreGraphicsScene::CoreGraphicsScene(QObject * parent)
    : QGraphicsScene ( parent  )
    , mIsModified(true)
{
    //NOOP
}

CoreGraphicsScene::~CoreGraphicsScene()
{
    //we must delete removed items that are no more in any scene
    //at groups deleting some items can be added to mItemsToDelete, so we need to use iterators.
    foreach(QGraphicsItem* item, mItemsToDelete)
    {
        if (item)
        {
            if (item->scene() == NULL || item->scene() == this)
            {
                delete item;
                item = NULL;
            }
        }
    }
    mItemsToDelete.clear();
}

void CoreGraphicsScene::addItem(QGraphicsItem* item)
{
    addItemToDeletion(item);

    if (item->type() == GraphicsGroupContainerItem::Type && item->childItems().count()) {
        foreach (QGraphicsItem *curItem, item->childItems()) {
            removeItemFromDeletion(curItem);
        }
    }

    if (item->scene() != this)
        QGraphicsScene::addItem(item);

    setModified(true);
}


void CoreGraphicsScene::removeItem(QGraphicsItem* item, bool forceDelete)
{
    QGraphicsScene::removeItem(item);
    if (forceDelete)
    {
        deleteItem(item);
    }
    setModified(true);
}

bool CoreGraphicsScene::deleteItem(QGraphicsItem* item)
{
    if(mItemsToDelete.contains(item))
    {
        GraphicsItem *item_casted = dynamic_cast<GraphicsItem *>(item);
        if (item_casted != NULL)
            item_casted->clearSource();

        mItemsToDelete.remove(item);
        delete item;
        item = NULL;
        return true;
    }
    else
        return false;
}

void CoreGraphicsScene::removeItemFromDeletion(QGraphicsItem *item)
{
    if(NULL != item){
        mItemsToDelete.remove(item);
    }
}

void CoreGraphicsScene::addItemToDeletion(QGraphicsItem *item)
{
    if (item) {
        mItemsToDelete.insert(item);
    }
}
