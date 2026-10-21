#include "whiteboard/GraphicsProxyWidget.h"

#include <QtWidgets>

#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsItemDelegate.h"

#include "whiteboard/GraphicsDelegateFrame.h"

#include "whiteboard/BoardController.h"

GraphicsProxyWidget::GraphicsProxyWidget(QGraphicsItem *parent) : QGraphicsProxyWidget(parent, Qt::FramelessWindowHint)
{
    setData(GraphicsItemData::ItemLayerType, ItemLayerType::Object);

    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    QGraphicsProxyWidget::setAcceptHoverEvents(true);
}

GraphicsProxyWidget::~GraphicsProxyWidget()
{
}

void GraphicsProxyWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->save();
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    QGraphicsProxyWidget::paint(painter, option, widget);
    Delegate()->postpaint(painter, option, widget);
    painter->restore();
}

QVariant GraphicsProxyWidget::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == QGraphicsItem::ItemCursorHasChanged && scene())
    {
        unsetCursor();
    }
    if ((change == QGraphicsItem::ItemSelectedHasChanged) && scene())
    {
        if (isSelected())
        {
            scene()->setActiveWindow(this);
        }
        else
        {
            if (scene()->activeWindow() == this)
            {
                scene()->setActiveWindow(0);
            }
        }
    }

    if (Delegate())
    {
        QVariant newValue = Delegate()->itemChange(change, value);
        return QGraphicsProxyWidget::itemChange(change, newValue);
    }
    else
    {
        return QGraphicsProxyWidget::itemChange(change, value);
    }
}

void GraphicsProxyWidget::setUuid(const QUuid &pUuid)
{
    Item::setUuid(pUuid);
    setData(GraphicsItemData::ItemUuid,
        QVariant(pUuid)); // 把 UUID 同时存入图形项数据，便于场景快速操作项
}

void GraphicsProxyWidget::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate()->mousePressEvent(event))
    {
        // 有意为空：委托已消费该事件
    }
    else
    {
        // 代理控件不会主动选中自己，这里强制设为选中

        setSelected(true);
    }
    QGraphicsProxyWidget::mousePressEvent(event);
}

void GraphicsProxyWidget::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate()->mouseMoveEvent(event))
    {
        // 有意为空：委托已消费该事件
    }
    else
    {
        QGraphicsProxyWidget::mouseMoveEvent(event);
    }
}

void GraphicsProxyWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate()->mouseReleaseEvent(event))
    {
        // 有意为空：委托已消费该事件
    }
    else
    {
        QGraphicsProxyWidget::mouseReleaseEvent(event);
    }
}

void GraphicsProxyWidget::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    if (Delegate()->wheelEvent(event))
    {
        QGraphicsProxyWidget::wheelEvent(event);
        event->accept();
    }
}

void GraphicsProxyWidget::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event)
    // 有意为空：悬停进入无需处理，光标由代理内部控件自行管理
}
void GraphicsProxyWidget::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event)
    // 有意为空：悬停离开无需处理，光标由代理内部控件自行管理
}

void GraphicsProxyWidget::resize(qreal w, qreal h)
{
    GraphicsProxyWidget::resize(QSizeF(w, h));
}

void GraphicsProxyWidget::resize(const QSizeF &pSize)
{
    if (pSize != size())
    {
        qreal sizeX = 0;
        qreal sizeY = 0;

        if (widget())
        {
            QSizeF minimumItemSize(widget()->minimumSize());
            if (minimumItemSize.width() > pSize.width())
            {
                sizeX = minimumItemSize.width();
            }
            else
            {
                sizeX = pSize.width();
            }

            if (minimumItemSize.height() > pSize.height())
            {
                sizeY = minimumItemSize.height();
            }
            else
            {
                sizeY = pSize.height();
            }
        }
        QSizeF size(sizeX, sizeY);

        QGraphicsProxyWidget::setMaximumSize(size.width(), size.height());
        QGraphicsProxyWidget::resize(size.width(), size.height());
        if (widget())
        {
            widget()->resize(size.width(), size.height());
        }
        if (Delegate())
        {
            Delegate()->positionHandles();
        }
        if (scene())
        {
            scene()->setModified(true);
        }
    }
}

QSizeF GraphicsProxyWidget::size() const
{
    return QGraphicsProxyWidget::size();
}

GraphicsScene *GraphicsProxyWidget::scene()
{
    return static_cast<GraphicsScene *>(QGraphicsItem::scene());
}
