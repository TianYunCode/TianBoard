/**
 * @file Snapping.h
 * @brief 画板网格吸附的纯几何计算。
 *
 * 拖动图元时把它吸到最近的网格交点上，是白板上最常用的对齐手段。这里只做与场景无关的
 * 数学，便于脱离整个应用栈单独测试；网格尺寸、当前背景类型这些状态由 GraphicsScene 提供。
 */

#pragma once

#include <QPointF>

#include <QList>

/// @brief 网格吸附的纯计算入口。
namespace Snapping
{
    /**
     * @brief 一次吸附的结果。
     *
     * offset 是把图元挪到吸附位置所需的位移量；force 是吸得力，取“吸附点到原点的曼哈顿距离
     * 占一格的比例”的反值，越接近 1 表示原本就离网格交点越近。
     */
    struct Result
    {
        QPointF offset;       ///< 需要叠加的位移；不该吸附时为 (0,0)
        qreal   force  = 0.0; ///< 0..1 的吸得力
        int     corner = -1;  ///< 参与吸附的角点下标；单点吸附时为 -1
    };

    /**
     * @brief 把一个点吸到最近的网格交点。
     * @param point 当前点（场景坐标）。
     * @param gridSize 一格边长，必须为正。
     * @param snapX 是否也吸 X：只有页面上真有竖线（或干脆空白页）时才该吸，横线稿纸上吸 X
     *              会把图元推到没有线可对的位置。
     * @param proposed 备选点；其吸得力更高时改吸它，用于同一次拖动里的候选位置择优。
     * @param hasProposed proposed 是否有效。
     */
    Result toGrid(const QPointF &point, qreal gridSize, bool snapX, const QPointF &proposed = QPointF(), bool hasProposed = false);

    /**
     * @brief 在一组图元角点中挑吸得力最强的那个，返回它的位移。
     * @param corners 角点列表（场景坐标），通常是图元或选框的四角。
     * @param gridSize 一格边长。
     * @param snapX 是否也吸 X。
     */
    Result bestOfCorners(const QList<QPointF> &corners, qreal gridSize, bool snapX);

    /**
     * @brief 把旋转角吸到最接近的常用角度。
     * @param angle 累计角度（度，任意范围）。
     * @param steps 候选步长，按优先级排列（例如 45 在前、30 在后）。
     * @param tolerance 容差：离某个整数倍不超过它才吸。
     * @return 吸完并折回 0..360 的角度；没有一个步长够近时原样返回（同样折回 0..360）。
     */
    qreal toNearestAngle(qreal angle, const QList<qreal> &steps, qreal tolerance);
} // namespace Snapping
