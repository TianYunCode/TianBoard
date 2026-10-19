#include <QtTest>

#include "utils/GeometryUtils.h"

class TestGeometryUtils : public QObject
{
    Q_OBJECT

private slots:
    void pointConstrainedInRectClamps();
    void pointConstrainedInRectIntegerOverload();
    void angleAtMiddlePoint();
    void quadraticBezierSampling();
};

void TestGeometryUtils::pointConstrainedInRectClamps()
{
    const QRectF rect(10, 20, 100, 50);

    QCOMPARE(GeometryUtils::pointConstrainedInRect(QPointF(5, 5), rect), QPointF(10, 20));
    QCOMPARE(GeometryUtils::pointConstrainedInRect(QPointF(500, 500), rect), QPointF(110, 70));
    QCOMPARE(GeometryUtils::pointConstrainedInRect(QPointF(40, 30), rect), QPointF(40, 30));

    // A degenerate rect pins everything to its origin.
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
    // Right angle at the middle argument.
    // comes out right within the floating point noise of acos(1e-16).
    QVERIFY(qAbs(GeometryUtils::angle(QPointF(0, 0), QPointF(0, 1), QPointF(1, 1)) - 90.0) < 1e-9);

    // Straight line through the middle point.
    QVERIFY(qAbs(GeometryUtils::angle(QPointF(0, 0), QPointF(1, 0), QPointF(2, 0)) - 180.0) < 1e-9);

    // The degenerate branches (middle point coinciding with an endpoint) are the
    // ones the drawing tools hit when a stroke has duplicated points, so they must
    // return a number rather than NaN.
    const qreal degenerate = GeometryUtils::angle(QPointF(3, 3), QPointF(3, 3), QPointF(9, 9));
    QVERIFY(!qIsNaN(degenerate));
    QVERIFY(qAbs(degenerate - 180.0) < 1e-9);
}

void TestGeometryUtils::quadraticBezierSampling()
{
    const QList<QPointF> points =
        GeometryUtils::quadraticBezier(QPointF(0, 0), QPointF(50, 100), QPointF(100, 0), 10);

    // nPoints requests nPoints + 1 samples, the first and last of which are the
    // curve endpoints.
    QCOMPARE(points.size(), 11);
    QCOMPARE(points.first(), QPointF(0, 0));
    QCOMPARE(points.last(), QPointF(100, 0));

    // Symmetric control point: the midpoint sits on the axis of symmetry.
    QVERIFY(qFuzzyCompare(points.at(5).x(), 50.0));

    QCOMPARE(GeometryUtils::quadraticBezier(QPointF(0, 0), QPointF(1, 1), QPointF(2, 0), 1).size(), 0);
    QCOMPARE(GeometryUtils::quadraticBezier(QPointF(0, 0), QPointF(1, 1), QPointF(2, 0), 0).size(), 0);
}

QTEST_MAIN(TestGeometryUtils)
#include "tst_geometryutils.moc"
