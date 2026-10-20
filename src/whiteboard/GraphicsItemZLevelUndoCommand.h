/**
 * @file GraphicsItemZLevelUndoCommand.h
 * @brief 声明调整图形项叠放层级的撤销命令。
 */

#pragma once

#include <QGraphicsItem>

#include "whiteboard/UndoCommand.h"
#include "whiteboard/GraphicsScene.h"

/**
 * @brief 记录一次层级移动（上移/下移/置顶/置底）。
 *
 * 命令保存移动前的 z 值、目标方向与受影响的项列表。undo 按 z 差值反向移动；
 * redo 按原方向再次移动。命令不拥有图形项，项由场景管理。
 * mHack 用于跳过 QUndoStack 入栈时自动触发的首次 redo。
 */
class GraphicsItemZLevelUndoCommand : public UndoCommand
{
public:
    /**
     * @brief 构造批量层级命令。
     * @param _scene 目标场景，不能为空。
     * @param _items 受影响的项列表。
     * @param _previousZLevel 移动前的 z 值。
     * @param dest 移动方向。
     */
    GraphicsItemZLevelUndoCommand(
        GraphicsScene *_scene, const QList<QGraphicsItem *> &_items, qreal _previousZLevel, ZLayerController::moveDestination dest);
    /**
     * @brief 构造单项层级命令。
     * @param _scene 目标场景，不能为空。
     * @param _item 受影响的项，可为空。
     * @param _previousZLevel 移动前的 z 值。
     * @param dest 移动方向。
     */
    GraphicsItemZLevelUndoCommand(GraphicsScene *_scene, QGraphicsItem *_item, qreal _previousZLevel, ZLayerController::moveDestination dest);
    ~GraphicsItemZLevelUndoCommand();

    /** @brief 返回撤销命令类型。 */
    virtual int getType() const
    {
        return UndoType::undotype_GRAPHICITEMZVALUE;
    }

protected:
    /** @brief 按记录的方向反向移动若干层，恢复原层级。 */
    virtual void undo();
    /** @brief 按记录的方向再次移动；首次调用仅清除标志。 */
    virtual void redo();

private:
    /** @brief 主动刷新场景与选择框，规避重绘延迟。 */
    void updateLazyScene();

    qreal                             mPreviousZLevel;
    QList<QGraphicsItem *>            mItems;
    GraphicsScene                    *mpScene;
    ZLayerController::moveDestination mDest;
    bool                              mHack;
};
