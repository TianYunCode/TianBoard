/**
 * @file tst_cookies.cpp
 * @brief Cookie 隐私链路的回归测试：让应用自己跑一遍镜像罐与退出清理计数的自检。
 *
 * 白名单匹配与策略名映射由 tst_cookierules 单独链接 utils/CookieRules.cpp 覆盖；这里查的是要跑起
 * 整个应用才有的那一层——Cookie 罐装的是哪一款、三项设置按名字读不读得到、计数有没有按白名单
 * 把带点的域留下的顶级域与子域算住。判据取退出码（非 0 = 不一致
 * 条数，75 = 已有实例在跑、按约定让路并跳过）。
 */

#include <QtTest>

#include <QFileInfo>
#include <QProcess>

#include "TestProcessSandbox.h"

class TestCookies : public QObject
{
    Q_OBJECT

private slots:
    /// @brief 镜像罐类型、设置项名字与退出清理计数都必须成立。
    void cookieJarAndCountingWiringHolds();
};

void TestCookies::cookieJarAndCountingWiringHolds()
{
    const QString exe = qEnvironmentVariable("TIANBOARD_EXE");
    QVERIFY2(!exe.isEmpty(), "set TIANBOARD_EXE to the built TianBoard.exe (ctest does it through the "
                             "test's ENVIRONMENT property)");
    QVERIFY2(QFileInfo::exists(exe), qPrintable(QStringLiteral("missing executable: %1").arg(exe)));

    QProcess process;
    // 应用往 userDataDirectory() 下写文档与日志；不给它一个临时根目录，
    // 每跑一次用例就在用户的真实文档库里多攒几份没人认领的文档。
    TestProcessSandbox sandbox(process, QStringLiteral("cookies"));
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(exe, {QStringLiteral("--cookie-selftest")});
    QVERIFY2(process.waitForStarted(20000), "TianBoard.exe 没能启动");

    if (!process.waitForFinished(120000))
    {
        process.kill();
        process.waitForFinished();
        QFAIL("Cookie 自检 120 秒内没退出（多半是撞上 Qt 断言对话框或清理链路里卡住）");
    }

    QCOMPARE(process.exitStatus(), QProcess::NormalExit);

    const int code = process.exitCode();
    if (code == 75)
    {
        QSKIP("已有 TianBoard 实例在运行，Cookie 自检按约定让路（不算失败）");
    }

    const QString log = QString::fromLocal8Bit(process.readAll());
    QVERIFY2(code == 0, qPrintable(QStringLiteral("Cookie 隐私链路有 %1 处不一致：\n%2").arg(code).arg(log)));
}

QTEST_GUILESS_MAIN(TestCookies)
#include "tst_cookies.moc"
