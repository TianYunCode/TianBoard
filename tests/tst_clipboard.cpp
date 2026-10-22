/**
 * @file tst_clipboard.cpp
 * @brief 画板图元剪贴板链路的回归测试：让应用自己跑一遍复制/粘贴/存读。
 *
 * 这条链路（copy → 剪贴板 → paste → duplicateItem → deepCopy）以前只过编译：剪贴板里放的是
 * 副本、粘贴再做一次副本，两次的 UUID 归属决定"删掉原件会不会把副本引用的资源文件一起删"。
 * 判据取退出码（非 0 = 不一致条数，75 = 已有实例在跑、按约定让路并跳过）。
 */

#include <QtTest>

#include <QFileInfo>
#include <QProcess>

#include "TestProcessSandbox.h"

class TestClipboard : public QObject
{
    Q_OBJECT

private slots:
    /// @brief 复制→粘贴→删原件→存读之后，两份副本必须带着自己的内容回来。
    void copyPasteAndReloadKeepsItems();
};

void TestClipboard::copyPasteAndReloadKeepsItems()
{
    const QString exe = qEnvironmentVariable("TIANBOARD_EXE");
    QVERIFY2(!exe.isEmpty(), "set TIANBOARD_EXE to the built TianBoard.exe (ctest does it through the "
                             "test's ENVIRONMENT property)");
    QVERIFY2(QFileInfo::exists(exe), qPrintable(QStringLiteral("missing executable: %1").arg(exe)));

    QProcess process;
    // 自检会新建一份临时文档并往画板里贴图元；不给它一个临时根目录，每跑一次就在用户的
    // 真实文档库里多攒一份没人认领的文档。
    TestProcessSandbox sandbox(process, QStringLiteral("clipboard"));
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(exe, {QStringLiteral("--clipboard-selftest")});
    QVERIFY2(process.waitForStarted(20000), "TianBoard.exe 没能启动");

    if (!process.waitForFinished(120000))
    {
        process.kill();
        process.waitForFinished();
        QFAIL("剪贴板自检 120 秒内没退出（多半是撞上 Qt 断言对话框或粘贴链路里卡住）");
    }

    QCOMPARE(process.exitStatus(), QProcess::NormalExit);

    const int code = process.exitCode();
    if (code == 75)
    {
        QSKIP("已有 TianBoard 实例在运行，剪贴板自检按约定让路（不算失败）");
    }

    const QString log = QString::fromLocal8Bit(process.readAll());
    QVERIFY2(code == 0, qPrintable(QStringLiteral("剪贴板链路有 %1 处不一致：\n%2").arg(code).arg(log)));
}

QTEST_GUILESS_MAIN(TestClipboard)
#include "tst_clipboard.moc"
