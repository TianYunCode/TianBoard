#include "core/ApplicationController.h"
#include <algorithm>
#include "platform/VirtualDesktop.h"

#include "platform/PlatformUtils.h"
#include "utils/FileSystemUtils.h"

#include "core/Application.h"
#include "core/PersistenceManager.h"
#include "settings/Settings.h"
#include "settings/Setting.h"
#include "core/DocumentManager.h"
#include "core/DisplayManager.h"
#include "core/DownloadManager.h"

#include "whiteboard/BoardView.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/BoardPaletteManager.h"
#include "whiteboard/DrawingController.h"

#include "model/DocumentProxy.h"
#include "ui/DocumentController.h"

#include "whiteboard/GraphicsWidgetItem.h"

#include "ui/ScreenMirror.h"
#include "ui/MainWindow.h"

#include "whiteboard/GraphicsPixmapItem.h"

#include "core/NetworkAccessManager.h"
#include "core/IDesktopAnnotation.h"
#include "core/IWebController.h"

namespace
{
    /// 录制显隐的转发实现；由装配点装上 recording 里的懒建入口。
    ApplicationController::RecordingToggle gRecordingToggle;

    /// 桌面批注层的工厂；由装配点装上 desktop 里的实现类。
    ApplicationController::DesktopAnnotationFactory gDesktopAnnotationFactory;
} // namespace

ApplicationController::ApplicationController(
    BoardView *pControlView, BoardView *pDisplayView, MainWindow *pMainWindow, QObject *parent, RightPalette *rightPalette)
    : QObject(parent), mDesktopAnnotation(0), mMainWindow(pMainWindow), mControlView(pControlView), mDisplayView(pDisplayView), mMirror(0),
      mMainMode(Board), mDisplayManager(0), mIsShowingDesktop(false)
{
    mDisplayManager = new DisplayManager(this);

    // 桌面批注是四个主模式之一，不是可选装饰：装配点没装上工厂就是装配漏了，立刻大声失败，
    // 别等到切进桌面模式时拿空指针去崩一处看不出根因的地方。
    if (!gDesktopAnnotationFactory)
    {
        qFatal("ApplicationController: 桌面批注层工厂没装上（应在 app/main.cpp 装）");
    }
    mDesktopAnnotation = gDesktopAnnotationFactory(this, rightPalette);

    connect(mDisplayManager, &DisplayManager::screenLayoutChanged, this, &ApplicationController::screenLayoutChanged);
    connect(mDisplayManager, &DisplayManager::screenLayoutChanged, mDesktopAnnotation, &IDesktopAnnotation::screenLayoutChanged);
    connect(mDisplayManager, &DisplayManager::screenLayoutChanged, Application::webController, &IWebController::screenLayoutChanged);
    connect(mDisplayManager, &DisplayManager::adjustDisplayViewsRequired, BoardController::instance(), &BoardController::adjustDisplayViews);
    // 黑屏解除后要把画板上的网页小部件解冻；多屏服务只回报“黑屏没了”，应答方由装配点连上。
    connect(mDisplayManager, &DisplayManager::blackoutLifted, BoardController::instance(), &BoardController::blackoutLifted);
    // 本地拖进来的文件在进下载队列前就要算出它在文档包里的落点，而“当前是哪份文档”只有画板知道。
    DownloadManager::installTargetDocumentProvider(
        []()
        {
            BoardController *board = BoardController::instance();
            return board ? board->selectedDocument() : nullptr;
        });
    // 槽第三个参数有默认值，成员指针形式要求参数个数不超过信号，这里显式补默认值
    connect(mDesktopAnnotation, &IDesktopAnnotation::imageCaptured, this,
        [this](const QPixmap &pixmap, bool pageMode)
        {
            addCapturedPixmap(pixmap, pageMode);
        });

    for (int i = 0; i < mDisplayManager->numPreviousViews(); i++)
    {
        BoardView *previousView = new BoardView(BoardController::instance(), ItemLayerType::FixedBackground, ItemLayerType::Tool, 0);
        previousView->setInteractive(false);
        mPreviousViews.append(previousView);
    }

    mBlackScene = new GraphicsScene(0); // 由 ApplicationController 析构函数删除
    mBlackScene->setBackground(true, PageBackground::plain);

    if (mDisplayManager->numScreens() >= 2 && mDisplayManager->useMultiScreen())
    {
        mMirror = new ScreenMirror();
    }

    connect(Application::webController, &IWebController::imageCaptured, this, &ApplicationController::addCapturedPixmap);
}

