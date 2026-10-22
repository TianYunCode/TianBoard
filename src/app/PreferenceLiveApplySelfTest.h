/**
 * @file PreferenceLiveApplySelfTest.h
 * @brief 「偏好设置改完当场生效」这条约定的端到端自检声明。
 */

#pragma once

/// @brief 偏好设置改完之后不重启、不换页就能生效的自检入口。
namespace PreferenceLiveApplySelfTest
{
    /**
     * @brief 在真实场景上改设置，量画板是不是当场就按新值走。
     *
     * 详细说明：偏好设置里每一项都应当「改完立刻算」——读的地方每次读。这条自检钉住
     * 已经发现的那一类反面情况：钢笔预览圈的**图元**是在建页时按设置决定建不建的，
     * 于是在「关闭」状态下建的那一页上根本没有圈可显示，后来把设置打开也只对新页生效
     * （反过来「关闭」倒是当场生效，因为画的时候每次都读设置，这种不对称界面上完全看不出来）。
     * 临时文档自建自删，用户数据不碰。
     *
     * @return 不一致的条数；0 表示通过。
     */
    int run();
} // namespace PreferenceLiveApplySelfTest
