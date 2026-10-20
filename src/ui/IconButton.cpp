#include <QtWidgets>

#include "ui/IconButton.h"

IconButton::IconButton(QWidget *parent, const QIcon &icon) : QAbstractButton(parent), mToggleable(false)
{
    setIcon(icon);
    // 按钮始终可勾选；是否真的允许取消勾选由 mToggleable 决定。
    setCheckable(true);
    // 用图标实际大小作为按钮尺寸基准，最大取 128。
    mIconSize = icon.actualSize(QSize(128, 128));
}

void IconButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    // 选中时改用 Selected 模式取图，这是按钮唯一的选中态反馈。
    QPixmap pixmap = icon().pixmap(mIconSize, isChecked() ? QIcon::Selected : QIcon::Normal);
    painter.drawPixmap((width() - pixmap.width()) / 2, 0, pixmap);
}

void IconButton::mousePressEvent(QMouseEvent *event)
{
    // 不可切换时按下即保持选中，表现成单选按钮。
    if (!mToggleable)
    {
        setChecked(true);
    }

    QAbstractButton::mousePressEvent(event);
}

void IconButton::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    emit doubleClicked();
}

QSize IconButton::minimumSizeHint() const
{
    return mIconSize;
}
