#pragma once

#include <QtWidgets>

class Resources : public QObject {
    Q_OBJECT

public:
    static Resources *resources();
    QStringList customFontList() { return mCustomFontList; }

private:
    Resources(QObject *pParent = 0);
    virtual ~Resources();

    void init();

    static Resources *sSingleton;
    void buildFontList();
    QStringList mCustomFontList;

public:
    QCursor penCursor;
    QCursor eraserCursor;
    QCursor markerCursor;
    QCursor pointerCursor;
    QCursor handCursor;
    QCursor zoomInCursor;
    QCursor zoomOutCursor;
    QCursor arrowCursor;
    QCursor playCursor;
    QCursor textCursor;
    QCursor rotateCursor;
    QCursor drawLineRulerCursor;
};
