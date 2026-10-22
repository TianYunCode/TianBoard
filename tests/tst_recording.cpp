/**
 * @file tst_recording.cpp
 * @brief 录屏端到端的回归测试：让应用真录一段，再验容器、时长与可播性。
 *
 * 录屏这条链路（抓屏定时器 → Windows Media 写入器 → waveIn 音频队列）此前只过编译与冒烟跑，
 * 从没产出过一个真文件。判据取退出码（非 0 = 不一致条数，75 = 已有实例在跑、按约定让路并跳过）。
 */

#include <QtTest>

#include <QFileInfo>
#include <QProcess>

#include "TestProcessSandbox.h"

class TestRecording : public QObject
{
    Q_OBJECT

private slots:
    /// @brief 录两秒屏幕必须产出一个能被多媒体后端读出时长的 .wmv。
    void recordingProducesPlayableFile();
};

void TestRecording::recordingProducesPlayableFile()
{
    const QString exe = qEnvironmentVariable("TIANBOARD_EXE");
    QVERIFY2(!exe.isEmpty(), "set TIANBOARD_EXE to the built TianBoard.exe (ctest injects it via the "
                             "test's ENVIRONMENT property)");
    QVERIFY2(QFileInfo::exists(exe), qPrintable(QStringLiteral("missing executable: %1").arg(exe)));

    QProcess process;
    // 录屏产物落在 userRecordingDirectory()，它跟着 userDataDirectory() 走；不给临时根目录
    // 就会往用户真实的应用数据目录里写 .wmv。
    TestProcessSandbox sandbox(process, QStringLiteral("recording"));
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(exe, {QStringLiteral("--recording-selftest")});
    QVERIFY2(process.waitForStarted(20000), "TianBoard.exe 没能启动");

    if (!process.waitForFinished(120000))
    {
        process.kill();
        process.waitForFinished();
        QFAIL("录制自检 120 秒内没退出（多半卡在编码器收尾或多媒体后端探测）");
    }

    QCOMPARE(process.exitStatus(), QProcess::NormalExit);

    const int code = process.exitCode();
    if (code == 75)
    {
        QSKIP("已有 TianBoard 实例在运行，录制自检按约定让路（不算失败）");
    }

    const QString log = QString::fromLocal8Bit(process.readAll());
    QVERIFY2(code == 0, qPrintable(QStringLiteral("录屏链路有 %1 处不一致：\n%2").arg(code).arg(log)));
}

QTEST_GUILESS_MAIN(TestRecording)
#include "tst_recording.moc"
