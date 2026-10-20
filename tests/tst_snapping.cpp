/**
 * @file tst_snapping.cpp
 * @brief 网格吸附纯几何的单元测试。
 */

#include <QtTest>

#include "utils/Snapping.h"

class TestSnapping : public QObject
{
    Q_OBJECT

private slots:
    /// @brief 每个坐标都落到最近的一条网格线上，正好差半格时进到远处那条。
    void snapsToTheNearestGridLine();
    /// @brief 负坐标同样按格取整，不能因为向零取整而偏一格。
    void handlesNegativeCoordinates();
    /// @brief 稿纸类背景只吸 Y：横线之间没有竖线可对，吸 X 会把图元推歪。
    void canSnapYOnly();
    /// @brief 吸得力随偏离距离线性衰减，落在交点上是 1，偏出半格是 0。
    void reportsSnapForce();
    /// @brief 候选点更近时改吸候选点。
    void prefersTheProposedPointWhenCloser();
    /// @brief 网格尺寸非正时不吸，也不该除零。
    void ignoresNonPositiveGridSize();
    /// @brief 多角点取吸得力最强的那个，并报告是第几个角。
    void picksTheStrongestCorner();
};

void TestSnapping::snapsToTheNearestGridLine()
{
    const Snapping::Result near = Snapping::toGrid(QPointF(12, 108), 100, true);
    QCOMPARE(near.offset, QPointF(-12, -8));

    // 两个方向都正好差半格：按“进到远处那条线”处理，保证结果稳定不抖。
    const Snapping::Result half = Snapping::toGrid(QPointF(50, 50), 100, true);
    QCOMPARE(half.offset, QPointF(50, 50));

    // 已经在交点上就不该动。
    const Snapping::Result on = Snapping::toGrid(QPointF(200, -300), 100, true);
    QCOMPARE(on.offset, QPointF(0, 0));
}

void TestSnapping::handlesNegativeCoordinates()
{
    // floor 语义：-130 在 -200 与 -100 之间，离 -100 更近。
    const Snapping::Result r = Snapping::toGrid(QPointF(-130, -170), 100, true);
    QCOMPARE(r.offset, QPointF(30, -30));
}

void TestSnapping::canSnapYOnly()
{
    const Snapping::Result r = Snapping::toGrid(QPointF(12, 108), 100, false);
    QCOMPARE(r.offset, QPointF(0, -8));
}

void TestSnapping::reportsSnapForce()
{
    QCOMPARE(Snapping::toGrid(QPointF(100, 100), 100, true).force, 1.0);

    // 偏一格（两轴各半格）时吸得力归零，等于不吸。
    const Snapping::Result far = Snapping::toGrid(QPointF(150, 150), 100, true);
    QCOMPARE(far.force, 0.0);

    // 只偏 25 时剩 0.75。
    const Snapping::Result some = Snapping::toGrid(QPointF(125, 100), 100, true);
    QVERIFY(qFuzzyCompare(some.force, 0.75));
}

void TestSnapping::prefersTheProposedPointWhenCloser()
{
    const Snapping::Result r = Snapping::toGrid(QPointF(120, 100), 100, true, QPointF(123, 100), true);
    QCOMPARE(r.offset, QPointF(3, 0));
    QVERIFY(qFuzzyCompare(r.force, 0.97));

    // 候选点更远时保持吸网格。
    const Snapping::Result keep = Snapping::toGrid(QPointF(120, 100), 100, true, QPointF(300, 100), true);
    QCOMPARE(keep.offset, QPointF(-20, 0));
}

void TestSnapping::ignoresNonPositiveGridSize()
{
    const Snapping::Result r = Snapping::toGrid(QPointF(12, 108), 0, true);
    QCOMPARE(r.offset, QPointF(0, 0));
    QCOMPARE(r.force, 0.0);
}

void TestSnapping::picksTheStrongestCorner()
{
    const QList<QPointF> corners{QPointF(140, 100), QPointF(203, 500), QPointF(-10, -10), QPointF(0, 0)};

    const Snapping::Result best = Snapping::bestOfCorners(corners, 100, true);
    QCOMPARE(best.corner, 3); // 第四个角正好在交点上，吸得力 1
    QCOMPARE(best.offset, QPointF(0, 0));
    QCOMPARE(best.force, 1.0);

    // 没有角点时既不该吸，也不该给出角标。
    const Snapping::Result none = Snapping::bestOfCorners(QList<QPointF>(), 100, true);
    QCOMPARE(none.corner, -1);
    QCOMPARE(none.offset, QPointF(0, 0));
}

QTEST_GUILESS_MAIN(TestSnapping)
#include "tst_snapping.moc"
