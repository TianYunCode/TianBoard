#pragma once

#include <QtWidgets>
#include <QtWebEngineWidgets>
#include <QWebEnginePage>
#include <QWebEngineView>

class BrowserWindow;
class Application;
class MainWindow;
class WebToolsPalette;
class WebView;

class WebController : public QObject
{
    Q_OBJECT

public:
    WebController(MainWindow* mainWindow);
    virtual ~WebController();
    void closing();
    void adaptToolBar();

    void showTabAtTop(bool attop);

    void loadUrl(const QUrl& url);

    QWebEngineView* createNewTab();

    void show();

protected:
    void setupPalettes();
    QPixmap getScreenPixmap();

signals:
	void imageCaptured(const QPixmap& pCapturedPixmap, bool pageMode, const QUrl& source);
	void activeWebPageChanged(WebView* pWebView);

public slots:
    void screenLayoutChanged();

    void setSourceWidget(QWidget* pWidget);
    void customCapture();
    void toogleMirroring(bool checked);

    QWidget* controlView()
    {
        return mBrowserWidget;
    }

    void copy();
    void paste();
    void cut();
	void triggerWebTools(bool checked);

private slots:
	void activePageChanged();
	void toggleWebTrap(bool checked);

private:
    void webBrowserInstance();

    MainWindow *mMainWindow;

    BrowserWindow* mCurrentWebBrowser;

    QWidget* mBrowserWidget;

    WebToolsPalette* mToolsCurrentPalette;

    bool mDownloadViewIsVisible;

};

