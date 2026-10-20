/**
 * @file GraphicsItemUndoCommand.h
 * @brief 声明图形项的添加/删除撤销命令，支持恢复组合归属。
 */

#pragma once

#include <QtWidgets>
#include "whiteboard/UndoCommand.h"
#include "whiteboard/GraphicsGroupContainerItem.h"

class GraphicsScene;
/**
 * @brief 记录一组图形项从场景中移除或加入的撤销命令。
 *
 * 命令保存两份集合：mRemovedItems（执行时被移除的项）与 mAddedItems（执行时新增的项）。
 * undo 将其反向应用：移出的项重新插回场景，加入的项再次移出；redo 反之。
 * 命令不拥有图形项，项的生命周期由场景与删除队列管理；命令本身由撤销栈拥有。
 *
 * mExcludedFromGroup 记录“项原本属于哪个组合”，因为移出/重新加入组合会改变父子关系，
 * 单靠场景无法还原，需要在 undo/redo 时按 UUID 重新挂回组合。
 * mFirstRedo 用于跳过 QUndoStack 在 push 时触发的首次 redo。
 */
class GraphicsItemUndoCommand : public UndoCommand {
public:
    typedef QMultiMap<GraphicsGroupContainerItem *, QUuid> GroupDataTable;

    /**
     * @brief 构造批量增删命令。
     * @param pScene 目标场景，不能为空。
     * @param pRemovedItems 被移除的项集合。
     * @param pAddedItems 被新增的项集合；与移除集合的交集会被剔除。
     * @param groupsMap 项与所属组合的对应关系，用于恢复组合归属。
     */
    GraphicsItemUndoCommand(GraphicsScene *pScene, const QSet<QGraphicsItem *> &pRemovedItems,
                            const QSet<QGraphicsItem *> &pAddedItems,
                            const GroupDataTable &groupsMap = GroupDataTable());

    /**
     * @brief 构造单项增删命令。
     * @param pScene 目标场景。
     * @param pRemovedItem 被移除的项，可为空。
     * @param pAddedItem 被新增的项，可为空。
     */
    GraphicsItemUndoCommand(GraphicsScene *pScene, QGraphicsItem *pRemovedItem, QGraphicsItem *pAddedItem);

    virtual ~GraphicsItemUndoCommand();

    /** @brief 返回本命令新增的项集合。 */
    QSet<QGraphicsItem *> GetAddedList() const { return mAddedItems; }
    /** @brief 返回本命令移除的项集合。 */
    QSet<QGraphicsItem *> GetRemovedList() const { return mRemovedItems; }

    /** @brief 返回撤销命令类型。 */
    virtual int getType() const { return UndoType::undotype_GRAPHICITEM; }

protected:
    /** @brief 撤销：重新插入被移除的项，移出被新增的项，并恢复组合归属。 */
    virtual void undo();
    /** @brief 重做：再次移出被移除的项、插入被新增的项；首次调用仅清除标志。 */
    virtual void redo();

private:
    GraphicsScene *mScene;
    QSet<QGraphicsItem *> mRemovedItems;
    QSet<QGraphicsItem *> mAddedItems;
    GroupDataTable mExcludedFromGroup;

    bool mFirstRedo;
};
