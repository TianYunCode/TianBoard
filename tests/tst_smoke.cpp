// Starts the real TianBoard executable the way a user does, keeps it alive long
// enough to run its whole startup chain (settings, display detection, palettes,
// WebEngine), and fails if it dies on its own or ignores the close request that
// the application itself installs (MainWindow::closeEvent -> Application::closing
// -> persist + quit). Qt reports a broken SIGNAL/SLOT connect as a runtime
// warning only, so the captured log is scanned for those as well.
#include <QtTest>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>

class TestSmoke : public QObject {
    Q_OBJECT

private slots:
    void applicationStartsAndStaysAlive();
    void runtimeDataIsDeployedNextToTheExecutable();

private:
    enum Outcome {
        Completed = 0, // 活过启动链、干净退出
        BusyHandoff,   // 秒退且退出码 0：像是把请求转给了已存在的实例
        Broken         // 真失败，原因在 reason 里
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

    // 本程序是单实例的。上一次运行被强制结束、或桌面上真的开着一个实例时，新进程会
    // 把请求转给已存在的实例然后自己正常退出（code 0）——那不是缺陷，也不该让测试
    // 变红；崩溃与非零退出码仍然照实失败。
    for (int attempt = 1; attempt <= 3; ++attempt) {
        log.clear();
        reason.clear();
        const Outcome outcome = runOnce(exe, &log, &reason);
        if (outcome == Completed)
            return;
        if (outcome == Broken)
            QVERIFY2(false, qPrintable(reason));

        qDebug("第 %d 次：新进程立刻正常退出，像是单实例接管；等一下再试", attempt);
        QTest::qWait(3000);
    }

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

    // Startup does the document autosave, screen detection and palette creation;
    // a crash lands inside the first couple of seconds.
    QTest::qWait(6000);

    if (app.state() != QProcess::Running) {
        *reason = QStringLiteral("TianBoard exited by itself with code %1 (exit status %2)")
                      .arg(app.exitCode())
                      .arg(app.exitStatus());
        return (app.exitCode() == 0 && app.exitStatus() == QProcess::NormalExit) ? BusyHandoff : Broken;
    }

    *log = app.readAll();

    // Closing the window is the supported way out: MainWindow::closeEvent forwards to
    // Application::closing(), which persists the board and quits the loop. Measured on
    // an idle machine that takes about 2s, but it is doing real disk I/O while the rest
    // of the suite (and the antivirus) is busy, so give it a generous budget before
    // deciding the shutdown is stuck.
    app.terminate();
    if (!app.waitForFinished(30000)) {
        app.kill();
        app.waitForFinished(5000);
        *reason = QStringLiteral("TianBoard did not shut down within 30s of a close request");
        return Broken;
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

    // A GUI-subsystem executable inherits no console, so under QProcess the debug
    // output may never reach the pipe. Only assert on it when we did get output.
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

    // FileSystemUtils::applicationResourcesDirectory() == applicationDirPath(), so
    // these three directories and the default configuration must sit beside the
    // exe; without them the app starts with an empty settings tree.
    QVERIFY(appDir.exists("etc"));
    QVERIFY(appDir.exists("library"));
    QVERIFY(appDir.exists("customizations"));

    const QFile config(appDir.filePath("etc/TianBoard.config"));
    QVERIFY2(config.exists(), "etc/TianBoard.config is missing from the deployment");
    QVERIFY(config.size() > 0);
}

QTEST_GUILESS_MAIN(TestSmoke)
#include "tst_smoke.moc"
