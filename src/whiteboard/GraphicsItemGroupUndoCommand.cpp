#include "whiteboard/GraphicsItemGroupUndoCommand.h"

#include "whiteboard/GraphicsGroupContainerItem.h"
#include "whiteboard/GraphicsScene.h"


GraphicsItemGroupUndoCommand::GraphicsItemGroupUndoCommand(GraphicsScene *pScene, GraphicsGroupContainerItem *pGroupCreated) : UndoCommand()
  , mScene (pScene)
  , mGroup(pGroupCreated)
  , mFirstRedo(true)
{
    if (pGroupCreated->childItems().count()) {
        foreach (QGraphicsItem *item, pGroupCreated->childItems()) {
            mItems << item;
        }
    }
}

GraphicsItemGroupUndoCommand::~GraphicsItemGroupUndoCommand()
{
}

void GraphicsItemGroupUndoCommand::undo()
{
    mGroup->destroy(false);
    foreach(QGraphicsItem *item, mItems) {
        item->setSelected(true);
    }
}

void GraphicsItemGroupUndoCommand::redo()
{
    if (mFirstRedo) {
        //Work around. TODO determine why does Qt call the redo function on pushing to undo
        mFirstRedo = false;
        return;
    }

    foreach (QGraphicsItem *item, mItems) {
        if (item->type() == GraphicsGroupContainerItem::Type) {
            QList<QGraphicsItem*> childItems = item->childItems();
            GraphicsGroupContainerItem *currentGroup = dynamic_cast<GraphicsGroupContainerItem*>(item);
            if (currentGroup) {
                currentGroup->destroy(false);
            }
            foreach (QGraphicsItem *chItem, childItems) {
                mGroup->addToGroup(chItem);
            }
        } else {
            mGroup->addToGroup(item);
        }
    }

    mScene->addItem(mGroup);
    mGroup->setVisible(true);
    mGroup->setFocus();
    mGroup->setSelected(true);
}
