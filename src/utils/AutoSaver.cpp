#include "utils/AutoSaver.h"

#include <QtCore>

#define AUTOSAVE_IN 1000 * 3  // 毫秒：最后一次变更后的等待时间
#define MAXWAIT     1000 * 15 // 毫秒：从首次变更起的最大延迟

AutoSaver::AutoSaver(QObject *parent) : QObject(parent)
{
    Q_ASSERT(parent);
}

AutoSaver::~AutoSaver()
{
    if (mTimer.isActive())
    {
        qWarning() << "AutoSaver: 析构时定时器还在跑，改动没来得及保存。";
    }
}

void AutoSaver::changeOccurred()
{
    if (!mFirstChange.isValid())
    {
        mFirstChange.start();
    }

    if (mFirstChange.elapsed() > MAXWAIT)
    {
        saveIfNeccessary();
    }
    else
    {
        mTimer.start(AUTOSAVE_IN, this);
    }
}

void AutoSaver::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == mTimer.timerId())
    {
        saveIfNeccessary();
    }
    else
    {
        QObject::timerEvent(event);
    }
}

void AutoSaver::saveIfNeccessary()
{
    if (!mTimer.isActive())
    {
        return;
    }

    mTimer.stop();
    mFirstChange.invalidate();

    if (!QMetaObject::invokeMethod(parent(), "save", Qt::DirectConnection))
    {
        qWarning() << "AutoSaver: 调用父对象的 save() 槽失败";
    }
}
