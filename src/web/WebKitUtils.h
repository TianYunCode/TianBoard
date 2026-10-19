#pragma once

#include <QtWebEngineWidgets>
#include <QWebEnginePage>

class WebKitUtils {
public:
    WebKitUtils();
    virtual ~WebKitUtils();

    class HtmlObject {
    public:
        HtmlObject(const QString &pSource, int pWidth, int pHeight)
            : source(pSource), width(pWidth), height(pHeight) {}

        QString source;
        int width;
        int height;
    };

    static QList<WebKitUtils::HtmlObject> objectsInFrame(QWebEnginePage *frame);
};
