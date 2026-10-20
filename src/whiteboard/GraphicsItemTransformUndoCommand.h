/**
 * @file GraphicsItemTransformUndoCommand.h
 * @brief 声明记录图形项位置/变换/尺寸/z 值变化的撤销命令。
 */

#pragma once

#include <QtWidgets>

#include "whiteboard/UndoCommand.h"

/**
 * @brief 图形项几何变换的撤销命令。
 *
 * 构造时保存“旧值”（由调用方传入）并立即读取图形项的“当前值”。undo 恢复旧值，
 * redo 写回当前值，因此命令只在构造瞬间生效快照，之后的修改不会被记录。
 * 若 setToBackground 为 true，撤销/重做还会同步切换页面背景对象。
 * 命令不拥有图形项；调用方需保证项在撤销栈存活期间仍然有效。
 */
class GraphicsItemTransformUndoCommand : public UndoCommand {
public:
    /**
     * @brief 构造命令并抓取项的当前状态。
     * @param pItem 被记录的图形项，不能为空。
     * @param prevPos 变化前的位置。
     * @param prevTransform 变化前的变换。
     * @param prevZValue 变化前的 z 值。
     * @param prevSize 变化前的尺寸，仅对 ResizableGraphicsItem 有效。
     * @param setToBackground 撤销/重做时是否同步切换背景状态。
     */
    GraphicsItemTransformUndoCommand(QGraphicsItem *pItem, const QPointF &prevPos,
                                     const QTransform &prevTransform, const qreal &prevZValue,
                                     const QSizeF &prevSize = QSizeF(), bool setToBackground = false);
    virtual ~GraphicsItemTransformUndoCommand();

    /** @brief 返回撤销命令类型。 */
    virtual int getType() const { return UndoType::undotype_GRAPHICITEMTRANSFORM; }

protected:
    /** @brief 恢复位置、变换、z 值与尺寸；必要时取消背景。 */
    virtual void undo();
    /** @brief 写回记录时的当前值；必要时重新设为背景。 */
    virtual void redo();

private:
    QGraphicsItem *mItem;
    QTransform mPreviousTransform;
    QTransform mCurrentTransform;
    QPointF mPreviousPosition;
    QPointF mCurrentPosition;
    QSizeF mPreviousSize;
    QSizeF mCurrentSize;

    qreal mPreviousZValue;
    qreal mCurrentZValue;

    bool mSetToBackground;
};
