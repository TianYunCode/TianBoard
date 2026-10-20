/**
 * @file tst_geometryutils.cpp
 * @brief 守住 GeometryUtils 中绘图工具直接依赖的三条几何契约：点约束到矩形、
 *        三点夹角、二次贝塞尔采样。
 */

#include <QtTest>

#include "utils/GeometryUtils.h"

class TestGeometryUtils : public QObject
{
    Q_OBJECT

private slots:
    /// @brief 浮点版把越界点夹到矩形边界上、界内点原样返回，0x0 的退化矩形把点钉在其原点。
    void pointConstrainedInRectClamps();
    /// @brief 整数重载与浮点版给出同样的夹紧结果，调用方不会因重载选错而拿到未约束的坐标。
    void pointConstrainedInRectIntegerOverload();
    /// @brief 顶点上的直角、平角以及"顶点与端点重合"的退化情形都返回确定角度，绝不为 NaN。
    void angleAtMiddlePoint();
    /// @brief nPoints 得到 nPoints+1 个采样点且首尾为曲线端点，nPoints 不足 2 时返回空列表。
    void quadraticBezierSampling();
};

void TestGeometryUtils::pointConstrainedInRectClamps()
{
    const QRectF rect(10, 20, 100, 50);

    QCOMPARE(GeometryUtils::pointConstrainedInRect(QPointF(5, 5), rect), QPointF(10, 20));
    QCOMPARE(GeometryUtils::pointConstrainedInRect(QPointF(500, 500), rect), QPointF(110, 70));
    QCOMPARE(GeometryUtils::pointConstrainedInRect(QPointF(40, 30), rect), QPointF(40, 30));

    // 退化成 0x0 的矩形把所有点都钉在它的原点上。
    QCOMPARE(GeometryUtils::pointConstrainedInRect(QPointF(1, 2), QRectF(7, 8, 0, 0)), QPointF(7, 8));
}

void TestGeometryUtils::pointConstrainedInRectIntegerOverload()
{
    const QRect rect(10, 20, 100, 50);

    QCOMPARE(GeometryUtils::pointConstrainedInRect(QPoint(-30, 0), rect), QPoint(10, 20));
    QCOMPARE(GeometryUtils::pointConstrainedInRect(QPoint(1234, 99), rect), QPoint(110, 70));
    QCOMPARE(GeometryUtils::pointConstrainedInRect(QPoint(40, 30), rect), QPoint(40, 30));
}

void TestGeometryUtils::angleAtMiddlePoint()
{
    // 中间那个点是直角顶点。
    // 结果要落在 acos(1e-16) 这类浮点噪声之内仍然算正确。
    QVERIFY(qAbs(GeometryUtils::angle(QPointF(0, 0), QPointF(0, 1), QPointF(1, 1)) - 90.0) < 1e-9);

    // 三点共线，中间点被穿过，夹角应为 180 度。
    QVERIFY(qAbs(GeometryUtils::angle(QPointF(0, 0), QPointF(1, 0), QPointF(2, 0)) - 180.0) < 1e-9);

    // 退化分支（中间点与某个端点重合）正是绘图工具在笔画里出现重复采样点时会走到的
    // 那条路，所以它必须返回一个数，而不是 NaN。
    const qreal degenerate = GeometryUtils::angle(QPointF(3, 3), QPointF(3, 3), QPointF(9, 9));
    QVERIFY(!qIsNaN(degenerate));
    QVERIFY(qAbs(degenerate - 180.0) < 1e-9);
}

void TestGeometryUtils::quadraticBezierSampling()
{
    const QList<QPointF> points = GeometryUtils::quadraticBezier(QPointF(0, 0), QPointF(50, 100), QPointF(100, 0), 10);

    // nPoints 请求的是 nPoints + 1 个采样点，其中第一个与最后一个正好是曲线的两个端点。
    QCOMPARE(points.size(), 11);
    QCOMPARE(points.first(), QPointF(0, 0));
    QCOMPARE(points.last(), QPointF(100, 0));

    // 控制点左右对称，中间那个采样点必然落在对称轴上。
    QVERIFY(qFuzzyCompare(points.at(5).x(), 50.0));

    // 段数小于 2 时采样没有意义，返回空列表而不是崩溃或只给一个点。
    QCOMPARE(GeometryUtils::quadraticBezier(QPointF(0, 0), QPointF(1, 1), QPointF(2, 0), 1).size(), 0);
    QCOMPARE(GeometryUtils::quadraticBezier(QPointF(0, 0), QPointF(1, 1), QPointF(2, 0), 0).size(), 0);
}

QTEST_MAIN(TestGeometryUtils)
#include "tst_geometryutils.moc"
