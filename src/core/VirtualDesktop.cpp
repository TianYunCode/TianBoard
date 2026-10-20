#include "core/VirtualDesktop.h"

#include <QGuiApplication>
#include <QScreen>
#include <qt_windows.h>

struct VirtualDesktop::Private
{
    int mLastScreenCount{-1};
};

VirtualDesktop *VirtualDesktop::instance()
{
    static VirtualDesktop *sInstance(0);

    if (!sInstance)
    {
        sInstance = new VirtualDesktop;
    }

    return sInstance;
}

VirtualDesktop::VirtualDesktop() : QWidget(), d(new Private)
{
    // Qt6 没有 QGuiApplication::screensChanged，屏幕插拔只能靠原生 WM_DISPLAYCHANGE。
    qApp->installNativeEventFilter(this);

    connect(qApp, &QGuiApplication::primaryScreenChanged, this, &VirtualDesktop::onPrimaryScreenChanged);

    syncScreens();
}

VirtualDesktop::~VirtualDesktop() = default;

bool VirtualDesktop::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(result);

    MSG *msg = static_cast<MSG *>(message);
    if (msg && msg->message == WM_DISPLAYCHANGE)
    {
        syncScreens();
    }

    return false;
}

void VirtualDesktop::onPrimaryScreenChanged(QScreen *screen)
{
    Q_UNUSED(screen);
    syncScreens();
}

void VirtualDesktop::syncScreens()
{
    const QList<QScreen *> screens = QGuiApplication::screens();

    foreach (QScreen *scr, screens)
    {
        disconnect(scr, 0, this, 0);
        connect(scr, &QScreen::geometryChanged, this, &VirtualDesktop::onScreenGeometryChanged);
        connect(scr, &QScreen::availableGeometryChanged, this, &VirtualDesktop::onAvailableGeometryChanged);
    }

    // 本窗口从不 show，但 RecordingController / DesktopAnnotationController 会读它的
    // geometry()/size()，所以手动跟住虚拟桌面的外接矩形。
    setGeometry(virtualGeometry());

    const int count = screens.size();
    if (d->mLastScreenCount != -1 && count != d->mLastScreenCount)
    {
        emit screenCountChanged(count);
    }

    d->mLastScreenCount = count;
}

void VirtualDesktop::onScreenGeometryChanged()
{
    setGeometry(virtualGeometry());
    emit resized(indexOf(qobject_cast<QScreen *>(sender())));
}

void VirtualDesktop::onAvailableGeometryChanged()
{
    emit workAreaResized(indexOf(qobject_cast<QScreen *>(sender())));
}

int VirtualDesktop::indexOf(const QScreen *scr) const
{
    return QGuiApplication::screens().indexOf(const_cast<QScreen *>(scr));
}

int VirtualDesktop::numScreens() const
{
    return QGuiApplication::screens().size();
}

int VirtualDesktop::primaryScreen() const
{
    const int index = indexOf(QGuiApplication::primaryScreen());
    return index == -1 ? 0 : index;
}

int VirtualDesktop::normalizeScreen(int screen) const
{
    if (screen == -1)
    {
        return -1;
    }

    const int count = QGuiApplication::screens().size();

    if (screen < 0 || screen >= count)
    {
        return primaryScreen();
    }

    return screen;
}

QScreen *VirtualDesktop::screen(int screen) const
{
    const QList<QScreen *> screens = QGuiApplication::screens();
    const int              index   = normalizeScreen(screen);

    return (index >= 0 && index < screens.size()) ? screens.at(index) : QGuiApplication::primaryScreen();
}

QRect VirtualDesktop::screenGeometry(int screen) const
{
    const int index = normalizeScreen(screen);

    if (index == -1)
    {
        return virtualGeometry();
    }

    const QList<QScreen *> screens = QGuiApplication::screens();
    return (index < screens.size()) ? screens.at(index)->geometry() : QRect();
}

QRect VirtualDesktop::screenGeometry(const QPoint &point) const
{
    return screenGeometry(screenNumber(point));
}

QRect VirtualDesktop::screenGeometry(const QWidget *widget) const
{
    return screenGeometry(screenNumber(widget));
}

QRect VirtualDesktop::availableGeometry(int screen) const
{
    const int index = normalizeScreen(screen);

    if (index == -1)
    {
        return virtualGeometry();
    }

    const QList<QScreen *> screens = QGuiApplication::screens();
    return (index < screens.size()) ? screens.at(index)->availableGeometry() : QRect();
}

QRect VirtualDesktop::availableGeometry(const QPoint &point) const
{
    return availableGeometry(screenNumber(point));
}

QRect VirtualDesktop::availableGeometry(const QWidget *widget) const
{
    return availableGeometry(screenNumber(widget));
}

int VirtualDesktop::screenNumber(const QWidget *widget) const
{
    if (!widget)
    {
        return -1;
    }

    const int index = indexOf(widget->screen());
    return index == -1 ? primaryScreen() : index;
}

int VirtualDesktop::screenNumber(const QPoint &point) const
{
    const QList<QScreen *> screens = QGuiApplication::screens();

    for (int i = 0; i < screens.size(); i++)
    {
        if (screens.at(i)->geometry().contains(point))
        {
            return i;
        }
    }

    return primaryScreen();
}

QRect VirtualDesktop::virtualGeometry() const
{
    const QScreen *primary = QGuiApplication::primaryScreen();
    if (primary)
    {
        return primary->virtualGeometry();
    }

    QRect unionRect;
    foreach (QScreen *scr, QGuiApplication::screens())
    {
        unionRect = unionRect.united(scr->geometry());
    }

    return unionRect;
}

int VirtualDesktop::physicalDpi() const
{
    const QScreen *primary = QGuiApplication::primaryScreen();
    return primary ? qRound((primary->physicalDotsPerInchX() + primary->physicalDotsPerInchY()) / 2.0) : 96;
}

QPixmap VirtualDesktop::grabScreen(QScreen *screen, const QRect &globalRect)
{
    if (!screen)
    {
        screen = QGuiApplication::primaryScreen();
    }

    if (!screen)
    {
        return QPixmap();
    }

    // QWindowsScreen::grabWindow(0, ...) 会把传入坐标加上该屏原点，所以这里要先把
    // 全局坐标折算成屏内坐标。
    const QPoint origin = screen->geometry().topLeft();

    return screen->grabWindow(0, globalRect.x() - origin.x(), globalRect.y() - origin.y(), globalRect.width(), globalRect.height());
}
