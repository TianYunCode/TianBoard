/**
 * @file ResizableGraphicsItem.h
 * @brief 可缩放图元的纯接口声明。
 */

#pragma once

#include <QtWidgets>


/**
 * @brief 为图元提供统一的尺寸调整入口。
 *
 * 仅定义协议，不持有资源；由具体图元继承实现。调用方（如属性面板或拖拽手柄）
 * 运行在 GUI 线程，实现方需自行处理重绘与撤销记录。
 */
class ResizableGraphicsItem {
public:
    ResizableGraphicsItem();
    virtual ~ResizableGraphicsItem();

    /// @brief 以尺寸对象重载转调 resize(w, h)。
    virtual void resize(const QSizeF &pSize);
    /// @brief 按宽高调整图元尺寸，具体语义（是否保持比例等）由实现决定。
    virtual void resize(qreal w, qreal h) = 0;

    /// @brief 返回图元当前尺寸。
    virtual QSizeF size() const = 0;
};
