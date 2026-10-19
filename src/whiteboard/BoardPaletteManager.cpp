#include "whiteboard/BoardPaletteManager.h"

#include "platform/PlatformUtils.h"
#include "utils/FileSystemUtils.h"

#include "core/Application.h"
#include "core/ApplicationController.h"
#include "settings/Settings.h"
#include "settings/Setting.h"
#include "core/DisplayManager.h"

#include "ui/MainWindow.h"
#include "ui/StylusPalette.h"
#include "platform/KeyboardPalette.h"
#include "ui/ToolWidget.h"
#include "ui/ZoomPalette.h"
#include "ui/WebToolsPalette.h"
#include "ui/ActionPalette.h"
#include "ui/BackgroundPalette.h"
#include "ui/FavoriteToolPalette.h"

#include "web/WebController.h"
#include "web/BrowserWindow.h"
#include "web/TabWidget.h"
#include "web/WebView.h"

#include "desktop/DesktopAnnotationController.h"


#include "core/NetworkAccessManager.h"

#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsPixmapItem.h"

#include "model/DocumentProxy.h"
#include "recording/RecordingController.h"
#include "whiteboard/DrawingController.h"

#include "whiteboard/ToolsManager.h"

#include "whiteboard/BoardController.h"

#include "model/DocumentController.h"

#include "core/PersistenceManager.h"

BoardPaletteManager::BoardPaletteManager(QWidget *container, BoardController *pBoardController)
    : QObject(container),
      mKeyboardPalette(0),
      mWebToolsCurrentPalette(0),
      mContainer(container),
      mBoardControler(pBoardController),
      mStylusPalette(0),
      mZoomPalette(0),
      mLeftPalette(NULL),
      mRightPalette(NULL),
      mBackgroundsPalette(0),
      mToolsPalette(0),
      mAddItemPalette(0),
      mErasePalette(NULL),
      mPagePalette(NULL),
      mPendingPageButtonPressed(false),
      mPendingZoomButtonPressed(false),
      mPendingPanButtonPressed(false),
      mPendingEraseButtonPressed(false),
      mpPageNavigWidget(NULL),
      mpCachePropWidget(NULL),
      mpDownloadWidget(NULL),
      mDownloadInProgress(false) {
    setupPalettes();
    connectPalettes();
}


BoardPaletteManager::~BoardPaletteManager() {}

void BoardPaletteManager::initPalettesPosAtStartup() {
    mStylusPalette->initPosition();
}

void BoardPaletteManager::setupLayout() {}

/**
 * \brief Set up the dock palette widgets
 */
void BoardPaletteManager::setupDockPaletteWidgets() {
    // Create the widgets for the dock palettes
    mpPageNavigWidget = new PageNavigationWidget();

    mpCachePropWidget = new CachePropertiesWidget();

    mpDownloadWidget = new DockDownloadWidget();
    // Add the dock palettes
    mLeftPalette = new LeftPalette(mContainer);

    // LEFT palette widgets
    mLeftPalette->registerWidget(mpPageNavigWidget);
    mLeftPalette->addTab(mpPageNavigWidget);

    mLeftPalette->connectSignals();
    mLeftPalette->showTabWidget(0);

    mRightPalette = new RightPalette(mContainer);
    // RIGHT palette widgets
    mpFeaturesWidget = new FeaturesWidget();
    mRightPalette->registerWidget(mpFeaturesWidget);
    mRightPalette->addTab(mpFeaturesWidget);

    // The cache widget will be visible only if a cache is put on the page
    mRightPalette->registerWidget(mpCachePropWidget);

    //  The download widget will be part of the right palette but
    //  will become visible only when the first download starts
    mRightPalette->registerWidget(mpDownloadWidget);
    mRightPalette->connectSignals();
    changeMode(DockWidgetMode::Board, true);

    // Hide the tabs that must be hidden
    mRightPalette->removeTab(mpDownloadWidget);
    mRightPalette->removeTab(mpCachePropWidget);
}

void BoardPaletteManager::slot_changeMainMode(ApplicationController::MainMode mainMode) {
    //    Board = 0, Internet, Document, Tutorial, WebDocument

    switch (mainMode) {
        case ApplicationController::Board: {
            // call changeMode only when switch NOT from desktop mode
            if (!Application::applicationController->isShowingDesktop())
                changeMode(DockWidgetMode::Board);
        } break;
        case ApplicationController::Internet:
            changeMode(DockWidgetMode::Web);
            break;

        case ApplicationController::Document:
            changeMode(DockWidgetMode::Document);
            break;

        default: {
            if (PlatformUtils::hasVirtualKeyboard() && mKeyboardPalette != NULL)
                mKeyboardPalette->hide();
        } break;
    }
}

