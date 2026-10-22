/**
 * @file tst_trash.cpp
 * @brief 文档回收站这条链路的回归测试：让应用自己把文档移进回收站、清空、再按天数清理，逐项查盘。
 *
 * 这条链路每一步都动磁盘：分组归属只写在各文档自己的 metadata.rdf 里（folders.xml 只存目录树），
 * 清空回收站靠 deleteDocument 把整包从盘上摘掉。任何一侧写漏都不崩不报错，只表现为
 * "重启之后删掉的文档又冒出来"或"清空之后磁盘上留着一包没人认领的文档"。判据取退出码
 * （非 0 = 不符的条数，75 = 已有实例在跑、按约定让路并跳过）。
 */

#include <QtTest>

#include <QFileInfo>
#include <QProcess>

#include "TestProcessSandbox.h"

class TestTrash : public QObject
{
    Q_OBJECT

private slots:
    /// @brief 移入回收站、清空删净、按天数只清过期那一份。
    void trashMovesPersistsAndEmptiesOnDisk();
};

void TestTrash::trashMovesPersistsAndEmptiesOnDisk()
{
    const QString exe = qEnvironmentVariable("TIANBOARD_EXE");
    QVERIFY2(!exe.isEmpty(), "set TIANBOARD_EXE to the built TianBoard.exe (ctest does it through the "
                             "test's ENVIRONMENT property)");
    QVERIFY2(QFileInfo::exists(exe), qPrintable(QStringLiteral("missing executable: %1").arg(exe)));

    QProcess process;
    // 自检只在沙箱数据目录里建临时文档，用完随目录一起删；用户真实的文档库不碰。
    TestProcessSandbox sandbox(process, QStringLiteral("trash"));
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(exe, {QStringLiteral("--trash-selftest")});
    QVERIFY2(process.waitForStarted(20000), "TianBoard.exe 没能启动");

    if (!process.waitForFinished(120000))
    {
        process.kill();
        process.waitForFinished();
        QFAIL("回收站自检 120 秒内没退出（多半是撞上了 Qt 断言对话框）");
    }

    QCOMPARE(process.exitStatus(), QProcess::NormalExit);

    const int code = process.exitCode();
    if (code == 75)
    {
        QSKIP("已有 TianBoard 实例在运行，回收站自检按约定让路（不算失败）");
    }

    const QString log = QString::fromLocal8Bit(process.readAll());
    QVERIFY2(code == 0, qPrintable(QStringLiteral("回收站这条链路有 %1 处不符：\n%2").arg(code).arg(log)));
}

QTEST_GUILESS_MAIN(TestTrash)
#include "tst_trash.moc"
