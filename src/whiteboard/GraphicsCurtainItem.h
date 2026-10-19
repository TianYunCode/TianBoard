#pragma once

#include <QtWidgets>

#include "utils/Enums.h"

#include "whiteboard/Item.h"

class GraphicsItemDelegate;

class GraphicsCurtainItem : public QObject, public QGraphicsRectItem, public Item, public GraphicsItem {
    Q_OBJECT

public:
    GraphicsCurtainItem(QGraphicsItem *parent = 0);
    virtual ~GraphicsCurtainItem();

    enum { Type = GraphicsItemType::CurtainItemType };

    virtual int type() const { return Type; }

    virtual Item *deepCopy() const;
    virtual void copyItemParameters(Item *copy) const;

    void triggerRemovedSignal();
    virtual void clearSource() { ; }

    virtual void setUuid(const QUuid &pUuid);

signals:
    void removed();

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    QColor drawColor() const;
    QColor opaqueControlColor() const;

    //        GraphicsItemDelegate* mDelegate;

    static const QColor sDrawColor;
    static const QColor sDarkBackgroundDrawColor;
    static const QColor sOpaqueControlColor;
    static const QColor sDarkBackgroundOpaqueControlColor;
};
