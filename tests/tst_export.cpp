/**
 * @file tst_export.cpp
 * @brief 导出链路的回归测试：让应用自己把两页文档导成 PDF 与 .tbz 再对账。
 *
 * 导出器唯一的非交互入口是 persistsDocument()，界面上点"导出"只看得到没报错；这条用例把
 * 产出物真读回来查页数、每页尺寸、每页文字属于哪一页、渲染出来有没有墨，以及 .tbz 打包再
 * 导入之后每页内容是否一致。判据取退出码（非 0 = 不一致条数，75 = 已有实例在跑、让路并跳过）。
 */

#include <QtTest>

#include <QFileInfo>
#include <QProcess>

#include "TestProcessSandbox.h"

class TestExport : public QObject
{
    Q_OBJECT

private slots:
    /// @brief 两个 PDF 导出器与 .tbz 往返都必须对得上账。
    void pdfAndPackageRoundTripHolds();
};

void TestExport::pdfAndPackageRoundTripHolds()
{
    const QString exe = qEnvironmentVariable("TIANBOARD_EXE");
    QVERIFY2(!exe.isEmpty(), "set TIANBOARD_EXE to the built TianBoard.exe (ctest does it through the "
                             "test's ENVIRONMENT property)");
    QVERIFY2(QFileInfo::exists(exe), qPrintable(QStringLiteral("missing executable: %1").arg(exe)));

    QProcess process;
    // 自检会新建一份两页的临时文档并往画板里贴图元；不给它一个临时根目录，每跑一次就在
    // 用户的真实文档库里多攒一份没人认领的文档。
    TestProcessSandbox sandbox(process, QStringLiteral("export"));
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(exe, {QStringLiteral("--export-selftest")});
    QVERIFY2(process.waitForStarted(20000), "TianBoard.exe 没能启动");

    if (!process.waitForFinished(120000))
    {
        process.kill();
        process.waitForFinished();
        QFAIL("导出自检 120 秒内没退出（多半是撞上 Qt 断言对话框或导出链路里卡住）");
    }

    QCOMPARE(process.exitStatus(), QProcess::NormalExit);

    const int code = process.exitCode();
    if (code == 75)
    {
        QSKIP("已有 TianBoard 实例在运行，导出自检按约定让路（不算失败）");
    }

    const QString log = QString::fromLocal8Bit(process.readAll());
    QVERIFY2(code == 0, qPrintable(QStringLiteral("导出链路有 %1 处不一致：\n%2").arg(code).arg(log)));
}

QTEST_GUILESS_MAIN(TestExport)
#include "tst_export.moc"