void BoardPaletteManager::slot_changeDesktopMode(bool isDesktop) {
    ApplicationController::MainMode currMode = Application::applicationController->displayMode();
    if (!isDesktop) {
        switch (currMode) {
            case ApplicationController::Board:
                changeMode(DockWidgetMode::Board);
                break;

            default:
                break;
        }
    } else
        changeMode(DockWidgetMode::Desktop);
}

void BoardPaletteManager::setupPalettes() {
    if (PlatformUtils::hasVirtualKeyboard()) {
        mKeyboardPalette = new KeyboardPalette(0);
#ifndef Q_OS_WIN
        connect(mKeyboardPalette, &KeyboardPalette::closed, mKeyboardPalette,
                &KeyboardPalette::onDeactivated);
#endif
    }

    setupDockPaletteWidgets();

    // Add the other palettes
    mStylusPalette = new StylusPalette(
        mContainer,
        Settings::settings()->appToolBarOrientationVertical->get().toBool() ? Qt::Vertical : Qt::Horizontal);
    connect(mStylusPalette, qOverload<int>(&StylusPalette::stylusToolDoubleClicked),
            Application::boardController, &BoardController::stylusToolDoubleClicked, Qt::UniqueConnection);
    mStylusPalette->show(); // always show stylus palette at startup

    mZoomPalette = new ZoomPalette(mContainer);

    mStylusPalette->stackUnder(mZoomPalette);

    QList<QAction *> backgroundsActions;

    backgroundsActions << Application::mainWindow->ui().actionPlainLightBackground;
    backgroundsActions << Application::mainWindow->ui().actionCrossedLightBackground;
    backgroundsActions << Application::mainWindow->ui().actionRuledLightBackground;
    backgroundsActions << Application::mainWindow->ui().actionPlainDarkBackground;
    backgroundsActions << Application::mainWindow->ui().actionCrossedDarkBackground;
    backgroundsActions << Application::mainWindow->ui().actionRuledDarkBackground;

    mBackgroundsPalette = new BackgroundPalette(backgroundsActions, mContainer);
    mBackgroundsPalette->setButtonIconSize(QSize(128, 128));
    mBackgroundsPalette->groupActions();
    mBackgroundsPalette->setClosable(true);
    mBackgroundsPalette->setAutoClose(false);
    mBackgroundsPalette->adjustSizeAndPosition();
    mBackgroundsPalette->hide();

    QList<QAction *> addItemActions;

    addItemActions << Application::mainWindow->ui().actionAddItemToCurrentPage;
    addItemActions << Application::mainWindow->ui().actionAddItemToNewPage;
    addItemActions << Application::mainWindow->ui().actionAddItemToLibrary;

    mAddItemPalette = new ActionPalette(addItemActions, Qt::Horizontal, mContainer);
    mAddItemPalette->setButtonIconSize(QSize(128, 128));
    mAddItemPalette->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mAddItemPalette->groupActions();
    mAddItemPalette->setClosable(true);
    mAddItemPalette->adjustSizeAndPosition();
    mAddItemPalette->hide();

    QList<QAction *> eraseActions;

    eraseActions << Application::mainWindow->ui().actionEraseAnnotations;
    eraseActions << Application::mainWindow->ui().actionEraseItems;
    eraseActions << Application::mainWindow->ui().actionClearPage;
    eraseActions << Application::mainWindow->ui().actionEraseBackground;

    mErasePalette = new ActionPalette(eraseActions, Qt::Horizontal, mContainer);
    mErasePalette->setButtonIconSize(QSize(128, 128));
    mErasePalette->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mErasePalette->groupActions();
    mErasePalette->setClosable(true);
    mErasePalette->adjustSizeAndPosition();
    mErasePalette->hide();

    QList<QAction *> pageActions;

    pageActions << Application::mainWindow->ui().actionNewPage;
    pageActions << Application::mainWindow->ui().actionDuplicatePage;
    pageActions << Application::mainWindow->ui().actionImportPage;

    mPagePalette = new ActionPalette(pageActions, Qt::Horizontal, mContainer);
    mPagePalette->setButtonIconSize(QSize(128, 128));
    mPagePalette->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mPagePalette->groupActions();
    mPagePalette->setClosable(true);
    mPagePalette->adjustSizeAndPosition();
    mPagePalette->hide();

    connect(Settings::settings()->appToolBarOrientationVertical, &Setting::changed, this,
            &BoardPaletteManager::changeStylusPaletteOrientation);
}

