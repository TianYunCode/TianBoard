#include <QtWidgets>
#include <QTextCodec>

#include "platform/PlatformUtils.h"
#include "utils/FileSystemUtils.h"

#include "api/WebWidgetBoardAPI.h"
#include "core/Application.h"
#include "settings/Settings.h"
#include "whiteboard/IBoardDropHandler.h"


void messageOutputHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    // We must temporarily remove the handler to avoid the infinite recursion of
    // messageOutputHandler -> qt_message_output -> messageOutputHandler -> qt_message_output ...
    QtMessageHandler previousHandler = qInstallMessageHandler(0);

#if defined(QT_NO_DEBUG)
    // Suppress qDebug output in release builds
    if (type != QtDebugMsg) {
        qt_message_output(type, context, msg);
    }

#else
    // Default output in debug builds
    qt_message_output(type, context, msg);
#endif

    if (Application::app() && Application::app()->isVerbose()) {
        QString logFileNamePath = Settings::userDataDirectory() + "/log/" + qApp->applicationName() + ".log";
        QFile logFile(logFileNamePath);

        if (logFile.exists() && logFile.size() > 10000000)
            logFile.remove();

        if (logFile.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&logFile);
            out << QDateTime::currentDateTime().toString(Qt::ISODate) << "      " << msg << "\n";
            logFile.close();
        }
    }

    qInstallMessageHandler(previousHandler);
}

int main(int argc, char *argv[]) {
    // Uncomment next section to have memory leaks information
    // tracing in VC++ debug mode under Windows
    /*
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif
*/

    // QT_NO_GLIB=1 is set by default on Linux, and prevents media playback
    if (qEnvironmentVariableIsSet("QT_NO_GLIB"))
        qunsetenv("QT_NO_GLIB");

    Q_INIT_RESOURCE(TianBoard);

    qInstallMessageHandler(messageOutputHandler);

    // 装配点认识所有模块：画板只声明了 IBoardDropHandler 接口，实现是 api 的门面
    // （它要 DownloadManager 和文档的对象目录）。装上之前画板不会崩，只是不接拖放。
    // 见 docs/architecture.md 拆环顺序第 3 步。
    installBoardDropHandlerFactory(
        [](GraphicsScene *scene, GraphicsWidgetItem *widget) -> IBoardDropHandler * {
            return new WebWidgetBoardAPI(scene, widget);
        });

    Application app("TianBoard", argc, argv);

    QStringList args = app.arguments();

    QString dumpPath = Settings::userDataDirectory() + "/log";
    QDir logDir(dumpPath);
    if (!logDir.exists())
        logDir.mkdir(dumpPath);

    QString fileToOpen;

    if (args.size() > 1) {
        // On Windows/Linux first argument is the file that has been double clicked.
        // On Mac OSX we use FileOpen QEvent to manage opening file in current instance. So we will never
        // have file to open as a parameter on OSX.

        QFile f(args[1]);

        if (f.exists()) {
            fileToOpen += args[1];

            if (app.sendMessage(Settings::appPingMessage, 20000)) {
                app.sendMessage(fileToOpen, 1000000);
                return 0;
            }
        }
    }

    qDebug() << "file name argument" << fileToOpen;
    int result = app.exec(fileToOpen);

    app.cleanup();

    qDebug() << "application is quitting";


    return result;
}
