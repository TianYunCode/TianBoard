#pragma once

#include <QtWidgets>
#include "whiteboard/UndoCommand.h"
#include "whiteboard/GraphicsGroupContainerItem.h"

class GraphicsScene;
class GraphicsItemUndoCommand : public UndoCommand {
public:
    typedef QMultiMap<GraphicsGroupContainerItem *, QUuid> GroupDataTable;

    GraphicsItemUndoCommand(GraphicsScene *pScene, const QSet<QGraphicsItem *> &pRemovedItems,
                            const QSet<QGraphicsItem *> &pAddedItems,
                            const GroupDataTable &groupsMap = GroupDataTable());

    GraphicsItemUndoCommand(GraphicsScene *pScene, QGraphicsItem *pRemovedItem, QGraphicsItem *pAddedItem);

    virtual ~GraphicsItemUndoCommand();

    QSet<QGraphicsItem *> GetAddedList() const { return mAddedItems; }
    QSet<QGraphicsItem *> GetRemovedList() const { return mRemovedItems; }

    virtual int getType() const { return UndoType::undotype_GRAPHICITEM; }

protected:
    virtual void undo();
    virtual void redo();

private:
    GraphicsScene *mScene;
    QSet<QGraphicsItem *> mRemovedItems;
    QSet<QGraphicsItem *> mAddedItems;
    GroupDataTable mExcludedFromGroup;

    bool mFirstRedo;
};
