/**
 * @file tst_media.cpp
 * @brief 音视频插入这条链路的回归测试：让应用自己把文档外的音频插进画板再冷读回来。
 *
 * 页写出器永远按 `audios/<文件名>` 这种文档内相对路径写引用，所以插入时只要没把媒体文件
 * 复制进文档，这份文档重开就听不到声音——不崩不报错，只是东西没了。判据取退出码
 * （非 0 = 不一致条数，75 = 已有实例在跑、按约定让路并跳过）。
 */

#include <QtTest>

#include <QFileInfo>
#include <QProcess>

#include "TestProcessSandbox.h"

class TestMedia : public QObject
{
    Q_OBJECT

private slots:
    /// @brief 带字节与只给路径两条插入分支都必须让音频成为文档的一部分。
    void insertedMediaBecomesPartOfDocument();
};

void TestMedia::insertedMediaBecomesPartOfDocument()
{
    const QString exe = qEnvironmentVariable("TIANBOARD_EXE");
    QVERIFY2(!exe.isEmpty(), "set TIANBOARD_EXE to the built TianBoard.exe (ctest does it through the "
                             "test's ENVIRONMENT property)");
    QVERIFY2(QFileInfo::exists(exe), qPrintable(QStringLiteral("missing executable: %1").arg(exe)));

    QProcess process;
    // 自检会新建一份临时文档并把媒体插进去；沙箱同时把文档仓库与临时文件都关进临时目录。
    TestProcessSandbox sandbox(process, QStringLiteral("media"));
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(exe, {QStringLiteral("--media-selftest")});
    QVERIFY2(process.waitForStarted(20000), "TianBoard.exe 没能启动");

    if (!process.waitForFinished(120000))
    {
        process.kill();
        process.waitForFinished();
        QFAIL("音视频插入自检 120 秒内没退出（多半是撞上 Qt 断言对话框或播放器线程卡住）");
    }

    QCOMPARE(process.exitStatus(), QProcess::NormalExit);

    const int code = process.exitCode();
    if (code == 75)
    {
        QSKIP("已有 TianBoard 实例在运行，音视频插入自检按约定让路（不算失败）");
    }

    const QString log = QString::fromLocal8Bit(process.readAll());
    QVERIFY2(code == 0, qPrintable(QStringLiteral("音视频插入有 %1 处不一致：\n%2").arg(code).arg(log)));
}

QTEST_GUILESS_MAIN(TestMedia)
#include "tst_media.moc"
