#include "whiteboard/GraphicsProxyWidget.h"

#include <QtWidgets>

#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsItemDelegate.h"

#include "whiteboard/GraphicsDelegateFrame.h"

#include "core/Application.h"
#include "core/PersistenceManager.h"

#include "whiteboard/BoardController.h"

#include "utils/FileSystemUtils.h"


GraphicsProxyWidget::GraphicsProxyWidget(QGraphicsItem *parent)
    : QGraphicsProxyWidget(parent, Qt::FramelessWindowHint) {
    setData(GraphicsItemData::ItemLayerType, ItemLayerType::Object);

    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    QGraphicsProxyWidget::setAcceptHoverEvents(true);
}


GraphicsProxyWidget::~GraphicsProxyWidget() {}

void GraphicsProxyWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    painter->save();
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    QGraphicsProxyWidget::paint(painter, option, widget);
    Delegate()->postpaint(painter, option, widget);
    painter->restore();
}

QVariant GraphicsProxyWidget::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == QGraphicsItem::ItemCursorHasChanged && scene()) {
        unsetCursor();
    }
    if ((change == QGraphicsItem::ItemSelectedHasChanged) && scene()) {
        if (isSelected()) {
            scene()->setActiveWindow(this);
        } else {
            if (scene()->activeWindow() == this) {
                scene()->setActiveWindow(0);
            }
        }
    }

    if (Delegate()) {
        QVariant newValue = Delegate()->itemChange(change, value);
        return QGraphicsProxyWidget::itemChange(change, newValue);
    } else
        return QGraphicsProxyWidget::itemChange(change, value);
}

void GraphicsProxyWidget::setUuid(const QUuid &pUuid) {
    Item::setUuid(pUuid);
    setData(GraphicsItemData::ItemUuid,
            QVariant(
                pUuid)); //store item uuid inside the QGraphicsItem to fast operations with Items on the scene
}

void GraphicsProxyWidget::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (Delegate()->mousePressEvent(event)) {
        //NOOP
    } else {
        // QT Proxy Widget is a bit lazy, we force the selection ...

        setSelected(true);
    }
    QGraphicsProxyWidget::mousePressEvent(event);
}


void GraphicsProxyWidget::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (Delegate()->mouseMoveEvent(event)) {
        // NOOP;
    } else {
        QGraphicsProxyWidget::mouseMoveEvent(event);
    }
}


void GraphicsProxyWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (Delegate()->mouseReleaseEvent(event)) {
    } else
        QGraphicsProxyWidget::mouseReleaseEvent(event);
}

void GraphicsProxyWidget::wheelEvent(QGraphicsSceneWheelEvent *event) {
    if (Delegate()->wheelEvent(event)) {
        QGraphicsProxyWidget::wheelEvent(event);
        event->accept();
    }
}

void GraphicsProxyWidget::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
    Q_UNUSED(event)
    //    NOOP
}
void GraphicsProxyWidget::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
    Q_UNUSED(event)
    //    NOOP
}

void GraphicsProxyWidget::resize(qreal w, qreal h) {
    GraphicsProxyWidget::resize(QSizeF(w, h));
}


void GraphicsProxyWidget::resize(const QSizeF &pSize) {
    if (pSize != size()) {
        qreal sizeX = 0;
        qreal sizeY = 0;

        if (widget()) {
            QSizeF minimumItemSize(widget()->minimumSize());
            if (minimumItemSize.width() > pSize.width())
                sizeX = minimumItemSize.width();
            else
                sizeX = pSize.width();

            if (minimumItemSize.height() > pSize.height())
                sizeY = minimumItemSize.height();
            else
                sizeY = pSize.height();
        }
        QSizeF size(sizeX, sizeY);


        QGraphicsProxyWidget::setMaximumSize(size.width(), size.height());
        QGraphicsProxyWidget::resize(size.width(), size.height());
        if (widget())
            widget()->resize(size.width(), size.height());
        if (Delegate())
            Delegate()->positionHandles();
        if (scene())
            scene()->setModified(true);
    }
}


QSizeF GraphicsProxyWidget::size() const {
    return QGraphicsProxyWidget::size();
}


GraphicsScene *GraphicsProxyWidget::scene() {
    return static_cast<GraphicsScene *>(QGraphicsItem::scene());
}
