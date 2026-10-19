// Starts the real TianBoard executable the way a user does, keeps it alive long
// enough to run its whole startup chain (settings, display detection, palettes,
// WebEngine) and fails if it dies on its own. Qt reports a broken
// SIGNAL/SLOT connect as a runtime warning only, so the captured log is scanned
// for those as well.
#include <QtTest>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>

class TestSmoke : public QObject
{
    Q_OBJECT

private slots:
    void applicationStartsAndStaysAlive();
    void runtimeDataIsDeployedNextToTheExecutable();
};

static const char *kBrokenConnectMarkers[] = {
    "No such signal",
    "No such slot",
    "QObject::connect",
};

void TestSmoke::applicationStartsAndStaysAlive()
{
    const QString exe = qEnvironmentVariable("TIANBOARD_EXE");
    QVERIFY2(!exe.isEmpty(),
             "set TIANBOARD_EXE to the built TianBoard.exe (ctest does it through the test's ENVIRONMENT property)");
    QVERIFY2(QFileInfo::exists(exe), qPrintable(QStringLiteral("missing executable: %1").arg(exe)));

    QProcess app;
    app.setProgram(exe);
    app.setWorkingDirectory(QFileInfo(exe).absolutePath());
    app.setProcessChannelMode(QProcess::MergedChannels);
    app.start();

    if (!app.waitForStarted(20000))
        QFAIL("TianBoard.exe did not start within 20s");

    // Startup does the document autosave, screen detection and palette creation;
    // a crash lands inside the first couple of seconds.
    QVERIFY2(app.state() == QProcess::Running, "process is not running right after start");
    QTest::qWait(6000);

    if (app.state() != QProcess::Running) {
        QVERIFY2(false, qPrintable(QStringLiteral("TianBoard exited by itself with code %1 (exit status %2)")
                                       .arg(app.exitCode())
                                       .arg(app.exitStatus())));
    }

    const QByteArray log = app.readAll();

    app.terminate();
    if (!app.waitForFinished(10000)) {
        app.kill();
        app.waitForFinished(5000);
    }

    const QByteArray fullLog = log + app.readAll();

    // A GUI-subsystem executable inherits no console, so under QProcess the debug
    // output may never reach the pipe. Only assert on it when we did get output.
    if (!fullLog.trimmed().isEmpty()) {
        for (const char *marker : kBrokenConnectMarkers) {
            QVERIFY2(!fullLog.contains(marker),
                     qPrintable(QStringLiteral("startup log contains \"%1\":\n%2")
                                    .arg(QLatin1String(marker))
                                    .arg(QString::fromLocal8Bit(fullLog))));
        }
    }

    if (app.exitStatus() == QProcess::CrashExit)
        QFAIL("TianBoard crashed while shutting down");
}

void TestSmoke::runtimeDataIsDeployedNextToTheExecutable()
{
    const QDir appDir = QFileInfo(qEnvironmentVariable("TIANBOARD_EXE")).absoluteDir();

    // PlatformUtils::applicationResourcesDirectory() == applicationDirPath(), so
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
