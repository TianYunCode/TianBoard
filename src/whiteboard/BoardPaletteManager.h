#pragma once

#include <QtWidgets>
#include <QtWebEngineWidgets>

#include "ui/LeftPalette.h"
#include "ui/RightPalette.h"
#include "ui/PageNavigationWidget.h"
#include "ui/CachePropertiesWidget.h"
#include "ui/DockDownloadWidget.h"
#include "core/ApplicationController.h"
#include "ui/FeaturesWidget.h"


class WebToolsPalette;
class StylusPalette;
class PageNumberPalette;
class ZoomPalette;
class ActionPalette;
class BackgroundPalette;
class BoardController;
class ServerXMLHttpRequest;
class KeyboardPalette;
class MainWindow;
class ApplicationController;

class BoardPaletteManager : public QObject {
    Q_OBJECT

public:
    BoardPaletteManager(QWidget *container, BoardController *controller);
    virtual ~BoardPaletteManager();

    void setupLayout();
    LeftPalette *leftPalette() { return mLeftPalette; }
    RightPalette *rightPalette() { return mRightPalette; }
    StylusPalette *stylusPalette() { return mStylusPalette; }
    ActionPalette *addItemPalette() { return mAddItemPalette; }
    void showVirtualKeyboard(bool show = true);
    void initPalettesPosAtStartup();
    void refreshPalettes();

    KeyboardPalette *mKeyboardPalette;

    void setCurrentWebToolsPalette(WebToolsPalette *palette) { mWebToolsCurrentPalette = palette; }
    WebToolsPalette *mWebToolsCurrentPalette;

    void processPalettersWidget(DockPalette *paletter, DockWidgetMode mode);
    void changeMode(DockWidgetMode newMode, bool isInit = false);
    void startDownloads();
    void stopDownloads();

public slots:
    void activeSceneChanged();
    void containerResized();
    void addItem(const QUrl &pUrl);
    void addItem(const QPixmap &pPixmap, const QPointF &p = QPointF(0.0, 0.0), qreal scale = 1.0,
                 const QUrl &sourceUrl = QUrl());

    void slot_changeMainMode(ApplicationController::MainMode);
    void slot_changeDesktopMode(bool);

    void toggleErasePalette(bool ckecked);

private:
    void setupPalettes();
    void connectPalettes();
    void positionFreeDisplayPalette();
    void setupDockPaletteWidgets();

    QWidget *mContainer;
    BoardController *mBoardControler;

    StylusPalette *mStylusPalette;

    ZoomPalette *mZoomPalette;
    LeftPalette *mLeftPalette;
    RightPalette *mRightPalette;

    BackgroundPalette *mBackgroundsPalette;
    ActionPalette *mToolsPalette;
    ActionPalette *mAddItemPalette;
    ActionPalette *mErasePalette;
    ActionPalette *mPagePalette;

    QUrl mItemUrl;
    QPixmap mPixmap;
    QPointF mPos;
    qreal mScaleFactor;

    QTime mPageButtonPressedTime;
    bool mPendingPageButtonPressed;

    QTime mZoomButtonPressedTime;
    bool mPendingZoomButtonPressed;

    QTime mPanButtonPressedTime;
    bool mPendingPanButtonPressed;

    QTime mEraseButtonPressedTime;
    bool mPendingEraseButtonPressed;

    PageNavigationWidget *mpPageNavigWidget;

    CachePropertiesWidget *mpCachePropWidget;

    FeaturesWidget *mpFeaturesWidget;

    DockDownloadWidget *mpDownloadWidget;

    bool mDownloadInProgress;

private slots:
    void changeBackground();

    void toggleBackgroundPalette(bool checked);
    void backgroundPaletteClosed();

    void toggleStylusPalette(bool checked);
    void toggleRecordingPalette(bool checked);

    void erasePaletteButtonPressed();
    void erasePaletteButtonReleased();

    void erasePaletteClosed();

    void togglePagePalette(bool ckecked);
    void pagePaletteClosed();

    void pagePaletteButtonPressed();
    void pagePaletteButtonReleased();

    void addItemToCurrentPage();
    void addItemToNewPage();
    void addItemToLibrary();

    void purchaseLinkActivated(const QString &);

    void linkClicked(const QUrl &url);

    void zoomButtonPressed();
    void zoomButtonReleased();
    void panButtonPressed();
    void panButtonReleased();

    void changeStylusPaletteOrientation(QVariant var);
};