ApplicationController::~ApplicationController()
{
    foreach (BoardView *view, mPreviousViews)
    {
        delete view;
    }

    delete mBlackScene;
    delete mMirror;
}

void ApplicationController::initViewState(int horizontalPosition, int verticalPostition)
{
    mInitialHScroll = horizontalPosition;
    mInitialVScroll = verticalPostition;
}

void ApplicationController::initScreenLayout(bool useMultiscreen)
{
    mDisplayManager->setControlWidget(mMainWindow);
    mDisplayManager->setDisplayWidget(mDisplayView);

    // 多屏服务只用到窗口的几何与全屏能力，不认画板视图：装配点按视图顺序交一份 QWidget 列表。
    QList<QWidget *> previousWidgets;
    for (BoardView *view : mPreviousViews)
    {
        previousWidgets.append(view);
    }
    mDisplayManager->setPreviousDisplaysWidgets(previousWidgets);
    mDisplayManager->setDesktopWidget(mDesktopAnnotation->drawingView());

    mDisplayManager->setUseMultiScreen(useMultiscreen);
    mDisplayManager->adjustScreens(-1);
}

void ApplicationController::screenLayoutChanged()
{
    initViewState(mControlView->horizontalScrollBar()->value(), mControlView->verticalScrollBar()->value());

    adaptToolBar();

    adjustDisplayView();

    if (mDisplayManager->hasDisplay())
    {
        BoardController::instance()->setBoxing(mDisplayView->geometry());
    }
    else
    {
        BoardController::instance()->setBoxing(QRect());
    }

    adjustPreviousViews(0, 0);
}

void ApplicationController::adaptToolBar()
{
    bool highResolution = mMainWindow->width() > 1024;

    mMainWindow->ui().actionClearPage->setVisible(Board == mMainMode && highResolution);
    mMainWindow->ui().actionBoard->setVisible(Board != mMainMode || highResolution);
    mMainWindow->ui().actionDocument->setVisible(Document != mMainMode || highResolution);
    mMainWindow->ui().actionWeb->setVisible(Internet != mMainMode || highResolution);
    mMainWindow->ui().boardToolBar->setIconSize(QSize(highResolution ? 48 : 42, mMainWindow->ui().boardToolBar->iconSize().height()));

    mMainWindow->ui().actionBoard->setEnabled(mMainMode != Board);
    mMainWindow->ui().actionWeb->setEnabled(mMainMode != Internet);
    mMainWindow->ui().actionDocument->setEnabled(mMainMode != Document);

    if (Document == mMainMode)
    {
        // adaptToolBar() 不只在做模式切换时被调（screenLayoutChanged 插拔显示器也会），
        // 已经在文档模式时再进来一次就会多连一条，focusChanged 一次触发多次。
        connect(qApp, &QApplication::focusChanged, Application::documentController, &DocumentController::focusChanged, Qt::UniqueConnection);
    }
    else
    {
        disconnect(qApp, &QApplication::focusChanged, Application::documentController, &DocumentController::focusChanged);
        if (Board == mMainMode)
        {
            mMainWindow->ui().actionDuplicate->setEnabled(true);
        }
    }

    BoardController::instance()->setToolbarTexts();

    Application::webController->adaptToolBar();
}

void ApplicationController::adjustDisplayView()
{
    if (mDisplayView)
    {
        qreal systemDisplayViewScaleFactor = 1.0;

        QSize pageSize                     = BoardController::instance()->activeScene()->nominalSize();
        QSize displaySize                  = mDisplayView->size();

        qreal hFactor                      = ((qreal)displaySize.width()) / ((qreal)pageSize.width());
        qreal vFactor                      = ((qreal)displaySize.height()) / ((qreal)pageSize.height());

        systemDisplayViewScaleFactor       = qMin(hFactor, vFactor);

        QTransform tr;
        qreal      scaleFactor = systemDisplayViewScaleFactor * BoardController::instance()->currentZoom();

        tr.scale(scaleFactor, scaleFactor);

        QRect  rect = mControlView->rect();
        QPoint center(rect.x() + rect.width() / 2, rect.y() + rect.height() / 2);

        QTransform recentTransform = mDisplayView->transform();

        if (recentTransform != tr)
        {
            mDisplayView->setTransform(tr);
        }

        mDisplayView->centerOn(mControlView->mapToScene(center));
    }
}

