#pragma once

#include <QtWidgets>

class WindowCaptureDelegate : public QObject {
    Q_OBJECT

public:
    WindowCaptureDelegate(QObject *parent = 0);
    ~WindowCaptureDelegate();

    int execute();
    const QPixmap getCapturedWindow();

private:
    bool eventFilter(QObject *target, QEvent *event);
    void processPos(QPoint pPoint);
    void drawSelectionRect();

    bool mIsCapturing;
    bool mCancel;
    QPoint mLastPoint;
    HWND mCurrentWindow;
    QPixmap mCapturedPixmap;
};
