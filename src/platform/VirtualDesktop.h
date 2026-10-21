/**
 * @file VirtualDesktop.h
 * @brief 声明虚拟桌面接口：屏幕枚举、几何查询、DPI 与屏幕抓取。
 */

#pragma once

#include <QWidget>
#include <QAbstractNativeEventFilter>

#include <memory>

class QScreen;

/**
 * @brief 虚拟桌面：以单例形式提供 QScreen 之上的屏幕查询接口与屏幕抓取能力。
 *
 * Qt6 移除了 QDesktopWidget。本类用 QScreen 重新实现 TianBoard 用到的那一小部分接口，
 * 语义对齐 Qt5：screen 编号是 QGuiApplication::screens() 的下标，-1 表示整个虚拟桌面，
 * 其它越界值落到主屏。
 *
 * 必须继承 QWidget 而不是 QObject：RecordingController 把 instance() 返回的指针本身
 * 当作“录制整块屏幕而不是某个控件”的哨兵值来比较，并且依赖它的 geometry()/size()。
 *
 * 生命周期覆盖整个进程，首次 instance() 时创建；通过原生事件过滤器跟踪屏幕变化。
 * 线程约束：GUI 线程专用。
 */
class VirtualDesktop : public QWidget, public QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    /// 返回进程内唯一实例。
    static VirtualDesktop *instance();

    /// 返回当前屏幕数量。
    int numScreens() const;
    /// numScreens 的同义接口。
    int screenCount() const
    {
        return numScreens();
    }
    /// 返回主屏下标；没有主屏时返回 0。
    int primaryScreen() const;

    /**
     * @brief 返回指定屏幕。
     * @param screen 屏幕下标；-1 表示虚拟桌面（此时返回主屏）。
     * @return 屏幕对象；越界下标回退到主屏。
     */
    QScreen *screen(int screen = -1) const;

    /**
     * @brief 返回屏幕几何区域。
     * @param screen 屏幕下标；-1 返回整个虚拟桌面的外接矩形。
     */
    QRect screenGeometry(int screen = -1) const;
    /// 返回包含该点的屏幕几何区域。
    QRect screenGeometry(const QPoint &point) const;
    /// 返回控件所在屏幕的几何区域。
    QRect screenGeometry(const QWidget *widget) const;

    /**
     * @brief 返回屏幕可用工作区（扣除任务栏等）。
     * @param screen 屏幕下标；-1 返回整个虚拟桌面的外接矩形。
     */
    QRect availableGeometry(int screen = -1) const;
    /// 返回包含该点的屏幕可用工作区。
    QRect availableGeometry(const QPoint &point) const;
    /// 返回控件所在屏幕的可用工作区。
    QRect availableGeometry(const QWidget *widget) const;

    /// 返回所有屏幕的外接矩形。
    QRect virtualGeometry() const;

    /// 返回控件所在屏幕下标；控件为空时返回 -1。
    int screenNumber(const QWidget *widget) const;
    /// 返回包含该点的屏幕下标；不在任何屏幕内时返回主屏。
    int screenNumber(const QPoint &point) const;

    // 场景像素折算成点/英寸时用的 DPI。原来散在导出、导入、SVG 读写器与画板里的
    // 11 处各自写 (X+Y)/2 或只取 Y，同一份文档可能按两套比例折算，现在只有这一个出口。
    // 取哪个屏仍未定：BoardView::resizeEvent 按“最后 resize 的那个视图”决定 sceneRect，
    // 并不存在唯一的“画板所在屏”，所以先固定取主屏。
    /// 返回用于像素与物理尺寸换算的 DPI，取主屏横纵 DPI 的平均值。
    int physicalDpi() const;

    // Qt6 移除了 QPixmap::grabWindow() 和 QWidget::effectiveWinId()，屏幕抓取统一走
    // QScreen::grabWindow(0, ...)，且坐标必须相对于该屏原点，而调用方手里的是全局坐标。
    /**
     * @brief 抓取指定屏幕的一块区域。
     * @param screen 目标屏幕；为空时用主屏。
     * @param globalRect 全局坐标下的矩形。
     * @return 抓取到的图像；无可用屏幕时为空。
     */
    static QPixmap grabScreen(QScreen *screen, const QRect &globalRect);

    /// 监听 WM_DISPLAYCHANGE 并刷新屏幕列表。
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

signals:
    /// 某屏幕的工作区尺寸变化。
    void workAreaResized(int screen);
    /// 某屏幕的几何区域变化。
    void resized(int screen);
    /// 屏幕数量变化。
    void screenCountChanged(int numberOfScreens);

private slots:
    /// 屏幕几何变化后同步虚拟桌面矩形并发出 resized。
    void onScreenGeometryChanged();
    /// 转发出工作区变化信号。
    void onAvailableGeometryChanged();
    /// 主屏切换后重新同步。
    void onPrimaryScreenChanged(QScreen *screen);

private:
    VirtualDesktop();
    ~VirtualDesktop() override;
    Q_DISABLE_COPY(VirtualDesktop)

    /// 重新连接所有屏幕信号并刷新缓存的几何信息。
    void syncScreens();
    /// 返回屏幕在列表中的下标；未知屏幕返回 -1。
    int indexOf(const QScreen *scr) const;
    /// 把 -1 与越界下标规整为 -1 或主屏下标。
    int normalizeScreen(int screen) const;

    struct Private;
    std::unique_ptr<Private> d;
};
