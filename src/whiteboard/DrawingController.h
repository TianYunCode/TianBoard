#pragma once

#include <QtCore>

#include "utils/Enums.h"
#include "ui/StylusPalette.h"

class AbstractDrawRuler;
class DrawingController : public QObject {
    Q_OBJECT

private:
    DrawingController(QObject *parent = 0);
    virtual ~DrawingController();

public:
    static DrawingController *drawingController();
    static void destroy();

    int stylusTool();
    int latestDrawingTool();

    bool isDrawingTool();

    int currentToolWidthIndex();
    qreal currentToolWidth();
    int currentToolColorIndex();
    QColor currentToolColor();
    QColor toolColor(bool onDarkBackground);

    void setPenColor(bool onDarkBackground, const QColor &color, int pIndex);
    void setMarkerColor(bool onDarkBackground, const QColor &color, int pIndex);
    void setMarkerAlpha(qreal alpha);

    AbstractDrawRuler *mActiveRuler;

    void setInDestopMode(bool mode) { mIsDesktopMode = mode; }

    bool isInDesktopMode() { return mIsDesktopMode; }

public slots:
    void setStylusTool(int tool);
    void setLineWidthIndex(int index);
    void setColorIndex(int index);
    void setEraserWidthIndex(int index);

signals:
    void stylusToolChanged(int tool);
    void colorPaletteChanged();

    void lineWidthIndexChanged(int index);
    void colorIndexChanged(int index);

private:
    StylusTool::Enum mStylusTool;
    StylusTool::Enum mLatestDrawingTool;
    bool mIsDesktopMode;

    static DrawingController *sDrawingController;

private slots:
    void penToolSelected(bool checked);
    void eraserToolSelected(bool checked);
    void markerToolSelected(bool checked);
    void selectorToolSelected(bool checked);
    void playToolSelected(bool checked);
    void handToolSelected(bool checked);
    void zoomInToolSelected(bool checked);
    void zoomOutToolSelected(bool checked);
    void pointerToolSelected(bool checked);
    void lineToolSelected(bool checked);
    void textToolSelected(bool checked);
    void captureToolSelected(bool checked);
};
