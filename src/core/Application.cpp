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
#include "utils/StringUtils.h"

#include "settings/Settings.h"
#include "settings/Setting.h"
#include "core/PersistenceManager.h"
#include "core/DocumentManager.h"
#include "core/PreferencesController.h"
#include "core/IdleTimer.h"
#include "core/ApplicationController.h"

#include "whiteboard/BoardController.h"
#include "whiteboard/DrawingController.h"
#include "whiteboard/BoardView.h"
#include "whiteboard/BoardPaletteManager.h"
#include "web/WebController.h"

#include "model/DocumentController.h"
#include "model/DocumentProxy.h"

#include "ui/MainWindow.h"
#include "ui/Resources.h"
#include "ui/ThumbnailWidget.h"

#include "whiteboard/ToolsManager.h"
#include "core/DisplayManager.h"

QPointer<QUndoStack> Application::undoStack;

ApplicationController *Application::applicationController = 0;
BoardController *Application::boardController = 0;
WebController *Application::webController = 0;
DocumentController *Application::documentController = 0;

MainWindow *Application::mainWindow = 0;

const QString Application::mimeTypeBoardDocument = QString("application/vnd.tianboard-document");
const QString Application::mimeTypeBoardPage = QString("application/vnd.tianboard-page");
const QString Application::mimeTypeBoardPageItem = QString("application/vnd.tianboard-page-item");
const QString Application::mimeTypeBoardPageThumbnail = QString("application/vnd.tianboard-thumbnail");

#if defined(Q_OS_OSX) || defined(Q_OS_LINUX)
bool bIsMinimized = false;
#endif

QObject *Application::staticMemoryCleaner = 0;


Application::Application(const QString &id, int &argc, char **argv)
    : QtSingleApplication(id, argc, argv), mPreferencesController(NULL) {
    staticMemoryCleaner = new QObject(0); // deleted in Application destructor

    setOrganizationName("TianBoard");
    setOrganizationDomain("");
    setApplicationName("TianBoard");

    setApplicationVersion(QString::fromLatin1(TIANBOARD_VERSION));

    // 登录名要早于任何路径设置被读；设置层只认这个值，不反向依赖 platform。
    Settings::setUserLoginName(PlatformUtils::osUserLoginName());

    QStringList args = arguments();

    mIsVerbose = args.contains("-v") || args.contains("-verbose") || args.contains("verbose") ||
                 args.contains("-log") || args.contains("log");


    setupTranslators(args);

    Resources::resources();

    if (!undoStack)
        undoStack = new QUndoStack(staticMemoryCleaner);

    PlatformUtils::init();

    Settings *settings = Settings::settings();

    connect(settings->appToolBarPositionedAtTop, &Setting::changed, this,
            &Application::toolBarPositionChanged);
    connect(settings->appToolBarDisplayText, &Setting::changed, this,
            &Application::toolBarDisplayTextChanged);
    updateProtoActionsState();

#ifndef Q_OS_OSX
    setWindowIcon(QIcon(":/images/TianBoardIcon.svg"));
#endif

    setStyle("fusion");

    QString css = FileSystemUtils::readTextFile(FileSystemUtils::applicationResourcesDirectory() + "/etc/" +
                                                qApp->applicationName() + ".css");
    if (css.length() > 0)
        setStyleSheet(css);

    QApplication::setStartDragDistance(8); // default is 4, and is a bit small for tablets

    installEventFilter(this);
}


