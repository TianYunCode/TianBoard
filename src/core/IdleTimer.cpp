#include "core/IdleTimer.h"

#include <QApplication>
#include <QInputEvent>
#include <QTimer>
#include <QWidget>

#include "core/Application.h"
#include "core/ApplicationController.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/BoardView.h"

IdleTimer::IdleTimer(QObject *parent) : QObject(parent), mCursorIsHidden(false)
{
    startTimer(100);
}

IdleTimer::~IdleTimer()
{
    // 无动作：startTimer() 注册的定时器由 QObject 析构时自动注销。
}

bool IdleTimer::eventFilter(QObject *obj, QEvent *event)
{
    // 鼠标 / 数位板 / 键盘都算“有活动”：刷新时间戳，并把隐藏的光标恢复回来。
    if (dynamic_cast<QInputEvent *>(event))
    {
        mLastInputEventTime = QDateTime::currentDateTime();
        if (mCursorIsHidden)
        {
            QApplication::restoreOverrideCursor();
            mCursorIsHidden = false;
        }
    }

    return QObject::eventFilter(obj, event);
}

void IdleTimer::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);

    bool hasWaitCuror = (QApplication::overrideCursor() && (QApplication::overrideCursor()->shape() == Qt::WaitCursor));

    if (!mCursorIsHidden && (mLastInputEventTime.secsTo(QDateTime::currentDateTime()) >= 2) && Application::boardController &&
        Application::boardController->controlView() && Application::boardController->controlView()->hasFocus() && !hasWaitCuror)
    {
        QApplication::setOverrideCursor(QCursor(Qt::BlankCursor));
        mCursorIsHidden = true;
    }
}
