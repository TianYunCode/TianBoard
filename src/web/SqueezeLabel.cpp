#include "web/SqueezeLabel.h"


struct SqueezeLabel::Private {};

SqueezeLabel::SqueezeLabel(QWidget *parent) : QLabel(parent), d(new Private) {}

SqueezeLabel::~SqueezeLabel() = default;

void SqueezeLabel::paintEvent(QPaintEvent *event) {
    QFontMetrics fm = fontMetrics();
    if (fm.horizontalAdvance(text()) > contentsRect().width()) {
        QString elided = fm.elidedText(text(), Qt::ElideMiddle, width());
        QString oldText = text();
        setText(elided);
        QLabel::paintEvent(event);
        setText(oldText);
    } else {
        QLabel::paintEvent(event);
    }
}
