#include "whiteboard/GraphicsItemZLevelUndoCommand.h"

GraphicsItemZLevelUndoCommand::GraphicsItemZLevelUndoCommand(
    GraphicsScene *_scene, const QList<QGraphicsItem *> &_items, qreal _previousZLevel, ZLayerController::moveDestination dest)
    : UndoCommand()
{
    Q_ASSERT(_scene != NULL);
    mpScene         = _scene;
    mItems          = _items;
    mPreviousZLevel = _previousZLevel;
    mDest           = dest;
    mHack           = false;
}

GraphicsItemZLevelUndoCommand::GraphicsItemZLevelUndoCommand(
    GraphicsScene *_scene, QGraphicsItem *_item, qreal _previousZLevel, ZLayerController::moveDestination dest)
    : UndoCommand()
{
    Q_ASSERT(_scene != NULL);
    mpScene = _scene;
    if (NULL != _item)
    {
        mItems.append(_item);
    }

    mPreviousZLevel = _previousZLevel;
    mDest           = dest;
    mHack           = false;
}

GraphicsItemZLevelUndoCommand::~GraphicsItemZLevelUndoCommand()
{
}

void GraphicsItemZLevelUndoCommand::undo()
{
    if (!mpScene || mItems.empty())
    {
        return;
    }

    // 用最后一项的原始 z 值与记录值之差推算需要反向移动的层数
    qreal zDiff = qAbs(mItems.at(mItems.size() - 1)->data(GraphicsItemData::ItemOwnZValue).toReal() - mPreviousZLevel);

    if (mDest == ZLayerController::down || mDest == ZLayerController::bottom)
    {
        // 原来是下移，撤销时上移；倒序处理以免相互挤占层级
        // end() 是 one-past-last，不能先解引用；倒序处理用前置自减。
        for (auto downIt = mItems.end(); downIt != mItems.begin();)
        {
            --downIt;
            for (int i = 0; i < zDiff; i++)
            {
                mpScene->changeZLevelTo(*downIt, ZLayerController::up);
            }
        }
    }
    else if (mDest == ZLayerController::up || mDest == ZLayerController::top)
    {
        // 原来是上移，撤销时下移；正序处理保持相对顺序
        foreach (QGraphicsItem *item, mItems)
        {
            for (int i = 0; i < zDiff; i++)
            {
                mpScene->changeZLevelTo(item, ZLayerController::down);
            }
        }
    }
}

void GraphicsItemZLevelUndoCommand::redo()
{
    if (!mHack)
    {
        // 入栈时 QUndoStack 会自动调用一次 redo，而层级在创建命令前已调整，跳过首次执行
        mHack = true;
    }
    else
    {
        if (!mpScene)
        {
            return;
        }

        foreach (QGraphicsItem *item, mItems)
        {
            mpScene->changeZLevelTo(item, mDest);
            updateLazyScene();
        }
    }
}

void GraphicsItemZLevelUndoCommand::updateLazyScene()
{
    mpScene->update(mpScene->sceneRect());
    mpScene->updateSelectionFrame();
}
