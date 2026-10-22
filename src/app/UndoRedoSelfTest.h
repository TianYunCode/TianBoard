/**
 * @file UndoRedoSelfTest.h
 * @brief 画板撤销/重做回路的端到端自检声明。
 */

#pragma once

/// @brief 加图元→撤销→重做→删除→撤销回来→清空撤销栈这条回路的自检入口。
namespace UndoRedoSelfTest
{
    /**
     * @brief 在画板真实的可撤销场景里跑一遍完整的撤销/重做回路。
     *
     * 详细说明：这条回路只在界面上"点 Ctrl+Z 看起来动了"，从没被量过：栈的计数对不对、
     * 撤销是不是真把图元移出场景、重做放回的是不是同一个对象（不是新建的副本，否则 UUID
     * 与资源文件会对不上）、删除命令撤销后图元内容还完不完整、`ClearUndoStack()` 之后
     * 场景不受影响而栈确实空了、换文档时栈有没有被清（撤销栈 2026-10-22 归还给了
     * `BoardController`，这一条就是它的生命周期钉）。
     * 最后一案量的是"画一笔 → 删掉当前这一页 → 再撤销/再画一笔"：删页会把那一页的场景
     * `deleteLater`，而 `ClearUndoStack` 里那个循环会把不在场景也不在剪贴板的图元直接 delete，
     * 命令若不跟着清掉，抱着的就是已释放的图元——下一次遍历（`findUniquesItems`）就是
     * use-after-free。这条链路的承重句因此是那句 `undoStack()->clear()`，不是收尾的装饰。
     * 临时文档自建自删，用户数据不碰。
     *
     * @return 不一致的条数；0 表示通过。
     */
    int run();
} // namespace UndoRedoSelfTest
