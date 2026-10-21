/**
 * @file tst_format.cpp
 * @brief 文档格式往返回归：让应用真存一页、再绕开缓存真读回来逐项比对。
 *
 * 判据以退出码为准（0 一致、非 0 是不一致的条数、75 是已有实例在跑让路），日志只用来
 * 定位是哪一项丢了——GUI 子系统的进程不一定接得上父进程的管道，不能拿日志当闸门。
 */

#include <QtTest>

#include <QFileInfo>
#include <QProcess>

#include "TestProcessSandbox.h"

class TestFormat : public QObject
{
    Q_OBJECT

private slots:
    /// @brief 一笔笔迹、一段文本、一张图和三件工具图元存进页文件再读回来，必须一模一样。
    void sceneSurvivesSaveAndReload();
};

void TestFormat::sceneSurvivesSaveAndReload()
{
    const QString exe = qEnvironmentVariable("TIANBOARD_EXE");
    QVERIFY2(!exe.isEmpty(), "set TIANBOARD_EXE to the built TianBoard.exe (ctest does it through the "
                             "test's ENVIRONMENT property)");
    QVERIFY2(QFileInfo::exists(exe), qPrintable(QStringLiteral("missing executable: %1").arg(exe)));

    QProcess process;
    // 应用往 userDataDirectory() 下写文档与日志；不给它一个临时根目录，
    // 每跑一次用例就在用户的真实文档库里多攒几份没人认领的文档。
    TestProcessSandbox sandbox(process, QStringLiteral("format"));
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(exe, {QStringLiteral("--format-selftest")});
    QVERIFY2(process.waitForStarted(20000), "TianBoard.exe 没能启动");

    if (!process.waitForFinished(120000))
    {
        process.kill();
        process.waitForFinished();
        QFAIL("格式往返自检 120 秒内没退出（多半是撞上 Qt 断言对话框或存/读链路里卡住）");
    }

    QCOMPARE(process.exitStatus(), QProcess::NormalExit);

    const int code = process.exitCode();
    if (code == 75)
    {
        QSKIP("已有 TianBoard 实例在运行，格式自检按约定让路（不算失败）");
    }

    const QString log = QString::fromLocal8Bit(process.readAll());
    QVERIFY2(code == 0, qPrintable(QStringLiteral("格式往返有 %1 处不一致：\n%2").arg(code).arg(log)));
}

QTEST_GUILESS_MAIN(TestFormat)
#include "tst_format.moc"
