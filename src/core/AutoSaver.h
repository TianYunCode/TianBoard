#pragma once

#include <QtCore>

class AutoSaver : public QObject {
    Q_OBJECT;

public:
    AutoSaver(QObject *parent);
    ~AutoSaver();
    void saveIfNeccessary();

public slots:
    void changeOccurred();

protected:
    void timerEvent(QTimerEvent *event);

private:
    QBasicTimer mTimer;
    QElapsedTimer mFirstChange;
};
