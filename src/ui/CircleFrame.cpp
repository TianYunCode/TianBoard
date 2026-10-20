#include "ui/CircleFrame.h"

struct CircleFrame::Private
{
    qreal currentPenWidth{0};
    qreal maxPenWidth{0};
};

CircleFrame::CircleFrame(QWidget *parent) : QFrame(parent), d(new Private)
{
}

CircleFrame::~CircleFrame() = default;

qreal CircleFrame::currentPenWidth() const
{
    return d->currentPenWidth;
}

void CircleFrame::setCurrentPenWidth(qreal width)
{
    d->currentPenWidth = width;
}

qreal CircleFrame::maxPenWidth() const
{
    return d->maxPenWidth;
}

void CircleFrame::setMaxPenWidth(qreal width)
{
    d->maxPenWidth = width;
}

void CircleFrame::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QBrush(Qt::black));

    // 直径按“当前笔宽 / 最大笔宽”归一化；下限 2 像素保证最细的笔也能看见。
    qreal diameter = (width() - 2) / d->maxPenWidth * d->currentPenWidth;
    diameter       = qMax(2.0, diameter);
    qreal x        = (width() - diameter) / 2;

    painter.drawEllipse(x, x, diameter, diameter);
}
