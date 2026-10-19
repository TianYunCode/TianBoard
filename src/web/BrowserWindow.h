#pragma once

#include <QtWidgets>
#include <QtWebEngineWidgets>

#include <memory>

class ChaseWidget;
class TabWidget;
class ToolbarSearch;
class WebView;
class MainWindow;
class WebDownloadManager;


class BrowserWindow : public QWidget
{
    Q_OBJECT;

public:
    BrowserWindow(QWidget *parent = 0, MainWindow* mainWindow = 0);
    ~BrowserWindow();
    QSize sizeHint() const;

public:
    static QUrl guessUrlFromString(const QString &url);
    TabWidget *tabWidget() const;
    WebView *currentTabWebView() const;

    //void adaptToolBar(bool wideRes);

    static WebDownloadManager *downloadManager();

public slots:
    void loadPage(const QString &url);
    void slotHome();

    void loadUrl(const QUrl &url);
    void loadUrlInNewTab(const QUrl &url);

    WebView *createNewTab();

    WebView* paintWidget();

    void tabCurrentChanged(int);

    void bookmarks();
    void addBookmark();

    void showTabAtTop(bool attop);

    void aboutToShowBackMenu();
    void aboutToShowForwardMenu();
    void openActionUrl(QAction *action);

signals:
    void activeViewPageChanged();
    void activeViewChange(QWidget*);
    void mirroringEnabled(bool checked);

protected:
    void closeEvent(QCloseEvent *event);

private slots:

    void slotLoadProgress(int);
    void slotUpdateStatusbar(const QString &string);
    void slotUpdateWindowTitle(const QString &title = QString());

    void slotFileSaveAs();

    void slotViewZoomIn();
    void slotViewZoomOut();
    void slotViewResetZoom();
    void slotViewZoomTextOnly(bool enable);

    void slotWebSearch();
    void slotToggleInspector(bool enable);
    void slotSelectLineEdit();
    void slotSwapFocus();

    void geometryChangeRequested(const QRect &geometry);

private:
    static WebDownloadManager *sWebDownloadManager;

    void setupMenu();
    void setupToolBar();
    void updateStatusbarActionText(bool visible);

    struct Private;
    std::unique_ptr<Private> d;
};


