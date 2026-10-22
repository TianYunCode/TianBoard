#include <QtWidgets>
#include "whiteboard/GraphicsGroupContainerItem.h"
#include "whiteboard/GraphicsTextItem.h"
#include "whiteboard/GraphicsTextItemDelegate.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsDelegateFrame.h"

#include "core/Application.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/BoardView.h"
#include "whiteboard/DrawingController.h"
#include "settings/Settings.h"

QColor GraphicsTextItem::lastUsedTextColor = QColor(Qt::black);

GraphicsTextItem::GraphicsTextItem(QGraphicsItem *parent)
    : QGraphicsTextItem(parent), GraphicsItem(), mTypeTextHereLabel(tr("<Type Text Here>")), mMultiClickState(0),
      mLastMousePressTime(QTime::currentTime()), isActivatedTextEditor(true)
{
    auto *textDelegate = new GraphicsTextItemDelegate(this, 0);
    setDelegate(textDelegate);

    // TODO: 控件本应按需创建，这里在构造时就建了出来，与之矛盾
    Delegate()->createControls();
    Delegate()->frame()->setOperationMode(GraphicsDelegateFrame::Resizing);
    Delegate()->setFrameFlag(GF_FLIPPABLE_ALL_AXIS, false);
    Delegate()->setFrameFlag(GF_REVOLVABLE, true);

    setData(GraphicsItemData::ItemLayerType, ItemLayerType::Object);
    setData(GraphicsItemData::itemLayerType,
        QVariant(itemLayerType::ObjectItem)); // 必须设好图层类型，Z 值才会被分到正确的层

    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    setTextInteractionFlags(Qt::TextEditorInteraction);

    setUuid(QUuid::createUuid());

    connect(document(), &QTextDocument::contentsChanged, textDelegate, &GraphicsTextItemDelegate::contentsChanged);
    connect(document(), &QTextDocument::undoCommandAdded, this, &GraphicsTextItem::undoCommandAdded);

    connect(document()->documentLayout(), &QAbstractTextDocumentLayout::documentSizeChanged, this, &GraphicsTextItem::documentSizeChanged);
}

GraphicsTextItem::~GraphicsTextItem()
{
}

void GraphicsTextItem::recolor()
{
    GraphicsTextItemDelegate *del = dynamic_cast<GraphicsTextItemDelegate *>(Delegate());
    if (del)
    {
        del->recolor();
    }
}

void GraphicsTextItem::setSelected(bool selected)
{
    if (selected)
    {
        Delegate()->createControls();
        Delegate()->frame()->setOperationMode(GraphicsDelegateFrame::Resizing);
    }
    QGraphicsTextItem::setSelected(selected);
}

QVariant GraphicsTextItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    QVariant newValue = value;

    if (Delegate())
    {
        newValue = Delegate()->itemChange(change, value);
    }

    return QGraphicsTextItem::itemChange(change, newValue);
}

void GraphicsTextItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // 放映模式下点击落在图元包围盒的空白处（没有压在字上）时，scene()->itemAt() 返回空，
    // 但事件仍然会送到本图元。这里是权宜写法：收下事件并让出焦点，绝不能进编辑态。
    if (StylusTool::Play == DrawingController::drawingController()->stylusTool())
    {
        QGraphicsTextItem::mousePressEvent(event);
        event->accept();
        clearFocus();
        return;
    }

    if (Delegate())
    {
        Delegate()->mousePressEvent(event);
        if (Delegate() && parentItem() && GraphicsGroupContainerItem::Type == parentItem()->type())
        {
            GraphicsGroupContainerItem *group = qgraphicsitem_cast<GraphicsGroupContainerItem *>(parentItem());
            if (group)
            {
                QGraphicsItem *curItem = group->getCurrentItem();
                if (curItem && this != curItem)
                {
                    group->deselectCurrentItem();
                }
                group->setCurrentItem(this);
                this->setSelected(true);
                Delegate()->positionHandles();
            }
        }
    }

    if (!data(GraphicsItemData::ItemEditable).toBool())
    {
        setTextInteractionFlags(Qt::NoTextInteraction);
        return;
    }

    setTextInteractionFlags(Qt::TextEditorInteraction);

    int elapsed = mLastMousePressTime.msecsTo(QTime::currentTime());

    if (elapsed < Application::app()->doubleClickInterval())
    {
        mMultiClickState++;
        if (mMultiClickState > 3)
        {
            mMultiClickState = 1;
        }
    }
    else
    {
        mMultiClickState = 1;
    }

    mLastMousePressTime = QTime::currentTime();

    if (mMultiClickState == 1)
    {
        activateTextEditor(true);
        QGraphicsTextItem::mousePressEvent(event);
        setFocus();
    }
    else if (mMultiClickState == 2)
    {
        QTextCursor tc = textCursor();
        tc.select(QTextCursor::WordUnderCursor);
        setTextCursor(tc);
    }
    else if (mMultiClickState == 3)
    {
        QTextCursor tc = textCursor();
        tc.select(QTextCursor::Document);
        setTextCursor(tc);
    }
    else
    {
        mMultiClickState = 0;
    }
}

void GraphicsTextItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (!Delegate() || !Delegate()->mouseMoveEvent(event))
    {
        QGraphicsTextItem::mouseMoveEvent(event);
    }
}

void GraphicsTextItem::dragMoveEvent(QGraphicsSceneDragDropEvent *event)
{
}

void GraphicsTextItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    // 放映模式下点击落在图元包围盒的空白处（没有压在字上）时，scene()->itemAt() 返回空，
    // 但事件仍然会送到本图元。这里是权宜写法：收下事件并让出焦点，绝不能进编辑态。
    if (StylusTool::Play == DrawingController::drawingController()->stylusTool())
    {
        event->accept();
        clearFocus();
        return;
    }

    if (mMultiClickState == 1)
    {
        QGraphicsTextItem::mouseReleaseEvent(event);

        if (Delegate())
        {
            Delegate()->mouseReleaseEvent(event);
        }
    }
    else
    {
        event->accept();
    }
}

void GraphicsTextItem::keyPressEvent(QKeyEvent *event)
{
    // 委托吃掉按键时（比如方向键留给控制手柄）不能再交给基类，否则文本会跟着动光标。
    if (Delegate() && !Delegate()->keyPressEvent(event))
    {
        return;
    }

    QGraphicsTextItem::keyPressEvent(event);
}

void GraphicsTextItem::keyReleaseEvent(QKeyEvent *event)
{
    if (Delegate() && !Delegate()->keyReleaseEvent(event))
    {
        return;
    }

    QGraphicsTextItem::keyReleaseEvent(event);
}

void GraphicsTextItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // 从不画 Qt 自己的选框：选中效果由委托的边框画，这里把选中与焦点状态摘掉。
    QStyleOptionGraphicsItem styleOption  = QStyleOptionGraphicsItem(*option);
    styleOption.state                    &= ~QStyle::State_Selected;
    styleOption.state                    &= ~QStyle::State_HasFocus;

    QGraphicsTextItem::paint(painter, &styleOption, widget);

    if (widget == BoardController::instance()->controlView()->viewport() && !isSelected())
    {
        setTextInteractionFlags(Qt::NoTextInteraction);
        if (toPlainText().isEmpty())
        {
            painter->setFont(font());
            painter->setPen(Settings::paletteColor);
            painter->drawText(boundingRect(), Qt::AlignCenter, mTypeTextHereLabel);
        }
    }

    Delegate()->postpaint(painter, option, widget);
}

Item *GraphicsTextItem::deepCopy() const
{
    GraphicsTextItem *copy = new GraphicsTextItem();

    copyItemParameters(copy);

    return copy;
}

