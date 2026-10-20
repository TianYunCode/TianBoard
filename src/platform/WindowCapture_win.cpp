#include "platform/WindowCapture.h"

#include "platform/WindowCaptureDelegate_win.h"


WindowCapture::WindowCapture(QWidget *grabTarget, QObject *parent)
    : QObject(parent), mGrabTarget(grabTarget) {
    // NOOP
}


WindowCapture::~WindowCapture() {
    // NOOP
}


const QPixmap WindowCapture::getCapturedWindow() {
    return mWindowPixmap;
}


int WindowCapture::execute() {
    mGrabTarget->grabMouse();
    mGrabTarget->grabKeyboard();

    WindowCaptureDelegate windowCaptureEventHandler;
    int result = windowCaptureEventHandler.execute();

    mGrabTarget->releaseMouse();
    mGrabTarget->releaseKeyboard();

    mWindowPixmap = windowCaptureEventHandler.getCapturedWindow();

    return result;
}