void BoardPaletteManager::pagePaletteButtonPressed() {
    mPageButtonPressedTime = QTime::currentTime();

    mPendingPageButtonPressed = true;
    QTimer::singleShot(1000, this, &BoardPaletteManager::pagePaletteButtonReleased);
}


void BoardPaletteManager::pagePaletteButtonReleased() {
    if (mPendingPageButtonPressed) {
        if (mPageButtonPressedTime.msecsTo(QTime::currentTime()) > 900) {
            // The palette is reinstanciated because the duplication depends on the current scene
            delete (mPagePalette);
            mPagePalette = 0;
            QList<QAction *> pageActions;
            pageActions << Application::mainWindow->ui().actionNewPage;
            pageActions << Application::mainWindow->ui().actionDuplicatePage;
            pageActions << Application::mainWindow->ui().actionImportPage;

            mPagePalette = new ActionPalette(pageActions, Qt::Horizontal, mContainer);
            mPagePalette->setButtonIconSize(QSize(128, 128));
            mPagePalette->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
            mPagePalette->groupActions();
            mPagePalette->setClosable(true);

            // As we recreate the pagePalette every time, we must reconnect the slots
            connect(Application::mainWindow->ui().actionNewPage, &QAction::triggered, mPagePalette,
                    &ActionPalette::close);
            connect(Application::mainWindow->ui().actionDuplicatePage, &QAction::triggered, mPagePalette,
                    &ActionPalette::close);
            connect(Application::mainWindow->ui().actionImportPage, &QAction::triggered, mPagePalette,
                    &ActionPalette::close);
            connect(mPagePalette, &ActionPalette::closed, this, &BoardPaletteManager::pagePaletteClosed);

            togglePagePalette(true);
        } else {
            Application::mainWindow->ui().actionNewPage->trigger();
        }

        mPendingPageButtonPressed = false;
    }
}

void BoardPaletteManager::erasePaletteButtonPressed() {
    mEraseButtonPressedTime = QTime::currentTime();

    mPendingEraseButtonPressed = true;
    QTimer::singleShot(1000, this, &BoardPaletteManager::erasePaletteButtonReleased);
}


void BoardPaletteManager::erasePaletteButtonReleased() {
    if (mPendingEraseButtonPressed) {
        if (mEraseButtonPressedTime.msecsTo(QTime::currentTime()) > 900) {
            toggleErasePalette(true);
        } else {
            Application::mainWindow->ui().actionClearPage->trigger();
        }

        mPendingEraseButtonPressed = false;
    }
}

void BoardPaletteManager::linkClicked(const QUrl &url) {
    Application::applicationController->showInternet();
    Application::webController->loadUrl(url);
}

void BoardPaletteManager::purchaseLinkActivated(const QString &link) {
    Application::applicationController->showInternet();
    Application::webController->loadUrl(QUrl(link));
}

