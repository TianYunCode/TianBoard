#include "whiteboard/GraphicsItemTransformUndoCommand.h"
#include "whiteboard/ResizableGraphicsItem.h"
#include "whiteboard/Item.h"
#include "whiteboard/GraphicsScene.h"


GraphicsItemTransformUndoCommand::GraphicsItemTransformUndoCommand(
    QGraphicsItem *pItem, const QPointF &prevPos, const QTransform &prevTransform, const qreal &prevZValue,
    const QSizeF &prevSize, bool setToBackground)
    : UndoCommand() {
    mItem = pItem;
    mPreviousTransform = prevTransform;
    mCurrentTransform = pItem->transform();

    mPreviousPosition = prevPos;
    mCurrentPosition = pItem->pos();

    mPreviousZValue = prevZValue;
    mCurrentZValue = pItem->zValue();

    mPreviousSize = prevSize;
    ResizableGraphicsItem *resizableItem = dynamic_cast<ResizableGraphicsItem *>(pItem);

    if (resizableItem)
        mCurrentSize = resizableItem->size();

    mSetToBackground = setToBackground;
}

GraphicsItemTransformUndoCommand::~GraphicsItemTransformUndoCommand() {
    // NOOP
}

void GraphicsItemTransformUndoCommand::undo() {
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
