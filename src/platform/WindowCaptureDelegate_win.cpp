#include "platform/WindowCaptureDelegate_win.h"
#include <windows.h>
#include <QDialog>


WindowCaptureDelegate::WindowCaptureDelegate(QObject *parent)
    :QObject(parent)
        , mIsCapturing(false)
        , mCancel(false)
        , mLastPoint(0, 0)
        , mCurrentWindow(nullptr)
{
    // NOOP
}


WindowCaptureDelegate::~WindowCaptureDelegate(void)
{
    // NOOP
}


const QPixmap WindowCaptureDelegate::getCapturedWindow()
{
    return mCapturedPixmap;
}


int WindowCaptureDelegate::execute()
{
    mIsCapturing = true;
    mCancel = false;
    qApp->installEventFilter(this);

    while (mIsCapturing && !mCancel)
    {
        QPoint curPos = QCursor::pos();
        if (mLastPoint != curPos)
        {
            processPos(curPos);
            mLastPoint = curPos;
        }
        drawSelectionRect();
        qApp->processEvents();
    }

    if (!mCancel && mCurrentWindow)
    {
        BringWindowToTop(mCurrentWindow);
        RedrawWindow(mCurrentWindow, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
        QWaitCondition sleep;
        QMutex mutex;
        mutex.lock();
        sleep.wait(&mutex, 200);
        mutex.unlock();
        mCapturedPixmap = QGuiApplication::primaryScreen()->grabWindow((WId)mCurrentWindow);
        return QDialog::Accepted;
    }
    else
    {
        RedrawWindow(NULL, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
        return QDialog::Rejected;
    }
}


void WindowCaptureDelegate::processPos(QPoint pPoint)
{
    POINT wPoint;

    // QCursor::pos() gives Qt logical coordinates while ChildWindowFromPointEx
    // wants physical device pixels.
    QScreen *screen = QGuiApplication::primaryScreen();
    const qreal dpr = screen ? screen->devicePixelRatio() : 1.0;

    wPoint.x = qRound(pPoint.x() * dpr);
    wPoint.y = qRound(pPoint.y() * dpr);
    HWND desktop = GetDesktopWindow();
    HWND newWindow = ChildWindowFromPointEx(desktop, wPoint, CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT);

    if (mCurrentWindow != newWindow && newWindow != desktop)
    {
        RedrawWindow(NULL, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
        mCurrentWindow = newWindow;
    }
}


void WindowCaptureDelegate::drawSelectionRect()
{
    RECT windowRect;
    if (GetWindowRect(mCurrentWindow, &windowRect))
    {
        HDC hDC;
        HPEN hOldPen;
        HBRUSH hOldBrush;

        hDC = GetDC(NULL);

        hOldPen = (HPEN)SelectObject(hDC, CreatePen(PS_SOLID, 5, RGB(255, 0, 0)));
        hOldBrush = (HBRUSH)SelectObject(hDC, GetStockObject(NULL_BRUSH));

        Rectangle(hDC, windowRect.left + 1, windowRect.top + 1, windowRect.right - 1, windowRect.bottom - 1);

        SelectObject(hDC, hOldBrush);
        DeleteObject(SelectObject(hDC, hOldPen));

        ReleaseDC(NULL, hDC);
    }
}


bool WindowCaptureDelegate::eventFilter(QObject *target, QEvent *event)
{
    Q_UNUSED(target);

    // once the application is deactivated, it means that the user clicked somewhere else and we shall
    // take the capture
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::WindowDeactivate)
    {
        mIsCapturing = false;
    }
    else if (event->type() == QEvent::KeyPress)
    {
        mCancel = true;
    }
    return true;
}

