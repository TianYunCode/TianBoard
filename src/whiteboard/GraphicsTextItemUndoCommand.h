/**
 * @file GraphicsTextItemUndoCommand.h
 * @brief 声明把文本编辑器自身的撤销/重做接入全局撤销栈的命令。
 */

#pragma once

#include <QtWidgets>
#include "whiteboard/UndoCommand.h"

#include "whiteboard/GraphicsTextItem.h"


/**
 * @brief 文本项编辑撤销命令的适配器。
 *
 * 文本项内部使用 QTextDocument 的撤销栈，本命令把一次外部撤销/重做转发给文档，
 * 从而与全局撤销栈保持同步。使用 QPointer 持有文本项，项被销毁后命令自动失效。
 */
class GraphicsTextItemUndoCommand : public UndoCommand {
public:
    /**
     * @brief 构造命令。
     * @param textItem 目标文本项，可为空；所有权不转移。
     */
    GraphicsTextItemUndoCommand(GraphicsTextItem *textItem);
    virtual ~GraphicsTextItemUndoCommand();

    /** @brief 返回撤销命令类型。 */
    virtual int getType() const { return UndoType::undotype_GRAPHICTEXTITEM; };

protected:
    /** @brief 调用文档的 undo()。 */
    virtual void undo();
    /** @brief 调用文档的 redo()。 */
    virtual void redo();

private:
    QPointer<GraphicsTextItem> mTextItem;
};
