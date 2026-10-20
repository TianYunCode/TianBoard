/**
 * @file UndoCommand.h
 * @brief 画板撤销命令的公共基类声明。
 */

#pragma once

#include <QtWidgets>
#include <QUndoCommand>
#include "utils/Enums.h"

/**
 * @brief 所有画板撤销命令的基类，用 getType() 区分命令种类。
 *
 * 命令由各功能模块创建后压入 Application::undoStack；命令对象归撤销栈所有，
 * 在栈清理或应用退出时被释放。撤销/重做回调运行在 GUI 线程。
 * 派生类通过重写 undo()/redo() 实现状态回滚。
 */
class UndoCommand : public QUndoCommand
{
public:
    UndoCommand(QUndoCommand *parent = 0);
    ~UndoCommand();

    /// @brief 命令类型标识，默认返回未知类型，供撤销栈按类型合并或过滤。
    virtual int getType() const
    {
        return UndoType::undotype_UNKNOWN;
    }
};
