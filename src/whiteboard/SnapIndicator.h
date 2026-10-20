/**
 * @file SnapIndicator.h
 * @brief 吸附命中时在视图上标出的那一角参考图形。
 */

#pragma once

#include <QLabel>

/// @brief 网格吸附命中后贴在吸附点上的小三角标记，出现后一秒钟内淡出自消。
class SnapIndicator : public QLabel
{
    Q_OBJECT

    /// @brief 供 QPropertyAnimation 驱动的不透明度，0 到 255。
    Q_PROPERTY(int alpha READ alpha WRITE setAlpha)

public:
    /// @brief 挂到画板视图上；父控件必须是 BoardView，否则无处换算场景坐标。
    explicit SnapIndicator(QWidget *parent);
    ~SnapIndicator() override;

    /**
     * @brief 在场景坐标处亮一次。
     * @param corner 吸附命中的是图元哪个角，决定三角朝哪边张开。
     * @param snapPoint 吸附点（场景坐标）。
     * @param angle 图元那条边的角度，标记跟着它转，图元歪着时也不脱开。
     */
    void appear(Qt::Corner corner, const QPointF &snapPoint, qreal angle = 0);

    /// @brief 当前不透明度。
    int alpha() const;
    /// @brief 设置不透明度并重绘。
    void setAlpha(int opacity);

    /// @brief 换标记颜色；不传就用默认的浅蓝。
    void setColor(const QColor &color);

protected:
    /// @brief 绘制那块随角度与角落转向的三角形。
    void paintEvent(QPaintEvent *event) override;

private:
    struct Private;   ///< Pimpl：只装动画句柄、角度、不透明度与颜色。
    Private *const d; ///< 私有数据
};
