/**
 * @file CircleFrame.h
 * @brief 提供绘制当前笔宽比例圆点的预览框。
 */

#pragma once

#include <QtWidgets>
#include <QFrame>

#include <memory>

/**
 * @brief 笔宽预览框：在方框中央画一个大小与当前笔宽成正比的黑色圆点。
 *
 * PreferencesController 在“显示设置”里为细/中/粗三档笔宽各创建一个实例，
 * 作为单选按钮旁的示意图形。它只负责绘制，不参与交互，生命周期随父控件。
 */
class CircleFrame : public QFrame
{
public:
    /**
     * @brief 构造预览框，笔宽初值为 0。
     * @param parent 父控件
     */
    CircleFrame(QWidget *parent);
    virtual ~CircleFrame();

    /** @brief 当前笔宽（用于计算圆点直径）。 */
    qreal currentPenWidth() const;
    /**
     * @brief 设置当前笔宽。
     * @param width 笔宽值
     * @note 不会自动重绘，调用方需自行触发 update()。
     */
    void setCurrentPenWidth(qreal width);

    /** @brief 计算圆点直径时参照的最大笔宽。 */
    qreal maxPenWidth() const;
    /**
     * @brief 设置最大笔宽，作为圆点直径的归一化基准。
     * @param width 最大笔宽；应大于 0，否则绘制时会出现除零
     */
    void setMaxPenWidth(qreal width);

protected:
    virtual void paintEvent(QPaintEvent *event);

private:
    struct Private;
    std::unique_ptr<Private> d;
};
