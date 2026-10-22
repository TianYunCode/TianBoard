/**
 * @file tst_undoredo.cpp
 * @brief 画板撤销/重做回路的回归测试：让应用自己在真实场景里跑一遍。
 *
 * 这条回路过去只体现在"界面上按 Ctrl+Z 看起来动了"：栈计数、撤销是不是真移出场景、重做放回的是
 * 不是同一个对象、删除撤销后内容还完不完整、ClearUndoStack 与换文档有没有按预期清栈，都没被量过。
 * 判据取退出码（非 0 = 不一致条数，75 = 已有实例在跑、按约定让路并跳过）。
 */

#include <QtTest>

#include <QFileInfo>
#include <QProcess>

#include "TestProcessSandbox.h"

class TestUndoRedo : public QObject
{
    Q_OBJECT

private slots:
    /// @brief 加、撤、重做、删、清空、换文档六步必须都按预期落地。
    void undoRedoRoundTripHolds();
};

void TestUndoRedo::undoRedoRoundTripHolds()
{
    const QString exe = qEnvironmentVariable("TIANBOARD_EXE");
    QVERIFY2(!exe.isEmpty(), "set TIANBOARD_EXE to the built TianBoard.exe (ctest does it through the "
                             "test's ENVIRONMENT property)");
    QVERIFY2(QFileInfo::exists(exe), qPrintable(QStringLiteral("missing executable: %1").arg(exe)));

    QProcess process;
    // 自检会新建两份临时文档往里面加图元；不给它一个临时根目录，每跑一次就在用户的真实
    // 文档库里多攒几份没人认领的文档。
    TestProcessSandbox sandbox(process, QStringLiteral("undoredo"));
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(exe, {QStringLiteral("--undoredo-selftest")});
    QVERIFY2(process.waitForStarted(20000), "TianBoard.exe 没能启动");

    if (!process.waitForFinished(120000))
    {
        process.kill();
        process.waitForFinished();
        QFAIL("撤销/重做自检 120 秒内没退出（多半是撞上 Qt 断言对话框或撤销回路里卡住）");
    }

    QCOMPARE(process.exitStatus(), QProcess::NormalExit);

    const int code = process.exitCode();
    if (code == 75)
    {
        QSKIP("已有 TianBoard 实例在运行，撤销/重做自检按约定让路（不算失败）");
    }

    const QString log = QString::fromLocal8Bit(process.readAll());
    QVERIFY2(code == 0, qPrintable(QStringLiteral("撤销/重做回路有 %1 处不一致：\n%2").arg(code).arg(log)));
}

QTEST_GUILESS_MAIN(TestUndoRedo)
#include "tst_undoredo.moc"
