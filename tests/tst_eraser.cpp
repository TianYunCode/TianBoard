/**
 * @file tst_eraser.cpp
 * @brief 橡皮擦这条画板主回路的回归测试：让应用自己画两笔、擦掉一段、撤销重做、再存盘冷读。
 *
 * 擦除不是"删掉整个图元"，而是把命中的每块笔迹多边形按擦除带切开、用残片顶替原图元，再合成一条
 * 撤销命令。切开的位置、误伤的范围、撤销还不还得原样、盘上写的是擦过的那一页——这四件事任何一件
 * 做错都不崩不报错，只在界面上表现为"擦了个寂寞"或"下次打开又长回来"。判据取退出码
 * （非 0 = 不符的条数，75 = 已有实例在跑、按约定让路并跳过）。
 */

#include <QtTest>

#include <QFileInfo>
#include <QProcess>

#include "TestProcessSandbox.h"

class TestEraser : public QObject
{
    Q_OBJECT

private slots:
    /// @brief 橡皮切开一笔的中段：不误伤、一条命令、撤销还原、存读一致。
    void eraserCutsAStrokeAndRoundTrips();
};

void TestEraser::eraserCutsAStrokeAndRoundTrips()
{
    const QString exe = qEnvironmentVariable("TIANBOARD_EXE");
    QVERIFY2(!exe.isEmpty(), "set TIANBOARD_EXE to the built TianBoard.exe (ctest does it through the "
                             "test's ENVIRONMENT property)");
    QVERIFY2(QFileInfo::exists(exe), qPrintable(QStringLiteral("missing executable: %1").arg(exe)));

    QProcess process;
    // 自检自建一份临时文档，在上面按输入设备那条路画与擦，用完连目录一起删。
    TestProcessSandbox sandbox(process, QStringLiteral("eraser"));
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(exe, {QStringLiteral("--eraser-selftest")});
    QVERIFY2(process.waitForStarted(20000), "TianBoard.exe 没能启动");

    if (!process.waitForFinished(120000))
    {
        process.kill();
        process.waitForFinished();
        QFAIL("橡皮擦自检 120 秒内没退出（多半是撞上了 Qt 断言对话框）");
    }

    QCOMPARE(process.exitStatus(), QProcess::NormalExit);

    const int code = process.exitCode();
    if (code == 75)
    {
        QSKIP("已有 TianBoard 实例在运行，橡皮擦自检按约定让路（不算失败）");
    }

    const QString log = QString::fromLocal8Bit(process.readAll());
    QVERIFY2(code == 0, qPrintable(QStringLiteral("橡皮擦这条回路有 %1 处不符：\n%2").arg(code).arg(log)));
}

QTEST_GUILESS_MAIN(TestEraser)
#include "tst_eraser.moc"
