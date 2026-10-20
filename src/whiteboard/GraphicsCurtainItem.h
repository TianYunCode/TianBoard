/**
 * @file GraphicsCurtainItem.h
 * @brief 幕布图元声明。
 */

#pragma once

#include <QtWidgets>

#include "utils/Enums.h"

#include "whiteboard/Item.h"

class GraphicsItemDelegate;


/**
 * @brief 幕布图元：整页不透明矩形，用于暂时遮住页面内容。
 *
 * 控制视图里画成设置中的调色板颜色，演示视图里按背景明暗画成白或黑；
 * 交互（移动、删除）交给 GraphicsCurtainItemDelegate。构造时注册 Curtain 层，
 * 以便 Z 值分配器把它放到内容之上、控件之下。
 *
 * 生命周期：归场景所有，委托随基类析构释放；仅在 GUI 线程使用。
 */
class GraphicsCurtainItem : public QObject, public QGraphicsRectItem, public Item, public GraphicsItem {
    Q_OBJECT

public:
    GraphicsCurtainItem(QGraphicsItem *parent = 0);
    virtual ~GraphicsCurtainItem();

    /// @brief 图元类型标识。
    enum { Type = GraphicsItemType::CurtainItemType };

    /// @brief 返回图元类型。
    virtual int type() const { return Type; }

    /// @brief 创建一份新的幕布并复制几何参数。
    virtual Item *deepCopy() const;
    /// @brief 复制矩形、位置、画刷、画笔、变换与层级到目标实例。
    virtual void copyItemParameters(Item *copy) const;

    /// @brief 主动发出 removed() 信号，供场景在删除前统一通知观察者。
    void triggerRemovedSignal();
    /// @brief 幕布不持有外部资源，空实现。
    virtual void clearSource() { ; }

    /// @brief 设置 UUID，同时写入图元 data 以加速场景内查找。
    virtual void setUuid(const QUuid &pUuid);

signals:
    /// @brief 幕布被移除前发出，供界面同步状态。
    void removed();

protected:
    /// @brief 先交给委托处理，未消费时回退到基类行为。
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 拖动时优先由委托处理，未消费时回退到基类行为。
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 松开时委托与基类都会收到，确保拖拽状态收尾。
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    /// @brief 填充矩形并按视图类型选择颜色，选中框由委托绘制。
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    /// @brief 把几何变更转交委托，用于撤销与层级修正。
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    /// @brief 幕布颜色，随背景明暗切换。
    QColor drawColor() const;
    /// @brief 无外接显示器时控制视图使用的半透明颜色。
    QColor opaqueControlColor() const;

    static const QColor sDrawColor;                        ///< 浅色背景（演示视图）下的幕布颜色
    static const QColor sDarkBackgroundDrawColor;          ///< 深色背景（演示视图）下的幕布颜色
    static const QColor sOpaqueControlColor;               ///< 浅色背景、无外接屏时的控制视图颜色
    static const QColor sDarkBackgroundOpaqueControlColor; ///< 深色背景、无外接屏时的控制视图颜色
};
