#pragma once

#include <QColor>
#include <QGraphicsSceneMouseEvent>

#include "whiteboard/Item.h"
#include "utils/Enums.h"

typedef enum
{
    eMaskShape_Circle,
    eMaskShap_Rectangle
}eMaskShape;

class GraphicsCache : public QGraphicsRectItem, public Item
{
public:
    static GraphicsCache* instance(GraphicsScene *scene);
    ~GraphicsCache();

    enum { Type = GraphicsItemType::cacheItemType };

    virtual int type() const{ return Type;}

    virtual Item* deepCopy() const;

    virtual void copyItemParameters(Item *copy) const;

    QColor maskColor();
    void setMaskColor(QColor color);
    eMaskShape maskshape();
    void setMaskShape(eMaskShape shape);
    int shapeWidth();
    void setShapeWidth(int width);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
    static QMap<GraphicsScene*, GraphicsCache*> sInstances;

    QColor mMaskColor;
    eMaskShape mMaskShape;
    int mShapeWidth;
    bool mDrawMask;
    QPointF mShapePos;
    int mOldShapeWidth;
    QPointF mOldShapePos;
    GraphicsScene* mScene;

    GraphicsCache(GraphicsScene *scene);
    
    void init();
    QRectF updateRect(QPointF currentPoint);
};

