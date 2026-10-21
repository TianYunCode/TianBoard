/**
 * @file SnapSelfTest.h
 * @brief 网格吸附接线自检的声明。
 */

#pragma once

/// @brief 网格吸附的场景侧自检入口。
namespace SnapSelfTest
{
    /**
     * @brief 检查吸附开关、格距取值、稿纸只吸 Y、外框缩回这些接线是否正确。
     *
     * 纯数学由 tests/tst_snapping 单独覆盖；这里查的是 GraphicsScene 那一层：它把设置、
     * 背景类型与图元外框喂给数学的姿势。临时文档/图元自建自删，不碰用户数据。
     *
     * @return 不一致的条数；0 表示通过。
     */
    int run();
} // namespace SnapSelfTest
