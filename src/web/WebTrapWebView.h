#pragma once

#include <QtWidgets>
#include <QtWebEngineWidgets>
#include <QWebEngineView>

#include <memory>

class WebTrapWebView : public QWebEngineView {
    Q_OBJECT;

public:
    WebTrapWebView(QWidget *parent = 0);
    virtual ~WebTrapWebView();

    void setIsTrapping(bool pIsTrapping);

    bool isTrapping();

    void highliteElementAtPos(const QPoint &pos);
    void trapElementAtPos(const QPoint &pos);

signals:
    void pixmapCaptured(const QPixmap &pixmap, bool pageMode);
    void objectCaptured(const QUrl &pUrl, const QString &pMimeType, int pWidth, int pHeight);
    void embedCodeCaptured(const QString &embedCode);

    void webElementCaptured(const QUrl &pUrl, const QString &query);

private slots:
    void viewLoadFinished(bool ok);

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void hideEvent(QHideEvent *event);

    virtual void paintEvent(QPaintEvent *event);

    QString potentialEmbedCodeAtPos(const QPoint &pos);

    enum WebContentType { Unknown = 0, Image, Input, ObjectOrEmbed, ElementByQuery };

private:
    struct Private;
    std::unique_ptr<Private> d;
};

class WebTrapMouseEventMask : public QWidget {
public:
    WebTrapMouseEventMask(WebTrapWebView *pWebView);
    virtual ~WebTrapMouseEventMask();

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void paintEvent(QPaintEvent *);

private:
    struct Private;
    std::unique_ptr<Private> d;
};
