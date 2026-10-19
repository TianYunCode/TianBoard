#pragma once

#include <QGraphicsItem>

#include "whiteboard/UndoCommand.h"
#include "whiteboard/GraphicsScene.h"

class GraphicsItemZLevelUndoCommand : public UndoCommand {
public:
    GraphicsItemZLevelUndoCommand(GraphicsScene *_scene, const QList<QGraphicsItem *> &_items,
                                  qreal _previousZLevel, ZLayerController::moveDestination dest);
    GraphicsItemZLevelUndoCommand(GraphicsScene *_scene, QGraphicsItem *_item, qreal _previousZLevel,
                                  ZLayerController::moveDestination dest);
    ~GraphicsItemZLevelUndoCommand();

    virtual int getType() const { return UndoType::undotype_GRAPHICITEMZVALUE; }

protected:
    virtual void undo();
    virtual void redo();

private:
    void updateLazyScene();

    qreal mPreviousZLevel;
    QList<QGraphicsItem *> mItems;
    GraphicsScene *mpScene;
    ZLayerController::moveDestination mDest;
    bool mHack;
};
