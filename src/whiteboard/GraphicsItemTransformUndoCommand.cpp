#include "whiteboard/GraphicsItemTransformUndoCommand.h"
#include "whiteboard/ResizableGraphicsItem.h"
#include "whiteboard/Item.h"
#include "whiteboard/GraphicsScene.h"


GraphicsItemTransformUndoCommand::GraphicsItemTransformUndoCommand(
    QGraphicsItem *pItem, const QPointF &prevPos, const QTransform &prevTransform, const qreal &prevZValue,
    const QSizeF &prevSize, bool setToBackground)
    : UndoCommand() {
    mItem = pItem;
    // 旧值来自调用方快照，当前值在这里立即读取，两者共同构成一次撤销记录
    mPreviousTransform = prevTransform;
    mCurrentTransform = pItem->transform();

    mPreviousPosition = prevPos;
    mCurrentPosition = pItem->pos();

    mPreviousZValue = prevZValue;
    mCurrentZValue = pItem->zValue();

    mPreviousSize = prevSize;
    ResizableGraphicsItem *resizableItem = dynamic_cast<ResizableGraphicsItem *>(pItem);

    // 不支持改变尺寸的项不记录尺寸
    if (resizableItem)
        mCurrentSize = resizableItem->size();

    mSetToBackground = setToBackground;
}

GraphicsItemTransformUndoCommand::~GraphicsItemTransformUndoCommand() {}

void GraphicsItemTransformUndoCommand::undo() {
    // 先取消背景，再恢复几何，避免背景对象仍指向该项时被改尺寸
    if (mSetToBackground) {
        GraphicsScene *scene = dynamic_cast<GraphicsScene *>(mItem->scene());
        if (scene && scene->isBackgroundObject(mItem)) {
            scene->unsetBackgroundObject();
        }
    }

    mItem->setPos(mPreviousPosition);
    mItem->setTransform(mPreviousTransform);
    mItem->setZValue(mPreviousZValue);

    ResizableGraphicsItem *resizableItem = dynamic_cast<ResizableGraphicsItem *>(mItem);

    if (resizableItem)
        resizableItem->resize(mPreviousSize);
}

void GraphicsItemTransformUndoCommand::redo() {
    if (mSetToBackground) {
        GraphicsScene *scene = dynamic_cast<GraphicsScene *>(mItem->scene());
        if (scene)
            scene->setAsBackgroundObject(mItem);
    }

    mItem->setPos(mCurrentPosition);
    mItem->setTransform(mCurrentTransform);
    mItem->setZValue(mCurrentZValue);

    ResizableGraphicsItem *resizableItem = dynamic_cast<ResizableGraphicsItem *>(mItem);

    if (resizableItem)
        resizableItem->resize(mCurrentSize);
}
