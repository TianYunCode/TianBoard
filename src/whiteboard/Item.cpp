#include "whiteboard/Item.h"


#include "whiteboard/GraphicsPixmapItem.h"
#include "whiteboard/GraphicsTextItem.h"
#include "whiteboard/GraphicsSvgItem.h"
#include "whiteboard/GraphicsMediaItem.h"
#include "whiteboard/GraphicsStrokesGroup.h"
#include "whiteboard/GraphicsGroupContainerItem.h"
#include "whiteboard/GraphicsWidgetItem.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsCurtainItem.h"
#include "whiteboard/GraphicsItemDelegate.h"

Item::Item() : mUuid(QUuid()), mRenderingQuality(Item::RenderingQualityNormal) {
    // 刻意留空
}

Item::~Item() {
    // 刻意留空
}

GraphicsItem::~GraphicsItem() {
    if (mDelegate != NULL)
        delete mDelegate;
}

void GraphicsItem::setDelegate(GraphicsItemDelegate *delegate) {
    Q_ASSERT(mDelegate == NULL);
    mDelegate = delegate;
}

GraphicsItemDelegate *GraphicsItem::Delegate() const {
    return mDelegate;
}

void GraphicsItem::assignZValue(QGraphicsItem *item, qreal value) {
    item->setZValue(value);
    item->setData(GraphicsItemData::ItemOwnZValue, value);
}

bool GraphicsItem::isFlippable(QGraphicsItem *item) {
    return item->data(GraphicsItemData::ItemFlippable).toBool();
}

bool GraphicsItem::isRotatable(QGraphicsItem *item) {
    return item->data(GraphicsItemData::ItemRotatable).toBool();
}

bool GraphicsItem::isLocked(QGraphicsItem *item) {
    return item->data(GraphicsItemData::ItemLocked).toBool();
}

QUuid GraphicsItem::getOwnUuid(QGraphicsItem *item) {
    QString idCandidate = item->data(GraphicsItemData::ItemUuid).toString();
    return idCandidate == QUuid().toString() ? QUuid() : QUuid(idCandidate);
}

void GraphicsItem::remove(bool canUndo) {
    if (Delegate())
        Delegate()->remove(canUndo);
}

GraphicsItemDelegate *GraphicsItem::Delegate(QGraphicsItem *pItem) {
    GraphicsItemDelegate *result = 0;

    switch (static_cast<int>(pItem->type())) {
        case GraphicsPixmapItem::Type:
            result = (static_cast<GraphicsPixmapItem *>(pItem))->Delegate();
            break;
        case GraphicsTextItem::Type:
            result = (static_cast<GraphicsTextItem *>(pItem))->Delegate();
            break;
        case GraphicsSvgItem::Type:
            result = (static_cast<GraphicsSvgItem *>(pItem))->Delegate();
            break;
        case GraphicsMediaItem::Type:
        case GraphicsVideoItem::Type:
        case GraphicsAudioItem::Type:
            result = (static_cast<GraphicsMediaItem *>(pItem))->Delegate();
            break;
        case GraphicsStrokesGroup::Type:
            result = (static_cast<GraphicsStrokesGroup *>(pItem))->Delegate();
            break;
        case GraphicsGroupContainerItem::Type:
            result = (static_cast<GraphicsGroupContainerItem *>(pItem))->Delegate();
            break;
        case GraphicsWidgetItem::Type:
            result = (static_cast<GraphicsWidgetItem *>(pItem))->Delegate();
            break;
        case GraphicsCurtainItem::Type:
            result = (static_cast<GraphicsCurtainItem *>(pItem))->Delegate();
            break;
    }

    return result;
}
