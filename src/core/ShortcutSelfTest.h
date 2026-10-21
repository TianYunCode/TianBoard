/**
 * @file ShortcutSelfTest.h
 * @brief 快捷键注册表自检的声明。
 */

#pragma once

/// @brief 快捷键注册表（ShortcutManager）的自检入口。
namespace ShortcutSelfTest
{
    /**
     * @brief 检查主窗口的动作是否全部登记、默认值是否还在、彼此是否撞键，
     *        再用一张自建注册表跑一遍改绑→落盘→读回→恢复默认的完整回路。
     *
     * 快捷键这条链路在界面上只体现为“双击一个格子按下一个组合键”，写错一侧
     * （键名拼错、装配漏登记、恢复默认没删干净配置键）都不会当场报错，
     * 只会让用户在下次启动时发现键不见了。临时动作自建自删，配置键用
     * selftest 前缀隔离并在收尾删掉，不碰用户真实的自定义。
     *
     * @return 不一致的条数；0 表示通过。
     */
    int run();
} // namespace ShortcutSelfTest
