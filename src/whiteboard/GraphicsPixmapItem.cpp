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

    // 构造函数已经给副本发了自己的 UUID，这里不再改回源图元那一份：PNG 按 UUID 命名，
    // 两份图元共用一个文件时，删掉任一份（CoreGraphicsScene::deleteItem → clearSource）
    // 会把另一份引用的文件也删掉。副本自己的文件由写出器在保存这一页时补上。
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
    // 归属拿不到就干脆不动手：退出期 BoardController 可能已经析构，selectedDocument() 也可能是空。
    // 另一层问题仍记在台账里：这里按"画板当前文档"算路径，删的是非当前文档里的图元时会漏。
    BoardController *board    = BoardController::instance();
    DocumentProxy   *document = board ? board->selectedDocument() : nullptr;
    if (!document)
    {
        qWarning() << "位图图元找不到所属文档，跳过资源清理:" << uuid();
        return;
    }

    const QString diskPath = document->persistencePath() + "/" + PackageLayout::imageDirectory + "/" + uuid().toString() + ".png";
    FileSystemUtils::deleteFile(diskPath);
}