void ApplicationController::adjustPreviousViews(int pActiveSceneIndex, DocumentProxy *pActiveDocument)
{
    int viewIndex = pActiveSceneIndex;

    foreach (BoardView *previousView, mPreviousViews)
    {
        if (viewIndex > 0)
        {
            viewIndex--;

            GraphicsScene *scene = PersistenceManager::persistenceManager()->loadDocumentScene(pActiveDocument, viewIndex);

            if (scene)
            {
                previousView->setScene(scene);

                qreal  ratio      = ((qreal)previousView->geometry().width()) / ((qreal)previousView->geometry().height());
                QRectF sceneRect  = scene->normalizedSceneRect(ratio);
                qreal  scaleRatio = previousView->geometry().width() / sceneRect.width();

                previousView->resetTransform();

                previousView->scale(scaleRatio, scaleRatio);

                previousView->centerOn(sceneRect.center());
            }
        }
        else
        {
            previousView->setScene(mBlackScene);
        }
    }
}

void ApplicationController::blackout()
{
    mDisplayManager->blackout();
}

void ApplicationController::addCapturedPixmap(const QPixmap &pPixmap, bool pageMode, const QUrl &sourceUrl)
{
    if (!pPixmap.isNull())
    {
        qreal sf              = BoardController::instance()->systemScaleFactor();
        qreal scaledWidth     = ((qreal)pPixmap.width()) / sf;
        qreal scaledHeight    = ((qreal)pPixmap.height()) / sf;

        QSize pageNominalSize = BoardController::instance()->activeScene()->nominalSize();

        int newWidth          = qMin((int)scaledWidth, pageNominalSize.width());
        int newHeight         = qMin((int)scaledHeight, pageNominalSize.height());

        if (pageMode)
        {
            newHeight = pPixmap.height();
        }

        QSizeF scaledSize(scaledWidth, scaledHeight);
        scaledSize.scale(newWidth, newHeight, Qt::KeepAspectRatio);

        qreal scaleFactor = qMin(scaledSize.width() / (qreal)pPixmap.width(), scaledSize.height() / (qreal)pPixmap.height());

        QPointF pos(0.0, 0.0);

        if (pageMode)
        {
            pos.setY(pageNominalSize.height() / -2 + scaledSize.height() / 2);
        }

        BoardController::instance()->paletteManager()->addItem(pPixmap, pos, scaleFactor, sourceUrl);
    }
}

void ApplicationController::addCapturedEmbedCode(const QString &embedCode)
{
    if (!embedCode.isEmpty())
    {
        showBoard();

        const QString userWidgetPath = Settings::settings()->userInteractiveDirectory() + "/" + tr("Web");
        QDir          userWidgetDir(userWidgetPath);

        int width          = 300;
        int height         = 150;

        QString widgetPath = GraphicsWebWidgetItem::createHtmlWrapperInDir(
            embedCode, userWidgetDir, QSize(width, height), StringUtils::toCanonicalUuid(QUuid::createUuid()));

        if (widgetPath.length() > 0)
        {
            BoardController::instance()->downloadURL(QUrl::fromLocalFile(widgetPath));
        }
    }
}

