/**
 * @file tst_download.cpp
 * @brief 本地下载这条链路的回归测试：让应用自己把一个文档外的文件交给下载队列，再看产物与应答连线。
 *
 * 下载服务不反过来命令画板：它只回报“有下载入队/队列排空”，目标文档也由装配点装上的提供者给出。
 * 这些接线丢了既不崩也不报错，只表现为“拖进来的东西不知去了哪、下载页签不出来”，所以必须
 * 有一条端到端的断言替用户盯着。判据取退出码（非 0 = 不符的条数，75 = 已有实例在跑、按约定让路并跳过）。
 */

#include <QtTest>

#include <QFileInfo>
#include <QProcess>

#include "TestProcessSandbox.h"

class TestDownload : public QObject
{
    Q_OBJECT

private slots:
    /// @brief 跨层事实各有应答方，且本地拖进来的文件按字节进了文档包。
    void localFileDownloadLandsInTheDocument();
};

void TestDownload::localFileDownloadLandsInTheDocument()
{
    const QString exe = qEnvironmentVariable("TIANBOARD_EXE");
    QVERIFY2(!exe.isEmpty(), "set TIANBOARD_EXE to the built TianBoard.exe (ctest does it through the "
                             "test's ENVIRONMENT property)");
    QVERIFY2(QFileInfo::exists(exe), qPrintable(QStringLiteral("missing executable: %1").arg(exe)));

    QProcess process;
    // 自检会新建一份临时文档并把下载沙箱关进临时目录；下载走工作线程，靠事件循环收尾。
    TestProcessSandbox sandbox(process, QStringLiteral("download"));
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(exe, {QStringLiteral("--download-selftest")});
    QVERIFY2(process.waitForStarted(20000), "TianBoard.exe 没能启动");

    if (!process.waitForFinished(120000))
    {
        process.kill();
        process.waitForFinished();
        QFAIL("本地下载自检 120 秒内没退出（多半是下载线程没收尾，或撞上了 Qt 断言对话框）");
    }

    QCOMPARE(process.exitStatus(), QProcess::NormalExit);

    const int code = process.exitCode();
    if (code == 75)
    {
        QSKIP("已有 TianBoard 实例在运行，本地下载自检按约定让路（不算失败）");
    }

    const QString log = QString::fromLocal8Bit(process.readAll());
    QVERIFY2(code == 0, qPrintable(QStringLiteral("本地下载链路有 %1 处不符：\n%2").arg(code).arg(log)));
}

QTEST_GUILESS_MAIN(TestDownload)
#include "tst_download.moc"
