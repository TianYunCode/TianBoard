/**
 * @file SqueezeLabel.h
 * @brief 声明 SqueezeLabel：文本过长时自动中间省略的标签。
 */

#pragma once

#include <QLabel>

#include <memory>


/**
 * @brief 自动省略标签：宽度不足时用 QFontMetrics 做中间省略，避免撑破下载条布局。
 *
 * 只在绘制期间临时替换文本，绘制完立即恢复，因此 text() 始终返回原文。
 * 用于下载项的文件名与进度信息。
 */
class SqueezeLabel : public QLabel {
    Q_OBJECT;

public:
    /** @brief 构造标签。 */
    SqueezeLabel(QWidget *parent = 0);

    /** @brief 析构。 */
    ~SqueezeLabel() override;

protected:
    /** @brief 文本超宽时以省略形式绘制，否则按默认方式绘制。 */
    void paintEvent(QPaintEvent *event);

private:
    /** @brief 私有实现占位。 */
    struct Private;
    std::unique_ptr<Private> d;
};