void ApplicationController::showBoard()
{
    mMainWindow->ui().webToolBar->hide();
    mMainWindow->ui().documentToolBar->hide();
    mMainWindow->ui().boardToolBar->show();

    if (mMainMode == Document)
    {
        int selectedSceneIndex = Application::documentController->getSelectedItemIndex();
        if (selectedSceneIndex != -1)
        {
            BoardController::instance()->setActiveDocumentScene(Application::documentController->selectedDocument(), selectedSceneIndex, true);
        }
    }

    mMainMode = Board;

    adaptToolBar();

    mirroringEnabled(false);

    mMainWindow->switchToBoardWidget();

    if (BoardController::instance())
    {
        BoardController::instance()->show();
    }

    mIsShowingDesktop = false;
    PlatformUtils::setDesktopMode(false);

    mDesktopAnnotation->hideWindow();

    PlatformUtils::showFullScreen(mMainWindow);

    emit mainModeChanged(Board);

    BoardController::instance()->freezeWebWidgets(false);
}

void ApplicationController::showInternet()
{
    if (BoardController::instance())
    {
        BoardController::instance()->persistCurrentScene();
        BoardController::instance()->hide();
    }

    if (Settings::settings()->webUseExternalBrowser->get().toBool())
    {
        showDesktop(true);
        Application::webController->show();
    }
    else
    {
        mMainWindow->ui().boardToolBar->hide();
        mMainWindow->ui().documentToolBar->hide();
        mMainWindow->ui().webToolBar->show();

        mMainMode = Internet;

        adaptToolBar();

        mMainWindow->show();
        mDesktopAnnotation->hideWindow();

        Application::webController->show();

        emit mainModeChanged(Internet);
    }
}

void ApplicationController::openInternetUrl(const QUrl &url)
{
    showInternet();
    Application::webController->loadUrl(url);
}

void ApplicationController::showDocument()
{
    mMainWindow->ui().webToolBar->hide();
    mMainWindow->ui().boardToolBar->hide();
    mMainWindow->ui().documentToolBar->show();

    mMainMode = Document;

    adaptToolBar();

    mirroringEnabled(false);

    mMainWindow->switchToDocumentsWidget();

    if (BoardController::instance())
    {
        if (BoardController::instance()->activeScene()->isModified())
        {
            BoardController::instance()->persistCurrentScene();
        }
        BoardController::instance()->hide();
    }

    if (Application::documentController)
    {
        emit Application::documentController->reorderDocumentsRequested();
        Application::documentController->show();
    }

    mMainWindow->show();

    mDesktopAnnotation->hideWindow();

    emit mainModeChanged(Document);
}

void ApplicationController::showDesktop(bool dontSwitchFrontProcess)
{
    int desktopWidgetIndex = VirtualDesktop::instance()->screenNumber(mMainWindow);

    if (BoardController::instance())
    {
        BoardController::instance()->hide();
    }

    mMainWindow->hide();
    mDesktopAnnotation->showWindow();

    if (mMirror)
    {
        QRect rect = VirtualDesktop::instance()->screenGeometry(desktopWidgetIndex);
        mMirror->setSourceRect(rect);
    }

    mIsShowingDesktop = true;
    emit desktopMode(true);

    if (!dontSwitchFrontProcess)
    {
        PlatformUtils::bringPreviousProcessToFront();
    }

    DrawingController::drawingController()->setInDestopMode(true);
    DrawingController::drawingController()->setStylusTool(StylusTool::Selector);
}

void ApplicationController::hideDesktop()
{
    if (mMainMode == Board)
    {
        showBoard();
    }
    else if (mMainMode == Internet)
    {
        showInternet();
    }
    else if (mMainMode == Document)
    {
        showDocument();
    }

    mIsShowingDesktop = false;

    mDisplayManager->adjustScreens(-1);

    emit desktopMode(false);
}

// 白板工具栏上的"录制"按钮过去由 BoardPaletteManager 代为转发，画板模块因此得
// include RecordingController；转发先挪到这里。这里也不再认识 recording：显隐动作
// 改走装配点装上的钩子（app/main.cpp），并且仍然只在第一次按下时才创建录制控制器
// ——instance() 是懒建的，创建发生在钩子里。
void ApplicationController::installRecordingToggle(RecordingToggle toggle)
{
    gRecordingToggle = std::move(toggle);
}

void ApplicationController::installDesktopAnnotationFactory(DesktopAnnotationFactory factory)
{
    gDesktopAnnotationFactory = std::move(factory);
}

