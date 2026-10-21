#include "core/Application.h"
#include "core/VirtualDesktop.h"
#include <QWebEngineProfile>
#include <QRegularExpression>
#include <QRegExp>

#include <QtWidgets>
#include <QtWebEngineWidgets>
#include <QtXml>
#include <QFontDatabase>
#include <QStyleFactory>

#include "platform/PlatformUtils.h"
#include "core/LanguageManager.h"
#include "utils/FileSystemUtils.h"

#include "settings/Settings.h"
#include "settings/Setting.h"
#include "core/PersistenceManager.h"
#include "core/PreferencesController.h"
#include "core/ApplicationController.h"

#include "whiteboard/BoardController.h"
#include "whiteboard/BoardView.h"
#include "whiteboard/DrawingController.h"
#include "whiteboard/BoardPaletteManager.h"
#include "web/WebController.h"

#include "ui/DocumentController.h"

#include "ui/MainWindow.h"
#include "ui/Resources.h"

#include "whiteboard/ToolsManager.h"
#include "core/CookieSelfTest.h"
#include "core/DisplayManager.h"
#include "core/FormatSelfTest.h"
#include "core/SnapSelfTest.h"
#include "core/ShortcutManager.h"
#include "core/ShortcutSelfTest.h"

QPointer<QUndoStack> Application::undoStack;

ApplicationController *Application::applicationController = 0;
BoardController       *Application::boardController       = 0;
WebController         *Application::webController         = 0;
DocumentController    *Application::documentController    = 0;

MainWindow *Application::mainWindow                       = 0;

const QString Application::mimeTypeBoardDocument          = QString("application/vnd.tianboard-document");
const QString Application::mimeTypeBoardPage              = QString("application/vnd.tianboard-page");
const QString Application::mimeTypeBoardPageItem          = QString("application/vnd.tianboard-page-item");
const QString Application::mimeTypeBoardPageThumbnail     = QString("application/vnd.tianboard-thumbnail");

QObject *Application::staticMemoryCleaner                 = 0;

Application::Application(const QString &id, int &argc, char **argv) : QtSingleApplication(id, argc, argv), mPreferencesController(NULL)
{
    staticMemoryCleaner = new QObject(0); // 由 Application 析构函数删除

    setOrganizationName("TianBoard");
    setOrganizationDomain("");
    setApplicationName("TianBoard");

    setApplicationVersion(QString::fromLatin1(TIANBOARD_VERSION));

    // 登录名要早于任何路径设置被读；设置层只认这个值，不反向依赖 platform。
    Settings::setUserLoginName(PlatformUtils::osUserLoginName());

    QStringList args = arguments();

    mIsVerbose       = args.contains("-v") || args.contains("-verbose") || args.contains("verbose") || args.contains("-log") || args.contains("log");

    setupTranslators(args);

    Resources::resources();

    if (!undoStack)
    {
        undoStack = new QUndoStack(staticMemoryCleaner);
    }

    PlatformUtils::init();

    Settings *settings = Settings::settings();

    connect(settings->appToolBarPositionedAtTop, &Setting::changed, this, &Application::toolBarPositionChanged);
    connect(settings->appToolBarDisplayText, &Setting::changed, this, &Application::toolBarDisplayTextChanged);
    updateProtoActionsState();

    setWindowIcon(QIcon(":/images/TianBoardIcon.svg"));

    setStyle("fusion");

    QString css = FileSystemUtils::readTextFile(FileSystemUtils::applicationResourcesDirectory() + "/etc/" + qApp->applicationName() + ".css");
    if (css.length() > 0)
    {
        setStyleSheet(css);
    }

    QApplication::setStartDragDistance(8); // 默认值是 4，对触摸屏来说偏小

    installEventFilter(this);
}

Application::~Application()
{
    PlatformUtils::destroy();

    // 快捷键注册表里存的是主窗口 QAction 的裸指针，必须赶在主窗口之前散掉。
    ShortcutManager::destroy();

    FileSystemUtils::deleteAllTempDirCreatedDuringSession();

    delete mainWindow;
    mainWindow = 0;

    PersistenceManager::destroy();

    DownloadManager::destroy();

    DrawingController::destroy();

    Settings::destroy();

    ToolsManager::destroy();

    delete staticMemoryCleaner;
    staticMemoryCleaner = 0;
}

