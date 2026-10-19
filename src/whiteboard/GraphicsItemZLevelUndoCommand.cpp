#include "whiteboard/GraphicsItemZLevelUndoCommand.h"

GraphicsItemZLevelUndoCommand::GraphicsItemZLevelUndoCommand(GraphicsScene *_scene, const QList<QGraphicsItem*>& _items, qreal _previousZLevel, ZLayerController::moveDestination dest):UndoCommand(){
    Q_ASSERT(_scene != NULL);
    mpScene = _scene;
    mItems = _items;
    mPreviousZLevel = _previousZLevel;
    mDest = dest;
    mHack = false;
}

GraphicsItemZLevelUndoCommand::GraphicsItemZLevelUndoCommand(GraphicsScene *_scene, QGraphicsItem* _item, qreal _previousZLevel, ZLayerController::moveDestination dest):UndoCommand(){
    Q_ASSERT(_scene != NULL);
    mpScene = _scene;
    if(NULL != _item)
        mItems.append(_item);

    mPreviousZLevel = _previousZLevel;
    mDest = dest;
    mHack = false;
}

GraphicsItemZLevelUndoCommand::~GraphicsItemZLevelUndoCommand(){

}

void GraphicsItemZLevelUndoCommand::undo(){
    if(!mpScene || mItems.empty())
        return;

    // Getting the difference between the initial z-value and the actual one
    qreal zDiff = qAbs(mItems.at(mItems.size()-1)->data(GraphicsItemData::ItemOwnZValue).toReal() - mPreviousZLevel);

    if(mDest == ZLayerController::down || mDest == ZLayerController::bottom){
        // Move up
        QList<QGraphicsItem*>::iterator downIt = mItems.end();
        for(; downIt >= mItems.begin(); downIt--){
            for(int i=0; i<zDiff; i++)
                mpScene->changeZLevelTo(*downIt, ZLayerController::up);
        }

    }else if(mDest == ZLayerController::up || mDest == ZLayerController::top){
        // Move down
        foreach(QGraphicsItem* item, mItems){
            for(int i=0; i<zDiff; i++)
                mpScene->changeZLevelTo(item, ZLayerController::down);
        }
    }

}

void GraphicsItemZLevelUndoCommand::redo(){
    if(!mHack){
        // Ugly! But pushing a new command to QUndoStack calls redo by itself.
        mHack = true;
    }else{
        if(!mpScene)
            return;

        foreach(QGraphicsItem* item, mItems){
            mpScene->changeZLevelTo(item, mDest);
            updateLazyScene();
        }
    }
}

void GraphicsItemZLevelUndoCommand::updateLazyScene(){
    mpScene->update(mpScene->sceneRect());
    mpScene->updateSelectionFrame();
}
