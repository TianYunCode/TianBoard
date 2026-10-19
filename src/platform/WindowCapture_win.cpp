#include "platform/WindowCapture.h"

#include "platform/WindowCaptureDelegate_win.h"
#include "desktop/DesktopAnnotationController.h"
#include "desktop/DesktopPalette.h"

#include "whiteboard/BoardView.h"


WindowCapture::WindowCapture(DesktopAnnotationController *parent) : QObject(parent), mParent(parent) {
    // NOOP
}


WindowCapture::~WindowCapture() {
    // NOOP
}


const QPixmap WindowCapture::getCapturedWindow() {
    return mWindowPixmap;
}


int WindowCapture::execute() {
    mParent->desktopPalette()->grabMouse();
    mParent->desktopPalette()->grabKeyboard();

    WindowCaptureDelegate windowCaptureEventHandler;
    int result = windowCaptureEventHandler.execute();

    mParent->desktopPalette()->releaseMouse();
    mParent->desktopPalette()->releaseKeyboard();

    mWindowPixmap = windowCaptureEventHandler.getCapturedWindow();

    return result;
}
