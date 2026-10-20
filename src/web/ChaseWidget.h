/**
 * @file ChaseWidget.h
 * @brief 声明 ChaseWidget：网页加载时显示旋转追逐点动画的小部件。
 */

#pragma once

#include <QtWidgets>
#include <QWidget>

#include <memory>

/**
 * @brief 加载动画部件：页面加载期间由 BrowserWindow 启动，显示一圈渐变圆点。
 *
 * 只存在于 web 域工具栏中，可由外部 pixmap 替换绘制内容。动画通过 QWidget 的定时器
 * 驱动，setAnimated(false) 即停止并释放定时器。
 */
class ChaseWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造部件。
     * @param parent 父控件。
     * @param pixmap 可选的静态图标，为空时绘制圆点。
     * @param pixmapEnabled 是否优先绘制静态图标。
     */
    ChaseWidget(QWidget *parent = 0, QPixmap pixmap = QPixmap(), bool pixmapEnabled = false);

    /** @brief 析构。 */
    ~ChaseWidget() override;

    /**
     * @brief 启动或停止动画。
     * @param value true 启动定时器，false 停止。
     */
    void setAnimated(bool value);

    /** @brief 切换是否绘制外部 pixmap。 */
    void setPixmapEnabled(bool enable);

    /** @brief 建议尺寸 32x32。 */
    QSize sizeHint() const;

protected:
    /** @brief 绘制圆点追逐动画或静态图标。 */
    void paintEvent(QPaintEvent *event);

    /** @brief 定时推进一段并请求重绘。 */
    void timerEvent(QTimerEvent *event);

private:
    /** @brief 圆点段数，由每段角度决定。 */
    int segmentCount() const;
    /** @brief 按当前相位计算某段的灰度颜色。 */
    QColor colorForSegment(int segment) const;

    /** @brief 私有实现。 */
    struct Private;
    std::unique_ptr<Private> d;
};
