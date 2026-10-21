/**
 * @file Magnifier.h
 * @brief 放大镜部件的声明：截取画布局部区域并放大显示，同时提供缩放、换形状与关闭等操作。
 */

#pragma once

#include <QtWidgets>
#include <QWidget>

#include <memory>

/**
 * @brief 放大镜的运行时参数。
 *
 * 由 Magnifier 内部持有，尺寸或倍率变化时更新，不参与持久化。
 */
class MagnifierParams
{
public:
    int   x;                    ///< 镜面对象的水平坐标（当前实现未使用）。
    int   y;                    ///< 镜面对象的垂直坐标（当前实现未使用）。
    qreal zoom;                 ///< 放大倍率，1.0 表示与画布 1:1 显示。
    qreal sizePercentFromScene; ///< 镜面尺寸占画布宽度的百分比。
};

/**
 * @brief 悬浮放大镜部件。
 *
 * 界面上是一个圆形或圆角矩形的半透明镜面，内部实时渲染 board 场景的局部放大图；
 * 交互模式下底部还有放大、缩小、切换形状、拖动缩放和关闭按钮。由
 * GraphicsScene::addMagnifier 创建：主控屏那份为交互模式并挂在 BoardController
 * 的 controlContainer 下，展示屏那份为非交互模式并挂在 displayView 下。关闭按钮只发
 * 信号，实际销毁由 BoardController 完成，因此它不是常驻部件。
 */
class Magnifier : public QWidget
{
    Q_OBJECT

public:
    /// 镜面形状；modesCount 仅用于在两种形状之间循环取模。
    enum DrawingMode
    {
        circular = 0, ///< 圆形镜面。
        rectangular,  ///< 圆角矩形（宽高比约 3:1）镜面。
        modesCount    ///< 形状总数，取模循环用。
    };

public:
    /**
     * @brief 构造放大镜。
     * @param parent 父部件；为空时按置顶无边框工具窗口创建。
     * @param isInteractive 是否绘制并响应缩放/换形状/关闭按钮，展示屏那份为 false。
     */
    Magnifier(QWidget *parent = 0, bool isInteractive = false);
    ~Magnifier();

    /**
     * @brief 按画布宽度的百分比设置镜面尺寸。
     * @param percentFromScene 尺寸百分比，内部会重新计算按钮位置与掩码。
     * @note 抓取视图或移动视图尚未设置时直接返回。
     */
    void setSize(qreal percentFromScene);
    /// 依据当前形状重建圆形/圆角矩形掩码，并刷新带掩码的绘制缓存。
    void createMask();
    /// 设置放大倍率，只记录数值，下一次抓图时生效。
    void setZoom(qreal zoom);
    /// 返回当前放大倍率。
    qreal zoom() const;

    /**
     * @brief 设置被放大的源视图。
     * @note 同时以 40ms 间隔启动内部刷新定时器，持续重新抓取画面。
     */
    void setGrabView(QWidget *view);
    /// 设置用于把全局坐标换算到屏幕位置的视图（决定镜面跟随哪个坐标系）。
    void setMoveView(QWidget *view);
    /**
     * @brief 切换圆形/矩形镜面。
     * @param mode DrawingMode 的整型值，并写入设置以便下次启动沿用。
     * @note 会触发 magnifierResized_Signal 并请求场景重新摆放放大镜。
     */
    void setDrawingMode(int mode);

    /// 用上一次记录的抓取点刷新镜面内容。
    void grabPoint();
    /// 抓取以 point 为中心的局部场景并绘制到镜面缓存。
    void grabPoint(const QPoint &point);
    /**
     * @brief 同时完成抓图与移动。
     * @param pGrab 全局坐标下的抓取中心。
     * @param pMove 全局坐标下的镜面中心。
     * @param needGrab 是否需要重新抓图。
     * @param needMove 是否需要移动镜面。
     */
    void grabNMove(const QPoint &pGrab, const QPoint &pMove, bool needGrab = true, bool needMove = true);

signals:
    /// 用户拖动镜面后发出新的中心点（主控屏全局坐标）。
    void magnifierMoved_Signal(QPoint newPos);
    /// 点击关闭按钮；由场景负责真正删除放大镜。
    void magnifierClose_Signal();
    /// 点击放大按钮，由场景调整 zoom 后重新抓图。
    void magnifierZoomIn_Signal();
    /// 点击缩小按钮。
    void magnifierZoomOut_Signal();
    /// 缩放或切换形状导致尺寸变化，携带新的尺寸百分比。
    void magnifierResized_Signal(qreal newPercentSize);
    /// 请求切换到下一种形状，携带 DrawingMode 值。
    void magnifierDrawingModeChange_Signal(int mode);

public slots:
    /// 定时刷新槽：重新抓图，并在鼠标离开缩放手柄时恢复光标。
    void slot_refresh();

private:
    /// 依据当前尺寸重新排布底部按钮的命中矩形。
    void calculateButtonsPositions();

protected:
    /// 绘制镜面底色、缓存画面与（交互模式下的）按钮图标。
    void paintEvent(QPaintEvent *);

    /// 按下时区分“拖动镜面”和“拖拽缩放手柄”。
    virtual void mousePressEvent(QMouseEvent *event);
    /// 拖动镜面、拖拽缩放或切换缩放手柄光标。
    virtual void mouseMoveEvent(QMouseEvent *event);
    /// 松开时命中按钮则发出对应信号。
    virtual void mouseReleaseEvent(QMouseEvent *event);

private:
    struct Private;
    std::unique_ptr<Private> d;
};