void BoardPaletteManager::connectPalettes() {
    connect(Application::mainWindow->ui().actionStylus, &QAction::toggled, this,
            &BoardPaletteManager::toggleStylusPalette);

    foreach (QWidget *widget, Application::mainWindow->ui().actionZoomIn->associatedWidgets()) {
        QAbstractButton *button = qobject_cast<QAbstractButton *>(widget);
        if (button) {
            connect(button, &QAbstractButton::pressed, this, &BoardPaletteManager::zoomButtonPressed);
            connect(button, &QAbstractButton::released, this, &BoardPaletteManager::zoomButtonReleased);
        }
    }

    foreach (QWidget *widget, Application::mainWindow->ui().actionZoomOut->associatedWidgets()) {
        QAbstractButton *button = qobject_cast<QAbstractButton *>(widget);
        if (button) {
            connect(button, &QAbstractButton::pressed, this, &BoardPaletteManager::zoomButtonPressed);
            connect(button, &QAbstractButton::released, this, &BoardPaletteManager::zoomButtonReleased);
        }
    }

    foreach (QWidget *widget, Application::mainWindow->ui().actionHand->associatedWidgets()) {
        QAbstractButton *button = qobject_cast<QAbstractButton *>(widget);
        if (button) {
            connect(button, &QAbstractButton::pressed, this, &BoardPaletteManager::panButtonPressed);
            connect(button, &QAbstractButton::released, this, &BoardPaletteManager::panButtonReleased);
        }
    }

    connect(Application::mainWindow->ui().actionBackgrounds, &QAction::toggled, this,
            &BoardPaletteManager::toggleBackgroundPalette);
    connect(mBackgroundsPalette, &ActionPalette::closed, this, &BoardPaletteManager::backgroundPaletteClosed);

    connect(Application::mainWindow->ui().actionPlainLightBackground, &QAction::triggered, this,
            &BoardPaletteManager::changeBackground);
    connect(Application::mainWindow->ui().actionCrossedLightBackground, &QAction::triggered, this,
            &BoardPaletteManager::changeBackground);
    connect(Application::mainWindow->ui().actionRuledLightBackground, &QAction::triggered, this,
            &BoardPaletteManager::changeBackground);
    connect(Application::mainWindow->ui().actionPlainDarkBackground, &QAction::triggered, this,
            &BoardPaletteManager::changeBackground);
    connect(Application::mainWindow->ui().actionCrossedDarkBackground, &QAction::triggered, this,
            &BoardPaletteManager::changeBackground);
    connect(Application::mainWindow->ui().actionRuledDarkBackground, &QAction::triggered, this,
            &BoardPaletteManager::changeBackground);
    connect(Application::mainWindow->ui().actionRecording, &QAction::triggered, this,
            &BoardPaletteManager::toggleRecordingPalette);

    connect(Application::mainWindow->ui().actionAddItemToCurrentPage, &QAction::triggered, this,
            &BoardPaletteManager::addItemToCurrentPage);
    connect(Application::mainWindow->ui().actionAddItemToNewPage, &QAction::triggered, this,
            &BoardPaletteManager::addItemToNewPage);
    connect(Application::mainWindow->ui().actionAddItemToLibrary, &QAction::triggered, this,
            &BoardPaletteManager::addItemToLibrary);

    connect(Application::mainWindow->ui().actionEraseItems, &QAction::triggered, mErasePalette,
            &ActionPalette::close);
    connect(Application::mainWindow->ui().actionEraseAnnotations, &QAction::triggered, mErasePalette,
            &ActionPalette::close);
    connect(Application::mainWindow->ui().actionClearPage, &QAction::triggered, mErasePalette,
            &ActionPalette::close);
    connect(Application::mainWindow->ui().actionEraseBackground, &QAction::triggered, mErasePalette,
            &ActionPalette::close);
    connect(mErasePalette, &ActionPalette::closed, this, &BoardPaletteManager::erasePaletteClosed);

    foreach (QWidget *widget, Application::mainWindow->ui().actionErase->associatedWidgets()) {
        QAbstractButton *button = qobject_cast<QAbstractButton *>(widget);
        if (button) {
            connect(button, &QAbstractButton::pressed, this, &BoardPaletteManager::erasePaletteButtonPressed);
            connect(button, &QAbstractButton::released, this,
                    &BoardPaletteManager::erasePaletteButtonReleased);
        }
    }

    connect(Application::mainWindow->ui().actionNewPage, &QAction::triggered, mPagePalette,
            &ActionPalette::close);
    connect(Application::mainWindow->ui().actionDuplicatePage, &QAction::triggered, mPagePalette,
            &ActionPalette::close);
    connect(Application::mainWindow->ui().actionImportPage, &QAction::triggered, mPagePalette,
            &ActionPalette::close);
    connect(mPagePalette, &ActionPalette::closed, this, &BoardPaletteManager::pagePaletteClosed);

    foreach (QWidget *widget, Application::mainWindow->ui().actionPages->associatedWidgets()) {
        QAbstractButton *button = qobject_cast<QAbstractButton *>(widget);
        if (button) {
            connect(button, &QAbstractButton::pressed, this, &BoardPaletteManager::pagePaletteButtonPressed);
            connect(button, &QAbstractButton::released, this,
                    &BoardPaletteManager::pagePaletteButtonReleased);
        }
    }
}


