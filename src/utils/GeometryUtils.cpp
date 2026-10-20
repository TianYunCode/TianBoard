#include "utils/GeometryUtils.h"

const double PI                                            = 4.0 * atan(1.0);
const int    GeometryUtils::centimeterGraduationHeight     = 15;
const int    GeometryUtils::halfCentimeterGraduationHeight = 10;
const int    GeometryUtils::millimeterGraduationHeight     = 5;
const int    GeometryUtils::millimetersPerCentimeter       = 10;
const int    GeometryUtils::millimetersPerHalfCentimeter   = 5;
const float  GeometryUtils::inchSize                       = 25.4f;

GeometryUtils::GeometryUtils()
{
    // 刻意留空
}

GeometryUtils::~GeometryUtils()
{
    // 刻意留空
}

QPolygonF GeometryUtils::lineToPolygon(const QLineF &pLine, const qreal &pWidth)
{
    qreal x1          = pLine.x1();
    qreal y1          = pLine.y1();

    qreal x2          = pLine.x2();
    qreal y2          = pLine.y2();

    // 法线方向：把线段角度旋转 90 度，再按半笔宽求偏移量
    qreal alpha       = (90.0 - pLine.angle()) * PI / 180.0;
    qreal hypothenuse = pWidth / 2;

    qreal opposite    = sin(alpha) * hypothenuse;
    qreal adjacent    = cos(alpha) * hypothenuse;

    QPointF p1a(x1 - adjacent, y1 - opposite);
    QPointF p1b(x1 + adjacent, y1 + opposite);

    QPointF p2a(x2 - adjacent, y2 - opposite);
    QPointF p2b(x2 + adjacent, y2 + opposite);

    QPainterPath painterPath;

    // 先沿一侧走到底，用半圆封口，再沿另一侧返回，最后封住起点
    painterPath.moveTo(p1a);
    painterPath.lineTo(p2a);

    painterPath.arcTo(x2 - hypothenuse, y2 - hypothenuse, pWidth, pWidth, (90.0 + pLine.angle()), -180.0);

    painterPath.lineTo(p1b);

    painterPath.arcTo(x1 - hypothenuse, y1 - hypothenuse, pWidth, pWidth, -1 * (90.0 - pLine.angle()), -180.0);

    painterPath.closeSubpath();

    return painterPath.toFillPolygon();
}

QPolygonF GeometryUtils::lineToPolygon(const QLineF &pLine, const qreal &pStartWidth, const qreal &pEndWidth)
{
    qreal x1               = pLine.x1();
    qreal y1               = pLine.y1();

    qreal x2               = pLine.x2();
    qreal y2               = pLine.y2();

    qreal alpha            = (90.0 - pLine.angle()) * PI / 180.0;
    qreal startHypothenuse = pStartWidth / 2;
    qreal endHypothenuse   = pEndWidth / 2;

    qreal startOpposite    = sin(alpha) * startHypothenuse;
    qreal startAdjacent    = cos(alpha) * startHypothenuse;

    qreal endOpposite      = sin(alpha) * endHypothenuse;
    qreal endAdjacent      = cos(alpha) * endHypothenuse;

    QPointF p1a(x1 - startAdjacent, y1 - startOpposite);
    QPointF p1b(x1 + startAdjacent, y1 + startOpposite);

    QPointF p2a(x2 - endAdjacent, y2 - endOpposite);
    QPointF p2b(x2 + endAdjacent, y2 + endOpposite);

    QPainterPath painterPath;
    painterPath.moveTo(p1a);
    painterPath.lineTo(p2a);

    painterPath.arcTo(x2 - endHypothenuse, y2 - endHypothenuse, pEndWidth, pEndWidth, (90.0 + pLine.angle()), -180.0);

    painterPath.lineTo(p1b);

    painterPath.arcTo(x1 - startHypothenuse, y1 - startHypothenuse, pStartWidth, pStartWidth, -1 * (90.0 - pLine.angle()), -180.0);

    painterPath.closeSubpath();

    return painterPath.toFillPolygon();
}

