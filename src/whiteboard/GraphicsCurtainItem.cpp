#include "whiteboard/GraphicsCurtainItem.h"

#include <QtWidgets>

#include "whiteboard/GraphicsScene.h"

#include "whiteboard/GraphicsCurtainItemDelegate.h"
#include "core/Application.h"
#include "core/ApplicationController.h"
#include "core/DisplayManager.h"
#include "settings/Settings.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/BoardView.h"

const QColor GraphicsCurtainItem::sDrawColor                        = Qt::white;
const QColor GraphicsCurtainItem::sDarkBackgroundDrawColor          = Qt::black;
const QColor GraphicsCurtainItem::sOpaqueControlColor               = QColor(191, 191, 191, 255);
const QColor GraphicsCurtainItem::sDarkBackgroundOpaqueControlColor = QColor(63, 63, 63, 255);

GraphicsCurtainItem::GraphicsCurtainItem(QGraphicsItem *parent) : QGraphicsRectItem(parent)
{
    GraphicsCurtainItemDelegate *delegate = new GraphicsCurtainItemDelegate(this, 0);
    delegate->init();
    setDelegate(delegate);

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);

#if QT_VERSION >= 0x040600 // 需要 Qt 4.6.0 及以上
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
#endif

    setData(GraphicsItemData::ItemLayerType, ItemLayerType::Tool);
    setPen(Qt::NoPen);
    this->setAcceptHoverEvents(true);

    setData(GraphicsItemData::itemLayerType,
        QVariant(itemLayerType::Curtain)); // 必须设置：场景按图层类型分配 z 值
}

GraphicsCurtainItem::~GraphicsCurtainItem()
{
}

QVariant GraphicsCurtainItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    QVariant newValue = value;

    if (Delegate())
    {
        newValue = Delegate()->itemChange(change, value);
    }

    return QGraphicsRectItem::itemChange(change, newValue);
}

void GraphicsCurtainItem::setUuid(const QUuid &pUuid)
{
    Item::setUuid(pUuid);
    setData(GraphicsItemData::ItemUuid,
        QVariant(pUuid)); // 把 UUID 同时存入图形项数据，便于场景快速操作项
}

void GraphicsCurtainItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate()->mousePressEvent(event))
    {
        // 有意为空：委托已消费该事件，无需再走基类逻辑
    }
    else
    {
        QGraphicsRectItem::mousePressEvent(event);
    }
}

void GraphicsCurtainItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate()->mouseMoveEvent(event))
    {
        // 有意为空：委托已消费该事件，无需再走基类逻辑
    }
    else
    {
        QGraphicsRectItem::mouseMoveEvent(event);
    }
}

void GraphicsCurtainItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Delegate()->mouseReleaseEvent(event);
    QGraphicsRectItem::mouseReleaseEvent(event);
}

void GraphicsCurtainItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QColor color = drawColor();
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    if (widget == BoardController::instance()->controlView()->viewport())
    {
        color = Settings::paletteColor;
        if (!Application::applicationController->displayManager()->hasDisplay())
        {
            color = opaqueControlColor();
        }
    }

    // 不绘制系统橡皮筋选中框，自定义选中效果由 DelegateFrame 负责
    QStyleOptionGraphicsItem styleOption  = QStyleOptionGraphicsItem(*option);
    styleOption.state                    &= ~QStyle::State_Selected;

    painter->fillRect(rect(), color);
    Delegate()->postpaint(painter, option, widget);
}

Item *GraphicsCurtainItem::deepCopy() const
{
    GraphicsCurtainItem *copy = new GraphicsCurtainItem();

    copyItemParameters(copy);

    return copy;
}

void GraphicsCurtainItem::copyItemParameters(Item *copy) const
{
    GraphicsCurtainItem *cp = dynamic_cast<GraphicsCurtainItem *>(copy);
    if (cp)
    {
        cp->setRect(this->rect());
        cp->setPos(this->pos());
        cp->setBrush(this->brush());
        cp->setPen(this->pen());
        cp->setTransform(this->transform());
        cp->setFlag(QGraphicsItem::ItemIsMovable, true);
        cp->setFlag(QGraphicsItem::ItemIsSelectable, true);
        cp->setData(GraphicsItemData::ItemLayerType, this->data(GraphicsItemData::ItemLayerType));
        cp->setZValue(this->zValue());
    }
}

QColor GraphicsCurtainItem::drawColor() const
{
    GraphicsScene *pScene = static_cast<GraphicsScene *>(QGraphicsRectItem::scene());
    return pScene->isDarkBackground() ? sDarkBackgroundDrawColor : sDrawColor;
}

QColor GraphicsCurtainItem::opaqueControlColor() const
{
    GraphicsScene *pScene = static_cast<GraphicsScene *>(QGraphicsRectItem::scene());
    return pScene->isDarkBackground() ? sDarkBackgroundOpaqueControlColor : sOpaqueControlColor;
}

void GraphicsCurtainItem::triggerRemovedSignal()
{
    emit removed();
}