bool isFirstResized = true;
void BoardPaletteManager::containerResized() {
    int innerMargin = Settings::boardMargin;

    int userLeft = innerMargin;
    int userWidth = mContainer->width() - (2 * innerMargin);
    int userTop = innerMargin;
    int userHeight = mContainer->height() - (2 * innerMargin);

    if (mStylusPalette) {
        mStylusPalette->move(userLeft, userTop);
        mStylusPalette->adjustSizeAndPosition();
        mStylusPalette->initPosition();
    }

    if (mZoomPalette) {
        mZoomPalette->move(userLeft + userWidth - mZoomPalette->width(),
                           userTop + userHeight /*- mPageNumberPalette->height()*/ - innerMargin -
                               mZoomPalette->height());
        mZoomPalette->adjustSizeAndPosition();
        mZoomPalette->refreshPalette();
    }

    if (isFirstResized && mKeyboardPalette &&
        mKeyboardPalette->parent() == Application::boardController->controlContainer()) {
        isFirstResized = false;
        mKeyboardPalette->move(userLeft + (userWidth - mKeyboardPalette->width()) / 2,
                               userTop + (userHeight - mKeyboardPalette->height()) / 2);
        mKeyboardPalette->adjustSizeAndPosition();
    }

    if (mLeftPalette) {
        mLeftPalette->resize(mLeftPalette->width(), mContainer->height());
        mLeftPalette->resize(mLeftPalette->width(), mContainer->height());
    }

    if (mRightPalette) {
        mRightPalette->resize(mRightPalette->width(), mContainer->height());
        mRightPalette->resize(mRightPalette->width(), mContainer->height());
    }
}


void BoardPaletteManager::changeBackground() {
    if (Application::mainWindow->ui().actionCrossedLightBackground->isChecked())
        Application::boardController->changeBackground(false, PageBackground::crossed);

    else if (Application::mainWindow->ui().actionRuledLightBackground->isChecked())
        Application::boardController->changeBackground(false, PageBackground::ruled);

    else if (Application::mainWindow->ui().actionPlainDarkBackground->isChecked())
        Application::boardController->changeBackground(true, PageBackground::plain);

    else if (Application::mainWindow->ui().actionCrossedDarkBackground->isChecked())
        Application::boardController->changeBackground(true, PageBackground::crossed);

    else if (Application::mainWindow->ui().actionRuledDarkBackground->isChecked())
        Application::boardController->changeBackground(true, PageBackground::ruled);

    else
        Application::boardController->changeBackground(false, PageBackground::plain);

    mBackgroundsPalette->backgroundChanged();
}


void BoardPaletteManager::activeSceneChanged() {
    GraphicsScene *activeScene = Application::boardController->activeScene();
    int pageIndex = Application::boardController->activeSceneIndex();

    if (mStylusPalette)
        connect(mStylusPalette, &FloatingPalette::mouseEntered, activeScene, &GraphicsScene::hideTool,
                Qt::UniqueConnection);

    if (mpPageNavigWidget) {
        mpPageNavigWidget->setPageNumber(DocumentContainer::pageFromSceneIndex(pageIndex),
                                         activeScene->document()->pageCount());
    }

    if (mZoomPalette)
        connect(mZoomPalette, &FloatingPalette::mouseEntered, activeScene, &GraphicsScene::hideTool,
                Qt::UniqueConnection);

    if (mBackgroundsPalette) {
        connect(mBackgroundsPalette, &FloatingPalette::mouseEntered, activeScene, &GraphicsScene::hideTool,
                Qt::UniqueConnection);
        mBackgroundsPalette->refresh();
    }
}


void BoardPaletteManager::toggleBackgroundPalette(bool checked) {
    mBackgroundsPalette->setVisible(checked);

    if (checked) {
        Application::mainWindow->ui().actionErase->setChecked(false);
        Application::mainWindow->ui().actionNewPage->setChecked(false);

        mBackgroundsPalette->adjustSizeAndPosition();
        mBackgroundsPalette->move((mContainer->width() - mBackgroundsPalette->width()) / 2,
                                  (mContainer->height() - mBackgroundsPalette->height()) / 5);
    }
}


void BoardPaletteManager::backgroundPaletteClosed() {
    Application::mainWindow->ui().actionBackgrounds->setChecked(false);
}


