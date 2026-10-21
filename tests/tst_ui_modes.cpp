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

    for (int attempt = 1; attempt <= 3; ++attempt)
    {
        QProcess      process;
        QElapsedTimer elapsed;
        elapsed.start();
        process.start(exe, {QStringLiteral("--ui-smoke")});
        QVERIFY2(process.waitForStarted(), "TianBoard.exe 没能启动");

        const bool   finished = process.waitForFinished(30000);
        const qint64 runMs    = elapsed.elapsed();

        if (!finished)
        {
            process.kill();
            process.waitForFinished();
            QFAIL(qPrintable(QStringLiteral("切换主模式后 30 秒内没有退出（多半是 Qt 断言对话框卡住了事件循环），"
                                            "已运行 %1 ms")
                    .arg(runMs)));
        }

        const bool normalExit = process.exitStatus() == QProcess::NormalExit;
        if (normalExit && process.exitCode() == 0 && runMs >= kExpectedRunMs)
        {
            return; // 正常跑完三套模式并自己退出
        }

        if (normalExit && process.exitCode() == 0)
        {
            QTest::qWait(3000); // 单实例接管，换一次时机
            continue;
        }

        QFAIL(qPrintable(QStringLiteral("切换主模式后异常退出：exitStatus=%1 code=%2 用时 %3 ms")
                .arg(process.exitStatus() == QProcess::NormalExit ? "normal" : "crashed")
                .arg(process.exitCode())
                .arg(runMs)));
    }

    QSKIP("三次都被已在运行的 TianBoard 实例接管，跳过");
}

QTEST_GUILESS_MAIN(TestUiModes)
#include "tst_ui_modes.moc"
