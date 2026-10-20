#pragma once

#include <QtWidgets>
#include <QTime>
#include <QTimer>

#include "ui/RightPalette.h"

class DesktopPalette;
class BoardView;
class GraphicsScene;
class DesktopPenPalette;
class DesktopMarkerPalette;
class DesktopEraserPalette;
class ActionPalette;
class MainWindow;
class RightPalette;

#define PROPERTY_PALETTE_TIMER 1000

class DesktopAnnotationController : public QObject {
    Q_OBJECT

public:
    DesktopAnnotationController(QObject *parent, RightPalette *rightPalette);
    virtual ~DesktopAnnotationController();
    void showWindow();
    void hideWindow();

    DesktopPalette *desktopPalette();
    BoardView *drawingView();

    void TransparentWidgetResized();

public slots:
    void screenLayoutChanged();
    void goToBoard();
    void customCapture();
    void windowCapture();
    void screenCapture();
    void updateShowHideState(bool pEnabled);

    void close();

    void stylusToolChanged(int tool);
    void updateBackground();

    //         void showKeyboard(bool show);
    //         void showKeyboard(); //X11 virtual keyboard working only needed

signals:
    void imageCaptured(const QPixmap &pCapturedPixmap, bool pageMode);
    void restoreBoard();

protected:
    QPixmap getScreenPixmap();

    BoardView *mTransparentDrawingView;
    GraphicsScene *mTransparentDrawingScene;

private slots:
    void updateColors();
    void desktopPenActionToggled(bool checked);
    void desktopMarkerActionToggled(bool checked);
    void desktopEraserActionToggled(bool checked);
    void eraseDesktopAnnotations();
    void penActionPressed();
    void markerActionPressed();
    void eraserActionPressed();
    void penActionReleased();
    void markerActionReleased();
    void eraserActionReleased();
    void selectorActionPressed();
    void selectorActionReleased();
    void pointerActionPressed();
    void pointerActionReleased();

    void switchCursor(int tool);
    void onDesktopPaletteMaximized();
    void onDesktopPaletteMinimize();
    void onTransparentWidgetResized();
    void onToolClicked();

private:
    void setAssociatedPalettePosition(ActionPalette *palette, const QString &actionName);
    void togglePropertyPalette(ActionPalette *palette);

    DesktopPalette *mDesktopPalette;
    //KeyboardPalette *mKeyboardPalette;
    DesktopPenPalette *mDesktopPenPalette;
    DesktopMarkerPalette *mDesktopMarkerPalette;
    DesktopEraserPalette *mDesktopEraserPalette;

    RightPalette *mRightPalette;

    QTime mPenHoldTimer;
    QTime mMarkerHoldTimer;
    QTime mEraserHoldTimer;
    QTimer mHoldTimerPen;
    QTimer mHoldTimerMarker;
    QTimer mHoldTimerEraser;

    bool mWindowPositionInitialized;
    bool mIsFullyTransparent;
    bool mDesktopToolsPalettePositioned;
    bool mPendingPenButtonPressed;
    bool mPendingMarkerButtonPressed;
    bool mPendingEraserButtonPressed;
    bool mbArrowClicked;

    int mBoardStylusTool;
    int mDesktopStylusTool;
};
