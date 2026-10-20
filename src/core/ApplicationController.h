#pragma once

#include <QtWidgets>
#include <QtNetwork>
#include <QNetworkAccessManager>

#include <QtNetwork/QHttpPart>
#include <new>
#include <memory>
#include <QJSEngine>

class BoardView;
class DocumentProxy;
class GraphicsScene;
class DesktopAnnotationController;
class ScreenMirror;
class MainWindow;
class DisplayManager;
class Version;
class QNetworkAccessManager;
class QNetworkReply;
class RightPalette;

class ApplicationController : public QObject {
    Q_OBJECT

public:
    ApplicationController(BoardView *pControlView, BoardView *pDisplayView, MainWindow *pMainWindow,
                          QObject *parent, RightPalette *rightPalette);
    virtual ~ApplicationController();

    int initialHScroll() { return mInitialHScroll; }
    int initialVScroll() { return mInitialVScroll; }

    void adaptToolBar();
    void adjustDisplayView();
    void adjustPreviousViews(int pActiveSceneIndex, DocumentProxy *pActiveDocument);

    void blackout();

    void initScreenLayout(bool useMultiscreen);

    void closing();

    void setMirrorSourceWidget(QWidget *);

    void mirroringEnabled(bool);

    void initViewState(int horizontalPosition, int verticalPosition);

    void showBoard();

    void showInternet();

    void showDocument();

    void showMessage(const QString &message, bool showSpinningWheel);

    void importFile(const QString &pFilePath);

    DisplayManager *displayManager() { return mDisplayManager; }

    DesktopAnnotationController *uninotesController() { return mUninoteController; }

    // 桌面批注模式下承载画板调色板的宿主窗口，以及它的尺寸变化通知。画板只需要
    // "一个能当父控件的窗口"，不需要认识 DesktopAnnotationController 本身——装配层
    // 提供这两个入口，whiteboard 就不必为这事 include desktop/。
    QWidget *desktopDrawingHost();
    void desktopDrawingResized();

    enum MainMode { Board = 0, Internet, Document, WebDocument };

    MainMode displayMode() { return mMainMode; }


    bool isShowingDesktop() { return mIsShowingDesktop; }

signals:
    void mainModeChanged(ApplicationController::MainMode pMode);
    void desktopMode(bool displayed);

public slots:
    void addCapturedPixmap(const QPixmap &pPixmap, bool pageMode, const QUrl &sourceUrl = QUrl());

    void addCapturedEmbedCode(const QString &embedCode);

    void screenLayoutChanged();

    void showDesktop(bool dontSwitchFrontProcess = false);

    void hideDesktop();

    void toggleRecordingPalette(bool visible);

    void useMultiScreen(bool use);

    void actionCut();
    void actionCopy();
    void actionPaste();

private slots:

protected:
    DesktopAnnotationController *mUninoteController;

    MainWindow *mMainWindow;

    BoardView *mControlView;
    BoardView *mDisplayView;
    QList<BoardView *> mPreviousViews;

    GraphicsScene *mBlackScene;

    ScreenMirror *mMirror;

    int mInitialHScroll, mInitialVScroll;

private:
    MainMode mMainMode;

    DisplayManager *mDisplayManager;


    bool mIsShowingDesktop;
};
