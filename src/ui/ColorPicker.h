/**
 * @file ColorPicker.h
 * @brief 提供用色块循环切换预设颜色的取色控件。
 */

#pragma once

#include <QtWidgets>
#include <QFrame>

#include <memory>

/**
 * @brief 预设颜色取色框：显示当前颜色，左键点击切换到下一个颜色。
 *
 * PreferencesController 创建多个实例，用于背景色、网格色等偏好设置；控件只负责
 * 维护颜色列表与选中下标，并通过 colorSelected() 通知外部，自己不改任何业务状态。
 */
class ColorPicker : public QFrame
{
    Q_OBJECT

public:
    /**
     * @brief 构造没有颜色的取色框。
     * @param parent 父控件
     */
    ColorPicker(QWidget *parent);
    /**
     * @brief 用给定颜色列表构造取色框。
     * @param parent 父控件
     * @param colors 可选颜色
     * @param pSelectedColorIndex 初始选中下标
     */
    ColorPicker(QWidget *parent, const QList<QColor> &colors, int pSelectedColorIndex = 0);
    virtual ~ColorPicker();
    /** @brief 返回当前颜色列表。 */
    QList<QColor> getColors() const;

    /**
     * @brief 替换颜色列表并立即重绘。
     * @param pColors 新颜色列表
     * @note 不校正 selectedColorIndex，越界时 paintEvent 会跳过绘制。
     */
    void setColors(const QList<QColor> &pColors);

    /** @brief 当前选中颜色下标，未选择时为 -1。 */
    int selectedColorIndex() const;

    /**
     * @brief 设置选中下标并立即重绘。
     * @param pSelectedColorIndex 新下标；越界时不会绘制色块
     */
    void setSelectedColorIndex(int pSelectedColorIndex);

signals:
    /** @brief 点击切换到新颜色后发出。 */
    void colorSelected(const QColor &color);

protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);

private:
    struct Private;
    std::unique_ptr<Private> d;
};
