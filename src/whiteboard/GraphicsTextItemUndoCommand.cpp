#include "whiteboard/GraphicsTextItemUndoCommand.h"
#include "whiteboard/GraphicsTextItem.h"


GraphicsTextItemUndoCommand::GraphicsTextItemUndoCommand(GraphicsTextItem *textItem) : mTextItem(textItem) {
    // NOOP
}

GraphicsTextItemUndoCommand::~GraphicsTextItemUndoCommand() {
    // NOOP
}

void GraphicsTextItemUndoCommand::undo() {
    if (mTextItem && mTextItem->document())
        mTextItem->document()->undo();
}

void GraphicsTextItemUndoCommand::redo() {
    if (mTextItem && mTextItem->document())
        mTextItem->document()->redo();
}
