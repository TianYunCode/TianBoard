#pragma once

#include <QGraphicsItem>

#include "whiteboard/Item.h"
#include "whiteboard/CoreGraphicsScene.h"

class GraphicsGroupContainerItem : public QGraphicsItem, public Item, public GraphicsItem {
public:
    GraphicsGroupContainerItem(QGraphicsItem *parent = 0);
    virtual ~GraphicsGroupContainerItem();

    void addToGroup(QGraphicsItem *item);
    void removeFromGroup(QGraphicsItem *item);
    void setCurrentItem(QGraphicsItem *item) { mCurrentItem = item; }
    QGraphicsItem *getCurrentItem() const { return mCurrentItem; }
    void deselectCurrentItem();

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    virtual CoreGraphicsScene *corescene();
    GraphicsGroupContainerItem *deepCopyNoChildDuplication() const;
    virtual GraphicsGroupContainerItem *deepCopy() const;
    virtual void copyItemParameters(Item *copy) const;

    enum { Type = GraphicsItemType::groupContainerType };

    virtual int type() const { return Type; }

    virtual void setUuid(const QUuid &pUuid);
    void destroy(bool canUndo = true);

    virtual void clearSource();

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    void pRemoveFromGroup(QGraphicsItem *item);

private:
    QRectF itemsBoundingRect;
    QGraphicsItem *mCurrentItem;
};
