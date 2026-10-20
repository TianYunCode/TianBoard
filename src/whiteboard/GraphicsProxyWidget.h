/**
 * @file GraphicsProxyWidget.h
 * @brief 声明把普通 QWidget 嵌入场景的代理图形项基类。
 */

#pragma once

#include <QtWidgets>

#include "whiteboard/Item.h"
#include "whiteboard/ResizableGraphicsItem.h"

class GraphicsItemDelegate;

/**
 * @brief 内嵌 QWidget 的代理图形项。
 *
 * 在 QGraphicsProxyWidget 之上接入 Item/GraphicsItem 体系，使内嵌控件（浏览器、
 * 视频等）具备尺寸调整、UUID 与选择/拖动能力。控件本身由子类通过 setWidget 挂入，
 * 代理项接管其显示区域；代理项被删除时控件随 Qt 的父子关系释放。
 *
 * 与委托的关系：鼠标事件先交给委托，未消费时再转给内嵌控件，避免选择框与控件争抢事件。
 */
class GraphicsProxyWidget : public QGraphicsProxyWidget, public Item, public ResizableGraphicsItem, public GraphicsItem
{
public:
    virtual ~GraphicsProxyWidget();

    /** @brief 按宽高调整代理项与内嵌控件尺寸。 */
    virtual void resize(qreal w, qreal h);
    /** @brief 按 QSizeF 调整尺寸。 */
    virtual void resize(const QSizeF &size);

    /** @brief 返回当前尺寸。 */
    virtual QSizeF size() const;

    /** @brief 返回所属的 GraphicsScene，失败时返回空。 */
    virtual GraphicsScene *scene();

    /** @brief 设置 UUID 并写入图形项数据。 */
    virtual void setUuid(const QUuid &pUuid);

protected:
    /** @brief 构造代理项；仅子类可实例化。 */
    GraphicsProxyWidget(QGraphicsItem *parent = 0);

    /** @brief 先交给委托处理，未消费时再转给内嵌控件。 */
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    /** @brief 先交给委托处理，未消费时再转给内嵌控件。 */
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    /** @brief 先交给委托处理，未消费时再转给内嵌控件。 */
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    /** @brief 滚轮优先交给内嵌控件，便于网页滚动。 */
    virtual void wheelEvent(QGraphicsSceneWheelEvent *event);
    /** @brief 悬停进入时通知委托显示控件。 */
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    /** @brief 悬停离开时通知委托隐藏控件。 */
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

    /** @brief 绘制代理项背景（通常无内容）。 */
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    /** @brief 状态变化时同步委托与内嵌控件。 */
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
};
