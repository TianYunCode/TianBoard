/**
 * @file GraphicsStroke.h
 * @brief 一次落笔的笔迹对象：收集采样点、插值、简化并登记生成的多边形图元。
 */

#pragma once

#include <QtWidgets>

#include "utils/Enums.h"

class GraphicsPolygonItem;
class GraphicsScene;

/**
 * @brief 笔迹（一次按压画出的连续轨迹）
 *
 * 输入设备按下到抬起之间的采样点（坐标 + 压感笔宽）经 addPoint() 喂入；
 * 内部区分两组点：mReceivedPoints 是原始采样点，mDrawnPoints 是插值后
 * 实际落画的点。生成的 GraphicsPolygonItem 通过 addPolygon() 登记到这里，
 * 多边形图元反向持有本对象指针，本类析构时只解绑、不删除多边形
 * （图元所有权在场景）。simplify() 用共线合并算法减点并重建多边形，
 * 返回一个新笔迹，原笔迹由调用方处置。
 *
 * 非 QObject，只能在 GUI 线程使用。
 */
class GraphicsStroke
{
    friend class GraphicsPolygonItem;

public:
    /// @brief 构造笔迹；scene 供 simplify()/几何换算取用，可为空但为空时不能简化
    GraphicsStroke(GraphicsScene *scene = NULL);
    /// @brief 析构：解绑全部多边形的回指指针，不删除图元本身
    virtual ~GraphicsStroke();

    /// @brief 笔迹是否带压感（存在非等宽或非标称线段即视为带压感）
    bool hasPressure();

    /// @brief 本笔迹登记的全部多边形图元
    QList<GraphicsPolygonItem *> polygons() const;

    /// @brief 从笔迹中摘除一个多边形（只解除登记，不删除图元）
    void remove(GraphicsPolygonItem *polygonItem);

    /// @brief 创建一个空笔迹副本（点与多边形均不复制），由调用方再挂多边形
    GraphicsStroke *deepCopy();

    /// @brief 首个多边形的颜色是否半透明（决定叠加处要不要做减法）
    bool hasAlpha() const;

    /// @brief 清空多边形登记（不删除图元）
    void clear();

    /**
     * @brief 喂入一个采样点
     * @param point 场景坐标
     * @param width 该点笔宽（已按系统缩放与页面缩放折算）
     * @param interpolate true 时用二次贝塞尔在相邻采样点中点之间平滑
     * @return 本次新产生的待绘制点列，调用方拿去 drawCurve() 生成图元
     */
    QList<QPair<QPointF, qreal>> addPoint(const QPointF &point, qreal width, bool interpolate = false);

    /// @brief 插值后实际落画的点列（坐标 + 笔宽）
    const QList<QPair<QPointF, qreal>> &points()
    {
        return mDrawnPoints;
    }

    /// @brief 生成简化后的新笔迹（点数少于 3 返回 NULL）；新笔迹的多边形未入场景
    GraphicsStroke *simplify();

protected:
    /// @brief 登记一个多边形到本笔迹（已存在则先摘除，避免重复）
    void addPolygon(GraphicsPolygonItem *pol);

private:
    // 所属场景，仅取用其多边形工厂与缩放信息，不拥有
    GraphicsScene *mScene;

    // 本笔迹登记的多边形图元，所有权在场景
    QList<GraphicsPolygonItem *> mPolygons;

    // 原始采样点（坐标 + 笔宽）
    QList<QPair<QPointF, qreal>> mReceivedPoints;

    // 插值后实际落画的点
    QList<QPair<QPointF, qreal>> mDrawnPoints;

    // 系统缩放 × 页面缩放的倒数（屏幕像素折算回场景尺寸）；目前仅构造时记录，未被读取
    qreal mAntiScaleRatio;
};
