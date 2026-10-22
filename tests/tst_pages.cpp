/**
 * @file tst_pages.cpp
 * @brief 页面操作（新增/复制/移动/删除）的回归测试：让应用自己做完四步再逐页冷读对账。
 *
 * 页面操作同时牵着页文件改名、场景缓存搬移与资源复制，任何一处错位都不报错，
 * 只表现为"某一页变成了另一页"或"这一页的东西没了"。判据取退出码
 * （非 0 = 不一致条数，75 = 已有实例在跑、按约定让路并跳过）。
 */

#include <QtTest>

#include <QFileInfo>
#include <QProcess>

#include "TestProcessSandbox.h"

class TestPages : public QObject
{
    Q_OBJECT

private slots:
    /// @brief 四步页面操作之后，每一页在内存里与在盘上必须是同一套内容。
    void pageOperationsKeepMemoryAndDiskAgreed();
};

void TestPages::pageOperationsKeepMemoryAndDiskAgreed()
{
    const QString exe = qEnvironmentVariable("TIANBOARD_EXE");
    QVERIFY2(!exe.isEmpty(), "set TIANBOARD_EXE to the built TianBoard.exe (ctest does it through the "
                             "test's ENVIRONMENT property)");
    QVERIFY2(QFileInfo::exists(exe), qPrintable(QStringLiteral("missing executable: %1").arg(exe)));

    QProcess process;
    // 自检会新建一份三页文档并反复增删移动页面；不给它一个临时根目录，每跑一次就往用户
    // 真实的文档库里塞一份没人认领的文档。
    TestProcessSandbox sandbox(process, QStringLiteral("pages"));
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(exe, {QStringLiteral("--pages-selftest")});
    QVERIFY2(process.waitForStarted(20000), "TianBoard.exe 没能启动");

    if (!process.waitForFinished(180000))
    {
        process.kill();
        process.waitForFinished();
        QFAIL("页面操作自检 180 秒内没退出（多半是撞上 Qt 断言对话框或某一步页文件读写卡住）");
    }

    QCOMPARE(process.exitStatus(), QProcess::NormalExit);

    const int code = process.exitCode();
    if (code == 75)
    {
        QSKIP("已有 TianBoard 实例在运行，页面操作自检按约定让路（不算失败）");
    }

    const QString log = QString::fromLocal8Bit(process.readAll());
    QVERIFY2(code == 0, qPrintable(QStringLiteral("页面操作有 %1 处不一致：\n%2").arg(code).arg(log)));
}

QTEST_GUILESS_MAIN(TestPages)
#include "tst_pages.moc"
