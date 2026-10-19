#pragma once

#include <QWidget>
#include <QAbstractNativeEventFilter>

#include <memory>

class QScreen;

// Qt6 移除了 QDesktopWidget。本类用 QScreen 重新实现 TianBoard 用到的那一小部分接口，
// 语义对齐 Qt5：screen 编号是 QGuiApplication::screens() 的下标，-1 表示整个虚拟桌面，
// 其它越界值落到主屏。
//
// 必须继承 QWidget 而不是 QObject：RecordingController 把 instance() 返回的指针本身
// 当作“录制整块屏幕而不是某个控件”的哨兵值来比较，并且依赖它的 geometry()/size()。
class VirtualDesktop : public QWidget, public QAbstractNativeEventFilter
{
    Q_OBJECT

    public:
        static VirtualDesktop* instance();

        int numScreens() const;
        int screenCount() const           { return numScreens(); }
        int primaryScreen() const;

        QScreen* screen(int screen = -1) const;

        QRect screenGeometry(int screen = -1) const;
        QRect screenGeometry(const QPoint &point) const;
        QRect screenGeometry(const QWidget *widget) const;

        QRect availableGeometry(int screen = -1) const;
        QRect availableGeometry(const QPoint &point) const;
        QRect availableGeometry(const QWidget *widget) const;

        QRect virtualGeometry() const;

        int screenNumber(const QWidget *widget) const;
        int screenNumber(const QPoint &point) const;

        int physicalDpiX() const;
        int physicalDpiY() const;

        // Qt6 移除了 QPixmap::grabWindow() 和 QWidget::effectiveWinId()，屏幕抓取统一走
        // QScreen::grabWindow(0, ...)，且坐标必须相对于该屏原点，而调用方手里的是全局坐标。
        static QPixmap grabScreen(QScreen *screen, const QRect &globalRect);

        bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

    signals:
        void workAreaResized(int screen);
        void resized(int screen);
        void screenCountChanged(int numberOfScreens);

    private slots:
        void onScreenGeometryChanged();
        void onAvailableGeometryChanged();
        void onPrimaryScreenChanged(QScreen *screen);

    private:
        VirtualDesktop();
        ~VirtualDesktop() override;
        Q_DISABLE_COPY(VirtualDesktop)

        void syncScreens();
        int indexOf(const QScreen *scr) const;
        int normalizeScreen(int screen) const;

        struct Private;
        std::unique_ptr<Private> d;
};

