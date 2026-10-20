/**
 * @file GraphicsPolygonItem.h
 * @brief 声明用于手写笔迹的多边形图形项。
 */

#pragma once

#include <QtWidgets>

#include "utils/Enums.h"
#include "whiteboard/Item.h"
#include "whiteboard/GraphicsStrokesGroup.h"
#include "whiteboard/GraphicsGroupContainerItem.h"

class Item;
class GraphicsScene;
class GraphicsStroke;

/**
 * @brief 一条笔迹对应的多边形图形项。
 *
 * 笔迹由 GraphicsStroke 生成填充多边形，本类负责显示、着色、擦除（多边形相减）与
 * 复制。笔迹可归属于一个 GraphicsStrokesGroup（同一页/同一批次的笔迹集合），
 * 也可作为组合的子项存在；这两种父子关系会影响坐标变换。
 * 对象拥有其 GraphicsStroke（通过 clearStroke 释放），但不拥有 strokesGroup。
 */
class GraphicsPolygonItem : public QGraphicsPolygonItem, public Item {
public:
    /**
     * @brief 构造空多边形项。
     * @param parent 父图形项，可为空。
     */
    GraphicsPolygonItem(QGraphicsItem *parent = 0);
    /**
     * @brief 由线段与线宽构造条状多边形。
     * @param line 笔迹线段。
     * @param pWidth 线宽。
     */
    GraphicsPolygonItem(const QLineF &line, qreal pWidth);
    /**
     * @brief 由线段与首尾不同线宽构造多边形（模拟笔压）。
     * @param pLine 笔迹线段。
     * @param pStartWidth 起点线宽。
     * @param pEndWidth 终点线宽。
     */
    GraphicsPolygonItem(const QLineF &pLine, qreal pStartWidth, qreal pEndWidth);
    /**
     * @brief 由现有多边形构造。
     * @param polygon 多边形顶点。
     * @param parent 父图形项，可为空。
     */
    GraphicsPolygonItem(const QPolygonF &polygon, QGraphicsItem *parent = 0);

    ~GraphicsPolygonItem();

    /** @brief 初始化图形项属性（标志、数据、委托）。 */
    void initialize();

    /** @brief 设置 UUID 并写入图形项数据。 */
    void setUuid(const QUuid &pUuid);

    /** @brief 设置所属的笔迹组，并处理与组合的父子关系。 */
    void setStrokesGroup(GraphicsStrokesGroup *group);
    /** @brief 返回所属的笔迹组，可能为空。 */
    GraphicsStrokesGroup *strokesGroup() const { return mpGroup; }
    /** @brief 设置填充色。 */
    void setColor(const QColor &color);

    /** @brief 返回当前填充色。 */
    QColor color() const;

    /** @brief 返回所属的 GraphicsScene，失败时返回空。 */
    virtual GraphicsScene *scene();

    /**
     * @brief 从本多边形中减去另一多边形（擦除）。
     * @param pi 要减去的多边形。
     * @note 仅当包围盒相交时才计算，结果变化后不再是原始线段。
     */
    inline void subtract(GraphicsPolygonItem *pi) {
        if (boundingRect().intersects(pi->boundingRect())) {
            QPolygonF subtractedPolygon = polygon().subtracted(pi->polygon());

            if (polygon() != subtractedPolygon) {
                mIsNominalLine = false;
                QGraphicsPolygonItem::setPolygon(subtractedPolygon);
            }
        }
    }

    /**
     * @brief 直接从本多边形中减去另一多边形，不做包围盒预判。
     * @param pi 要减去的多边形。
     */
    inline void subtractIntersecting(GraphicsPolygonItem *pi) {
        QPolygonF subtractedPolygon = polygon().subtracted(pi->polygon());

        if (polygon() != subtractedPolygon) {
            mIsNominalLine = false;
            QGraphicsPolygonItem::setPolygon(subtractedPolygon);
        }
    }

    enum { Type = GraphicsItemType::PolygonItemType };

    /** @brief 返回图形项类型标识。 */
    virtual int type() const { return Type; }

    /** @brief 替换多边形并标记为已编辑（不再是原始线段）。 */
    void setPolygon(const QPolygonF pPolygon) {
        mIsNominalLine = false;
        QGraphicsPolygonItem::setPolygon(pPolygon);
    }

    /** @brief 深拷贝笔迹多边形。 */
    virtual Item *deepCopy() const;

    /** @brief 复制颜色、多边形与笔迹参数。 */
    virtual void copyItemParameters(Item *copy) const;

    /** @brief 返回生成该多边形的原始线段。 */
    QLineF originalLine() { return mOriginalLine; }
    /** @brief 返回原始线宽。 */
    qreal originalWidth() { return mOriginalWidth; }
    /** @brief 是否仍为未编辑的原始线段。 */
    bool isNominalLine() { return mIsNominalLine; }

    /** @brief 设置是否视为原始线段（影响后续擦除与重绘方式）。 */
    void setNominalLine(bool isNominalLine) { mIsNominalLine = isNominalLine; }

    /** @brief 返回深色背景上的替代颜色。 */
    QColor colorOnDarkBackground() const { return mColorOnDarkBackground; }

    /** @brief 设置深色背景上的替代颜色。 */
    void setColorOnDarkBackground(QColor pColorOnDarkBackground) {
        mColorOnDarkBackground = pColorOnDarkBackground;
    }

    /** @brief 返回浅色背景上的替代颜色。 */
    QColor colorOnLightBackground() const { return mColorOnLightBackground; }

    /** @brief 设置浅色背景上的替代颜色。 */
    void setColorOnLightBackground(QColor pColorOnLightBackground) {
        mColorOnLightBackground = pColorOnLightBackground;
    }

    /** @brief 关联生成此多边形的笔迹对象，本类接管其所有权。 */
    void setStroke(GraphicsStroke *stroke);
    /** @brief 返回关联的笔迹对象，可能为空。 */
    GraphicsStroke *stroke() const;

protected:
    /** @brief 绘制多边形；原始线段可能按动态颜色绘制。 */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
    void clearStroke();

    bool mHasAlpha;

    QLineF mOriginalLine;
    qreal mOriginalWidth;
    bool mIsNominalLine;

    QColor mColorOnDarkBackground;
    QColor mColorOnLightBackground;

    GraphicsStroke *mStroke;
    GraphicsStrokesGroup *mpGroup;
};