void Application::setupTranslators(const QStringList &args)
{
    LanguageManager *languages = LanguageManager::instance();

    QString requested;

    const int langIndex = args.indexOf("-lang");
    if (langIndex >= 0 && langIndex + 1 < args.size())
    {
        requested = args.at(langIndex + 1);
    }

    if (requested.isEmpty())
    {
        requested = languages->savedPreference();
    }

    if (requested.isEmpty())
    {
        requested = PlatformUtils::systemLanguage();
    }

    // 没有对应 .qm 时退回源语言（英文），而不是像以前那样声称在某个语言下运行
    // 却一个翻译器都没装。
    if (!languages->setLanguage(requested))
    {
        languages->setLanguage(QStringLiteral("en"));
    }

    qDebug() << "Running application in:" << languages->currentLanguage();
}

int Application::exec(const QString &pFileToImport)
{
    QPixmapCache::setCacheLimit(1024 * 100);

    QString webDbPath = Settings::userDataDirectory() + "/web-databases";
    QDir    webDbDir(webDbPath);
    if (!webDbDir.exists(webDbPath))
    {
        webDbDir.mkpath(webDbPath);
    }

    QWebEngineSettings *gs = QWebEngineProfile::defaultProfile()->settings();
    gs->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    gs->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    gs->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    gs->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, true);
    gs->setAttribute(QWebEngineSettings::HyperlinkAuditingEnabled, true);
    gs->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
    gs->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);

    mainWindow = new MainWindow(0, Qt::FramelessWindowHint);
    mainWindow->setAttribute(Qt::WA_NativeWindow, true);

    ThumbnailUI::_private::initCatalog();

    connect(mainWindow->ui().actionBoard, &QAction::triggered, this, &Application::showBoard);
    connect(mainWindow->ui().actionWeb, &QAction::triggered, this, &Application::showInternet);
    connect(mainWindow->ui().actionWeb, &QAction::triggered, this, &Application::stopScript);
    connect(mainWindow->ui().actionDocument, &QAction::triggered, this, &Application::showDocument);
    connect(mainWindow->ui().actionDocument, &QAction::triggered, this, &Application::stopScript);
    connect(mainWindow->ui().actionQuit, &QAction::triggered, this, &Application::closing);
    connect(mainWindow, &MainWindow::closeEvent_Signal, this, &Application::closeEvent);

    boardController = new BoardController(mainWindow);
    boardController->init();

    webController      = new WebController(mainWindow);
    documentController = new DocumentController(mainWindow);

    DrawingController::drawingController()->setStylusTool((int)StylusTool::Pen);

    applicationController = new ApplicationController(boardController->controlView(), boardController->displayView(), mainWindow, staticMemoryCleaner,
        boardController->paletteManager()->rightPalette());

    connect(
        applicationController, &ApplicationController::mainModeChanged, boardController->paletteManager(), &BoardPaletteManager::slot_changeMainMode);

    connect(
        applicationController, &ApplicationController::desktopMode, boardController->paletteManager(), &BoardPaletteManager::slot_changeDesktopMode);

    connect(applicationController, &ApplicationController::mainModeChanged, boardController, &BoardController::appMainModeChanged);

    // 只有白板和网页模式真的实现了 剪切/复制/粘贴；文档库模式下这三个动作过去是
    // "看着能点、按了没反应"（DocumentController 里是三个空函数），现在直接灰掉。
    connect(applicationController, &ApplicationController::mainModeChanged, this,
        [this](ApplicationController::MainMode mode)
        {
            const bool hasClipboard = (mode == ApplicationController::Board || mode == ApplicationController::Internet);
            mainWindow->ui().actionCut->setEnabled(hasClipboard);
            mainWindow->ui().actionCopy->setEnabled(hasClipboard);
            mainWindow->ui().actionPaste->setEnabled(hasClipboard);
        });

    connect(mainWindow->ui().actionDesktop, &QAction::triggered, applicationController, &ApplicationController::showDesktop);
    connect(mainWindow->ui().actionDesktop, &QAction::triggered, this, &Application::stopScript);
    connect(mainWindow->ui().actionRecording, &QAction::triggered, applicationController, &ApplicationController::toggleRecordingPalette);
    connect(mainWindow->ui().actionHideApplication, &QAction::triggered, mainWindow, &MainWindow::showMinimized);

    mPreferencesController = new PreferencesController(mainWindow);

    connect(mainWindow->ui().actionPreferences, &QAction::triggered, mPreferencesController, &PreferencesController::show);

    toolBarPositionChanged(Settings::settings()->appToolBarPositionedAtTop->get());

    bool bUseMultiScreen = Settings::settings()->appUseMultiscreen->get().toBool();
    mainWindow->ui().actionMultiScreen->setChecked(bUseMultiScreen);
    connect(mainWindow->ui().actionMultiScreen, &QAction::triggered, applicationController, &ApplicationController::useMultiScreen);
    connect(mainWindow->ui().actionWidePageSize, &QAction::triggered, boardController, &BoardController::setWidePageSize);
    connect(mainWindow->ui().actionRegularPageSize, &QAction::triggered, boardController, &BoardController::setRegularPageSize);

    connect(mainWindow->ui().actionCut, &QAction::triggered, applicationController, &ApplicationController::actionCut);
    connect(mainWindow->ui().actionCopy, &QAction::triggered, applicationController, &ApplicationController::actionCopy);
    connect(mainWindow->ui().actionPaste, &QAction::triggered, applicationController, &ApplicationController::actionPaste);

    applicationController->initScreenLayout(bUseMultiScreen);
    boardController->setupLayout();

    if (pFileToImport.length() > 0)
    {
        Application::applicationController->importFile(pFileToImport);
    }

    if (Settings::settings()->appStartMode->get().toInt())
    {
        applicationController->showDesktop();
    }
    else
    {
        applicationController->showBoard();
    }

    emit DrawingController::drawingController() -> colorPaletteChanged();

    onScreenCountChanged(1);
    connect(VirtualDesktop::instance(), &VirtualDesktop::screenCountChanged, this, &Application::onScreenCountChanged);

    // 第二个实例把要打开的文档路径通过单实例通道发过来；此前没人收这条消息，
    // 双击文档只会让新进程退出、已运行的实例毫无反应。
    setActivationWindow(mainWindow);
    connect(this, &QtSingleApplication::messageReceived, this, &Application::handleOpenMessage);

    // 冒烟自检（tst_smoke 用 `--ui-smoke` 拉起）：依次切到文档 / 网页 / 画板模式再退出。
    // 这些切换以前从来没有自动化覆盖；文档模式的代理模型索引错用就是这样漏到用户手里的
    // （Debug 下 Qt 弹断言对话框、事件循环卡死）。断言一旦出现，进程不会按时退出，测试判红。
    if (arguments().contains("--ui-smoke"))
    {
        QTimer::singleShot(2000, applicationController, &ApplicationController::showDocument);
        QTimer::singleShot(4000, applicationController, &ApplicationController::showInternet);
        QTimer::singleShot(6000, applicationController, &ApplicationController::showBoard);
        QTimer::singleShot(8000, qApp, &QCoreApplication::quit);
    }

    // 自检模式：让应用带着真实装配跑一段只查对错的代码，用退出码回报不一致的条数。
    // 判据写在各自的 *SelfTest.cpp 里，这里只管拉起跑。
    // 约定：0 = 通过，非 0 = 不一致条数；75 = 已有实例在跑、让路——测试据此判“跳过”而不是
    // 判失败，因为自检会写临时文档、动场景缓存与用户配置，两个进程并行没有意义。
    // 崩溃时 stderr 是块缓冲的，最后几条日志会跟着进程一起丢，看不出停在哪；verbose 分支
    // 按条 open/close 落盘（见 app/main.cpp 的消息处理器），所以自检一律开它。
    struct SelfTestMode
    {
        const char *flag;
        int (*run)();
    };

    static const SelfTestMode selfTestModes[] = {
        {"--format-selftest",   FormatSelfTest::run  }, // 文档格式读写往返（tst_format）
        {"--snap-selftest",     SnapSelfTest::run    }, // 网格吸线的场景侧接线（tst_snap）
        {"--shortcut-selftest", ShortcutSelfTest::run}, // 快捷键注册表的读写与冲突（tst_shortcuts）
        {"--cookie-selftest",   CookieSelfTest::run  }, // Cookie 镜像罐与退出清理计数（tst_cookies）
    };

    for (const SelfTestMode &mode : selfTestModes)
    {
        if (!arguments().contains(QLatin1String(mode.flag)))
        {
            continue;
        }

        setVerbose(true);

        const bool anotherInstanceRunning = isRunning();

        if (anotherInstanceRunning)
        {
            qWarning() << mode.flag << "：已有实例在运行，让路";
        }

        QTimer::singleShot(anotherInstanceRunning ? 0 : 1500, qApp,
            [mode, anotherInstanceRunning]
            {
                qApp->exit(anotherInstanceRunning ? 75 : mode.run());
            });
    }

    return QApplication::exec();
}

