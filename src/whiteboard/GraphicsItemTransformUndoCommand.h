#pragma once

#include <QtWidgets>

#include "whiteboard/UndoCommand.h"

class GraphicsItemTransformUndoCommand : public UndoCommand {
public:
    GraphicsItemTransformUndoCommand(QGraphicsItem *pItem, const QPointF &prevPos,
                                     const QTransform &prevTransform, const qreal &prevZValue,
                                     const QSizeF &prevSize = QSizeF(), bool setToBackground = false);
    virtual ~GraphicsItemTransformUndoCommand();

    virtual int getType() const { return UndoType::undotype_GRAPHICITEMTRANSFORM; }

protected:
    virtual void undo();
    virtual void redo();

private:
    QGraphicsItem *mItem;
    QTransform mPreviousTransform;
    QTransform mCurrentTransform;
    QPointF mPreviousPosition;
    QPointF mCurrentPosition;
    QSizeF mPreviousSize;
    QSizeF mCurrentSize;

    qreal mPreviousZValue;
    qreal mCurrentZValue;

    bool mSetToBackground;
};
