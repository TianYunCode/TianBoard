/**
 * @file GraphicsPixmapItem.h
 * @brief 声明位图图形项，用于图片、导入内容与 PDF 页的栅格化结果。
 */

#pragma once

#include <QtWidgets>

#include "utils/Enums.h"

#include "whiteboard/Item.h"

class GraphicsItemDelegate;

/**
 * @brief 位图图形项。
 *
 * 在 QGraphicsPixmapItem 之上接入白板的 Item/GraphicsItem 体系，支持选择、拖动、
 * 透明度调整、复制与 UUID 标识。像素数据由 QGraphicsPixmapItem 持有，本类不额外
 * 管理资源；位置与 z 值由场景统一分配。
 */
class GraphicsPixmapItem : public QObject, public QGraphicsPixmapItem, public Item, public GraphicsItem
{
    Q_OBJECT

public:
    /**
     * @brief 构造空位图项。
     * @param parent 父图形项，可为空。
     */
    GraphicsPixmapItem(QGraphicsItem *parent = 0);
    virtual ~GraphicsPixmapItem();

    enum
    {
        Type = GraphicsItemType::PixmapItemType
    };

    /** @brief 返回图形项类型标识。 */
    virtual int type() const
    {
        return Type;
    }
    /** @brief 深拷贝位图项。 */
    virtual Item *deepCopy() const;

    /** @brief 复制位置、变换与透明度等参数。 */
    virtual void copyItemParameters(Item *copy) const;

    /** @brief 返回所属的 GraphicsScene，失败时返回空。 */
    virtual GraphicsScene *scene();

    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

    /** @brief 设置透明度。 */
    void setOpacity(qreal op);
    /** @brief 返回当前透明度。 */
    qreal opacity() const;

    /** @brief 释放位图数据。 */
    virtual void clearSource();

    /** @brief 设置 UUID 并写入图形项数据。 */
    virtual void setUuid(const QUuid &pUuid);

protected:
    /** @brief 按下时选中本项。 */
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    /** @brief 拖动时移动位图。 */
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    /** @brief 释放时结束拖动。 */
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    /** @brief 绘制位图。 */
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    /** @brief 位置/可见性变化时通知委托刷新选择框。 */
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
};
