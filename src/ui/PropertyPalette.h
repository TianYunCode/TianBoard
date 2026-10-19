#pragma once

#include <QMouseEvent>
#include <QPoint>

#include <memory>

#include "ui/ActionPalette.h"

class PropertyPalette : public ActionPalette {
    Q_OBJECT
public:
    PropertyPalette(QWidget *parent = 0, const char *name = "propertyPalette");
    PropertyPalette(Qt::Orientation orientation, QWidget *parent = 0);
    ~PropertyPalette();

private:
    struct Private;
    std::unique_ptr<Private> d;

private slots:
    void onMouseRelease();
};
