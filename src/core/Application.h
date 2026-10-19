#pragma once

#include <QtWidgets>
#include <QUndoStack>
#include <QToolBar>
#include <QMenu>

#include "third/singleapp/qtsingleapplication.h"

namespace Ui {
class MainWindow;
}

class BoardController;
class WebController;
class ControlView;
class PreferencesController;
class Resources;
class Settings;
class PersistenceManager;
class ApplicationController;
class DocumentController;
class MainWindow;

class Application : public QtSingleApplication {
    Q_OBJECT

public:
    Application(const QString &id, int &argc, char **argv);
    virtual ~Application();

    int exec(const QString &pFileToImport);

    void cleanup();

    static QPointer<QUndoStack> undoStack;

    static ApplicationController *applicationController;
    static BoardController *boardController;
    static WebController *webController;
    static DocumentController *documentController;

    static MainWindow *mainWindow;

    static Application *app() { return static_cast<Application *> qApp; }

    static const QString mimeTypeBoardDocument;
    static const QString mimeTypeBoardPage;
    static const QString mimeTypeBoardPageItem;
    static const QString mimeTypeBoardPageThumbnail;

    static void showMessage(const QString &message, bool showSpinningWheel = false);
    static void setDisabled(bool disable);

    static QObject *staticMemoryCleaner;

    void decorateActionMenu(QAction *action);
    void insertSpaceToToolbarBeforeAction(QToolBar *toolbar, QAction *action, int width = -1);

    int toolBarHeight();
    bool eventFilter(QObject *obj, QEvent *event);

    bool isVerbose() { return mIsVerbose; }
    void setVerbose(bool verbose) { mIsVerbose = verbose; }
    static QString urlFromHtml(QString html);
    static bool isFromWeb(QString url);

    static QScreen *controlScreen();
    static int controlScreenIndex();

public slots:
    void showBoard();
    void showInternet();
    void showDocument();
    void startScript();
    void stopScript();

    void toolBarPositionChanged(QVariant topOrBottom);
    void toolBarDisplayTextChanged(QVariant display);

    void closeEvent(QCloseEvent *event);

    bool handleOpenMessage(const QString &pMessage);

private slots:
    void closing();
    //#ifdef Q_OS_OSX // for some reason this is not compiled if the ifdef is uncommented
    void showMinimized();
    //#endif
    void onScreenCountChanged(int newCount);

private:
    void updateProtoActionsState();
    void setupTranslators(const QStringList &args);
    QList<QMenu *> mProtoMenus;
    bool mIsVerbose;

protected:
    /*
#if defined(Q_OS_MAC) && !defined(QT_MAC_USE_COCOA)
        bool macEventFilter(EventHandlerCallRef caller, EventRef event);
#endif
        */

    PreferencesController *mPreferencesController;
};
