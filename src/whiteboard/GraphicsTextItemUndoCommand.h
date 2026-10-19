#pragma once

#include <QtWidgets>
#include "whiteboard/UndoCommand.h"

#include "whiteboard/GraphicsTextItem.h"


class GraphicsTextItemUndoCommand : public UndoCommand
{
    public:
        GraphicsTextItemUndoCommand(GraphicsTextItem *textItem);
        virtual ~GraphicsTextItemUndoCommand();

        virtual int getType() const { return UndoType::undotype_GRAPHICTEXTITEM; };

    protected:
        virtual void undo();
        virtual void redo();

    private:
        QPointer<GraphicsTextItem> mTextItem;
};

