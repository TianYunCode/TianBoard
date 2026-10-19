#pragma once

#include <QList>
#include <QUrl>
#include <algorithm>

class ForeighnObjectsHandlerPrivate;

class ForeighnObjectsHandler {
public:
    ForeighnObjectsHandler();
    ~ForeighnObjectsHandler();

    void cure(const QList<QUrl> &dirs);
    void cure(const QUrl &dir);

    void copyPage(const QUrl &fromDir, int fromIndex, const QUrl &toDir, int toIndex);

private:
    ForeighnObjectsHandlerPrivate *d;

    friend class ForeighnObjectsHandlerPrivate;
};
