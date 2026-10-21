/**
 * @file CookieSelfTest.h
 * @brief Cookie 隐私链路自检的声明。
 */

#pragma once

/// @brief Cookie 隐私（策略、镜像罐、退出清理计数）的自检入口。
namespace CookieSelfTest
{
    /**
     * @brief 检查应用共享的 Cookie 罐确实是可枚举的那一款、设置项按名字读得到，
     *        并往罐里种三条已知 Cookie，验证“退出时会清掉几个”的计数与白名单一致。
     *
     * 详细：策略过滤器与真正的删除动作都跑在 QtWebEngine 的浏览器线程里，无头环境验不了
     * 它们收不收得到站点 Cookie；这一层查的是我们自己的接线——镜像罐装没装、设置名拼没拼对、
     * 计数有没有按白名单把顶级域留下。用户原有的三项设置在收尾原样还原，不留痕迹。
     *
     * @return 不一致的条数；0 表示通过。
     */
    int run();
} // namespace CookieSelfTest
