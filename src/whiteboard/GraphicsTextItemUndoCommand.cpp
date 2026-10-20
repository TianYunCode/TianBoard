#include "whiteboard/GraphicsTextItemUndoCommand.h"
#include "whiteboard/GraphicsTextItem.h"

GraphicsTextItemUndoCommand::GraphicsTextItemUndoCommand(GraphicsTextItem *textItem) : mTextItem(textItem)
{
}

GraphicsTextItemUndoCommand::~GraphicsTextItemUndoCommand()
{
}

void GraphicsTextItemUndoCommand::undo()
{
    // 文本项可能已被删除，QPointer 会自动置空
    if (mTextItem && mTextItem->document())
    {
        mTextItem->document()->undo();
    }
}

void GraphicsTextItemUndoCommand::redo()
{
    if (mTextItem && mTextItem->document())
    {
        mTextItem->document()->redo();
    }
}
