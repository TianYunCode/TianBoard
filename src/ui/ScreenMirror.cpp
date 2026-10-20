#include "core/VirtualDesktop.h"

#include "ui/ScreenMirror.h"

#include "settings/Settings.h"
#include "settings/Setting.h"
#include "core/Application.h"
#include "whiteboard/BoardController.h"

struct ScreenMirror::Private
{
    QPointer<QWidget> sourceWidget;
    QRect             rect;
    QPixmap           lastPixmap;
    int               timerID{0};
};

ScreenMirror::ScreenMirror(QWidget *parent) : QWidget(parent), d(new Private)
{
    // 刻意留空
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

        // 先算出源部件当前所在的屏幕区域并记下来，供之后切换成矩形抓屏时复用。
        QPoint topLeft        = sourceWidget->mapToGlobal(sourceWidget->geometry().topLeft());
        QPoint bottomRight    = sourceWidget->mapToGlobal(sourceWidget->geometry().bottomRight());

        d->rect.setTopLeft(topLeft);
        d->rect.setBottomRight(bottomRight);
        d->lastPixmap = sourceWidget->grab();
    }
    else
    {
        // 没有源部件时（桌面镜像）按上一次记录的屏幕矩形抓取；这个矩形可能来自
        // 之前镜像过的部件，因此只是近似区域，并非精确的桌面布局。
        d->lastPixmap = VirtualDesktop::grabScreen(Application::controlScreen(), d->rect);
    }

    // 等比缩放并保持宽高比，避免镜像画面被拉伸变形。
    if (!d->lastPixmap.isNull())
    {
        d->lastPixmap = d->lastPixmap.scaled(width(), height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
}

void ScreenMirror::setSourceWidget(QWidget *sourceWidget)
{
    d->sourceWidget = sourceWidget;

    grabPixmap();

    update();
}

void ScreenMirror::setSourceRect(const QRect &pRect)
{
    d->rect         = pRect;
    d->sourceWidget = nullptr;
}

void ScreenMirror::start()
{
    qDebug() << "mirroring START";
    // 镜像期间冻结网页控件，避免每帧抓屏都触发网页重绘而拖慢整体帧率。
    Application::boardController->freezeWebWidgets(true);
    if (d->timerID == 0)
    {
        int ms = 125;

        bool success;
        int  fps = Settings::settings()->mirroringRefreshRateInFps->get().toInt(&success);

        // 设置项有效时按帧率换算间隔，否则退回 8fps 的默认值。
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
