/**
 * @file GraphicsStrokesGroup.h
 * @brief 一次落笔产生的多边形组（笔迹批注的场景图元）。
 */

#pragma once

#include <QGraphicsItemGroup>
#include <QGraphicsSceneMouseEvent>

#include "utils/Enums.h"
#include "whiteboard/Item.h"

/**
 * @brief 一次落笔画出的多边形组
 *
 * 输入设备抬起时，GraphicsScene 把这条笔迹的所有 GraphicsPolygonItem 打包进本组，
 * 之后选中、移动、缩放、变色与撤销都以组为单位进行。颜色分三套
 * （当前色 / 浅背景色 / 深背景色），明暗背景切换时由场景统一换色。
 * 子图元随 QGraphicsItemGroup 的所有权规则由场景管理，组本身不额外持有资源。
 */
class GraphicsStrokesGroup : public QObject, public QGraphicsItemGroup, public Item, public GraphicsItem
{
    Q_OBJECT
public:
    /**
     * @brief 颜色槽位
     *
     * currentColor 当前显示色、colorOnLightBackground 浅背景下用色、
     * colorOnDarkBackground 深背景下用色。
     */
    enum colorType
    {
        currentColor = 0,
        colorOnLightBackground,
        colorOnDarkBackground
    };

    /// @brief 构造组并挂上 Delegate（可移动、可旋转、可沿任意轴翻转）
    GraphicsStrokesGroup(QGraphicsItem *parent = 0);
    ~GraphicsStrokesGroup();
    /// @brief 深拷贝：复制全部子多边形并重建共享的 GraphicsStroke；不动原组的变换
    virtual Item *deepCopy() const;
    /// @brief 把变换、位置、锁定标记与 Z 值复制到 copy
    virtual void copyItemParameters(Item *copy) const;
    /// @brief QGraphicsItem::type() 的识别值
    enum
    {
        Type = GraphicsItemType::StrokeItemType
    };
    /// @brief 返回 Type，供 qgraphicsitem_cast 识别
    virtual int type() const
    {
        return Type;
    }
    /// @brief 设置 UUID，同时冗余存进 item data 以便场景内快速检索
    virtual void setUuid(const QUuid &pUuid);
    /// @brief 把组内全部多边形的指定颜色槽位设为 color
    void setColor(const QColor &color, colorType pColorType = currentColor);
    /// @brief 取组内多边形在指定槽位上的颜色（以最后一个子多边形为准）
    QColor color(colorType pColorType = currentColor) const;

protected:
    /// @brief 选中时用整块包围盒做命中区域，未选中时按子图元实际形状（便于点选笔迹）
    virtual QPainterPath shape() const;

    /// @brief 按下：开启一步撤销并立即取消选中（笔迹组不走常规选中路径）
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 拖动：未锁定时移动整组
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 抬起：提交撤销步并交给 Delegate 处理
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 绘制：抹掉 State_Selected 以屏蔽 Qt 默认橡皮筋，选框由 Delegate 自绘
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    /// @brief 拦截 Z 值变化等通知：先过 Delegate，再走基类；调试开关打开时叠加显示 Z 值
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    /// @brief 调试开关：置 true 时在组上叠加显示当前 Z 值文本
    bool debugTextEnabled;
    /// @brief 调试用 Z 值文本图元，仅 debugTextEnabled 时惰性创建
    QGraphicsSimpleTextItem *mDebugText;
};
