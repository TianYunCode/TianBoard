#pragma once

#include <QtWidgets>
#include <QtWebEngineWidgets>
#include <QWebEnginePage>
#include <QWebEngineView>

#include "web/OEmbedParser.h"

class BrowserWindow;
class Application;
class TrapFlashController;
class MainWindow;
class WebToolsPalette;
class WebView;
class ServerXMLHttpRequest;

class WebController : public QObject
{
    Q_OBJECT

public:
    WebController(MainWindow* mainWindow);
    virtual ~WebController();
    void closing();
    void adaptToolBar();

    QPixmap captureCurrentPage();

    void showTabAtTop(bool attop);

    void loadUrl(const QUrl& url);

    QWebEngineView* createNewTab();

    QUrl currentPageUrl() const;

    void show();

    BrowserWindow* GetCurrentWebBrowser(){return mCurrentWebBrowser;}

protected:
    void setupPalettes();
    QPixmap getScreenPixmap();

signals:
	void imageCaptured(const QPixmap& pCapturedPixmap, bool pageMode, const QUrl& source);
	void activeWebPageChanged(WebView* pWebView);

public slots:
    void screenLayoutChanged();

    void setSourceWidget(QWidget* pWidget);
    void captureWindow();
    void customCapture();
    void toogleMirroring(bool checked);

    QWidget* controlView()
    {
        return mBrowserWidget;
    }

    void captureoEmbed();
    void captureEduMedia();

    bool isOEmbedable(const QUrl& pUrl);
    bool hasEmbeddedContent();
    void getEmbeddableContent();

    bool isEduMedia(const QUrl& pUrl);

    void copy();
    void paste();
    void cut();
	void triggerWebTools(bool checked);

private slots:
	void activePageChanged();
	void toggleWebTrap(bool checked);
	void onOEmbedParsed(QVector<sOEmbedContent> contents);

private:
    void initialiazemOEmbedProviders();
    void webBrowserInstance();
    void lookForEmbedContent(QString* pHtml, QString tag, QString attribute, QList<QUrl>* pList);
    void checkForOEmbed(QString* pHtml);

    MainWindow *mMainWindow;

    BrowserWindow* mCurrentWebBrowser;

    QWidget* mBrowserWidget;

    WebToolsPalette* mToolsCurrentPalette;

    bool mToolsPalettePositionned;

    bool mDownloadViewIsVisible;

    QStringList mOEmbedProviders;

    OEmbedParser mOEmbedParser;

};