void BoardPaletteManager::toggleStylusPalette(bool checked) {
    mStylusPalette->setVisible(checked);
}


void BoardPaletteManager::toggleErasePalette(bool checked) {
    mErasePalette->setVisible(checked);
    if (checked) {
        Application::mainWindow->ui().actionBackgrounds->setChecked(false);
        Application::mainWindow->ui().actionNewPage->setChecked(false);

        mErasePalette->adjustSizeAndPosition();
        mErasePalette->move((mContainer->width() - mErasePalette->width()) / 2,
                            (mContainer->height() - mErasePalette->height()) / 5);
    }
}


void BoardPaletteManager::erasePaletteClosed() {
    Application::mainWindow->ui().actionErase->setChecked(false);
}


void BoardPaletteManager::togglePagePalette(bool checked) {
    mPagePalette->setVisible(checked);
    if (checked) {
        Application::mainWindow->ui().actionBackgrounds->setChecked(false);
        Application::mainWindow->ui().actionErase->setChecked(false);

        mPagePalette->adjustSizeAndPosition();
        mPagePalette->move((mContainer->width() - mPagePalette->width()) / 2,
                           (mContainer->height() - mPagePalette->height()) / 5);
    }
}


void BoardPaletteManager::pagePaletteClosed() {
    Application::mainWindow->ui().actionPages->setChecked(false);
}


void BoardPaletteManager::toggleRecordingPalette(bool checked) {
    RecordingController::instance()->toggleRecordingPalette(checked);
}


void BoardPaletteManager::addItem(const QUrl &pUrl) {
    mItemUrl = pUrl;
    mPixmap = QPixmap();
    mPos = QPointF(0, 0);
    mScaleFactor = 1.;

    mAddItemPalette->show();
    mAddItemPalette->adjustSizeAndPosition();

    mAddItemPalette->move((mContainer->width() - mAddItemPalette->width()) / 2,
                          (mContainer->height() - mAddItemPalette->height()) / 5);
}

