/**
 * @file GraphicsGroupContainerItem.h
 * @brief 声明用于把多个图形项组合在一起的容器图形项。
 */

#pragma once

#include <QGraphicsItem>

#include "whiteboard/Item.h"
#include "whiteboard/CoreGraphicsScene.h"

/**
 * @brief 图形项组合容器。
 *
 * 组合是一个没有自身绘制的父图形项，子项通过 setParentItem 挂到它下面。加入组合时会把
 * 子项的场景变换折算进其局部变换，使子项在视觉上保持原位；移出时再把局部变换折算回
 * 场景坐标。组合的包围盒由子项包围盒的并集维护，用于绘制选择框。
 *
 * 组合的旋转/翻转/锁定能力由所有子项的公共能力决定，并写回自己的委托框架标志。
 * 组合拥有其子项：销毁组合时子项会先被移出并重新设为可选，再删除组合本身。
 * 组合的 z 值遵循 Object 图层的统一规则，加入控制类子项时组合也会被视为控制类。
 */
class GraphicsGroupContainerItem : public QGraphicsItem, public Item, public GraphicsItem
{
public:
    /**
     * @brief 构造空组合。
     * @param parent 父图形项，可为空；组合不接管父项。
     */
    GraphicsGroupContainerItem(QGraphicsItem *parent = 0);
    virtual ~GraphicsGroupContainerItem();

    /**
     * @brief 把一个图形项加入组合。
     * @param item 待加入的项；为空、等于自身或无法求得变换时给出警告并返回。
     * @note 加入后子项不再可选中/移动，改用组合的选择框操作；子项的场景变换会被并入局部变换。
     */
    void addToGroup(QGraphicsItem *item);
    /**
     * @brief 把子项移出组合。
     * @param item 待移出的子项，为空时直接返回。
     * @note 子项会保留当前视觉位置，并恢复可选中、可移动标志。
     */
    void removeFromGroup(QGraphicsItem *item);
    /** @brief 记录当前正在交互的子项（如媒体项），用于释放时收尾。 */
    void setCurrentItem(QGraphicsItem *item)
    {
        mCurrentItem = item;
    }
    /** @brief 返回当前正在交互的子项，可能为空。 */
    QGraphicsItem *getCurrentItem() const
    {
        return mCurrentItem;
    }
    /** @brief 若当前子项是媒体项，隐藏其工具栏并取消选中。 */
    void deselectCurrentItem();

    /** @brief 返回所有子项包围盒的并集。 */
    QRectF boundingRect() const;
    /** @brief 组合本身不绘制内容，只把绘制转交给委托以画出选择框。 */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    /** @brief 把场景指针转换为 CoreGraphicsScene，失败时返回空。 */
    virtual CoreGraphicsScene *corescene();
    /** @brief 复制组合参数但不复制子项，供特殊场景（如剪贴板占位）使用。 */
    GraphicsGroupContainerItem *deepCopyNoChildDuplication() const;
    /** @brief 深拷贝组合及其全部子项。 */
    virtual GraphicsGroupContainerItem *deepCopy() const;
    /** @brief 把位置、变换、图层、锁定状态等参数复制到另一个组合。 */
    virtual void copyItemParameters(Item *copy) const;

    enum
    {
        Type = GraphicsItemType::groupContainerType
    };

    /** @brief 返回图形项类型标识。 */
    virtual int type() const
    {
        return Type;
    }

    /**
     * @brief 设置 UUID，并同步写入图形项数据以便场景快速查找。
     * @param pUuid 新的唯一标识。
     */
    virtual void setUuid(const QUuid &pUuid);
    /**
     * @brief 解散组合：先释放所有子项，再删除自身。
     * @param canUndo 是否把删除纳入撤销栈。
     */
    void destroy(bool canUndo = true);

    /** @brief 递归清空所有子项的内容源引用。 */
    virtual void clearSource();

protected:
    /** @brief 鼠标按下：先交给委托处理，未消费时按普通图形项处理并选中自身。 */
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    /** @brief 鼠标移动：委托未消费时走默认拖动。 */
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    /** @brief 鼠标释放：交给默认实现，确保拖动状态收尾。 */
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    /**
     * @brief 转发状态变化给委托，并刷新所有子项的选择框位置。
     * @note 选中状态改变时会先释放当前交互子项。
     */
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    /** @brief 移出子项的内部实现：恢复父项、位置和变换。 */
    void pRemoveFromGroup(QGraphicsItem *item);

private:
    QRectF         itemsBoundingRect;
    QGraphicsItem *mCurrentItem;
};
