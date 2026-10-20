#include "utils/Snapping.h"

#include <algorithm>
#include <cmath>

namespace
{
    /// @brief 把坐标落到最近的一条网格线上。
    qreal nearestLine(qreal coordinate, qreal gridSize)
    {
        const qreal floorLine = std::floor(coordinate / gridSize) * gridSize;
        return (coordinate - floorLine < gridSize / 2.0) ? floorLine : floorLine + gridSize;
    }

    /// @brief 吸得力：吸附点离原点多近，一格之内线性折算成 0..1。
    qreal forceOf(const QPointF &from, const QPointF &target, qreal gridSize)
    {
        return std::max(1.0 - (target - from).manhattanLength() / gridSize, 0.0);
    }
} // namespace

namespace Snapping
{
    Result toGrid(const QPointF &point, qreal gridSize, bool snapX, const QPointF &proposed, bool hasProposed)
    {
        if (gridSize <= 0.0)
        {
            return Result(); // 网格尺寸来自用户设置，为 0 时没什么可对
        }

        Result result;
        result.offset = QPointF(snapX ? nearestLine(point.x(), gridSize) : point.x(), nearestLine(point.y(), gridSize)) - point;
        result.force  = forceOf(point, point + result.offset, gridSize);

        if (hasProposed)
        {
            const qreal proposedForce = forceOf(point, proposed, gridSize);
            if (proposedForce > result.force)
            {
                result.offset = proposed - point;
                result.force  = proposedForce;
            }
        }

        return result;
    }

    Result bestOfCorners(const QList<QPointF> &corners, qreal gridSize, bool snapX)
    {
        Result best;
        if (corners.isEmpty())
        {
            return best;
        }

        for (int i = 0; i < corners.size(); i++)
        {
            const Result candidate = toGrid(corners.at(i), gridSize, snapX);
            if (i == 0 || candidate.force > best.force)
            {
                best        = candidate;
                best.corner = i;
            }
        }

        return best;
    }
} // namespace Snapping