void Application::onScreenCountChanged(int newCount)
{
    Q_UNUSED(newCount);
    DisplayManager displayManager;
    mainWindow->ui().actionMultiScreen->setEnabled(displayManager.numScreens() > 1);
}

void Application::startScript()
{
    this->boardController->freezeWebWidgets(false);
}

void Application::stopScript()
{
    this->boardController->freezeWebWidgets(true);
}

void Application::showBoard()
{
    applicationController->showBoard();
}

void Application::showInternet()
{
    applicationController->showInternet();
    webController->showTabAtTop(true);
}

void Application::showDocument()
{
    applicationController->showDocument();
}

int Application::toolBarHeight()
{
    return mainWindow->ui().boardToolBar->rect().height();
}

void Application::toolBarPositionChanged(QVariant topOrBottom)
{
    Qt::ToolBarArea area;

    if (topOrBottom.toBool())
    {
        area = Qt::TopToolBarArea;
    }
    else
    {
        area = Qt::BottomToolBarArea;
    }

    mainWindow->addToolBar(area, mainWindow->ui().boardToolBar);
    mainWindow->addToolBar(area, mainWindow->ui().webToolBar);
    mainWindow->addToolBar(area, mainWindow->ui().documentToolBar);

    webController->showTabAtTop(topOrBottom.toBool());
}

