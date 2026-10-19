#include <QtWidgets>

#include "ui/IconButton.h"


IconButton::IconButton(QWidget *parent, const QIcon &icon) : QAbstractButton(parent), mToggleable(false) {
    setIcon(icon);
    setCheckable(true);
    mIconSize = icon.actualSize(QSize(128, 128));
}

void IconButton::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    QPixmap pixmap = icon().pixmap(mIconSize, isChecked() ? QIcon::Selected : QIcon::Normal);
    painter.drawPixmap((width() - pixmap.width()) / 2, 0, pixmap);
}

void IconButton::mousePressEvent(QMouseEvent *event) {
    if (!mToggleable)
        setChecked(true);

    QAbstractButton::mousePressEvent(event);
}

void IconButton::mouseDoubleClickEvent(QMouseEvent *event) {
    Q_UNUSED(event);

    emit doubleClicked();
}

QSize IconButton::minimumSizeHint() const {
    return mIconSize;
}
