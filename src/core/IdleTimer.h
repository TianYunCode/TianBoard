#pragma once

#include <QObject>
#include <QDateTime>

class QEvent;

class IdleTimer : public QObject {
    Q_OBJECT

public:
    IdleTimer(QObject *parent = 0);
    virtual ~IdleTimer();

protected:
    bool eventFilter(QObject *obj, QEvent *event);
    virtual void timerEvent(QTimerEvent *event);

private:
    QDateTime mLastInputEventTime;
    bool mCursorIsHidden;
};