void BoardPaletteManager::changeMode(DockWidgetMode newMode, bool isInit) {
    bool rightPaletteVisible = mRightPalette->switchMode(newMode);
    bool leftPaletteVisible = mLeftPalette->switchMode(newMode);

    switch (newMode) {
        case DockWidgetMode::Board: {
            // On Application start up the mAddItemPalette isn't initialized yet
            if (mAddItemPalette) {
                mAddItemPalette->setParent(Application::boardController->controlContainer());
            }
            mLeftPalette->assignParent(mContainer);
            mRightPalette->assignParent(mContainer);
            mRightPalette->stackUnder(mStylusPalette);
            mLeftPalette->stackUnder(mStylusPalette);
            if (PlatformUtils::hasVirtualKeyboard() && mKeyboardPalette != NULL &&
                Settings::settings()->useSystemOnScreenKeyboard->get().toBool() == false) {
                if (mKeyboardPalette->keyboardVisible()) {
                    mKeyboardPalette->hide();
                    mKeyboardPalette->setParent(Application::boardController->controlContainer());
                    mKeyboardPalette->show();
                } else
                    mKeyboardPalette->setParent(Application::boardController->controlContainer());
            }

            mLeftPalette->setVisible(leftPaletteVisible);
            mRightPalette->setVisible(rightPaletteVisible);
#ifdef Q_OS_WIN
            if (rightPaletteVisible)
                mRightPalette->setAdditionalVOffset(0);
#endif

            if (!isInit)
                containerResized();
            if (mWebToolsCurrentPalette)
                mWebToolsCurrentPalette->hide();
        } break;

        case DockWidgetMode::Desktop: {
            mAddItemPalette->setParent(
                (QWidget *)Application::applicationController->uninotesController()->drawingView());
            mLeftPalette->assignParent(
                (QWidget *)Application::applicationController->uninotesController()->drawingView());
            mRightPalette->assignParent(
                (QWidget *)Application::applicationController->uninotesController()->drawingView());
            mStylusPalette->raise();

            if (PlatformUtils::hasVirtualKeyboard() && mKeyboardPalette != NULL &&
                Settings::settings()->useSystemOnScreenKeyboard->get().toBool() == false) {
                if (mKeyboardPalette->keyboardVisible()) {
                    mKeyboardPalette->hide();
#ifndef Q_OS_LINUX
                    mKeyboardPalette->setParent(
                        (QWidget *)Application::applicationController->uninotesController()->drawingView());
#else
                    mKeyboardPalette->setParent(0);
#endif
#ifdef Q_OS_OSX
                    mKeyboardPalette->setWindowFlags(Qt::Dialog | Qt::Popup | Qt::FramelessWindowHint);
#endif
                    mKeyboardPalette->show();
                } else
// In linux keyboard in desktop mode have to allways be with null parent
#ifdef Q_OS_LINUX
                    mKeyboardPalette->setParent(0);
#else
                    mKeyboardPalette->setParent(
                        (QWidget *)Application::applicationController->uninotesController()->drawingView());
#endif //Q_OS_LINUX
#ifdef Q_OS_OSX
                mKeyboardPalette->setWindowFlags(Qt::Dialog | Qt::Popup | Qt::FramelessWindowHint);
#endif
            }

            mLeftPalette->setVisible(leftPaletteVisible);
            mRightPalette->setVisible(rightPaletteVisible);
#ifdef Q_OS_WIN
            if (rightPaletteVisible && Settings::settings()->appToolBarPositionedAtTop->get().toBool())
                mRightPalette->setAdditionalVOffset(30);
#endif

            if (!isInit)
                Application::applicationController->uninotesController()->TransparentWidgetResized();

            if (mWebToolsCurrentPalette)
                mWebToolsCurrentPalette->hide();
        } break;

        case DockWidgetMode::Web: {
            mAddItemPalette->setParent(Application::mainWindow);
            if (PlatformUtils::hasVirtualKeyboard() && mKeyboardPalette != NULL &&
                Settings::settings()->useSystemOnScreenKeyboard->get().toBool() == false) {
                //                    tmp variable?
                //                    BrowserWindow* brWnd = Application::webController->GetCurrentWebBrowser();

                if (mKeyboardPalette->keyboardVisible()) {
                    mKeyboardPalette->hide();
                    mKeyboardPalette->setParent(Application::mainWindow);
                    mKeyboardPalette->show();
                } else
                    mKeyboardPalette->setParent(Application::mainWindow);
            }

        } break;

        case DockWidgetMode::Document: {
            mLeftPalette->setVisible(leftPaletteVisible);
            mRightPalette->setVisible(rightPaletteVisible);
            mLeftPalette->assignParent(Application::documentController->controlView());
            mRightPalette->assignParent(Application::documentController->controlView());
            if (PlatformUtils::hasVirtualKeyboard() && mKeyboardPalette != NULL &&
                Settings::settings()->useSystemOnScreenKeyboard->get().toBool() == false) {
                if (mKeyboardPalette->keyboardVisible()) {
                    mKeyboardPalette->hide();
                    mKeyboardPalette->setParent(Application::documentController->controlView());
                    mKeyboardPalette->show();
                } else
                    mKeyboardPalette->setParent(Application::documentController->controlView());
            }
            if (mWebToolsCurrentPalette)
                mWebToolsCurrentPalette->hide();
        } break;

        default: {
            mLeftPalette->setVisible(leftPaletteVisible);
            mRightPalette->setVisible(rightPaletteVisible);
            mLeftPalette->assignParent(0);
            mRightPalette->assignParent(0);
            if (PlatformUtils::hasVirtualKeyboard() && mKeyboardPalette != NULL &&
                Settings::settings()->useSystemOnScreenKeyboard->get().toBool() == false) {
                if (mKeyboardPalette->keyboardVisible()) {
                    mKeyboardPalette->hide();
                    mKeyboardPalette->setParent(0);
                    mKeyboardPalette->show();
                } else
                    mKeyboardPalette->setParent(0);
            }
        } break;
    }

    if (!isInit)
        Application::boardController->notifyPageChanged();
}

void BoardPaletteManager::addItem(const QPixmap &pPixmap, const QPointF &pos, qreal scaleFactor,
                                  const QUrl &sourceUrl) {
    mItemUrl = sourceUrl;
    mPixmap = pPixmap;
    mPos = pos;
    mScaleFactor = scaleFactor;

    mAddItemPalette->show();
    mAddItemPalette->adjustSizeAndPosition();

    mAddItemPalette->move((mContainer->width() - mAddItemPalette->width()) / 2,
                          (mContainer->height() - mAddItemPalette->height()) / 5);
}


