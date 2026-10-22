/**
 * @file ClipboardSelfTest.h
 * @brief 画板图元剪贴板链路端到端自检的声明。
 */

#pragma once

/// @brief 复制→粘贴→删原件→存读往返的自检入口。
namespace ClipboardSelfTest
{
    /**
     * @brief 检查"复制图元再粘贴回来"这条路能不能把图元连同它的资源文件一起搬过去。
     *
     * 详细说明：`BoardController::copy()` 往剪贴板放的是 `deepCopy()` 出来的副本，
     * `paste()` 又对副本再做一次 `deepCopy()` 进当前场景；两次的 UUID 归属决定了
     * "删掉原件会不会顺手把副本引用的资源文件删掉"（`deleteItem` 按 UUID 走 `clearSource()`
     * 清盘）。这条链路以前只过编译，从没被任何用例跑过。
     * 临时文档自建自删，用户数据不碰；剪贴板在收尾时清空，不把测试内容留给用户。
     *
     * @return 不一致的条数；0 表示通过。
     */
    int run();
} // namespace ClipboardSelfTest