void Application::toolBarDisplayTextChanged(QVariant display)
{
    Qt::ToolButtonStyle toolButtonStyle = display.toBool() ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonIconOnly;
    mainWindow->ui().boardToolBar->setToolButtonStyle(toolButtonStyle);
    mainWindow->ui().webToolBar->setToolButtonStyle(toolButtonStyle);
    mainWindow->ui().documentToolBar->setToolButtonStyle(toolButtonStyle);
}

void Application::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);

    closing();
}

void Application::closing()
{
    if (Settings::settings()->emptyTrashForOlderDocuments->get().toBool())
    {
        DocumentTreeModel *docModel = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;
        documentController->deleteDocumentsInFolderOlderThan(docModel->trashIndex(), Settings::settings()->emptyTrashDaysValue->get().toInt());
        if (docModel->hasChildren(docModel->trashIndex()))
        {
            documentController->deleteEmptyFolders(docModel->trashIndex());
        }
    }

    if (boardController)
    {
        boardController->closing();
    }

    if (applicationController)
    {
        applicationController->closing();
    }

    if (webController)
    {
        webController->closing();
    }

    // 工具栏位置要写在落盘之前：closing() 里已经 save() 过，之后 set() 只会进队列。
    Settings::settings()->appToolBarPositionedAtTop->set(mainWindow->toolBarArea(mainWindow->ui().boardToolBar) == Qt::TopToolBarArea);

    Settings::settings()->closing();

    quit();
}

void Application::showMessage(const QString &message, bool showSpinningWheel)
{
    if (applicationController)
    {
        applicationController->showMessage(message, showSpinningWheel);
    }
}

void Application::setDisabled(bool disable)
{
    boardController->setDisabled(disable);
}

void Application::decorateActionMenu(QAction *action)
{
    foreach (QObject *associated, action->associatedObjects())
    {
        QToolButton *tb = qobject_cast<QToolButton *>(associated);

        if (tb && !tb->menu())
        {
            tb->setObjectName("buttonMenu");
            tb->setPopupMode(QToolButton::InstantPopup);
            QMenu *menu                 = new QMenu(mainWindow);

            QActionGroup *pageSizeGroup = new QActionGroup(mainWindow);
            pageSizeGroup->addAction(mainWindow->ui().actionWidePageSize);
            pageSizeGroup->addAction(mainWindow->ui().actionRegularPageSize);
            pageSizeGroup->addAction(mainWindow->ui().actionCustomPageSize);

            QMenu *documentSizeMenu = menu->addMenu(QIcon(":/images/toolbar/pageSize.png"), tr("Page Size"));
            documentSizeMenu->addAction(mainWindow->ui().actionWidePageSize);
            documentSizeMenu->addAction(mainWindow->ui().actionRegularPageSize);
            documentSizeMenu->addAction(mainWindow->ui().actionCustomPageSize);
            menu->addAction(mainWindow->ui().actionCut);
            menu->addAction(mainWindow->ui().actionCopy);
            menu->addAction(mainWindow->ui().actionPaste);
            menu->addAction(mainWindow->ui().actionHideApplication);
            menu->addAction(mainWindow->ui().actionSleep);

            menu->addSeparator();
            menu->addAction(mainWindow->ui().actionPreferences);
            menu->addAction(mainWindow->ui().actionMultiScreen);
            menu->addSeparator();

            menu->addAction(mainWindow->ui().actionRecording);
            mainWindow->ui().actionRecording->setText(tr("Screen Recording"));

            menu->addSeparator();
            menu->addAction(mainWindow->ui().actionQuit);

            tb->setMenu(menu);
        }
    }
}

