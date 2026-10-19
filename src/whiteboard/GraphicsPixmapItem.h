#pragma once

#include <QtWidgets>

#include "utils/Enums.h"

#include "whiteboard/Item.h"

class GraphicsItemDelegate;

class GraphicsPixmapItem : public QObject, public QGraphicsPixmapItem, public Item, public GraphicsItem {
    Q_OBJECT

public:
    GraphicsPixmapItem(QGraphicsItem *parent = 0);
    virtual ~GraphicsPixmapItem();

    enum { Type = GraphicsItemType::PixmapItemType };

    virtual int type() const { return Type; }
    virtual Item *deepCopy() const;

    virtual void copyItemParameters(Item *copy) const;

    virtual GraphicsScene *scene();

    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

    void setOpacity(qreal op);
    qreal opacity() const;

    virtual void clearSource();

    virtual void setUuid(const QUuid &pUuid);

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
};
