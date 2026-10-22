#include "whiteboard/GraphicsPixmapItem.h"
#include "model/PackageLayout.hpp"

#include <QtWidgets>
#include <QMimeData>
#include <QDrag>

#include "whiteboard/GraphicsScene.h"

#include "whiteboard/GraphicsItemDelegate.h"

#include "utils/FileSystemUtils.h"

#include "whiteboard/BoardController.h"

GraphicsPixmapItem::GraphicsPixmapItem(QGraphicsItem *parent) : QGraphicsPixmapItem(parent)
{
    setDelegate(new GraphicsItemDelegate(this, 0, GF_COMMON | GF_FLIPPABLE_ALL_AXIS | GF_REVOLVABLE | GF_RESPECT_RATIO | GF_TOOLBAR_USED));

    setData(GraphicsItemData::ItemLayerType, ItemLayerType::Object);
    setTransformationMode(Qt::SmoothTransformation);

    setData(GraphicsItemData::itemLayerType,
        QVariant(itemLayerType::ObjectItem)); // 必须设置：场景按图层类型分配 z 值
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    setData(GraphicsItemData::ItemCanBeSetAsBackground, true);

    setUuid(QUuid::createUuid()); // 更合理的做法是在元素自身的构造函数里生成 UUID
}

GraphicsPixmapItem::~GraphicsPixmapItem()
{
}

QVariant GraphicsPixmapItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    QVariant newValue = Delegate()->itemChange(change, value);
    return QGraphicsPixmapItem::itemChange(change, newValue);
}

void GraphicsPixmapItem::setUuid(const QUuid &pUuid)
{
    Item::setUuid(pUuid);
    setData(GraphicsItemData::ItemUuid, QVariant(pUuid));
}

void GraphicsPixmapItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QMimeData *pMime = new QMimeData();
    pMime->setImageData(pixmap().toImage());
    Delegate()->setMimeData(pMime);
    qreal k = (qreal)pixmap().width() / 100.0;

    QSize newSize((int)(pixmap().width() / k), (int)(pixmap().height() / k));

    Delegate()->setDragPixmap(pixmap().scaled(newSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

    if (Delegate()->mousePressEvent(event))
    {
        // 有意为空：委托已消费该事件
    }
    else
    {
        // 有意为空：不调基类实现，避免其默认拖动逻辑与委托的选中/拖放冲突
    }
}

void GraphicsPixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate()->mouseMoveEvent(event))
    {
        // 有意为空：委托已消费该事件
    }
    else
    {
        QGraphicsPixmapItem::mouseMoveEvent(event);
    }
}

void GraphicsPixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Delegate()->mouseReleaseEvent(event);
    QGraphicsPixmapItem::mouseReleaseEvent(event);
}

void GraphicsPixmapItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->setRenderHint(QPainter::Antialiasing, false);

    // 不绘制系统橡皮筋选中框，自定义选中效果由 DelegateFrame 负责
    QStyleOptionGraphicsItem styleOption  = QStyleOptionGraphicsItem(*option);

    styleOption.state                    &= ~QStyle::State_Selected;
    QGraphicsPixmapItem::paint(painter, &styleOption, widget);
    Delegate()->postpaint(painter, option, widget);

    painter->setRenderHint(QPainter::Antialiasing, true);
}

Item *GraphicsPixmapItem::deepCopy() const
{
    GraphicsPixmapItem *copy = new GraphicsPixmapItem();

    copy->setUuid(this->uuid()); // 只要位图内容不可变，副本沿用同一 UUID 就是安全的

    copyItemParameters(copy);

    return copy;
}

void GraphicsPixmapItem::copyItemParameters(Item *copy) const
{
    GraphicsPixmapItem *cp = dynamic_cast<GraphicsPixmapItem *>(copy);
    if (cp)
    {
        cp->setPixmap(this->pixmap());
        cp->setPos(this->pos());
        cp->setTransform(this->transform());
        cp->setFlag(QGraphicsItem::ItemIsMovable, true);
        cp->setFlag(QGraphicsItem::ItemIsSelectable, true);
        cp->setData(GraphicsItemData::ItemLayerType, this->data(GraphicsItemData::ItemLayerType));
        cp->setData(GraphicsItemData::ItemLocked, this->data(GraphicsItemData::ItemLocked));
        cp->setSourceUrl(this->sourceUrl());

        cp->setZValue(this->zValue());
    }
}

GraphicsScene *GraphicsPixmapItem::scene()
{
    return qobject_cast<GraphicsScene *>(QGraphicsItem::scene());
}

void GraphicsPixmapItem::setOpacity(qreal op)
{
    QGraphicsPixmapItem::setOpacity(op);
}

qreal GraphicsPixmapItem::opacity() const
{
    return QGraphicsPixmapItem::opacity();
}

void GraphicsPixmapItem::clearSource()
{
    QString fileName = PackageLayout::imageDirectory + "/" + uuid().toString() + ".png";
    QString diskPath = BoardController::instance()->selectedDocument()->persistencePath() + "/" + fileName;
    FileSystemUtils::deleteFile(diskPath);
}
