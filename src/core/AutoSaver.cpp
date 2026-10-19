#include "core/AutoSaver.h"

#include <QtCore>


#define AUTOSAVE_IN 1000 * 3 // seconds
#define MAXWAIT 1000 * 15    // seconds

AutoSaver::AutoSaver(QObject *parent) : QObject(parent) {
    Q_ASSERT(parent);
}


AutoSaver::~AutoSaver() {
    if (mTimer.isActive())
        qWarning() << "AutoSaver: still active when destroyed, changes not saved.";
}


void AutoSaver::changeOccurred() {
    if (!mFirstChange.isValid())
        mFirstChange.start();

    if (mFirstChange.elapsed() > MAXWAIT) {
        saveIfNeccessary();
    } else {
        mTimer.start(AUTOSAVE_IN, this);
    }
}


void AutoSaver::timerEvent(QTimerEvent *event) {
    if (event->timerId() == mTimer.timerId()) {
        saveIfNeccessary();
    } else {
        QObject::timerEvent(event);
    }
}


void AutoSaver::saveIfNeccessary() {
    if (!mTimer.isActive())
        return;

    mTimer.stop();
    mFirstChange.invalidate();

    if (!QMetaObject::invokeMethod(parent(), "save", Qt::DirectConnection)) {
        qWarning() << "AutoSaver: error invoking slot save() on parent";
    }
}