QPolygonF GeometryUtils::lineToPolygon(const QPointF &pStart, const QPointF &pEnd, const qreal &pStartWidth, const qreal &pEndWidth)
{
    qreal x1 = pStart.x();
    qreal y1 = pStart.y();

    qreal x2 = pEnd.x();
    qreal y2 = pEnd.y();

    QLineF line(pStart, pEnd);

    qreal alpha            = (90.0 - line.angle()) * PI / 180.0;
    qreal hypothenuseStart = pStartWidth / 2;

    qreal hypothenuseEnd   = pEndWidth / 2;

    qreal sinAlpha         = sin(alpha);
    qreal cosAlpha         = cos(alpha);

    qreal oppositeStart    = sinAlpha * hypothenuseStart;
    qreal adjacentStart    = cosAlpha * hypothenuseStart;

    QPointF p1a(x1 - adjacentStart, y1 - oppositeStart);
    QPointF p1b(x1 + adjacentStart, y1 + oppositeStart);

    qreal oppositeEnd = sinAlpha * hypothenuseEnd;
    qreal adjacentEnd = cosAlpha * hypothenuseEnd;

    QPointF p2a(x2 - adjacentEnd, y2 - oppositeEnd);

    QPainterPath painterPath;

    painterPath.moveTo(p1a);
    painterPath.lineTo(p2a);

    painterPath.arcTo(x2 - hypothenuseEnd, y2 - hypothenuseEnd, pEndWidth, pEndWidth, (90.0 + line.angle()), -180.0);

    painterPath.lineTo(p1b);

    painterPath.arcTo(x1 - hypothenuseStart, y1 - hypothenuseStart, pStartWidth, pStartWidth, -1 * (90.0 - line.angle()), -180.0);

    painterPath.closeSubpath();

    return painterPath.toFillPolygon();
}

QPolygonF GeometryUtils::arcToPolygon(const QLineF &startRadius, qreal spanAngleInDegrees, qreal width)
{
    qreal startAngleInDegrees = -startRadius.angle();
    if (startAngleInDegrees > 180)
    {
        startAngleInDegrees -= 360;
    }
    else if (startAngleInDegrees < -180)
    {
        startAngleInDegrees += 360;
    }

    qreal radiusLength = startRadius.length();
    qreal angle        = 2 * asin(width / (2 * radiusLength)) * 180 / PI;
    bool  overlap      = qAbs(spanAngleInDegrees) > 360 - angle;
    if (overlap)
    {
        spanAngleInDegrees = spanAngleInDegrees < 0 ? -360 : 360;
    }

    qreal endAngleInDegrees = startAngleInDegrees + spanAngleInDegrees;

    qreal  innerRadius      = radiusLength - width / 2;
    QRectF innerSquare(startRadius.p1().x() - innerRadius, startRadius.p1().y() - innerRadius, 2 * innerRadius, 2 * innerRadius);
    qreal  outerRadius = radiusLength + width / 2;
    QRectF outerSquare(startRadius.p1().x() - outerRadius, startRadius.p1().y() - outerRadius, 2 * outerRadius, 2 * outerRadius);
    QRectF startSquare(startRadius.p2().x() - width / 2, startRadius.p2().y() - width / 2, width, width);
    QRectF endSquare(startRadius.p1().x() + radiusLength * cos(endAngleInDegrees * PI / 180.0) - width / 2,
        startRadius.p1().y() + radiusLength * sin(endAngleInDegrees * PI / 180.0) - width / 2, width, width);

    QPainterPath painterPath(QPointF(startRadius.p1().x() + innerRadius * cos(startAngleInDegrees * PI / 180.0),
        startRadius.p1().y() + innerRadius * sin(startAngleInDegrees * PI / 180.0)));
    startAngleInDegrees = -startAngleInDegrees;
    endAngleInDegrees   = -endAngleInDegrees;
    spanAngleInDegrees  = -spanAngleInDegrees;

    painterPath.setFillRule(Qt::WindingFill);
    painterPath.arcTo(innerSquare, startAngleInDegrees, spanAngleInDegrees);
    painterPath.arcTo(endSquare, 180.0 + endAngleInDegrees, spanAngleInDegrees > 0 ? -180.0 : 180.0);
    painterPath.arcTo(outerSquare, endAngleInDegrees, -spanAngleInDegrees);
    painterPath.arcTo(startSquare, startAngleInDegrees, spanAngleInDegrees > 0 ? -180.0 : 180.0);
    painterPath.closeSubpath();

    return painterPath.toFillPolygon();
}

