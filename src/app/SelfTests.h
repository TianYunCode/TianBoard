/**
 * @file SelfTests.h
 * @brief 装配层自检入口的声明：按命令行标志决定跑哪个自检。
 */

#pragma once

class Application;

/**
 * @brief 按命令行里的自检标志给应用装上对应的自检动作；没带标志就什么都不做。
 *
 * 详细说明：自检代码要同时伸手进 model/whiteboard/content/web/ui，只有装配层允许这样，
 * 所以它们与这张分发表都住在 app/，core 只提供 Application::setSelfTestRun() 那个通用钩子。
 * 约定：退出码 0 = 通过，非 0 = 不一致条数；75 = 已有实例在跑、让路（自检会写临时文档、
 * 动场景缓存与用户配置，两个进程并行没有意义，测试据此判"跳过"而不是判失败）。
 * 每条链路的判据写在对应的 *SelfTest.cpp 里。
 */
void installSelfTestFromArgs(Application &app);
