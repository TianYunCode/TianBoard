#pragma once

#include <QtWidgets>

#include "whiteboard/Item.h"
#include "whiteboard/ResizableGraphicsItem.h"

class GraphicsItemDelegate;

class GraphicsProxyWidget: public QGraphicsProxyWidget, public Item, public ResizableGraphicsItem, public GraphicsItem
{
    public:
        virtual ~GraphicsProxyWidget();

        virtual void resize(qreal w, qreal h);
        virtual void resize(const QSizeF & size);

        virtual QSizeF size() const;

        virtual GraphicsScene* scene();

        virtual void setUuid(const QUuid &pUuid);

    protected:
        GraphicsProxyWidget(QGraphicsItem* parent = 0);

        virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
        virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
        virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
        virtual void wheelEvent(QGraphicsSceneWheelEvent *event);
        virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
        virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

        virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
        virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

};

