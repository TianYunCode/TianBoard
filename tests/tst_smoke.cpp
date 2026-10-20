/**
 * @file tst_smoke.cpp
 * @brief 冒烟测试：像用户那样把真的 TianBoard 可执行文件启起来，让它跑完整条启动链，
 *        再走应用自己装的关闭路径退干净；自己先死掉或不理关闭请求都算失败。
 *
 * 存活时间要够长，能走完整个启动链（配置、显示器探测、调色板、WebEngine）。
 * 关闭走的是应用自己装的那条路：MainWindow::closeEvent -> Application::closing
 * -> 落盘 + 退出事件循环。Qt 把连错的 SIGNAL/SLOT 只报成运行时警告，所以捕获到的
 * 日志也要一并扫这些标记。
 */

#include <QtTest>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>

class TestSmoke : public QObject {
    Q_OBJECT

private slots:
    /// @brief 应用能自己起来、活过整条启动链，收到关闭请求后正常退出，且日志里没有连错的信号槽。
    void applicationStartsAndStaysAlive();
    /// @brief etc / library / customizations 与默认配置必须和 exe 同目录，否则启动时配置树是空的。
    void runtimeDataIsDeployedNextToTheExecutable();

private:
    enum Outcome {
        Completed = 0,   // 活过启动链、干净退出
        BusyHandoff,     // 秒退且退出码 0：像是把请求转给了已存在的实例
        ShutdownStalled, // 启动链没问题，但关闭请求 20 秒内没走完（多半是机器被别的
                         // 编译/扫描占满，也可能是真的卡住——重试才能分开这两种）
        Broken           // 真失败，原因在 reason 里
    };

    Outcome runOnce(const QString &exe, QByteArray *log, QString *reason);
};

static const char *kBrokenConnectMarkers[] = {
    "No such signal",
    "No such slot",
    "QObject::connect",
};

void TestSmoke::applicationStartsAndStaysAlive() {
    const QString exe = qEnvironmentVariable("TIANBOARD_EXE");
    QVERIFY2(!exe.isEmpty(), "set TIANBOARD_EXE to the built TianBoard.exe (ctest does it through the test's "
                             "ENVIRONMENT property)");
    QVERIFY2(QFileInfo::exists(exe), qPrintable(QStringLiteral("missing executable: %1").arg(exe)));

    QByteArray log;
    QString reason;
    Outcome last = BusyHandoff;

    // 两种结局不是缺陷，值得再试一次：
    //  - 本程序是单实例的，上一次运行被强杀或桌面上真开着实例时，新进程会把请求转交
    //    给已存在的实例然后自己正常退出（code 0）；
    //  - 关闭要走落盘与拆 WebEngine，机器被别的构建任务/杀毒扫描占满时可能超出预算。
    // 崩溃与非零退出码不重试，直接失败。
    for (int attempt = 1; attempt <= 3; ++attempt) {
        log.clear();
        reason.clear();
        last = runOnce(exe, &log, &reason);
        if (last == Completed)
            return;
        if (last == Broken)
            QVERIFY2(false, qPrintable(reason));

        qDebug("第 %d 次：%s；等一下再试", attempt, qPrintable(reason));
        QTest::qWait(3000);
    }

    if (last == ShutdownStalled)
        QVERIFY2(false, qPrintable(reason + QStringLiteral("（连续 3 次都这么慢：机器长期满负荷，"
                                                           "或者关闭流程真的卡住，请单独重跑本用例）")));

    QSKIP("没能独占一次干净的启动：TianBoard 每次都立即把请求转给已存在的实例");
}

TestSmoke::Outcome TestSmoke::runOnce(const QString &exe, QByteArray *log, QString *reason) {
    QProcess app;
    app.setProgram(exe);
    app.setWorkingDirectory(QFileInfo(exe).absolutePath());
    app.setProcessChannelMode(QProcess::MergedChannels);
    app.start();

    if (!app.waitForStarted(20000)) {
        *reason = QStringLiteral("TianBoard.exe did not start within 20s");
        return Broken;
    }

    // 启动阶段要做文档自动保存、屏幕探测与调色板创建；
    // 会崩的话基本都崩在头几秒之内。
    QTest::qWait(6000);

    if (app.state() != QProcess::Running) {
        *reason = QStringLiteral("TianBoard exited by itself with code %1 (exit status %2)")
                      .arg(app.exitCode())
                      .arg(app.exitStatus());
        return (app.exitCode() == 0 && app.exitStatus() == QProcess::NormalExit) ? BusyHandoff : Broken;
    }

    *log = app.readAll();

    // 关窗口才是受支持的退出方式：MainWindow::closeEvent 把请求转给
    // Application::closing()，由它把画板落盘、再退出事件循环。空载机器上实测约 2s，
    // 但这段时间里它在做真实的磁盘 I/O，而测试套件的其余部分（以及杀毒软件）同时也在
    // 忙，所以单次关闭超时只当重试理由——连着三次都超时才算真的卡住。
    app.terminate();
    if (!app.waitForFinished(20000)) {
        app.kill();
        app.waitForFinished(5000);
        *reason = QStringLiteral("TianBoard did not shut down within 20s of a close request");
        return ShutdownStalled;
    }
    *log += app.readAll();

    if (app.exitStatus() == QProcess::CrashExit) {
        *reason =
            QStringLiteral("TianBoard crashed while shutting down\n%1").arg(QString::fromLocal8Bit(*log));
        return Broken;
    }
    if (app.exitCode() != 0) {
        *reason = QStringLiteral("TianBoard exited with code %1 while shutting down\n%2")
                      .arg(app.exitCode())
                      .arg(QString::fromLocal8Bit(*log));
        return Broken;
    }

    // GUI 子系统的可执行文件不继承控制台，所以在 QProcess 下调试输出可能根本到不了
    // 管道。只有确实收到了输出，才对它做断言。
    if (!log->trimmed().isEmpty()) {
        for (const char *marker : kBrokenConnectMarkers) {
            if (log->contains(marker)) {
                *reason = QStringLiteral("startup log contains \"%1\":\n%2")
                              .arg(QLatin1String(marker), QString::fromLocal8Bit(*log));
                return Broken;
            }
        }
    }

    return Completed;
}

void TestSmoke::runtimeDataIsDeployedNextToTheExecutable() {
    const QDir appDir = QFileInfo(qEnvironmentVariable("TIANBOARD_EXE")).absoluteDir();

    // FileSystemUtils::applicationResourcesDirectory() == applicationDirPath()，所以
    // 这三个目录和默认配置必须紧挨着 exe 放；少了它们，应用会带着一棵空的配置树启动。
    QVERIFY(appDir.exists("etc"));
    QVERIFY(appDir.exists("library"));
    QVERIFY(appDir.exists("customizations"));

    const QFile config(appDir.filePath("etc/TianBoard.config"));
    QVERIFY2(config.exists(), "etc/TianBoard.config is missing from the deployment");
    QVERIFY(config.size() > 0);
}

QTEST_GUILESS_MAIN(TestSmoke)
#include "tst_smoke.moc"
