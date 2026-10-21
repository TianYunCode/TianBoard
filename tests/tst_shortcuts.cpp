/**
 * @file tst_shortcuts.cpp
 * @brief 快捷键自定义的回归测试：让应用自己跑一遍注册表与改绑回路的自检。
 *
 * 判据取退出码（非 0 = 不一致条数，75 = 已有实例在跑、按约定让路并跳过）。这条链路查的是
 * 装配与持久化：默认值有没有登记进表、出厂默认之间有没有撞键、改绑能不能落盘并在“重启”
 * 后读回来、恢复默认有没有把配置键真删掉——都在界面上看不见，只能这样跑一遍。
 */

#include <QtTest>

#include <QFileInfo>
#include <QProcess>

#include "TestProcessSandbox.h"

class TestShortcuts : public QObject
{
    Q_OBJECT

private slots:
    /// @brief 注册表完整、默认值不撞键、改绑与恢复默认的读写回路都必须成立。
    void shortcutRegistryRoundTripHolds();
};

void TestShortcuts::shortcutRegistryRoundTripHolds()
{
    const QString exe = qEnvironmentVariable("TIANBOARD_EXE");
    QVERIFY2(!exe.isEmpty(), "set TIANBOARD_EXE to the built TianBoard.exe (ctest does it through the "
                             "test's ENVIRONMENT property)");
    QVERIFY2(QFileInfo::exists(exe), qPrintable(QStringLiteral("missing executable: %1").arg(exe)));

    QProcess process;
    // 应用往 userDataDirectory() 下写文档与日志；不给它一个临时根目录，
    // 每跑一次用例就在用户的真实文档库里多攒几份没人认领的文档。
    TestProcessSandbox sandbox(process, QStringLiteral("shortcuts"));
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(exe, {QStringLiteral("--shortcut-selftest")});
    QVERIFY2(process.waitForStarted(20000), "TianBoard.exe 没能启动");

    if (!process.waitForFinished(120000))
    {
        process.kill();
        process.waitForFinished();
        QFAIL("快捷键自检 120 秒内没退出（多半是撞上 Qt 断言对话框或改绑回路里卡住）");
    }

    QCOMPARE(process.exitStatus(), QProcess::NormalExit);

    const int code = process.exitCode();
    if (code == 75)
    {
        QSKIP("已有 TianBoard 实例在运行，快捷键自检按约定让路（不算失败）");
    }

    const QString log = QString::fromLocal8Bit(process.readAll());
    QVERIFY2(code == 0, qPrintable(QStringLiteral("快捷键自定义有 %1 处不一致：\n%2").arg(code).arg(log)));
}

QTEST_GUILESS_MAIN(TestShortcuts)
#include "tst_shortcuts.moc"