Application::~Application() {
    PlatformUtils::destroy();

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

void Application::setupTranslators(const QStringList &args) {
    LanguageManager *languages = LanguageManager::instance();

    QString requested;

    if (args.contains("-lang"))
        requested = args.at(args.indexOf("-lang") + 1);

    if (requested.isEmpty())
        requested = languages->savedPreference();

    if (requested.isEmpty())
        requested = PlatformUtils::systemLanguage();

    // 没有对应 .qm 时退回源语言（英文），而不是像以前那样声称在某个语言下运行
    // 却一个翻译器都没装。
    if (!languages->setLanguage(requested))
        languages->setLanguage(QStringLiteral("en"));

    qDebug() << "Running application in:" << languages->currentLanguage();
}

int Application::exec(const QString &pFileToImport) {
    QPixmapCache::setCacheLimit(1024 * 100);

    QString webDbPath = Settings::userDataDirectory() + "/web-databases";
    QDir webDbDir(webDbPath);
    if (!webDbDir.exists(webDbPath))
        webDbDir.mkpath(webDbPath);

    //QWebEngineSettings::setIconDatabasePath(webDbPath);
    //   QWebEngineSettings::setOfflineStoragePath (webDbPath);

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

    mainWindow->ui().actionCopy->setShortcuts(QKeySequence::Copy);
    mainWindow->ui().actionPaste->setShortcuts(QKeySequence::Paste);
    mainWindow->ui().actionCut->setShortcuts(QKeySequence::Cut);

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

    webController = new WebController(mainWindow);
    documentController = new DocumentController(mainWindow);

    DrawingController::drawingController()->setStylusTool((int)StylusTool::Pen);

    applicationController =
        new ApplicationController(boardController->controlView(), boardController->displayView(), mainWindow,
                                  staticMemoryCleaner, boardController->paletteManager()->rightPalette());


    connect(applicationController, &ApplicationController::mainModeChanged, boardController->paletteManager(),
            &BoardPaletteManager::slot_changeMainMode);

    connect(applicationController, &ApplicationController::desktopMode, boardController->paletteManager(),
            &BoardPaletteManager::slot_changeDesktopMode);

    connect(applicationController, &ApplicationController::mainModeChanged, boardController,
            &BoardController::appMainModeChanged);

    // 只有白板和网页模式真的实现了 剪切/复制/粘贴；文档库模式下这三个动作过去是
    // "看着能点、按了没反应"（DocumentController 里是三个空函数），现在直接灰掉。
    connect(applicationController, &ApplicationController::mainModeChanged, this,
            [this](ApplicationController::MainMode mode) {
                const bool hasClipboard =
                    (mode == ApplicationController::Board || mode == ApplicationController::Internet);
                mainWindow->ui().actionCut->setEnabled(hasClipboard);
                mainWindow->ui().actionCopy->setEnabled(hasClipboard);
                mainWindow->ui().actionPaste->setEnabled(hasClipboard);
            });

    connect(mainWindow->ui().actionDesktop, &QAction::triggered, applicationController,
            &ApplicationController::showDesktop);
    connect(mainWindow->ui().actionDesktop, &QAction::triggered, this, &Application::stopScript);
    connect(mainWindow->ui().actionRecording, &QAction::triggered, applicationController,
            &ApplicationController::toggleRecordingPalette);
#if defined(Q_OS_OSX) || defined(Q_OS_LINUX)
    connect(mainWindow->ui().actionHideApplication, &QAction::triggered, this, &Application::showMinimized);
#else
    connect(mainWindow->ui().actionHideApplication, &QAction::triggered, mainWindow,
            &MainWindow::showMinimized);
#endif

    mPreferencesController = new PreferencesController(mainWindow);

    connect(mainWindow->ui().actionPreferences, &QAction::triggered, mPreferencesController,
            &PreferencesController::show);


    toolBarPositionChanged(Settings::settings()->appToolBarPositionedAtTop->get());

    bool bUseMultiScreen = Settings::settings()->appUseMultiscreen->get().toBool();
    mainWindow->ui().actionMultiScreen->setChecked(bUseMultiScreen);
    connect(mainWindow->ui().actionMultiScreen, &QAction::triggered, applicationController,
            &ApplicationController::useMultiScreen);
    connect(mainWindow->ui().actionWidePageSize, &QAction::triggered, boardController,
            &BoardController::setWidePageSize);
    connect(mainWindow->ui().actionRegularPageSize, &QAction::triggered, boardController,
            &BoardController::setRegularPageSize);

    connect(mainWindow->ui().actionCut, &QAction::triggered, applicationController,
            &ApplicationController::actionCut);
    connect(mainWindow->ui().actionCopy, &QAction::triggered, applicationController,
            &ApplicationController::actionCopy);
    connect(mainWindow->ui().actionPaste, &QAction::triggered, applicationController,
            &ApplicationController::actionPaste);

    applicationController->initScreenLayout(bUseMultiScreen);
    boardController->setupLayout();

    if (pFileToImport.length() > 0)
        Application::applicationController->importFile(pFileToImport);

    if (Settings::settings()->appStartMode->get().toInt())
        applicationController->showDesktop();
    else
        applicationController->showBoard();

    emit DrawingController::drawingController() -> colorPaletteChanged();

    onScreenCountChanged(1);
    connect(VirtualDesktop::instance(), &VirtualDesktop::screenCountChanged, this,
            &Application::onScreenCountChanged);
    return QApplication::exec();
}

void Application::onScreenCountChanged(int newCount) {
    Q_UNUSED(newCount);
    DisplayManager displayManager;
    mainWindow->ui().actionMultiScreen->setEnabled(displayManager.numScreens() > 1);
}

void Application::showMinimized() {
#ifdef Q_OS_OSX
    mainWindow->hide();
    bIsMinimized = true;
#elif defined(Q_OS_LINUX)
    mainWindow->showMinimized();
    bIsMinimized = true;
#endif
}


void Application::startScript() {
    this->boardController->freezeWebWidgets(false);
}

void Application::stopScript() {
    this->boardController->freezeWebWidgets(true);
}

void Application::showBoard() {
    applicationController->showBoard();
}

void Application::showInternet() {
    applicationController->showInternet();
    webController->showTabAtTop(true);
}

void Application::showDocument() {
    applicationController->showDocument();
}

int Application::toolBarHeight() {
    return mainWindow->ui().boardToolBar->rect().height();
}


void Application::toolBarPositionChanged(QVariant topOrBottom) {
    Qt::ToolBarArea area;

    if (topOrBottom.toBool())
        area = Qt::TopToolBarArea;
    else
        area = Qt::BottomToolBarArea;

    mainWindow->addToolBar(area, mainWindow->ui().boardToolBar);
    mainWindow->addToolBar(area, mainWindow->ui().webToolBar);
    mainWindow->addToolBar(area, mainWindow->ui().documentToolBar);

    webController->showTabAtTop(topOrBottom.toBool());
}


void Application::toolBarDisplayTextChanged(QVariant display) {
    Qt::ToolButtonStyle toolButtonStyle =
        display.toBool() ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonIconOnly;
    mainWindow->ui().boardToolBar->setToolButtonStyle(toolButtonStyle);
    mainWindow->ui().webToolBar->setToolButtonStyle(toolButtonStyle);
    mainWindow->ui().documentToolBar->setToolButtonStyle(toolButtonStyle);
}


void Application::closeEvent(QCloseEvent *event) {
    Q_UNUSED(event);

    closing();
}

void Application::closing() {
    if (Settings::settings()->emptyTrashForOlderDocuments->get().toBool()) {
        DocumentTreeModel *docModel = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;
        documentController->deleteDocumentsInFolderOlderThan(
            docModel->trashIndex(), Settings::settings()->emptyTrashDaysValue->get().toInt());
        if (docModel->hasChildren(docModel->trashIndex()))
            documentController->deleteEmptyFolders(docModel->trashIndex());
    }

    if (boardController)
        boardController->closing();

    if (applicationController)
        applicationController->closing();

    if (webController)
        webController->closing();

    Settings::settings()->closing();

    Settings::settings()->appToolBarPositionedAtTop->set(
        mainWindow->toolBarArea(mainWindow->ui().boardToolBar) == Qt::TopToolBarArea);

    quit();
}


void Application::showMessage(const QString &message, bool showSpinningWheel) {
    if (applicationController)
        applicationController->showMessage(message, showSpinningWheel);
}


void Application::setDisabled(bool disable) {
    boardController->setDisabled(disable);
}


void Application::decorateActionMenu(QAction *action) {
    foreach (QWidget *menuWidget, action->associatedWidgets()) {
        QToolButton *tb = qobject_cast<QToolButton *>(menuWidget);

        if (tb && !tb->menu()) {
            tb->setObjectName("buttonMenu");
            tb->setPopupMode(QToolButton::InstantPopup);
            QMenu *menu = new QMenu(mainWindow);

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


void Application::updateProtoActionsState() {
    if (mainWindow) {
        mainWindow->ui().actionMultiScreen->setVisible(true);
    }

    foreach (QMenu *protoMenu, mProtoMenus)
        protoMenu->setVisible(true);
}


void Application::insertSpaceToToolbarBeforeAction(QToolBar *toolbar, QAction *action, int width) {
    QWidget *spacer = new QWidget();

    if (width >= 0) {
        QHBoxLayout *layout = new QHBoxLayout();
        layout->addSpacing(width);
        spacer->setLayout(layout);
    } else
        spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

    toolbar->insertWidget(action, spacer);
}


bool Application::eventFilter(QObject *obj, QEvent *event) {
    bool result = QObject::eventFilter(obj, event);

    if (event->type() == QEvent::FileOpen) {
        QFileOpenEvent *fileToOpenEvent = static_cast<QFileOpenEvent *>(event);

        PlatformUtils::setFrontProcess();

        applicationController->importFile(fileToOpenEvent->file());
    }

    if (event->type() == QEvent::TabletLeaveProximity) {
        if (boardController && boardController->controlView())
            boardController->controlView()->forcedTabletRelease();
    }


    if (event->type() == QEvent::ApplicationActivate) {
        if (boardController && boardController->controlView())
            boardController->controlView()->setMultiselection(false);

#if defined(Q_OS_OSX)
        if (bIsMinimized) {
            if (mainWindow->isHidden())
                mainWindow->show();
            bIsMinimized = false;
        }
#elif defined(Q_OS_LINUX)
        if (bIsMinimized) {
            bIsMinimized = false;
            PlatformUtils::showFullScreen(mainWindow);
        }
#endif
    }

    return result;
}


bool Application::handleOpenMessage(const QString &pMessage) {
    qDebug() << "received message" << pMessage;

    if (pMessage == Settings::appPingMessage) {
        qDebug() << "received ping";
        return true;
    }

    qDebug() << "importing file" << pMessage;

    Application::applicationController->importFile(pMessage);

    return true;
}

void Application::cleanup() {
    if (applicationController)
        delete applicationController;
    if (boardController)
        delete boardController;
    if (webController)
        delete webController;
    if (documentController)
        delete documentController;

    applicationController = NULL;
    boardController = NULL;
    webController = NULL;
    documentController = NULL;
}

QString Application::urlFromHtml(QString html) {
    QString _html;
    QRegularExpression comments("<[ \r\n\t]*(--([^\\-]|[\r\n]|-[^\\-])*--[ \r\n\t]*)>");
    QString url;
    QDomDocument domDoc;

    //    We remove all the comments & CRLF of this html
    _html = html.remove(comments);
    domDoc.setContent(_html.remove(QRegularExpression("[\\0]")));
    QDomElement rootElem = domDoc.documentElement();

    //  QUICKFIX: Here we have to check rootElem. Sometimes it can be a <meta> tag
    //  In such a case we will not be able to retrieve the src value
    if (rootElem.tagName().toLower().contains("meta")) {
        qDebug() << rootElem.firstChildElement().tagName();
        //  In that case we get the next element
        url = rootElem.firstChildElement().attribute("src");
    } else {
        url = rootElem.attribute("src");
    }

    return url;
}

bool Application::isFromWeb(QString url) {
    bool res = true;

    if (url.startsWith("tianboardtool://") || url.startsWith("file://") || url.startsWith("/")) {
        res = false;
    }

    return res;
}

QScreen *Application::controlScreen() {
    QList<QScreen *> screenList = screens();
    if (screenList.size() == 1)
        return screenList.first();

    return screenList[controlScreenIndex()];
}


int Application::controlScreenIndex() {
    return applicationController->displayManager()->controleScreenIndex();
}
