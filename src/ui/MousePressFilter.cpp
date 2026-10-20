#include <QApplication>

#include "ui/MousePressFilter.h"

MousePressFilter::MousePressFilter(QObject *parent) : QObject(parent), mPendingEvent(0)
{
    // 刻意留空
}

MousePressFilter::~MousePressFilter()
{
    // 刻意留空
}

bool MousePressFilter::eventFilter(QObject *obj, QEvent *event)
{
    const bool isMousePress   = event->type() == QEvent::MouseButtonPress;
    const bool isMouseRelease = event->type() == QEvent::MouseButtonRelease;
    const bool isMouseMove    = event->type() == QEvent::MouseMove;

    if (isMousePress)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

        if (mouseEvent)
        {
            if (mPendingEvent)
            {
                delete mPendingEvent;
            }

            // 先构造一个双击事件暂存；若 1 秒内没有松开或移动，就把它投递出去。
            mPendingEvent = new QMouseEvent(QEvent::MouseButtonDblClick, mouseEvent->pos(), mouseEvent->globalPos(), mouseEvent->button(),
                mouseEvent->buttons(), mouseEvent->modifiers());

            mObject       = obj;

            QTimer::singleShot(1000, this, SLOT(mouseDownElapsed()));
        }
    }
    else if (isMouseRelease || isMouseMove)
    {
        if (mPendingEvent)
        {
            QTabletEvent *tabletEvent = static_cast<QTabletEvent *>(event);
            QPoint        point       = tabletEvent->globalPos() - mPendingEvent->globalPos();
            // 松开或移动超过拖拽阈值都说明不是长按，丢弃暂存的双击事件。
            if (isMouseRelease || point.manhattanLength() > QApplication::startDragDistance())
            {
                delete mPendingEvent;
                mPendingEvent = 0;
                mObject       = 0;
            }
        }
    }

    return false;
}

void MousePressFilter::mouseDownElapsed()
{
    if (mPendingEvent && mObject)
    {
        QApplication::postEvent(mObject, mPendingEvent);
        mPendingEvent = 0;
        mObject       = 0;
    }
}
