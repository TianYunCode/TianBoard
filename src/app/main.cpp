/**
 * @file main.cpp
 * @brief 程序入口：安装消息处理与拖放实现，创建 Application 并处理命令行文件。
 */

#include <QtWidgets>
#include <QTextCodec>

#include "platform/PlatformUtils.h"
#include "utils/FileSystemUtils.h"

#include "api/WebWidgetBoardAPI.h"
#include "core/Application.h"
#include "settings/Settings.h"
#include "whiteboard/IBoardDropHandler.h"


/**
 * @brief 全局 Qt 消息处理：控制台输出、重复处理保护与日志文件写入。
 *
 * 先摘掉自身再转发，避免 handler 递归调用 qt_message_output 造成无限递归；
 * verbose 模式下把日志追加到用户数据目录的 log/ 下，超过阈值时先删除旧文件。
 */
void messageOutputHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    // 必须先暂时摘掉 handler，否则会形成
    // messageOutputHandler -> qt_message_output -> messageOutputHandler 的无限递归
    QtMessageHandler previousHandler = qInstallMessageHandler(0);

#if defined(QT_NO_DEBUG)
    // release 构建下抑制 qDebug 输出
    if (type != QtDebugMsg) {
        qt_message_output(type, context, msg);
    }

#else
    // debug 构建下原样输出
    qt_message_output(type, context, msg);
#endif

    if (Application::app() && Application::app()->isVerbose()) {
        QString logFileNamePath = Settings::userDataDirectory() + "/log/" + qApp->applicationName() + ".log";
        QFile logFile(logFileNamePath);

        // 日志超过约 10 MB 时先删除，避免无限增长
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
    // QT_NO_GLIB=1 在 Linux 上会阻止媒体播放，这里主动清掉
    if (qEnvironmentVariableIsSet("QT_NO_GLIB"))
        qunsetenv("QT_NO_GLIB");

    Q_INIT_RESOURCE(TianBoard);

    qInstallMessageHandler(messageOutputHandler);

    // 装配点认识所有模块：画板只声明了 IBoardDropHandler 接口，实现是 api 的门面
    // （它需要 DownloadManager 和文档的对象目录）。装上之前画板不会崩，只是不接拖放。
    // 见 docs/architecture.md 拆环顺序第 3 步。
    installBoardDropHandlerFactory(
        [](GraphicsScene *scene, GraphicsWidgetItem *widget) -> IBoardDropHandler * {
            return new WebWidgetBoardAPI(scene, widget);
        });

    Application app("TianBoard", argc, argv);

    QStringList args = app.arguments();

    // 日志目录可能尚未建立，先确保存在
    QString dumpPath = Settings::userDataDirectory() + "/log";
    QDir logDir(dumpPath);
    if (!logDir.exists())
        logDir.mkdir(dumpPath);

    QString fileToOpen;

    if (args.size() > 1) {
        // Windows/Linux 下第一个参数是双击打开的文件；
        // macOS 通过 FileOpen 事件在现有实例中处理，因此这里不会收到文件参数。

        QFile f(args[1]);

        if (f.exists()) {
            fileToOpen += args[1];

            // 已有实例在运行时把文件路径转发过去，本次进程直接退出
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
