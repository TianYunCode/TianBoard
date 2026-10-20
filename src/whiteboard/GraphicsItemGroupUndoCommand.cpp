#include "whiteboard/GraphicsItemGroupUndoCommand.h"

#include "whiteboard/GraphicsGroupContainerItem.h"
#include "whiteboard/GraphicsScene.h"


GraphicsItemGroupUndoCommand::GraphicsItemGroupUndoCommand(GraphicsScene *pScene,
                                                           GraphicsGroupContainerItem *pGroupCreated)
    : UndoCommand(), mScene(pScene), mGroup(pGroupCreated), mFirstRedo(true) {
    // 缓存当前成员；undo 解散后靠这份列表重建组合
    if (pGroupCreated->childItems().count()) {
        foreach (QGraphicsItem *item, pGroupCreated->childItems()) {
            mItems << item;
        }
    }
}

GraphicsItemGroupUndoCommand::~GraphicsItemGroupUndoCommand() {}

void GraphicsItemGroupUndoCommand::undo() {
    // destroy(false) 不进入撤销栈，避免在撤销过程中再生成命令
    mGroup->destroy(false);
    foreach (QGraphicsItem *item, mItems) {
        item->setSelected(true);
    }
}

void GraphicsItemGroupUndoCommand::redo() {
    if (mFirstRedo) {
        // 入栈时 QUndoStack 会自动调用一次 redo，而组合在创建时已经生效，
        // 这里跳过首次执行，避免重复加入
        mFirstRedo = false;
        return;
    }

    // 成员中可能还有嵌套组合，需要先解散它并把其子项提升到当前组合
    foreach (QGraphicsItem *item, mItems) {
        if (item->type() == GraphicsGroupContainerItem::Type) {
            QList<QGraphicsItem *> childItems = item->childItems();
            GraphicsGroupContainerItem *currentGroup = dynamic_cast<GraphicsGroupContainerItem *>(item);
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
