#pragma once

#include <QAbstractButton>

class IconButton : public QAbstractButton {
    Q_OBJECT

public:
    IconButton(QWidget *parent = 0, const QIcon &iconOn = QIcon());

    void setToggleable(bool pToggleable) { mToggleable = pToggleable; }

protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseDoubleClickEvent(QMouseEvent *event);
    QSize minimumSizeHint() const;

private:
    QSize mIconSize;
    bool mToggleable;

signals:
    void doubleClicked();
};
