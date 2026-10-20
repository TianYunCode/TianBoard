/**
 * @file GraphicsItemGroupUndoCommand.h
 * @brief 声明“创建组合”操作的撤销命令。
 */

#pragma once

#include <QList>
#include "whiteboard/UndoCommand.h"

class GraphicsScene;
class GraphicsGroupContainerItem;

/**
 * @brief 记录一次新建组合操作。
 *
 * 构造时缓存组合当前的全部子项。undo 解散组合并重新选中原子项；
 * redo 再次把缓存的子项加入组合并显示、聚焦。
 * 命令不拥有组合与子项，它们由场景管理；组合被解散后仍保留对象以便重做。
 */
class GraphicsItemGroupUndoCommand : public UndoCommand {
public:
    /**
     * @brief 构造命令并记录组合的子项。
     * @param pScene 组合所在场景，不能为空。
     * @param pGroupCreated 刚创建的组合，不能为空。
     */
    GraphicsItemGroupUndoCommand(GraphicsScene *pScene, GraphicsGroupContainerItem *pGroupCreated);
    virtual ~GraphicsItemGroupUndoCommand();

    /** @brief 返回撤销命令类型。 */
    virtual int getType() const { return UndoType::undotype_GRAPHICSGROUPITEM; }

protected:
    /** @brief 解散组合，并重新选中原来的子项。 */
    virtual void undo();
    /** @brief 重新把项加入组合并显示；首次调用仅清除标志。 */
    virtual void redo();

private:
    GraphicsScene *mScene;
    GraphicsGroupContainerItem *mGroup;
    QList<QGraphicsItem *> mItems;

    bool mFirstRedo;
};
