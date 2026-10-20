#include "platform/WindowCaptureDelegate_win.h"
#include <windows.h>
#include <QDialog>

WindowCaptureDelegate::WindowCaptureDelegate(QObject *parent)
    : QObject(parent), mIsCapturing(false), mCancel(false), mLastPoint(0, 0), mCurrentWindow(nullptr)
{
    // 刻意留空
}

WindowCaptureDelegate::~WindowCaptureDelegate(void)
{
    // 刻意留空
}

const QPixmap WindowCaptureDelegate::getCapturedWindow()
{
    return mCapturedPixmap;
}

int WindowCaptureDelegate::execute()
{
    mIsCapturing = true;
    mCancel      = false;
    // 应用级事件过滤器：鼠标按下或窗口失活视为选定，按键视为取消
    qApp->installEventFilter(this);

    // 自建消息循环：选择期间保持界面响应并不断重绘高亮框
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
        // 先激活并强制重绘目标窗口，否则可能抓到被遮挡的旧内容
        BringWindowToTop(mCurrentWindow);
        RedrawWindow(mCurrentWindow, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
        // 等待重绘完成：200 ms 是经验值，给合成器留出刷新时间
        QWaitCondition sleep;
        QMutex         mutex;
        mutex.lock();
        sleep.wait(&mutex, 200);
        mutex.unlock();
        mCapturedPixmap = QGuiApplication::primaryScreen()->grabWindow((WId)mCurrentWindow);
        return QDialog::Accepted;
    }
    else
    {
        // 取消时清掉屏幕上残留的高亮框
        RedrawWindow(NULL, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
        return QDialog::Rejected;
    }
}

void WindowCaptureDelegate::processPos(QPoint pPoint)
{
    POINT wPoint;

    // QCursor::pos() 返回 Qt 逻辑坐标，而 ChildWindowFromPointEx 需要物理像素
    QScreen    *screen = QGuiApplication::primaryScreen();
    const qreal dpr    = screen ? screen->devicePixelRatio() : 1.0;

    wPoint.x           = qRound(pPoint.x() * dpr);
    wPoint.y           = qRound(pPoint.y() * dpr);
    HWND desktop       = GetDesktopWindow();
    HWND newWindow     = ChildWindowFromPointEx(desktop, wPoint, CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT);

    // 跳过桌面窗口本身，只在真正的外部窗口之间切换
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
        HDC    hDC;
        HPEN   hOldPen;
        HBRUSH hOldBrush;

        // 直接画在屏幕 DC 上，避免创建额外窗口
        hDC       = GetDC(NULL);

        // 红色 5 像素空心框；用完必须恢复并释放 GDI 对象，防止句柄泄漏
        hOldPen   = (HPEN)SelectObject(hDC, CreatePen(PS_SOLID, 5, RGB(255, 0, 0)));
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

    // 应用失活说明用户点了别处，按“选定”处理；按键则视为取消。
    // 始终返回 true，把事件吞掉，防止选择期间触发本程序界面。
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
