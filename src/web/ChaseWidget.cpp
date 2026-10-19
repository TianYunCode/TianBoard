#include "web/ChaseWidget.h"

#include <QtWidgets>
#include <QApplication>


struct ChaseWidget::Private {
    int mSegment{0};
    int mDelay{0};
    int mStep{0};
    int mTimerID{0};
    bool mAnimated{false};
    QPixmap mPixmap;
    bool mPixmapEnabled{false};
};

ChaseWidget::ChaseWidget(QWidget *parent, QPixmap pixmap, bool pixmapEnabled)
    : QWidget(parent), d(new Private) {
    d->mDelay = 100;
    d->mStep = 40;
    d->mTimerID = -1;
    d->mPixmap = pixmap;
    d->mPixmapEnabled = pixmapEnabled;
}

ChaseWidget::~ChaseWidget() = default;


void ChaseWidget::setAnimated(bool value) {
    if (d->mAnimated == value)
        return;
    d->mAnimated = value;
    if (d->mTimerID != -1) {
        killTimer(d->mTimerID);
        d->mTimerID = -1;
    }
    if (d->mAnimated) {
        d->mSegment = 0;
        d->mTimerID = startTimer(d->mDelay);
    }
    update();
}


void ChaseWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter p(this);
    if (d->mPixmapEnabled && !d->mPixmap.isNull()) {
        p.drawPixmap(0, 0, d->mPixmap);
        return;
    }

    const int extent = qMin(width() - 8, height() - 8);
    const int displ = extent / 4;
    const int ext = extent / 4 - 1;

    p.setRenderHint(QPainter::Antialiasing, true);

    if (d->mAnimated)
        p.setPen(Qt::gray);
    else
        p.setPen(QPen(palette().dark().color()));

    p.translate(width() / 2, height() / 2); // center

    for (int segment = 0; segment < segmentCount(); ++segment) {
        p.rotate(QApplication::isRightToLeft() ? d->mStep : -d->mStep);
        if (d->mAnimated)
            p.setBrush(colorForSegment(segment));
        else
            p.setBrush(palette().brush(QPalette::Window));
        p.drawEllipse(QRect(displ, -ext / 2, ext, ext));
    }
}


QSize ChaseWidget::sizeHint() const {
    return QSize(32, 32);
}


void ChaseWidget::timerEvent(QTimerEvent *event) {
    if (event->timerId() == d->mTimerID) {
        ++d->mSegment;
        update();
    }
    QWidget::timerEvent(event);
}


QColor ChaseWidget::colorForSegment(int seg) const {
    int index = ((seg + d->mSegment) % segmentCount());
    int comp = qMax(0, 255 - (index * (255 / segmentCount())));
    return QColor(comp, comp, comp, 255);
}


int ChaseWidget::segmentCount() const {
    return 360 / d->mStep;
}


void ChaseWidget::setPixmapEnabled(bool enable) {
    d->mPixmapEnabled = enable;
}
