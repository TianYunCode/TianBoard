#include "platform/WindowCapture.h"

#include "platform/WindowCaptureDelegate_win.h"


WindowCapture::WindowCapture(QWidget *grabTarget, QObject *parent)
    : QObject(parent), mGrabTarget(grabTarget) {
    // 刻意留空
}


WindowCapture::~WindowCapture() {
    // 刻意留空
}


const QPixmap WindowCapture::getCapturedWindow() {
    return mWindowPixmap;
}


int WindowCapture::execute() {
    // 捕获期间独占输入，避免用户误操作本程序界面；委托内部跑消息循环
    mGrabTarget->grabMouse();
    mGrabTarget->grabKeyboard();

    WindowCaptureDelegate windowCaptureEventHandler;
    int result = windowCaptureEventHandler.execute();

    // 无论成功与否都要归还输入，否则界面会永久失去响应
    mGrabTarget->releaseMouse();
    mGrabTarget->releaseKeyboard();

    mWindowPixmap = windowCaptureEventHandler.getCapturedWindow();

    return result;
}
