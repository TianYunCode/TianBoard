#pragma once

#include <QWidget>

class SpinningWheel : public QWidget {
    Q_OBJECT

public:
    SpinningWheel(QWidget *parent = 0);
    virtual ~SpinningWheel();
    virtual void paintEvent(QPaintEvent *event);

public slots:
    void startAnimation();
    void stopAnimation();

protected:
    virtual QSize sizeHint() const;
    virtual void timerEvent(QTimerEvent *event);

private:
    QAtomicInt mPosition;
    int mTimerID;
};
