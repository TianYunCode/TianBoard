#include <QtWidgets>
#include <QPointer>

#include "ui/WidgetMirror.h"


struct WidgetMirror::Private {
    QPointer<QWidget> sourceWidget;
};

WidgetMirror::WidgetMirror(QWidget *sourceWidget, QWidget *parent) : QWidget(parent), d(new Private) {
    d->sourceWidget = sourceWidget;
    sourceWidget->installEventFilter(this);
}

WidgetMirror::~WidgetMirror() = default;

bool WidgetMirror::eventFilter(QObject *obj, QEvent *event) {
    bool result = QObject::eventFilter(obj, event);

    // 源部件每次重绘后把同一矩形区域标记为脏区，自身随后重绘并重新渲染。
    if (event->type() == QEvent::Paint && obj == d->sourceWidget.data()) {
        QPaintEvent *paintEvent = static_cast<QPaintEvent *>(event);
        update(paintEvent->rect());
    }

    return result;
}

void WidgetMirror::paintEvent(QPaintEvent *event) {
    if (!d->sourceWidget)
        return;

    QPainter painter(this);

    painter.setBackgroundMode(Qt::OpaqueMode);
    painter.setBackground(Qt::black);

    d->sourceWidget->render(&painter, event->rect().topLeft(), QRegion(event->rect()));
}


void WidgetMirror::setSourceWidget(QWidget *sourceWidget) {
    // 先解除对旧源的监听，否则旧源继续绘制时会刷新一个已经无关的镜像。
    if (d->sourceWidget) {
        d->sourceWidget->removeEventFilter(this);
    }

    d->sourceWidget = sourceWidget;

    sourceWidget->installEventFilter(this);

    update();
}
