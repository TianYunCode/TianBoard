#pragma once

#include <QtWidgets>

class MousePressFilter : public QObject {
    Q_OBJECT

public:
    MousePressFilter(QObject *parent = 0);
    virtual ~MousePressFilter();

protected:
    bool eventFilter(QObject *obj, QEvent *event);

protected slots:
    void mouseDownElapsed();

private:
    QObject *mObject;
    QMouseEvent *mPendingEvent;
};
