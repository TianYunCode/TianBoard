#pragma once

#include <QtWidgets>
#include "whiteboard/UndoCommand.h"

class GraphicsScene;

class PageSizeUndoCommand : public UndoCommand {
public:
    PageSizeUndoCommand(GraphicsScene *pScene, const QSize &previousSize, const QSize &newSize);
    virtual ~PageSizeUndoCommand();

    virtual int getType() { return UndoType::undotype_PAGESIZE; };

protected:
    virtual void undo();
    virtual void redo();

private:
    GraphicsScene *mScene;
    QSize mPreviousSize;
    QSize mNewSize;

    bool mFirstRedo;
};