void ApplicationController::toggleRecordingPalette(bool visible)
{
    if (!gRecordingToggle)
    {
        qWarning() << "ApplicationController: 未装上录制显隐钩子，按钮点了没反应";
        return;
    }
    gRecordingToggle(visible);
}

QWidget *ApplicationController::desktopDrawingHost()
{
    return mDesktopAnnotation->drawingView();
}

void ApplicationController::desktopDrawingResized()
{
    mDesktopAnnotation->relayout();
}

void ApplicationController::setMirrorSourceWidget(QWidget *pWidget)
{
    if (mMirror)
    {
        mMirror->setSourceWidget(pWidget);
    }
}

void ApplicationController::mirroringEnabled(bool enabled)
{
    if (mMirror)
    {
        if (enabled)
        {
            mMirror->start();
            mDisplayManager->setDisplayWidget(mMirror);
        }
        else
        {
            mDisplayManager->setDisplayWidget(mDisplayView);
            mMirror->stop();
        }

        mMirror->setVisible(enabled && mDisplayManager->numScreens() > 1);
        mDesktopAnnotation->updateShowHideState(enabled);
        Application::mainWindow->ui().actionWebShowHideOnDisplay->setChecked(enabled);
    }
    else
    {
        mDisplayManager->setDisplayWidget(mDisplayView);
    }
}

void ApplicationController::closing()
{
    if (mMirror)
    {
        mMirror->stop();
    }

    if (mDesktopAnnotation)
    {
        mDesktopAnnotation->hideWindow();
        mDesktopAnnotation->closeWindow();
    }

    // 退出前把文档目录树写回 documents/folders.xml。
    PersistenceManager::persistenceManager()->closing();
}

void ApplicationController::showMessage(const QString &message, bool showSpinningWheel)
{
    if (!Application::closingDown())
    {
        if (mMainMode == Document)
        {
            BoardController::instance()->hideMessage();
            Application::documentController->showMessage(message, showSpinningWheel);
        }
        else
        {
            Application::documentController->hideMessage();
            BoardController::instance()->showMessage(message, showSpinningWheel);
        }
    }
}

void ApplicationController::importFile(const QString &pFilePath)
{
    const QFile fileToOpen(pFilePath);

    if (!fileToOpen.exists())
    {
        return;
    }

    DocumentProxy *document = 0;

    bool success            = false;

    document                = DocumentManager::documentManager()->importFile(fileToOpen, "");

    success                 = (document != 0);

    if (success && document)
    {
        if (mMainMode == Board || mMainMode == Internet)
        {
            if (BoardController::instance())
            {
                BoardController::instance()->setActiveDocumentScene(document, 0);
                showBoard();
            }
        }
        else if (mMainMode == Document)
        {
            if (Application::documentController)
            {
                Application::documentController->selectDocument(document);
            }
        }
    }
}

void ApplicationController::useMultiScreen(bool use)
{
    if (use && !mMirror)
    {
        mMirror = new ScreenMirror();
    }
    if (!use && mMirror)
    {
        // 先把 DisplayManager 的显示控件换回 mDisplayView 再删：它保存的是裸指针，
        // 直接 delete 之后 adjustScreens()->positionScreens() 会对已释放的 mMirror 调 hide()。
        mirroringEnabled(false);
        delete mMirror;
        mMirror = NULL;
    }

    mDisplayManager->setUseMultiScreen(use);
    mDisplayManager->adjustScreens(0);
    Settings::settings()->appUseMultiscreen->set(use);
}

void ApplicationController::actionCut()
{
    if (!Application::closingDown())
    {
        if (mMainMode == Board)
        {
            BoardController::instance()->cut();
        }
        else if (mMainMode == Internet)
        {
            Application::webController->cut();
        }
    }
}

void ApplicationController::actionCopy()
{
    if (!Application::closingDown())
    {
        if (mMainMode == Board)
        {
            BoardController::instance()->copy();
        }
        else if (mMainMode == Internet)
        {
            Application::webController->copy();
        }
    }
}

void ApplicationController::actionPaste()
{
    if (!Application::closingDown())
    {
        if (mMainMode == Board)
        {
            BoardController::instance()->paste();
        }
        else if (mMainMode == Internet)
        {
            Application::webController->paste();
        }
    }
}
