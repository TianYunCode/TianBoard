#include "core/VirtualDesktop.h"

#include "ui/ScreenMirror.h"

#include "core/Settings.h"
#include "core/Setting.h"
#include "core/Application.h"
#include "whiteboard/BoardController.h"

#if defined(Q_OS_OSX)
#include <ApplicationServices/ApplicationServices.h>
#endif


struct ScreenMirror::Private
{
    QPointer<QWidget> sourceWidget;
    QRect rect;
    QPixmap lastPixmap;
    int timerID{0};
};


ScreenMirror::ScreenMirror(QWidget* parent)
    : QWidget(parent), d(new Private)
{
    // NOOP
}


ScreenMirror::~ScreenMirror() = default;


void ScreenMirror::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    painter.fillRect(0, 0, width(), height(), QBrush(Qt::black));

    if (!d->lastPixmap.isNull())
    {
        int x = (width() - d->lastPixmap.width()) / 2;
        int y = (height() - d->lastPixmap.height()) / 2;

        painter.drawPixmap(x, y, d->lastPixmap);
    }
}


void ScreenMirror::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);

    grabPixmap();

    update();
}

void ScreenMirror::grabPixmap()
{
    if (d->sourceWidget)
    {
        QWidget *sourceWidget = d->sourceWidget.data();

        QPoint topLeft = sourceWidget->mapToGlobal(sourceWidget->geometry().topLeft());
        QPoint bottomRight = sourceWidget->mapToGlobal(sourceWidget->geometry().bottomRight());

        d->rect.setTopLeft(topLeft);
        d->rect.setBottomRight(bottomRight);
        d->lastPixmap = sourceWidget->grab();
    }
    else{
        // WHY HERE?
        // this is the case we are showing the desktop but the is no widget and we use the last widget rectagle to know
        // what we have to grab. Not very good way of doing
        d->lastPixmap = VirtualDesktop::grabScreen(Application::controlScreen(), d->rect);
    }

    if (!d->lastPixmap.isNull())
        d->lastPixmap = d->lastPixmap.scaled(width(), height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
}


void ScreenMirror::setSourceWidget(QWidget *sourceWidget)
{
    d->sourceWidget = sourceWidget;

    grabPixmap();

    update();
}


void ScreenMirror::setSourceRect(const QRect& pRect)
{
    d->rect = pRect;
    d->sourceWidget = nullptr;
}


void ScreenMirror::start()
{
    qDebug() << "mirroring START";
    Application::boardController->freezeWebWidgets(true);
    if (d->timerID == 0)
    {
        int ms = 125;

        bool success;
        int fps = Settings::settings()->mirroringRefreshRateInFps->get().toInt(&success);

        if (success && fps > 0)
        {
            ms = 1000 / fps;
        }

        d->timerID = startTimer(ms);
    }
    else
    {
        qDebug() << "ScreenMirror::start() : Timer already running ...";
    }
}


void ScreenMirror::stop()
{
    qDebug() << "mirroring STOP";
    Application::boardController->freezeWebWidgets(false);
    if (d->timerID != 0)
    {
        killTimer(d->timerID);
        d->timerID = 0;
    }
}
