/**
 * @file IconButton.h
 * @brief 提供只绘制图标、可切换选中状态的小按钮。
 */

#pragma once

#include <QAbstractButton>

/**
 * @brief 仅绘制图标的抽象按钮，选中态使用图标的 Selected 模式。
 *
 * WebToolsPalette 用它做收藏/主页等入口：按钮可勾选，普通模式下按下即选中，
 * 关闭 toggleable 后由外部维护选中状态。按钮尺寸由图标实际大小决定。
 */
class IconButton : public QAbstractButton
{
    Q_OBJECT

public:
    /**
     * @brief 构造按钮。
     * @param parent 父控件
     * @param iconOn 显示的图标
     */
    IconButton(QWidget *parent = 0, const QIcon &iconOn = QIcon());

    /**
     * @brief 设置按住时是否切换选中状态。
     * @param pToggleable true 表示切换；false 表示按下后保持选中
     */
    void setToggleable(bool pToggleable)
    {
        mToggleable = pToggleable;
    }

protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseDoubleClickEvent(QMouseEvent *event);
    QSize        minimumSizeHint() const;

private:
    QSize mIconSize;
    bool  mToggleable;

signals:
    /** @brief 按钮被双击。 */
    void doubleClicked();
};
