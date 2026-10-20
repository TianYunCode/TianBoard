#include "ui/PropertyPalette.h"


struct PropertyPalette::Private {};

PropertyPalette::PropertyPalette(QWidget *parent, const char *name) : ActionPalette(parent), d(new Private) {
    setObjectName(name);
    setGripVisible(false);
}


PropertyPalette::PropertyPalette(Qt::Orientation orientation, QWidget *parent)
    : ActionPalette(orientation, parent), d(new Private) {
    setGripVisible(false);
}

PropertyPalette::~PropertyPalette() {}

void PropertyPalette::onMouseRelease() {}
