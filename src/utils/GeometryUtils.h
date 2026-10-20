/**
 * @file GeometryUtils.h
 * @brief 声明把线段、圆弧与曲线离散成多边形的几何工具。
 */

#pragma once

#include <QtWidgets>

/**
 * @brief 几何计算的静态工具集。
 *
 * 位于 utils 层（纯工具）：只处理 QPolygonF/QPointF 等几何数据，不依赖画板状态。
 * 不可实例化；所有函数无副作用，失败时返回空多边形或原值。
 * 结果坐标与输入处于同一坐标系，调用方自行负责视图变换。
 */
class GeometryUtils {
private:
    /** @brief 私有构造，禁止实例化。 */
    GeometryUtils();
    /** @brief 私有析构。 */
    virtual ~GeometryUtils();

public:
    /** @brief 把等宽线段转成矩形多边形。 */
    static QPolygonF lineToPolygon(const QLineF &pLine, const qreal &pWidth);
    /** @brief 把两端宽度不同的线段转成梯形多边形。 */
    static QPolygonF lineToPolygon(const QLineF &pLine, const qreal &pStartWidth, const qreal &pEndWidth);
    /** @brief 返回线段按宽度膨胀后的内接矩形。 */
    static QRectF lineToInnerRect(const QLineF &pLine, const qreal &pWidth);

    /** @brief 把以 startRadius 为半径基准的圆弧离散成带宽度的多边形。 */
    static QPolygonF arcToPolygon(const QLineF &startRadius, qreal spanAngle, qreal width);

    /** @brief 把两端宽度不同的线段转成多边形。 */
    static QPolygonF lineToPolygon(const QPointF &pStart, const QPointF &pEnd, const qreal &pStartWidth,
                                   const qreal &pEndWidth);
    /** @brief 把折线按线性变化的笔宽转成多边形。 */
    static QPolygonF curveToPolygon(const QList<QPointF> &points, qreal startWidth, qreal endWidth);
    /** @brief 把带逐点宽度的曲线转成多边形，可指定两端是否画圆头。 */
    static QPolygonF curveToPolygon(const QList<QPair<QPointF, qreal>> &points, bool roundStart,
                                    bool roundEnd);

    /** @brief 把点约束到矩形范围内（浮点版）。 */
    static QPointF pointConstrainedInRect(QPointF point, QRectF rect);
    /** @brief 把点约束到矩形范围内（整数版）。 */
    static QPoint pointConstrainedInRect(QPoint point, QRect rect);

    /** @brief 对点列做抽稀，减少后续绘制的顶点数量。 */
    static void crashPointList(QVector<QPointF> &points);

    /** @brief 返回 p1-p2-p3 形成的夹角（角度制）。 */
    static qreal angle(const QPointF &p1, const QPointF &p2, const QPointF &p3);

    /** @brief 在二次贝塞尔曲线上采样 nPoints 个点。 */
    static QList<QPointF> quadraticBezier(const QPointF &p0, const QPointF &p1, const QPointF &p2,
                                          unsigned int nPoints);

    const static int centimeterGraduationHeight;     ///< 厘米刻度线高度。
    const static int halfCentimeterGraduationHeight; ///< 半厘米刻度线高度。
    const static int millimeterGraduationHeight;     ///< 毫米刻度线高度。
    const static int millimetersPerCentimeter;       ///< 每厘米的毫米数。
    const static int millimetersPerHalfCentimeter;   ///< 每半厘米的毫米数。
    const static float inchSize;                     ///< 一英寸的像素尺寸。
};
