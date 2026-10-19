#include "ui/DockPaletteWidget.h"


struct DockPaletteWidget::Private {
    QPixmap mIconToRight;
    QPixmap mIconToLeft;
    QString mName;

    QVector<DockWidgetMode> mRegisteredModes;

    bool mVisibleState{false};
};

DockPaletteWidget::DockPaletteWidget(QWidget *parent, const char *name) : QWidget(parent), d(new Private) {
    setObjectName(name);
}

DockPaletteWidget::~DockPaletteWidget() {}

QPixmap DockPaletteWidget::iconToRight() {
    return d->mIconToRight;
}

QPixmap DockPaletteWidget::iconToLeft() {
    return d->mIconToLeft;
}

QString DockPaletteWidget::name() {
    return d->mName;
}

void DockPaletteWidget::setName(const QString &name) {
    d->mName = name;
}

void DockPaletteWidget::setIconToRight(const QPixmap &icon) {
    d->mIconToRight = icon;
}

void DockPaletteWidget::setIconToLeft(const QPixmap &icon) {
    d->mIconToLeft = icon;
}

bool DockPaletteWidget::visibleState() {
    return d->mVisibleState;
}

void DockPaletteWidget::setVisibleState(bool state) {
    d->mVisibleState = state;
}

/**
  * When a widget registers a mode it means that it would be displayed on that mode
  */
void DockPaletteWidget::registerMode(DockWidgetMode mode) {
    if (!d->mRegisteredModes.contains(mode))
        d->mRegisteredModes.append(mode);
}

void DockPaletteWidget::slot_changeMode(DockWidgetMode newMode) {
    this->setVisible(this->visibleInMode(newMode));
}
