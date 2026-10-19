#pragma once

#include <QtWidgets>

#include "whiteboard/Item.h"
#include "utils/Enums.h"
#include "content/PdfPageItem.h"

class GraphicsItemDelegate;
class GraphicsPixmapItem;

class GraphicsPDFItem: public PdfPageItem, public Item, public GraphicsItem
{
	Q_OBJECT
public:
    GraphicsPDFItem(PDFRenderer *renderer, int pageNumber, QGraphicsItem* parent = 0);
    virtual ~GraphicsPDFItem();

    enum { Type = GraphicsItemType::PDFItemType };

    virtual int type() const
    {
        return Type;
    }

    virtual Item* deepCopy() const;

    virtual void copyItemParameters(Item *copy) const;

    virtual void setRenderingQuality(RenderingQuality pRenderingQuality);

    virtual GraphicsScene* scene();

    virtual GraphicsPixmapItem* toPixmapItem() const;

    virtual void clearSource(){;}
    virtual void setUuid(const QUuid &pUuid);

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

};

