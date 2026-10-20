/**
 * @file GraphicsPDFItem.h
 * @brief 声明页面上的 PDF 页图形项（可选中、可复制、可栅格化）。
 */

#pragma once

#include <QtWidgets>

#include "whiteboard/Item.h"
#include "utils/Enums.h"
#include "whiteboard/PdfPageItem.h"

class GraphicsItemDelegate;
class GraphicsPixmapItem;

/**
 * @brief PDF 页面图形项。
 *
 * 继承 PdfPageItem 以获得页面渲染能力，同时接入白板的 Item/GraphicsItem 体系：
 * 可被选择、拖动、复制，并在操作时通过委托显示选择框。
 * 渲染器与页面数据由 PdfPageItem 持有；本类不额外拥有渲染资源。
 * z 值遵循场景分配的页面层级，通常位于背景之上、手写内容之下。
 */
class GraphicsPDFItem : public PdfPageItem, public Item, public GraphicsItem {
    Q_OBJECT
public:
    /**
     * @brief 构造某一页的 PDF 图形项。
     * @param renderer PDF 渲染器，所有权不转移。
     * @param pageNumber 页号，从 0 开始。
     * @param parent 父图形项，可为空。
     */
    GraphicsPDFItem(PDFRenderer *renderer, int pageNumber, QGraphicsItem *parent = 0);
    virtual ~GraphicsPDFItem();

    enum { Type = GraphicsItemType::PDFItemType };

    /** @brief 返回图形项类型标识。 */
    virtual int type() const { return Type; }

    /** @brief 深拷贝当前页图形项。 */
    virtual Item *deepCopy() const;

    /** @brief 复制位置、尺寸与缩放参数到另一个页面项。 */
    virtual void copyItemParameters(Item *copy) const;

    /** @brief 设置页面渲染质量。 */
    virtual void setRenderingQuality(RenderingQuality pRenderingQuality);

    /** @brief 返回所属的 GraphicsScene，失败时返回空。 */
    virtual GraphicsScene *scene();

    /** @brief 把当前页转换为位图图形项。 */
    virtual GraphicsPixmapItem *toPixmapItem() const;

    /** @brief PDF 页没有外部内容源，空实现。 */
    virtual void clearSource() { ; }
    /** @brief 设置 UUID 并写入图形项数据。 */
    virtual void setUuid(const QUuid &pUuid);

protected:
    /** @brief 按下时选中本项。 */
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    /** @brief 拖动时移动页面。 */
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    /** @brief 释放时结束拖动。 */
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    /** @brief 绘制 PDF 页面内容。 */
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    /** @brief 位置/可见性变化时通知委托刷新选择框。 */
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
};
