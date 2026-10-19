#pragma once

#include <QtWidgets>

class ResizableGraphicsItem {
public:
    ResizableGraphicsItem();
    virtual ~ResizableGraphicsItem();

    virtual void resize(const QSizeF &pSize);
    virtual void resize(qreal w, qreal h) = 0;

    virtual QSizeF size() const = 0;
};
