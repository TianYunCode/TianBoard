/**
 * @file tst_preflive.cpp
 * @brief 「偏好设置改完当场生效」这条约定的回归测试：让应用自己在真场景上改设置并当场量效果。
 *
 * 界面上的勾选框只写一个值，谁读它、什么时候读，全在各个消费方自己——读早了既不崩也不报错，
 * 只表现为"勾了没用，得重启"，而另一半（取消勾选）看着又是好的，这种不对称靠眼睛发现不了。
 * 所以必须有一条端到端的断言替用户盯着。判据取退出码（非 0 = 不符的条数，75 = 已有实例在跑、
 * 按约定让路并跳过）。
 */

#include <QtTest>

#include <QFileInfo>
#include <QProcess>

#include "TestProcessSandbox.h"

class TestPreferenceLiveApply : public QObject
{
    Q_OBJECT

private slots:
    /// @brief 钢笔预览圈在已经建好的页上也得随偏好当场起停。
    void penPreviewCircleFollowsPreference();
};

void TestPreferenceLiveApply::penPreviewCircleFollowsPreference()
{
    const QString exe = qEnvironmentVariable("TIANBOARD_EXE");
    QVERIFY2(!exe.isEmpty(), "set TIANBOARD_EXE to the built TianBoard.exe (ctest does it through the "
                             "test's ENVIRONMENT property)");
    QVERIFY2(QFileInfo::exists(exe), qPrintable(QStringLiteral("missing executable: %1").arg(exe)));

    QProcess process;
    // 自检自建一份临时文档并在上面建页，用完连目录一起删；不需要事件循环，页是直接构造的。
    TestProcessSandbox sandbox(process, QStringLiteral("preflive"));
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(exe, {QStringLiteral("--preflive-selftest")});
    QVERIFY2(process.waitForStarted(20000), "TianBoard.exe 没能启动");

    if (!process.waitForFinished(120000))
    {
        process.kill();
        process.waitForFinished();
        QFAIL("偏好当场生效自检 120 秒内没退出（多半是撞上了 Qt 断言对话框）");
    }

    QCOMPARE(process.exitStatus(), QProcess::NormalExit);

    const int code = process.exitCode();
    if (code == 75)
    {
        QSKIP("已有 TianBoard 实例在运行，偏好当场生效自检按约定让路（不算失败）");
    }

    const QString log = QString::fromLocal8Bit(process.readAll());
    QVERIFY2(code == 0, qPrintable(QStringLiteral("偏好设置当场生效有 %1 处不符：\n%2").arg(code).arg(log)));
}

QTEST_GUILESS_MAIN(TestPreferenceLiveApply)
#include "tst_preflive.moc"
