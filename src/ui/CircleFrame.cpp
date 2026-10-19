#include "ui/CircleFrame.h"

struct CircleFrame::Private {
    qreal currentPenWidth{0};
    qreal maxPenWidth{0};
};

CircleFrame::CircleFrame(QWidget *parent) : QFrame(parent), d(new Private) {
    // NOOP
}

CircleFrame::~CircleFrame() {
    // NOOP
}

qreal CircleFrame::currentPenWidth() const {
    return d->currentPenWidth;
}

void CircleFrame::setCurrentPenWidth(qreal width) {
    d->currentPenWidth = width;
}

qreal CircleFrame::maxPenWidth() const {
    return d->maxPenWidth;
}

void CircleFrame::setMaxPenWidth(qreal width) {
    d->maxPenWidth = width;
}


void CircleFrame::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QBrush(Qt::black));

    qreal diameter = (width() - 2) / d->maxPenWidth * d->currentPenWidth;
    diameter = qMax(2.0, diameter);
    qreal x = (width() - diameter) / 2;

    painter.drawEllipse(x, x, diameter, diameter);
}
