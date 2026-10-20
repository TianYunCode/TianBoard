/**
 * @file PageSizeUndoCommand.h
 * @brief 页面尺寸变更的撤销命令声明。
 */

#pragma once

#include <QtWidgets>
#include "whiteboard/UndoCommand.h"

class GraphicsScene;

/**
 * @brief 记录一次页面尺寸切换，撤销时恢复旧尺寸、重做时应用新尺寸。
 *
 * 命令由 BoardController 在切换页面尺寸时压入全局撤销栈，命令对象归撤销栈所有，
 * 持有的场景指针只在场景存活期间有效（场景销毁时撤销栈由上层清理）。
 * 首次 redo 用于跳过命令入栈时的重复应用，见实现文件中的说明。
 */
class PageSizeUndoCommand : public UndoCommand
{
public:
    /**
     * @param pScene 目标场景，可为空（空指针时撤销/重做直接返回）。
     * @param previousSize 变更前的页面尺寸。
     * @param newSize 变更后的页面尺寸。
     */
    PageSizeUndoCommand(GraphicsScene *pScene, const QSize &previousSize, const QSize &newSize);
    virtual ~PageSizeUndoCommand();

    /// @brief 命令类型为页面尺寸，供撤销栈识别。
    virtual int getType()
    {
        return UndoType::undotype_PAGESIZE;
    };

protected:
    /// @brief 恢复之前的页面尺寸并强制重绘场景。
    virtual void undo();
    /// @brief 应用新的页面尺寸；首次调用只清除标志，不重复应用。
    virtual void redo();

private:
    GraphicsScene *mScene;        ///< 应用尺寸的场景，不持有所有权
    QSize          mPreviousSize; ///< 变更前尺寸
    QSize          mNewSize;      ///< 变更后尺寸

    bool mFirstRedo;              ///< 首次 redo 标志：命令入栈时 Qt 会自动调用一次 redo
};
