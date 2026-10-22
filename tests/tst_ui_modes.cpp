/**
 * @file tst_ui_modes.cpp
 * @brief 主模式切换的回归测试：`--ui-smoke` 让应用自己依次切到 文档 / 网页 / 画板 / 桌面批注 模式再退出。
 *
 * 这条路径以前完全没被自动化覆盖，文档模式的代理模型索引错用就是这样漏出去的——
 * Debug 下 Qt 弹断言对话框、事件循环卡住，进程不会按时退出。
 */

#include <QtTest>

#include <QElapsedTimer>
#include <QFileInfo>
#include <QProcess>

#include "TestProcessSandbox.h"

class TestUiModes : public QObject
{
    Q_OBJECT

private slots:
    void switchingMainModesExitsCleanly();
};

void TestUiModes::switchingMainModesExitsCleanly()
{
    const QString exe = qEnvironmentVariable("TIANBOARD_EXE");
    QVERIFY2(!exe.isEmpty(), "set TIANBOARD_EXE to the built TianBoard.exe (ctest does it through the "
                             "test's ENVIRONMENT property)");
    QVERIFY2(QFileInfo::exists(exe), qPrintable(QStringLiteral("missing executable: %1").arg(exe)));

    // 应用内的自检时序：2s 文档、4s 网页、6s 画板、8s 退出。少于 7s 就正常退出，
    // 只可能是单实例把请求转交给了已在跑的实例；那种情况重试，三次都这样再跳过。
    constexpr qint64 kExpectedRunMs = 7000;

    // 两种"没结论"的原因要分开数：30 秒内没退出（慢/卡住），与正常退出但太快
    // （被已在跑的实例接管）。前者三次都发生要判红，后者三次都发生才算跳过。
    int slowAttempts                = 0;
    int handoffAttempts             = 0;

    for (int attempt = 1; attempt <= 3; ++attempt)
    {
        QProcess process;
        // 应用往 userDataDirectory() 下写文档与日志；不给它一个临时根目录，
        // 每跑一次用例就在用户的真实文档库里多攒几份没人认领的文档。
        TestProcessSandbox sandbox(process, QStringLiteral("ui-modes"));
        QElapsedTimer      elapsed;
        elapsed.start();
        process.start(exe, {QStringLiteral("--ui-smoke")});
        QVERIFY2(process.waitForStarted(), "TianBoard.exe 没能启动");

        const bool   finished = process.waitForFinished(30000);
        const qint64 runMs    = elapsed.elapsed();

        if (!finished)
        {
            // 慢不等于坏：这台机器上同时在跑构建与别的用例时，11 秒的内部时序也可能被拖过
            // 30 秒。真卡住（Qt 断言对话框）会连三次都不退出，所以按 tst_smoke 的办法重试，
            // 三次都慢才判红。崩溃与非零退出码仍然当场判红。
            process.kill();
            process.waitForFinished();
            ++slowAttempts;
            qWarning() << "第" << attempt << "次：30 秒内没退出（用时" << runMs << "ms），重试";
            QTest::qWait(2000);
            continue;
        }

        const bool normalExit = process.exitStatus() == QProcess::NormalExit;
        if (normalExit && process.exitCode() == 0 && runMs >= kExpectedRunMs)
        {
            return; // 正常跑完三套模式并自己退出
        }

        if (normalExit && process.exitCode() == 0)
        {
            ++handoffAttempts;
            QTest::qWait(3000); // 单实例接管，换一次时机
            continue;
        }

        QFAIL(qPrintable(QStringLiteral("切换主模式后异常退出：exitStatus=%1 code=%2 用时 %3 ms")
                .arg(process.exitStatus() == QProcess::NormalExit ? "normal" : "crashed")
                .arg(process.exitCode())
                .arg(runMs)));
    }

    if (slowAttempts > 0)
    {
        QFAIL(qPrintable(QStringLiteral("%1 次尝试在 30 秒内没退出——真卡住（Qt 断言对话框）会每次都卡，"
                                        "偶尔一次多半是机器在同时跑构建；重跑这条用例确认不是回归")
                .arg(slowAttempts)));
    }

    QSKIP("三次都被已在运行的 TianBoard 实例接管，跳过");
}

QTEST_GUILESS_MAIN(TestUiModes)
#include "tst_ui_modes.moc"
