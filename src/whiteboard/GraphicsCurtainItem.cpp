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


const QColor GraphicsCurtainItem::sDrawColor = Qt::white;
const QColor GraphicsCurtainItem::sDarkBackgroundDrawColor = Qt::black;
const QColor GraphicsCurtainItem::sOpaqueControlColor = QColor(191, 191, 191, 255);
const QColor GraphicsCurtainItem::sDarkBackgroundOpaqueControlColor = QColor(63, 63, 63, 255);

GraphicsCurtainItem::GraphicsCurtainItem(QGraphicsItem *parent) : QGraphicsRectItem(parent) {
    GraphicsCurtainItemDelegate *delegate = new GraphicsCurtainItemDelegate(this, 0);
    delegate->init();
    setDelegate(delegate);

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);

#if QT_VERSION >= 0x040600 // needs Qt 4.6.0 or better
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
#endif

    setData(GraphicsItemData::ItemLayerType, ItemLayerType::Tool);
    setPen(Qt::NoPen);
    this->setAcceptHoverEvents(true);

    setData(GraphicsItemData::itemLayerType,
            QVariant(itemLayerType::Curtain)); //Necessary to set if we want z value to be assigned correctly
}

GraphicsCurtainItem::~GraphicsCurtainItem() {}

QVariant GraphicsCurtainItem::itemChange(GraphicsItemChange change, const QVariant &value) {
    QVariant newValue = value;

    if (Delegate()) {
        newValue = Delegate()->itemChange(change, value);
    }

    return QGraphicsRectItem::itemChange(change, newValue);
}

void GraphicsCurtainItem::setUuid(const QUuid &pUuid) {
    Item::setUuid(pUuid);
    setData(GraphicsItemData::ItemUuid,
            QVariant(
                pUuid)); //store item uuid inside the QGraphicsItem to fast operations with Items on the scene
}

void GraphicsCurtainItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (Delegate()->mousePressEvent(event)) {
        //NOOP
    } else {
        QGraphicsRectItem::mousePressEvent(event);
    }
}

void GraphicsCurtainItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (Delegate()->mouseMoveEvent(event)) {
        // NOOP;
    } else {
        QGraphicsRectItem::mouseMoveEvent(event);
    }
}

void GraphicsCurtainItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    Delegate()->mouseReleaseEvent(event);
    QGraphicsRectItem::mouseReleaseEvent(event);
}


void GraphicsCurtainItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    QColor color = drawColor();
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    if (widget == Application::boardController->controlView()->viewport()) {
        color = Settings::paletteColor;
        if (!Application::applicationController->displayManager()->hasDisplay()) {
            color = opaqueControlColor();
        }
    }

    // Never draw the rubber band, we draw our custom selection with the DelegateFrame
    QStyleOptionGraphicsItem styleOption = QStyleOptionGraphicsItem(*option);
    styleOption.state &= ~QStyle::State_Selected;

    painter->fillRect(rect(), color);
    Delegate()->postpaint(painter, option, widget);
}


Item *GraphicsCurtainItem::deepCopy() const {
    GraphicsCurtainItem *copy = new GraphicsCurtainItem();

    copyItemParameters(copy);

    return copy;
}

void GraphicsCurtainItem::copyItemParameters(Item *copy) const {
    GraphicsCurtainItem *cp = dynamic_cast<GraphicsCurtainItem *>(copy);
    if (cp) {
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

QColor GraphicsCurtainItem::drawColor() const {
    GraphicsScene *pScene = static_cast<GraphicsScene *>(QGraphicsRectItem::scene());
    return pScene->isDarkBackground() ? sDarkBackgroundDrawColor : sDrawColor;
}


QColor GraphicsCurtainItem::opaqueControlColor() const {
    GraphicsScene *pScene = static_cast<GraphicsScene *>(QGraphicsRectItem::scene());
    return pScene->isDarkBackground() ? sDarkBackgroundOpaqueControlColor : sOpaqueControlColor;
}


void GraphicsCurtainItem::triggerRemovedSignal() {
    emit removed();
}