void Application::updateProtoActionsState()
{
    if (mainWindow)
    {
        mainWindow->ui().actionMultiScreen->setVisible(true);
    }

    foreach (QMenu *protoMenu, mProtoMenus)
    {
        protoMenu->setVisible(true);
    }
}

void Application::insertSpaceToToolbarBeforeAction(QToolBar *toolbar, QAction *action, int width)
{
    QWidget *spacer = new QWidget();

    if (width >= 0)
    {
        QHBoxLayout *layout = new QHBoxLayout();
        layout->addSpacing(width);
        spacer->setLayout(layout);
    }
    else
    {
        spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    }

    toolbar->insertWidget(action, spacer);
}

bool Application::eventFilter(QObject *obj, QEvent *event)
{
    bool result = QObject::eventFilter(obj, event);

    if (event->type() == QEvent::FileOpen)
    {
        QFileOpenEvent *fileToOpenEvent = static_cast<QFileOpenEvent *>(event);

        PlatformUtils::setFrontProcess();

        applicationController->importFile(fileToOpenEvent->file());
    }

    if (event->type() == QEvent::TabletLeaveProximity)
    {
        if (boardController && boardController->controlView())
        {
            boardController->controlView()->forcedTabletRelease();
        }
    }

    if (event->type() == QEvent::ApplicationActivate)
    {
        if (boardController && boardController->controlView())
        {
            boardController->controlView()->setMultiselection(false);
        }
    }

    return result;
}

bool Application::handleOpenMessage(const QString &pMessage)
{
    qDebug() << "received message" << pMessage;

    if (pMessage == Settings::appPingMessage)
    {
        qDebug() << "received ping";
        return true;
    }

    qDebug() << "importing file" << pMessage;

    Application::applicationController->importFile(pMessage);

    return true;
}

void Application::cleanup()
{
    if (applicationController)
    {
        delete applicationController;
    }
    if (boardController)
    {
        delete boardController;
    }
    if (webController)
    {
        delete webController;
    }
    if (documentController)
    {
        delete documentController;
    }

    applicationController = NULL;
    boardController       = NULL;
    webController         = NULL;
    documentController    = NULL;
}

QWidget *Application::dialogHost()
{
    return mainWindow;
}

QString Application::urlFromHtml(QString html)
{
    QString            _html;
    QRegularExpression comments("<[ \r\n\t]*(--([^\\-]|[\r\n]|-[^\\-])*--[ \r\n\t]*)>");
    QString            url;
    QDomDocument       domDoc;

    // 去掉 HTML 注释与 CRLF，避免解析器在提取 src 时被注释节点干扰。
    _html = html.remove(comments);
    domDoc.setContent(_html.remove(QRegularExpression("[\\0]")));
    QDomElement rootElem = domDoc.documentElement();

    // 根元素有时是 <meta> 标签，这时 src 挂在它的第一个子元素上。
    if (rootElem.tagName().toLower().contains("meta"))
    {
        qDebug() << rootElem.firstChildElement().tagName();
        // 取下一个元素的 src。
        url = rootElem.firstChildElement().attribute("src");
    }
    else
    {
        url = rootElem.attribute("src");
    }

    return url;
}

bool Application::isFromWeb(QString url)
{
    bool res = true;

    if (url.startsWith("tianboardtool://") || url.startsWith("file://") || url.startsWith("/"))
    {
        res = false;
    }

    return res;
}

QScreen *Application::controlScreen()
{
    QList<QScreen *> screenList = screens();
    if (screenList.size() == 1)
    {
        return screenList.first();
    }

    return screenList[controlScreenIndex()];
}

int Application::controlScreenIndex()
{
    return applicationController->displayManager()->controleScreenIndex();
}