/**
 * @brief 由点列（至少 2 个）与起止笔画宽度构造描边多边形。
 *
 * 多边形经过所有给定点；每个点处的粗细按起止宽度线性插值，段与段之间用近似的圆弧接头。
 * 与 lineToPolygon 一致，两端为半圆封口。
 */
QPolygonF GeometryUtils::curveToPolygon(const QList<QPointF> &points, qreal startWidth, qreal endWidth)
{
    int n_points = points.size();

    if (n_points < 2)
    {
        return QPolygonF();
    }

    // 只有两个点时退化为普通线段，避免走逐点插值的分支
    if (n_points == 2)
    {
        return lineToPolygon(points[0], points[1], startWidth, endWidth);
    }

    QList<QPair<QPointF, qreal>> pointsAndWidths;
    for (int i(0); i < n_points; ++i)
    {
        qreal width = startWidth + (qreal(i) / qreal(n_points - 1)) * (endWidth - startWidth);

        pointsAndWidths << QPair<QPointF, qreal>(points[i], width);
    }

    return curveToPolygon(pointsAndWidths, true, true);
}

/**
 * @brief 由逐点粗细的点列（至少 2 个）构造描边多边形。
 *
 * 多边形经过所有点，段间为近似圆弧接头；roundStart/roundEnd 控制两端是否用圆弧封口。
 */
QPolygonF GeometryUtils::curveToPolygon(const QList<QPair<QPointF, qreal>> &points, bool roundStart, bool roundEnd)
{
    int n_points = points.size();

    if (n_points == 0)
    {
        return QPolygonF();
    }
    // 单点没有方向，只能按该点宽度画一个圆头，交给线段重载处理
    if (n_points == 1)
    {
        return lineToPolygon(points.first().first, points.first().first, points.first().second, points.first().second);
    }

    qreal startWidth = points.first().second;
    qreal endWidth   = points.last().second;

    /* 顶点（x）按笔画宽度与角度、以及给定点（o）的位置计算：

          x----------x--------x

          o          o        o

          x----------x -------x

       每个 o 点上下对应的两个 x 暂时成对保存。
     */

    typedef QPair<QPointF, QPointF> pointPair;
    QList<pointPair>                newPoints;

    // 起点法线决定第一对顶点
    QLineF firstSegment = QLineF(points[0].first, points[1].first);
    QLineF normal       = firstSegment.normalVector();
    normal.setLength(startWidth / 2.0);
    newPoints << pointPair(normal.p2(), points[0].first - QPointF(normal.dx(), normal.dy()));

    /*
    中间点的顶点（下图的 d1、d2）取决于前后两段的夹角，计算稍复杂：

                      d1
         ------------x
                      \
         .a      b .   \
                        \
         --------x       \
               d2 \       \
                   \   .c  \

    其中 a、b、c 是 points 里依次给出的点。

    注意：示意图并不精确——这里不做尖角接头，而是近似圆角接头
    （b 到 d1 的距离等于笔画宽度的一半）。
    */

    for (int i(1); i < n_points - 1; ++i)
    {
        // 用前后点的连线求法线，使接头在拐角处平滑过渡
        QLineF cornerNormal = (QLineF(points[i - 1].first, points[i + 1].first)).normalVector();
        cornerNormal.setLength(points[i].second / 2.0);
        QPointF d1 = points[i].first + QPointF(cornerNormal.dx(), cornerNormal.dy());
        QPointF d2 = points[i].first - QPointF(cornerNormal.dx(), cornerNormal.dy());

        newPoints << pointPair(d1, d2);
    }

    // 末点与首点处理方式相同
    QLineF lastSegment = QLineF(points[n_points - 2].first, points[n_points - 1].first);
    normal             = lastSegment.normalVector();
    normal.setLength(endWidth / 2.0);

    QPointF d1 = points.last().first + QPointF(normal.dx(), normal.dy());
    QPointF d2 = points.last().first - QPointF(normal.dx(), normal.dy());

    newPoints << pointPair(d1, d2);

    QPainterPath path;
    path.setFillRule(Qt::WindingFill);
    path.moveTo(newPoints[0].first);

    for (int i(1); i < n_points; ++i)
    {
        path.lineTo(newPoints[i].first);
    }

    if (roundEnd)
    {
        path.arcTo(points.last().first.x() - endWidth / 2.0, points.last().first.y() - endWidth / 2.0, endWidth, endWidth,
            (90.0 + lastSegment.angle()), -180.0);
    }
    else
    {
        path.lineTo(newPoints.last().second);
    }

    for (int i(n_points - 1); i >= 0; --i)
    {
        path.lineTo(newPoints[i].second);
    }

    if (roundStart)
    {
        path.arcTo(points[0].first.x() - startWidth / 2.0, points[0].first.y() - startWidth / 2.0, startWidth, startWidth,
            (firstSegment.angle() - 90.0), -180.0);
    }
    else
    {
        path.lineTo(newPoints[0].first);
    }

    return path.toFillPolygon();
}

