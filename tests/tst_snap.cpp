/**
 * @file tst_snap.cpp
 * @brief 网格吸附接线的回归测试：让应用自己跑一遍场景侧的吸附自检。
 *
 * 纯几何由 tst_snapping 单独链接 utils/Snapping.cpp 覆盖；这里查的是要跑起整个应用才有的
 * 那一层——格距、背景类型、图元外框是怎么被喂进吸附数学的。判据取退出码（非 0 = 不一致
 * 条数，75 = 已有实例在跑、按约定让路并跳过）。
 */

#include <QtTest>

#include <QFileInfo>
#include <QProcess>

class TestSnap : public QObject
{
    Q_OBJECT

private slots:
    /// @brief 吸附开关、稿纸只吸 Y、角点择优与笔宽缩回都必须成立。
    void sceneSnappingWiringHolds();
};

void TestSnap::sceneSnappingWiringHolds()
{
    const QString exe = qEnvironmentVariable("TIANBOARD_EXE");
    QVERIFY2(!exe.isEmpty(), "set TIANBOARD_EXE to the built TianBoard.exe (ctest does it through the "
                             "test's ENVIRONMENT property)");
    QVERIFY2(QFileInfo::exists(exe), qPrintable(QStringLiteral("missing executable: %1").arg(exe)));

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(exe, {QStringLiteral("--snap-selftest")});
    QVERIFY2(process.waitForStarted(20000), "TianBoard.exe 没能启动");

    if (!process.waitForFinished(120000))
    {
        process.kill();
        process.waitForFinished();
        QFAIL("吸附自检 120 秒内没退出（多半是撞上 Qt 断言对话框或吸附接线里卡住）");
    }

    QCOMPARE(process.exitStatus(), QProcess::NormalExit);

    const int code = process.exitCode();
    if (code == 75)
    {
        QSKIP("已有 TianBoard 实例在运行，吸附自检按约定让路（不算失败）");
    }

    const QString log = QString::fromLocal8Bit(process.readAll());
    QVERIFY2(code == 0, qPrintable(QStringLiteral("吸附接线有 %1 处不一致：\n%2").arg(code).arg(log)));
}

QTEST_GUILESS_MAIN(TestSnap)
#include "tst_snap.moc"
