/**
 * @file tst_autosave.cpp
 * @brief 自动保存这条链路的回归测试：让应用自己验定时器与落盘。
 *
 * 自动保存过去没人量过：定时器到底存不存在、间隔按不按设置、在偏好设置里改成 0 分钟之后
 * 有没有真停下来（旧写法遇到 0 直接 return，正在跑的那个不停，用户以为关了其实还在写盘）。
 * 这条用例把"存在一个活动的定时器、间隔跟着设置起停、saveData 真落盘"量出来。
 * 判据取退出码（非 0 = 不一致条数，75 = 已有实例在跑、按约定让路并跳过）。
 */

#include <QtTest>

#include <QFileInfo>
#include <QProcess>

#include "TestProcessSandbox.h"

class TestAutoSave : public QObject
{
    Q_OBJECT

private slots:
    /// @brief 定时器随设置起停、saveData 之后页文件必须真的变了。
    void autoSaveTimerAndPersistenceHold();
};

void TestAutoSave::autoSaveTimerAndPersistenceHold()
{
    const QString exe = qEnvironmentVariable("TIANBOARD_EXE");
    QVERIFY2(!exe.isEmpty(), "set TIANBOARD_EXE to the built TianBoard.exe (ctest does it through the "
                             "test's ENVIRONMENT property)");
    QVERIFY2(QFileInfo::exists(exe), qPrintable(QStringLiteral("missing executable: %1").arg(exe)));

    QProcess process;
    // 自检会动设置里的自动保存间隔并往当前页加图元；沙箱把文档与配置都关进临时目录，
    // 不许把改过的间隔留在用户的配置里。
    TestProcessSandbox sandbox(process, QStringLiteral("autosave"));
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(exe, {QStringLiteral("--autosave-selftest")});
    QVERIFY2(process.waitForStarted(20000), "TianBoard.exe 没能启动");

    if (!process.waitForFinished(120000))
    {
        process.kill();
        process.waitForFinished();
        QFAIL("自动保存自检 120 秒内没退出（多半是撞上 Qt 断言对话框或落盘那一步卡住）");
    }

    QCOMPARE(process.exitStatus(), QProcess::NormalExit);

    const int code = process.exitCode();
    if (code == 75)
    {
        QSKIP("已有 TianBoard 实例在运行，自动保存自检按约定让路（不算失败）");
    }

    const QString log = QString::fromLocal8Bit(process.readAll());
    QVERIFY2(code == 0, qPrintable(QStringLiteral("自动保存链路有 %1 处不一致：\n%2").arg(code).arg(log)));
}

QTEST_GUILESS_MAIN(TestAutoSave)
#include "tst_autosave.moc"