QPointF GeometryUtils::pointConstrainedInRect(QPointF point, QRectF rect)
{
    return QPointF(qMax(rect.x(), qMin(rect.x() + rect.width(), point.x())), qMax(rect.y(), qMin(rect.y() + rect.height(), point.y())));
}

QPoint GeometryUtils::pointConstrainedInRect(QPoint point, QRect rect)
{
    return QPoint(qMax(rect.x(), qMin(rect.x() + rect.width(), point.x())), qMax(rect.y(), qMin(rect.y() + rect.height(), point.y())));
}

QRectF GeometryUtils::lineToInnerRect(const QLineF &pLine, const qreal &pWidth)
{
    qreal centerX  = (pLine.x1() + pLine.x2()) / 2;
    qreal centerY  = (pLine.y1() + pLine.y2()) / 2;

    // 以 pWidth 为对角线求正方形边长，得到一个与线宽同尺度的中心方框
    qreal side     = sqrt((pWidth * pWidth) / 2);
    qreal halfSide = side / 2;

    return QRectF(centerX - halfSide, centerY - halfSide, side, side);
}

void GeometryUtils::crashPointList(QVector<QPointF> &points)
{
    // 原地去掉连续重复点：重复点会让法线计算退化，导致描边出现尖刺
    int position = 1;

    while (position < points.size())
    {
        if (points.at(position) == points.at(position - 1))
        {
            points.remove(position);
        }
        else
        {
            ++position;
        }
    }
}

/**
 * @brief 返回三点之间以 p2 为顶点的夹角（角度制）。
 */
qreal GeometryUtils::angle(const QPointF &p1, const QPointF &p2, const QPointF &p3)
{
    // 用余弦定理：B 处的角 = arccos((a^2-b^2+c^2)/(2ac))，a、b、c 分别为 A、B、C 的对边

    qreal a, b, c, beta;
    a = qSqrt(qPow(p2.x() - p3.x(), 2) + qPow(p2.y() - p3.y(), 2));
    b = qSqrt(qPow(p1.x() - p3.x(), 2) + qPow(p1.y() - p3.y(), 2));
    c = qSqrt(qPow(p1.x() - p2.x(), 2) + qPow(p1.y() - p2.y(), 2));

    // 退化（顶点与端点重合）时直接返回 180 度，避免除零产生 NaN
    if (a == 0 || c == 0)
    {
        beta = M_PI;
    }
    else
    {
        beta = qAcos(std::max(-1.0, std::min(1.0, (a * a - b * b + c * c) / (2 * a * c))));
    }

    return qRadiansToDegrees(beta);
}

/**
 * @brief 采样二次贝塞尔曲线并返回点列。
 * @param p0 曲线起点。
 * @param p1 控制点。
 * @param p2 曲线终点。
 * @param nPoints 采样段数；返回 nPoints + 1 个点（含首尾）。
 * @return 可用于绘制的点列；nPoints <= 1 时返回空列表。
 */
QList<QPointF> GeometryUtils::quadraticBezier(const QPointF &p0, const QPointF &p1, const QPointF &p2, unsigned int nPoints)
{
    QPainterPath path(p0);
    path.quadTo(p1, p2);

    QList<QPointF> points;

    if (nPoints <= 1)
    {
        return points;
    }

    for (unsigned int i(0); i <= nPoints; ++i)
    {
        qreal percent = qreal(i) / qreal(nPoints);
        points << path.pointAtPercent(percent);
    }

    return points;
}
