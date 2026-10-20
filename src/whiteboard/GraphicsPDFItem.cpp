#include "whiteboard/GraphicsPDFItem.h"

#include <QtWidgets>

#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsPixmapItem.h"
#include "whiteboard/GraphicsItemDelegate.h"

GraphicsPDFItem::GraphicsPDFItem(PDFRenderer *renderer, int pageNumber, QGraphicsItem *parent) : PdfPageItem(renderer, pageNumber, parent)
{
    setData(GraphicsItemData::ItemLayerType, ItemLayerType::Object); // deprecated
    setData(GraphicsItemData::itemLayerType,
        QVariant(itemLayerType::BackgroundItem));                    // 必须设好图层类型，Z 值才会被分到正确的层

    setDelegate(new GraphicsItemDelegate(this, 0, GF_COMMON));
}

GraphicsPDFItem::~GraphicsPDFItem()
{
}

QVariant GraphicsPDFItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    QVariant newValue = Delegate()->itemChange(change, value);
    return PdfPageItem::itemChange(change, newValue);
}

void GraphicsPDFItem::setUuid(const QUuid &pUuid)
{
    Item::setUuid(pUuid);
    setData(GraphicsItemData::ItemUuid, QVariant(pUuid));
}

void GraphicsPDFItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate()->mousePressEvent(event))
    {
        // 刻意留空
    }
    else
    {
        PdfPageItem::mousePressEvent(event);
    }
}

void GraphicsPDFItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate()->mouseMoveEvent(event))
    {
        // 刻意留空
    }
    else
    {
        PdfPageItem::mouseMoveEvent(event);
    }
}

void GraphicsPDFItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Delegate()->mouseReleaseEvent(event);
    PdfPageItem::mouseReleaseEvent(event);
}

void GraphicsPDFItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    PdfPageItem::paint(painter, option, widget);
    Delegate()->postpaint(painter, option, widget);
}

Item *GraphicsPDFItem::deepCopy() const
{
    GraphicsPDFItem *copy = new GraphicsPDFItem(mRenderer, mPageNumber, parentItem());

    copy->setUuid(this->uuid()); // 小部件是不可变的，直接复用这份参数没问题

    copyItemParameters(copy);

    return copy;
}

void GraphicsPDFItem::copyItemParameters(Item *copy) const
{
    GraphicsPDFItem *cp = dynamic_cast<GraphicsPDFItem *>(copy);
    if (cp)
    {
        cp->setPos(this->pos());
        cp->setTransform(this->transform());
        cp->setFlag(QGraphicsItem::ItemIsMovable, true);
        cp->setFlag(QGraphicsItem::ItemIsSelectable, true);
        cp->setData(GraphicsItemData::ItemLayerType, this->data(GraphicsItemData::ItemLayerType));
        cp->setSourceUrl(this->sourceUrl());
        cp->setZValue(this->zValue());
    }
}

void GraphicsPDFItem::setRenderingQuality(RenderingQuality pRenderingQuality)
{
    Item::setRenderingQuality(pRenderingQuality);

    if (pRenderingQuality == RenderingQualityHigh)
    {
        setCacheMode(QGraphicsItem::NoCache);
    }
    else
    {
        setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    }
}

GraphicsScene *GraphicsPDFItem::scene()
{
    return qobject_cast<GraphicsScene *>(QGraphicsItem::scene());
}

GraphicsPixmapItem *GraphicsPDFItem::toPixmapItem() const
{
    QPixmap  pixmap(mRenderer->pageSizeF(mPageNumber).toSize());
    QPainter painter(&pixmap);
    mRenderer->render(&painter, mPageNumber);

    GraphicsPixmapItem *pixmapItem = new GraphicsPixmapItem();
    pixmapItem->setPixmap(pixmap);

    pixmapItem->setPos(this->pos());
    pixmapItem->setTransform(this->transform());
    pixmapItem->setFlag(QGraphicsItem::ItemIsMovable, true);
    pixmapItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
    pixmapItem->setData(GraphicsItemData::ItemLayerType, this->data(GraphicsItemData::ItemLayerType));

    return pixmapItem;
}
