/**
 * @file StaticMemory.h
 * @brief 进程级"静态对象宿主"：给没有天然父对象的单例当 QObject 父，关闭时一起回收。
 */

#pragma once

class QObject;

/**
 * @brief 一组没有天然归属、又必须在退出前被释放的 QObject 的共用父对象。
 *
 * 详细说明：撤销栈、工具管理器、录制控制器、资源表这些单例都既不属于某个窗口、也不属于
 * 某个场景，但它们都是 QObject，需要有人在退出时把连接与子对象一并收掉——这个宿主就是干
 * 这件事的。首次调用 cleaner() 时创建，之后一直是同一个对象。
 *
 * 为什么住在 utils 而不是 core/Application：它只是一个 QObject，不牵扯任何应用状态；挂在
 * 应用对象上时，`model/Resources` 这类只要个父对象的地方就得 include 最上层，`core` ↔ `model`
 * 那组环里有它一条。
 * 生命周期：release() 由 ~Application 调用（app/Application.cpp）；在那之后再调 cleaner()
 * 会再造一个宿主，所以别在退出流程开始之后才创建这些单例。
 * 线程约束：只在 GUI 线程使用（所有被托管的单例都住在 GUI 线程）。
 */
namespace StaticMemory
{
    /// @brief 返回宿主对象，首次调用时创建。
    QObject *cleaner();

    /// @brief 删除宿主并清空，好让它托管的子对象一起析构。
    void release();
} // namespace StaticMemory
