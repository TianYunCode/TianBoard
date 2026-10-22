/**
 * @file tst_import.cpp
 * @brief 导入这条路的回归：让应用用桩裁决器走完“取消 / 改名新建 / 同名替换”三条路径，
 *        再把 PDF / 图片导入与“往当前文档追加一页”的产物逐项对账。
 *
 * 这三条路径原本都要弹窗，人工点不全，而它们决定用户的文档会不会被顶掉、取消后磁盘上
 * 有没有残留一份没人认领的文档包。判据以退出码为准（非 0 是不符的条数、75 是已有实例
 * 在跑让路），日志只用来看是哪一条路径没落地。
 */

#include <QtTest>

#include <QFileInfo>
#include <QProcess>

#include "TestProcessSandbox.h"

class TestImport : public QObject
{
    Q_OBJECT

private slots:
    /// @brief 文档名撞车时，核心按用户裁决登记或回收；不撞名一次都不该问用户。
    void duplicateNameDecisionsLandCorrectly();
};

void TestImport::duplicateNameDecisionsLandCorrectly()
{
    const QString exe = qEnvironmentVariable("TIANBOARD_EXE");
    QVERIFY2(!exe.isEmpty(), "set TIANBOARD_EXE to the built TianBoard.exe (ctest does it through the "
                             "test's ENVIRONMENT property)");
    QVERIFY2(QFileInfo::exists(exe), qPrintable(QStringLiteral("missing executable: %1").arg(exe)));

    QProcess process;
    // 应用往 userDataDirectory() 下写文档与日志；不给它一个临时根目录，
    // 每跑一次用例就在用户的真实文档库里多攒几份没人认领的文档。
    TestProcessSandbox sandbox(process, QStringLiteral("import"));
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(exe, {QStringLiteral("--import-selftest")});
    QVERIFY2(process.waitForStarted(20000), "TianBoard.exe 没能启动");

    if (!process.waitForFinished(120000))
    {
        process.kill();
        process.waitForFinished();
        QFAIL("导入重名自检 120 秒内没退出（多半是撞上 Qt 断言对话框或登记链路里卡住）");
    }

    QCOMPARE(process.exitStatus(), QProcess::NormalExit);

    const int code = process.exitCode();
    if (code == 75)
    {
        QSKIP("已有 TianBoard 实例在运行，导入自检按约定让路（不算失败）");
    }

    const QString log = QString::fromLocal8Bit(process.readAll());
    QVERIFY2(code == 0, qPrintable(QStringLiteral("导入重名裁决有 %1 条不符：\n%2").arg(code).arg(log)));
}

QTEST_GUILESS_MAIN(TestImport)
#include "tst_import.moc"
