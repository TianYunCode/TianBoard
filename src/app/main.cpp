/**
 * @file main.cpp
 * @brief 程序入口：安装消息处理与拖放实现，创建 Application 并处理命令行文件。
 */

#include <QtWidgets>
#include <QTextCodec>

#include "platform/PlatformUtils.h"
#include "utils/FileSystemUtils.h"

#include "core/DisplayManager.h"
#include "core/UserNotices.h"
#include "ui/MainWindow.h"
#include "ui/BlackoutWidget.h"
#include "ui/MessagesDialog.h"
#include "core/NetworkAccessManager.h"
#include "ui/ProxyDialog.h"
#include "ui/DocumentReplaceDialog.h"
#include "api/WebWidgetBoardAPI.h"
#include "app/SelfTests.h"
#include "core/Application.h"
#include "core/ApplicationController.h"
#include "core/PersistenceManager.h"
#include "content/DocumentFormatServices.h"
#include "model/IDocumentFormatServices.h"
#include "recording/RecordingController.h"
#include "settings/Settings.h"
#include "whiteboard/IBoardDropHandler.h"

/**
 * @brief 全局 Qt 消息处理：控制台输出、重复处理保护与日志文件写入。
 *
 * 先摘掉自身再转发，避免 handler 递归调用 qt_message_output 造成无限递归；
 * verbose 模式下把日志追加到用户数据目录的 log/ 下，超过阈值时先删除旧文件。
 */
void messageOutputHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // 必须先暂时摘掉 handler，否则会形成
    // messageOutputHandler -> qt_message_output -> messageOutputHandler 的无限递归
    QtMessageHandler previousHandler = qInstallMessageHandler(0);

#if defined(QT_NO_DEBUG)
    // release 构建下抑制 qDebug 输出
    if (type != QtDebugMsg)
    {
        qt_message_output(type, context, msg);
    }

#else
    // debug 构建下原样输出
    qt_message_output(type, context, msg);
#endif

    if (Application::app() && Application::app()->isVerbose())
    {
        QString logFileNamePath = Settings::userDataDirectory() + "/log/" + qApp->applicationName() + ".log";
        QFile   logFile(logFileNamePath);

        // 日志超过约 10 MB 时先删除，避免无限增长
        if (logFile.exists() && logFile.size() > 10000000)
        {
            logFile.remove();
        }

        if (logFile.open(QIODevice::Append | QIODevice::Text))
        {
            QTextStream out(&logFile);
            out << QDateTime::currentDateTime().toString(Qt::ISODate) << "      " << msg << "\n";
            logFile.close();
        }
    }

    qInstallMessageHandler(previousHandler);
}

int main(int argc, char *argv[])
{
    // QT_NO_GLIB=1 在 Linux 上会阻止媒体播放，这里主动清掉
    if (qEnvironmentVariableIsSet("QT_NO_GLIB"))
    {
        qunsetenv("QT_NO_GLIB");
    }

    Q_INIT_RESOURCE(TianBoard);

    qInstallMessageHandler(messageOutputHandler);

    // 装配点认识所有模块：画板只声明了 IBoardDropHandler 接口，实现是 api 的门面
    // （它需要 DownloadManager 和文档的对象目录）。装上之前画板不会崩，只是不接拖放。
    // 见 docs/architecture.md 拆环顺序第 3 步。
    installBoardDropHandlerFactory(
        [](GraphicsScene *scene, GraphicsWidgetItem *widget) -> IBoardDropHandler *
        {
            return new WebWidgetBoardAPI(scene, widget);
        });

    // 同一套路：模型只声明“要读元数据、要取缩略图”，实现是 content 里现成的两个读写器
    // （content 本来就认识 DocumentProxy，反过来才行得通）。装的是 static 对象——
    // 进程级无状态，装配点既不接管也不留悬挂指针。见 docs/architecture.md 拆环第 4 步。
    static DocumentFormatServices documentFormatServices;
    installDocumentFormatServices(&documentFormatServices);

    // 服务层要问用户拿站点凭据，但它不认识对话框：装配点把 ProxyDialog 接上去。
    NetworkAccessManager::installCredentialPrompt(&ProxyDialog::prompt);

    // 录制按钮的转发同理：总控器只说"用户要显隐录屏调色板"，具体找谁由这里决定。
    // instance() 是懒建的，所以第一次按下才创建录制控制器这件事留在实现里。
    ApplicationController::installRecordingToggle(
        [](bool visible)
        {
            RecordingController::instance()->toggleRecordingPalette(visible);
        });

    // 同一套路：文档名撞车时 PersistenceManager 只问“这份要不要登记、叫什么名字”，
    // 弹窗问用户的是界面。没装这个钩子时撞名一律不登记，调用方照样回收刚导入的那一份。
    PersistenceManager::installDuplicateNameResolver(&DocumentReplaceDialog::ask);

    // 同一套路：服务层只说"该告诉用户什么"，落到哪个窗口由这里决定。
    UserNotices::installWarningPresenter(
        [](const QString &title, const QStringList &messages)
        {
            MessagesDialog *dialog = new MessagesDialog(title, Application::mainWindow);
            // 不设这个属性的话，每次导出警告都会在主窗口下面攒一个关不掉的对话框对象。
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->setMessages(messages);
            dialog->show();
        });
    UserNotices::installModalNoticePresenter(
        [](const QString &title, const QString &text)
        {
            if (Application::mainWindow)
            {
                Application::mainWindow->warning(title, text);
            }
            else
            {
                qWarning() << title << text;
            }
        });

    // 黑屏表面：由界面层提供窗体，DisplayManager 只管铺到哪些屏幕、什么时候收回来。
    DisplayManager::installBlackoutSurfaceFactory(
        [](bool returnControlsVisible, const QRect &geometry, std::function<void()> onActivity) -> QWidget *
        {
            BlackoutWidget *surface = new BlackoutWidget(); // 由 DisplayManager::unBlackout 关闭并释放
            QObject::connect(surface, &BlackoutWidget::activity, surface,
                [onActivity]
                {
                    onActivity();
                });
            surface->setReturnControlsVisible(returnControlsVisible);
            surface->setGeometry(geometry);
            return surface;
        });

    Application app("TianBoard", argc, argv);
    // 自检是装配层的活：分发表在 app/SelfTests.cpp，core 只有一个通用钩子。
    installSelfTestFromArgs(app);

    QStringList args = app.arguments();

    // 日志目录可能尚未建立，先确保存在
    QString dumpPath = Settings::userDataDirectory() + "/log";
    QDir    logDir(dumpPath);
    if (!logDir.exists())
    {
        logDir.mkdir(dumpPath);
    }

    QString fileToOpen;

    if (args.size() > 1)
    {
        // Windows/Linux 下第一个参数是双击打开的文件；
        // macOS 通过 FileOpen 事件在现有实例中处理，因此这里不会收到文件参数。

        QFile f(args[1]);

        if (f.exists())
        {
            fileToOpen += args[1];

            // 已有实例在运行时把文件路径转发过去，本次进程直接退出
            if (app.sendMessage(Settings::appPingMessage, 20000))
            {
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
