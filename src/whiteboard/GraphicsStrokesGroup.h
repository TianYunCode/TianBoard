#pragma once

#include <QGraphicsItemGroup>
#include <QGraphicsSceneMouseEvent>

#include "utils/Enums.h"
#include "whiteboard/Item.h"

class GraphicsStrokesGroup : public QObject, public QGraphicsItemGroup, public Item, public GraphicsItem {
    Q_OBJECT
public:
    enum colorType { currentColor = 0, colorOnLightBackground, colorOnDarkBackground };

    GraphicsStrokesGroup(QGraphicsItem *parent = 0);
    ~GraphicsStrokesGroup();
    virtual Item *deepCopy() const;
    virtual void copyItemParameters(Item *copy) const;
    enum { Type = GraphicsItemType::StrokeItemType };
    virtual int type() const { return Type; }
    virtual void setUuid(const QUuid &pUuid);
    void setColor(const QColor &color, colorType pColorType = currentColor);
    QColor color(colorType pColorType = currentColor) const;

protected:
    virtual QPainterPath shape() const;

    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    bool debugTextEnabled;
    QGraphicsSimpleTextItem *mDebugText;
};