void BoardPaletteManager::addItemToCurrentPage() {
    Application::applicationController->showBoard();
    mAddItemPalette->hide();
    if (mPixmap.isNull())
        Application::boardController->downloadURL(mItemUrl);
    else {
        GraphicsPixmapItem *item =
            Application::boardController->activeScene()->addPixmap(mPixmap, NULL, mPos, mScaleFactor);

        QString documentPath = Application::boardController->selectedDocument()->persistencePath();
        QString fileName = PersistenceManager::imageDirectory + "/" + item->uuid().toString() + ".png";
        QString path = documentPath + "/" + fileName;

        item->setSourceUrl(QUrl(path));
        item->setSelected(true);

        DrawingController::drawingController()->setStylusTool(StylusTool::Selector);
    }
}


void BoardPaletteManager::addItemToNewPage() {
    Application::boardController->addScene();
    addItemToCurrentPage();
}


void BoardPaletteManager::addItemToLibrary() {
    if (mPixmap.isNull()) {
        mPixmap = QPixmap(mItemUrl.toLocalFile());
    }

    if (!mPixmap.isNull()) {
        if (mScaleFactor != 1.) {
            mPixmap = mPixmap.scaled(mScaleFactor * mPixmap.width(), mScaleFactor * mPixmap.height(),
                                     Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        QImage image = mPixmap.toImage();

        QDateTime now = QDateTime::currentDateTime();
        QString capturedName = tr("CapturedImage") + "-" + now.toString("dd-MM-yyyy hh-mm-ss") + ".png";
        mpFeaturesWidget->importImage(image, capturedName);
    } else {
        Application::showMessage(tr("Error Adding Image to Library"));
    }

    mAddItemPalette->hide();
}

void BoardPaletteManager::zoomButtonPressed() {
    mZoomButtonPressedTime = QTime::currentTime();

    mPendingZoomButtonPressed = true;
    QTimer::singleShot(1000, this, &BoardPaletteManager::zoomButtonReleased);
}


void BoardPaletteManager::zoomButtonReleased() {
    if (mPendingZoomButtonPressed) {
        if (mZoomButtonPressedTime.msecsTo(QTime::currentTime()) > 900) {
            mBoardControler->zoomRestore();
        }

        mPendingZoomButtonPressed = false;
    }
}

void BoardPaletteManager::panButtonPressed() {
    mPanButtonPressedTime = QTime::currentTime();

    mPendingPanButtonPressed = true;
    QTimer::singleShot(1000, this, &BoardPaletteManager::panButtonReleased);
}


void BoardPaletteManager::panButtonReleased() {
    if (mPendingPanButtonPressed) {
        if (mPanButtonPressedTime.msecsTo(QTime::currentTime()) > 900) {
            mBoardControler->centerRestore();
        }

        mPendingPanButtonPressed = false;
    }
}

void BoardPaletteManager::showVirtualKeyboard(bool show) {
    if (mKeyboardPalette)
        mKeyboardPalette->setVisible(show);
}

void BoardPaletteManager::changeStylusPaletteOrientation(QVariant var) {
    bool bVertical = var.toBool();
    bool bVisible = mStylusPalette->isVisible();

    // Clean the old palette
    if (NULL != mStylusPalette) {
        delete mStylusPalette;
        mStylusPalette = NULL;
    }

    // Create the new palette
    if (bVertical) {
        mStylusPalette = new StylusPalette(mContainer, Qt::Vertical);
    } else {
        mStylusPalette = new StylusPalette(mContainer, Qt::Horizontal);
    }

    connect(mStylusPalette, qOverload<int>(&StylusPalette::stylusToolDoubleClicked),
            Application::boardController, &BoardController::stylusToolDoubleClicked, Qt::UniqueConnection);
    mStylusPalette->setVisible(bVisible); // always show stylus palette at startup
}


void BoardPaletteManager::refreshPalettes() {
    mRightPalette->update();
    mLeftPalette->update();
}

void BoardPaletteManager::startDownloads() {
    if (!mDownloadInProgress) {
        mDownloadInProgress = true;
        mpDownloadWidget->setVisibleState(true);
        mRightPalette->addTab(mpDownloadWidget);
    }
}

void BoardPaletteManager::stopDownloads() {
    if (mDownloadInProgress) {
        mDownloadInProgress = false;
        mpDownloadWidget->setVisibleState(false);
        mRightPalette->removeTab(mpDownloadWidget);
    }
}
