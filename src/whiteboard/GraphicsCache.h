/**
 * @file GraphicsCache.h
 * @brief 遮罩（聚光灯）图元声明。
 */

#pragma once

#include <QColor>
#include <QGraphicsSceneMouseEvent>

#include "whiteboard/Item.h"
#include "utils/Enums.h"

/// @brief 遮罩孔洞的形状。
typedef enum
{
    eMaskShape_Circle,  ///< 圆形孔洞
    eMaskShap_Rectangle ///< 矩形孔洞
} eMaskShape;

/**
 * @brief 覆盖整页的遮罩图元，用奇偶填充在遮罩上挖出跟随鼠标的孔洞。
 *
 * 每个场景至多一个实例，通过 instance(scene) 取得并登记在静态表中；
 * 实例以场景为键，场景销毁或显式 delete 时从表中移除。图元尺寸取控制视图
 * 的 30 倍，保证缩放后仍能盖住可视区域。
 *
 * 生命周期：归场景所有（作为场景图元），静态表不持有所有权。
 * 线程约束：鼠标事件与绘制都在 GUI 线程。
 */
class GraphicsCache : public QGraphicsRectItem, public Item
{
public:
    /// @brief 取得场景对应的遮罩实例，不存在时创建。
    static GraphicsCache *instance(GraphicsScene *scene);
    ~GraphicsCache();

    /// @brief 图元类型标识。
    enum
    {
        Type = GraphicsItemType::cacheItemType
    };

    /// @brief 返回图元类型。
    virtual int type() const
    {
        return Type;
    }

    /// @brief 复制遮罩参数；遮罩是场景级单例，副本使用同一场景。
    virtual Item *deepCopy() const;

    /// @brief 复制位置、矩形与变换到目标实例。
    virtual void copyItemParameters(Item *copy) const;

    /// @brief 孔洞外遮罩的颜色。
    QColor maskColor();
    /// @brief 设置遮罩颜色并请求重绘。
    void setMaskColor(QColor color);
    /// @brief 当前孔洞形状。
    eMaskShape maskshape();
    /// @brief 设置孔洞形状并请求重绘。
    void setMaskShape(eMaskShape shape);
    /// @brief 孔洞半径（圆形为半径，矩形为半边长）。
    int shapeWidth();
    /// @brief 设置孔洞半径并请求重绘。
    void setShapeWidth(int width);

protected:
    /// @brief 用奇偶填充把矩形遮罩与孔洞合成，得到“挖洞”效果。
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    /// @brief 拖动时更新孔洞中心并局部刷新旧的与新的孔洞区域。
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 按下时开启孔洞并记录初始位置与半径。
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 松开时关闭孔洞，仅保留遮罩本体。
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
    static QMap<GraphicsScene *, GraphicsCache *> sInstances; ///< 场景到遮罩实例的映射

    QColor         mMaskColor;                                ///< 遮罩颜色
    eMaskShape     mMaskShape;                                ///< 孔洞形状
    int            mShapeWidth;                               ///< 孔洞半径
    bool           mDrawMask;                                 ///< 是否正在显示孔洞
    QPointF        mShapePos;                                 ///< 孔洞中心（本地坐标）
    int            mOldShapeWidth;                            ///< 上一次的孔洞半径，用于计算刷新区域
    QPointF        mOldShapePos;                              ///< 上一次的孔洞中心，用于计算刷新区域
    GraphicsScene *mScene;                                    ///< 所属场景，不持有所有权

    GraphicsCache(GraphicsScene *scene);

    /// @brief 允许移动与选中，并启用几何变更通知。
    void init();
    /// @brief 根据新旧孔洞位置计算需要重绘的矩形。
    QRectF updateRect(QPointF currentPoint);
};