void GraphicsTextItem::copyItemParameters(Item *copy) const
{
    GraphicsTextItem *cp = dynamic_cast<GraphicsTextItem *>(copy);
    if (cp)
    {
        cp->setHtml(toHtml());
        cp->setPos(this->pos());
        cp->setTransform(this->transform());
        cp->setFlag(QGraphicsItem::ItemIsMovable, true);
        cp->setFlag(QGraphicsItem::ItemIsSelectable, true);
        cp->setData(GraphicsItemData::ItemLayerType, this->data(GraphicsItemData::ItemLayerType));
        cp->setData(GraphicsItemData::ItemLocked, this->data(GraphicsItemData::ItemLocked));
        cp->setData(GraphicsItemData::ItemEditable, data(GraphicsItemData::ItemEditable).toBool());
        cp->setTextWidth(this->textWidth());
        cp->setTextHeight(this->textHeight());

        cp->setSourceUrl(this->sourceUrl());
        cp->setZValue(this->zValue());
    }
}

QRectF GraphicsTextItem::boundingRect() const
{
    qreal width  = textWidth();
    qreal height = textHeight();
    return QRectF(QPointF(), QSizeF(width, height));
}

QPainterPath GraphicsTextItem::shape() const
{
    QPainterPath path;
    path.addRect(boundingRect());
    return path;
}

void GraphicsTextItem::setTextWidth(qreal width)
{
    qreal                     titleBarWidth = 0;
    GraphicsTextItemDelegate *del           = dynamic_cast<GraphicsTextItemDelegate *>(Delegate());
    if (del)
    {
        titleBarWidth = del->titleBarWidth();
    }

    qreal newWidth = qMax(titleBarWidth, width);

    QGraphicsTextItem::setTextWidth(newWidth);
}

void GraphicsTextItem::setTextHeight(qreal height)
{
    QFontMetrics fm(font());
    qreal        minHeight = fm.height() + document()->documentMargin() * 2;
    mTextHeight            = qMax(minHeight, height);
    update();
    setFocus();
}

qreal GraphicsTextItem::textHeight() const
{
    return mTextHeight;
}

qreal GraphicsTextItem::pixelsPerPoint() const
{
    QTextCursor cursor = textCursor();
    if (cursor.isNull())
    {
        return 0;
    }

    cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);

    QFontInfo fi(cursor.charFormat().font());

    qreal pixelSize = fi.pixelSize();
    qreal pointSize = fi.pointSizeF();

    if (pointSize == 0)
    {
        return 0;
    }

    return pixelSize / pointSize;
}

void GraphicsTextItem::contentsChanged()
{
    if (scene())
    {
        scene()->setModified(true);
    }

    if (toPlainText().isEmpty())
    {
        resize(QFontMetrics(font()).horizontalAdvance(mTypeTextHereLabel), QFontMetrics(font()).height());
    }
}

GraphicsScene *GraphicsTextItem::scene()
{
    return static_cast<GraphicsScene *>(QGraphicsItem::scene());
}

void GraphicsTextItem::resize(qreal w, qreal h)
{
    setTextWidth(w);
    setTextHeight(h);
    if (Delegate())
    {
        Delegate()->positionHandles();
    }
}

QSizeF GraphicsTextItem::size() const
{
    return QSizeF(textWidth(), textHeight());
}

void GraphicsTextItem::setUuid(const QUuid &pUuid)
{
    Item::setUuid(pUuid);
    // UUID 同时存进图元数据：场景按 UUID 找回图元，不必遍历整棵树。
    setData(GraphicsItemData::ItemUuid, QVariant(pUuid));
}

void GraphicsTextItem::undoCommandAdded()
{
    emit textUndoCommandAdded(this);
}

void GraphicsTextItem::documentSizeChanged(const QSizeF &newSize)
{
    resize(newSize.width(), newSize.height());
}

void GraphicsTextItem::activateTextEditor(bool activate)
{
    this->isActivatedTextEditor = activate;

    if (!activate)
    {
        setTextInteractionFlags(Qt::TextSelectableByMouse);
    }
    else
    {
        setTextInteractionFlags(Qt::TextEditorInteraction);
    }
}
