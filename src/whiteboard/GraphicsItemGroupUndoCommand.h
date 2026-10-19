#pragma once

#include <QList>
#include "whiteboard/UndoCommand.h"

class GraphicsScene;
class GraphicsGroupContainerItem;

class GraphicsItemGroupUndoCommand : public UndoCommand {
public:
    GraphicsItemGroupUndoCommand(GraphicsScene *pScene, GraphicsGroupContainerItem *pGroupCreated);
    virtual ~GraphicsItemGroupUndoCommand();

    virtual int getType() const { return UndoType::undotype_GRAPHICSGROUPITEM; }

protected:
    virtual void undo();
    virtual void redo();

private:
    GraphicsScene *mScene;
    GraphicsGroupContainerItem *mGroup;
    QList<QGraphicsItem *> mItems;

    bool mFirstRedo;
};
