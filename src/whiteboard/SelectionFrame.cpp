#include "whiteboard/SelectionFrame.h"
#include <algorithm>

#include <QtWidgets>

#include "whiteboard/Item.h"
#include "whiteboard/GraphicsItemZLevelUndoCommand.h"
#include "whiteboard/GraphicsGroupContainerItem.h"
#include "whiteboard/BoardController.h"
#include "settings/Settings.h"
#include "utils/Snapping.h"
#include "core/Application.h"
#include "model/Resources.h"
#include "core/Application.h"
#include "whiteboard/BoardView.h"
#include "whiteboard/DrawingController.h"

SelectionFrame::SelectionFrame()
    : mThickness(Settings::settings()->objectFrameWidth), mAntiscaleRatio(1.0), mCursorRotationAngle(0), mAppliedRotationAngle(0), mDeleteButton(0),
      mDuplicateButton(0), mZOrderUpButton(0), mZOrderDownButton(0), mGroupButton(0), mRotateButton(0)
{
    setLocalBrush(QBrush(Settings::paletteColor));
    setPen(Qt::NoPen);
    setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Control));
    setData(GraphicsItemData::itemLayerType,
        QVariant(itemLayerType::SelectionFrame)); // 必须设好图层类型，Z 值才会被分到正确的层
    setFlags(QGraphicsItem::ItemSendsGeometryChanges | QGraphicsItem::ItemIsSelectable | ItemIsMovable);

    connect(BoardController::instance(), &BoardController::zoomChanged, this, &SelectionFrame::onZoomChanged);

    onZoomChanged(BoardController::instance()->currentZoom());
}

void SelectionFrame::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QPainterPath path;
    QRectF       shRect = option->rect;
    path.addRoundedRect(shRect, adjThickness() / 2, adjThickness() / 2);

    if (rect().width() > 1 && rect().height() > 1)
    {
        QPainterPath extruded;
        extruded.addRect(shRect.adjusted(adjThickness(), adjThickness(), (adjThickness() * -1), (adjThickness() * -1)));
        path = path.subtracted(extruded);
    }

    painter->fillPath(path, mLocalBrush);
}

QRectF SelectionFrame::boundingRect() const
{
    return rect().adjusted(-adjThickness(), -adjThickness(), adjThickness(), adjThickness());
}

QPainterPath SelectionFrame::shape() const
{
    QPainterPath resShape;
    QRectF       ownRect = rect();
    QRectF       shRect  = ownRect.adjusted(-adjThickness(), -adjThickness(), adjThickness(), adjThickness());
    resShape.addRoundedRect(shRect, adjThickness() / 2, adjThickness() / 2);

    if (rect().width() > 1 && rect().height() > 1)
    {
        QPainterPath extruded;
        extruded.addRect(ownRect);
        resShape = resShape.subtracted(extruded);
    }

    return resShape;
}

void SelectionFrame::setEnclosedItems(const QList<QGraphicsItem *> pGraphicsItems)
{
    mButtons.clear();
    mButtons.append(mDeleteButton);
    mCursorRotationAngle  = 0;
    mAppliedRotationAngle = 0;

    QRegion       resultRegion;
    GraphicsFlags resultFlags;
    mEnclosedtems.clear();

    // 被选集合里只要有一个锁定图元，整组选中就按锁定处理：
    // 否则用户会以为能拖动整组，结果只带动了其中没锁的那几个。
    mIsLocked = false;

    foreach (QGraphicsItem *nextItem, pGraphicsItems)
    {
        GraphicsItemDelegate *nextDelegate = GraphicsItem::Delegate(nextItem);
        if (nextDelegate)
        {
            mIsLocked = (mIsLocked || nextDelegate->isLocked());
            mEnclosedtems.append(nextDelegate);
            resultRegion |= nextItem->boundingRegion(nextItem->sceneTransform());
            resultFlags  |= nextDelegate->frameFlags();
        }
    }

    QRectF resultRect = resultRegion.boundingRect();
    setRect(resultRect);

    mButtons = buttonsForFlags(resultFlags);
    placeButtons();

    if (resultRect.isEmpty())
    {
        hide();
    }

    if (mIsLocked)
    {
        QColor baseColor = Settings::paletteColor;
        baseColor.setAlphaF(baseColor.alphaF() / 3);
        setLocalBrush(QBrush(baseColor));
    }
    else
    {
        setLocalBrush(QBrush(Settings::paletteColor));
    }
}

void SelectionFrame::updateRect()
{
    QRegion resultRegion;
    foreach (GraphicsItemDelegate *curDelegateItem, mEnclosedtems)
    {
        resultRegion |= curDelegateItem->delegated()->boundingRegion(curDelegateItem->delegated()->sceneTransform());
    }

    QRectF result = resultRegion.boundingRect();
    setRect(result);

    placeButtons();

    if (result.isEmpty())
    {
        setVisible(false);
    }
}

void SelectionFrame::updateScale()
{
    setScale(-BoardController::instance()->currentZoom());
}

void SelectionFrame::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    mPressedPos = mLastMovedPos = event->pos();
    mLastTranslateOffset        = QPointF();
    mCursorRotationAngle        = 0;
    mAppliedRotationAngle       = 0;
    mStartingBounds             = sceneBoundingRect();

    if (scene()->itemAt(event->scenePos(), transform()) == mRotateButton)
    {
        mOperationMode = om_rotating;
    }
    else
    {
        mOperationMode = om_moving;
    }
}

void SelectionFrame::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (mIsLocked)
    {
        return;
    }

    QPointF dp                = event->pos() - mPressedPos;
    QPointF rotCenter         = mapToScene(rect().center());

    GraphicsScene *boardScene = dynamic_cast<GraphicsScene *>(scene());
    const bool     snapping   = boardScene && boardScene->isSnapping();

    // 整组平移吸网格：拿“按下时的选区 + 这次的总位移”去算，同一次拖动里反复算
    // 也不会把误差累积进位移。
    if (snapping && static_cast<int>(mOperationMode) == om_moving)
    {
        dp += boardScene->snapToGrid(mStartingBounds.translated(dp));
    }

    // 整组旋转：先把累计角算出来，再决定这一次真正要转多少。旋转量对每个被选图元都一样，
    // 所以必须在循环外算一次，不能放进 foreach 里逐个改。
    qreal rotationDelta = 0;
    if (static_cast<int>(mOperationMode) == om_rotating)
    {
        const QLineF startLine(sceneBoundingRect().center(), event->lastScenePos());
        const QLineF currentLine(sceneBoundingRect().center(), event->scenePos());

        mCursorRotationAngle = std::fmod(mCursorRotationAngle - startLine.angleTo(currentLine) + 360.0, 360.0);

        qreal target         = mCursorRotationAngle;
        if (snapping)
        {
            // 与单图元旋转框同一套规则：先试 45° 的整数倍，再试 30°，容差共用一个设置项。
            target = Snapping::toNearestAngle(mCursorRotationAngle, {45, 30}, Settings::settings()->angleTolerance->get().toReal());
        }

        // 只补“目标角与已施加之差”：吸住时增量为 0，拖出容差范围后又能跟上光标。
        rotationDelta         = target - mAppliedRotationAngle;
        mAppliedRotationAngle = target;
    }

    foreach (GraphicsItemDelegate *curDelegate, mEnclosedtems)
    {
        switch (static_cast<int>(mOperationMode))
        {
            case om_moving:
            {
                QGraphicsItem *item         = curDelegate->delegated();
                QTransform     ownTransform = item->transform();
                QTransform     dTransform(ownTransform.m11(), ownTransform.m12(), ownTransform.m13()

                                                                                      ,
                    ownTransform.m21(), ownTransform.m22(), ownTransform.m23()

                                                                ,
                    ownTransform.m31() + (dp - mLastTranslateOffset).x(), ownTransform.m32() + (dp - mLastTranslateOffset).y(), ownTransform.m33());

                item->setTransform(dTransform);
            }
            break;

            case om_rotating:
            {
                QGraphicsItem *item         = curDelegate->delegated();
                QTransform     ownTransform = item->transform();

                QPointF nextRotCenter       = item->mapFromScene(rotCenter);

                ownTransform.translate(nextRotCenter.x(), nextRotCenter.y());
                ownTransform.rotate(rotationDelta);
                ownTransform.translate(-nextRotCenter.x(), -nextRotCenter.y());

                item->update();
                item->setTransform(ownTransform, false);
            }
            break;
        }
    }

    updateRect();
    mLastMovedPos        = event->pos();
    mLastTranslateOffset = dp;
}

void SelectionFrame::mouseReleaseEvent(QGraphicsSceneMouseEvent *)
{
    mPressedPos = mLastMovedPos = mLastTranslateOffset = QPointF();

    if (mOperationMode == om_moving || mOperationMode == om_rotating)
    {
        Application::undoStack->beginMacro(Settings::undoCommandTransactionName);
        foreach (GraphicsItemDelegate *d, mEnclosedtems)
        {
            d->commitUndoStep();
        }
        Application::undoStack->endMacro();
    }
    mOperationMode = om_idle;
}

void SelectionFrame::onZoomChanged(qreal pZoom)
{
    mAntiscaleRatio = 1 / (BoardController::instance()->systemScaleFactor() * pZoom);
}

void SelectionFrame::remove()
{
    Application::undoStack->beginMacro(Settings::undoCommandTransactionName);
    foreach (GraphicsItemDelegate *d, mEnclosedtems)
    {
        d->remove(true);
    }
    Application::undoStack->endMacro();

    updateRect();
}

static bool sortByZ(GraphicsItemDelegate *A, GraphicsItemDelegate *B)
{
    return (A->delegated()->data(GraphicsItemData::ItemOwnZValue).toReal() < B->delegated()->data(GraphicsItemData::ItemOwnZValue).toReal());
}

void SelectionFrame::duplicate()
{
    Application::undoStack->beginMacro(Settings::undoCommandTransactionName);

    // mEnclosedtems 是按选中先后排的，复制前必须按 Z 序重排，
    // 否则副本之间原本的上下层关系会被选中顺序覆盖掉。
    std::sort(mEnclosedtems.begin(), mEnclosedtems.end(), sortByZ);

    foreach (GraphicsItemDelegate *d, mEnclosedtems)
    {
        d->duplicate();
    }
    Application::undoStack->endMacro();

    updateRect();
}

void SelectionFrame::increaseZlevelUp()
{
    QList<QGraphicsItem *> selItems         = sortedByZ(scene()->selectedItems());

    QList<QGraphicsItem *>::iterator itemIt = selItems.end();
    while (itemIt != selItems.begin())
    {
        itemIt--;
        QGraphicsItem *item = *itemIt;
        graphicsScene()->changeZLevelTo(item, ZLayerController::up);
    }

    addSelectionUndo(selItems, ZLayerController::up);
}

void SelectionFrame::increaseZlevelTop()
{
    QList<QGraphicsItem *> selItems = sortedByZ(scene()->selectedItems());
    foreach (QGraphicsItem *item, selItems)
    {
        graphicsScene()->changeZLevelTo(item, ZLayerController::top);
    }
    addSelectionUndo(selItems, ZLayerController::top);
}

void SelectionFrame::increaseZlevelDown()
{
    QList<QGraphicsItem *> selItems = sortedByZ(scene()->selectedItems());
    foreach (QGraphicsItem *item, selItems)
    {
        graphicsScene()->changeZLevelTo(item, ZLayerController::down);
    }
    addSelectionUndo(selItems, ZLayerController::down);
}

void SelectionFrame::increaseZlevelBottom()
{
    QList<QGraphicsItem *>         selItems = sortedByZ(scene()->selectedItems());
    QListIterator<QGraphicsItem *> iter(selItems);
    iter.toBack();
    while (iter.hasPrevious())
    {
        graphicsScene()->changeZLevelTo(iter.previous(), ZLayerController::bottom);
    }
    addSelectionUndo(selItems, ZLayerController::bottom);
}

void SelectionFrame::groupItems()
{
    GraphicsGroupContainerItem *groupItem = graphicsScene()->createGroup(enclosedGraphicsItems());

    groupItem->setSelected(true);
    DrawingController::drawingController()->setStylusTool(StylusTool::Selector);
    qDebug() << "Grouping items";
}

void SelectionFrame::addSelectionUndo(QList<QGraphicsItem *> items, ZLayerController::moveDestination dest)
{
    if (!items.empty())
    {
        qreal                          topItemLevel = items.at(0)->data(GraphicsItemData::ItemOwnZValue).toReal();
        GraphicsItemZLevelUndoCommand *cmd          = new GraphicsItemZLevelUndoCommand(graphicsScene(), items, topItemLevel, dest);
        Application::undoStack->push(cmd);
    }
}

void SelectionFrame::placeButtons()
{
    if (!mButtons.count())
    {
        return;
    }

    QTransform tr;
    tr.scale(mAntiscaleRatio, mAntiscaleRatio);
    mDeleteButton->setParentItem(this);
    mDeleteButton->setTransform(tr);

    QRectF frRect = boundingRect();

    qreal topX    = frRect.left() - mDeleteButton->renderer()->viewBox().width() * mAntiscaleRatio / 2;
    qreal topY    = frRect.top() - mDeleteButton->renderer()->viewBox().height() * mAntiscaleRatio / 2;

    qreal bottomX = topX;
    qreal bottomY = frRect.bottom() - mDeleteButton->renderer()->viewBox().height() * mAntiscaleRatio / 2;

    mDeleteButton->setPos(topX, topY);
    mDeleteButton->show();

    int i = 1, j = 0, k = 0;
    while ((i + j + k) < mButtons.size())
    {
        DelegateButton *button = mButtons[i + j];

        if (button->getSection() == Qt::TopLeftSection)
        {
            button->setParentItem(this);
            button->setPos(topX + (i++ * 1.6 * adjThickness()), topY);
            button->setTransform(tr);
        }
        else if (button->getSection() == Qt::BottomLeftSection)
        {
            button->setParentItem(this);
            button->setPos(bottomX + (++j * 1.6 * adjThickness()), bottomY);
            button->setTransform(tr);
        }
        else if (button->getSection() == Qt::NoSection)
        {
            button->setParentItem(this);
            placeExceptionButton(button, tr);
            k++;
        }
        else
        {
            ++k;
        }
        button->show();
    }
}

void SelectionFrame::placeExceptionButton(DelegateButton *pButton, QTransform pTransform)
{
    QRectF frRect = boundingRect();

    if (pButton == mRotateButton)
    {
        qreal topX = frRect.right() - (mRotateButton->renderer()->viewBox().width()) * mAntiscaleRatio - 5;
        qreal topY = frRect.top() + 5;
        mRotateButton->setPos(topX, topY);
        mRotateButton->setTransform(pTransform);
    }
}

void SelectionFrame::clearButtons()
{
    foreach (DelegateButton *b, mButtons)
    {
        b->setParentItem(0);
        b->hide();
    }

    mButtons.clear();
}

inline GraphicsScene *SelectionFrame::graphicsScene()
{
    return qobject_cast<GraphicsScene *>(scene());
}

void SelectionFrame::setCursorFromAngle(QString angle)
{
    QWidget *controlViewport = BoardController::instance()->controlView()->viewport();

    QSize cursorSize(45, 30);

    QImage mask_img(cursorSize, QImage::Format_Mono);
    mask_img.fill(0xff);
    QPainter mask_ptr(&mask_img);
    mask_ptr.setBrush(QBrush(QColor(0, 0, 0)));
    mask_ptr.drawRoundedRect(0, 0, cursorSize.width() - 1, cursorSize.height() - 1, 6, 6);
    QBitmap bmpMask = QBitmap::fromImage(mask_img);

    QPixmap pixCursor(cursorSize);
    pixCursor.fill(QColor(Qt::white));

    QPainter painter(&pixCursor);

    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    painter.setBrush(QBrush(Qt::white));
    painter.setPen(QPen(QColor(Qt::black)));
    painter.drawRoundedRect(1, 1, cursorSize.width() - 2, cursorSize.height() - 2, 6, 6);
    painter.setFont(QFont("Arial", 10));
    painter.drawText(1, 1, cursorSize.width(), cursorSize.height(), Qt::AlignCenter, angle.append(QChar(176)));
    painter.end();

    pixCursor.setMask(bmpMask);
    controlViewport->setCursor(QCursor(pixCursor));
}

QList<QGraphicsItem *> SelectionFrame::sortedByZ(const QList<QGraphicsItem *> &pItems)
{
    // 选中框自己也在 selectedItems() 里，必须摘掉，
    // 不然它会被当成普通被选图元一起排序、一起搬动。
    QMultiMap<qreal, QGraphicsItem *> sortedItems;
    foreach (QGraphicsItem *tmpItem, pItems)
    {
        if (tmpItem->type() == Type)
        {
            continue;
        }
        sortedItems.insert(tmpItem->data(GraphicsItemData::ItemOwnZValue).toReal(), tmpItem);
    }

    return sortedItems.values();
}

QList<DelegateButton *> SelectionFrame::buttonsForFlags(GraphicsFlags fls)
{
    QList<DelegateButton *> result;

    if (!mDeleteButton)
    {
        mDeleteButton = new DelegateButton(":/images/close.svg", this, 0, Qt::TopLeftSection);
        connect(mDeleteButton, &DelegateButton::clicked, this, &SelectionFrame::remove);
    }
    result << mDeleteButton;

    if (fls.testFlag(GF_DUPLICATION_ENABLED))
    {
        if (!mDuplicateButton)
        {
            mDuplicateButton = new DelegateButton(":/images/duplicate.svg", this, 0, Qt::TopLeftSection);
            connect(mDuplicateButton, &DelegateButton::clicked, this, &SelectionFrame::duplicate);
        }
        result << mDuplicateButton;
    }

    if (mEnclosedtems.count() >= 1)
    {
        if (!mGroupButton)
        {
            mGroupButton = new DelegateButton(":/images/groupItems.svg", this, 0, Qt::TopLeftSection);
            mGroupButton->setShowProgressIndicator(false);
            connect(mGroupButton, &DelegateButton::clicked, this, &SelectionFrame::groupItems);
        }
        result << mGroupButton;
    }

    if (fls.testFlag(GF_ZORDER_MANIPULATIONS_ALLOWED))
    {
        if (!mZOrderUpButton)
        {
            mZOrderUpButton = new DelegateButton(":/images/z_layer_up.svg", this, 0, Qt::BottomLeftSection);
            mZOrderUpButton->setShowProgressIndicator(true);
            connect(mZOrderUpButton, &DelegateButton::clicked, this, &SelectionFrame::increaseZlevelUp);
            connect(mZOrderUpButton, &DelegateButton::longClicked, this, &SelectionFrame::increaseZlevelTop);
        }

        if (!mZOrderDownButton)
        {
            mZOrderDownButton = new DelegateButton(":/images/z_layer_down.svg", this, 0, Qt::BottomLeftSection);
            mZOrderDownButton->setShowProgressIndicator(true);
            connect(mZOrderDownButton, &DelegateButton::clicked, this, &SelectionFrame::increaseZlevelDown);
            connect(mZOrderDownButton, &DelegateButton::longClicked, this, &SelectionFrame::increaseZlevelBottom);
        }

        result << mZOrderUpButton;
        result << mZOrderDownButton;
    }

    if (fls.testFlag(GF_REVOLVABLE))
    {
        if (!mRotateButton)
        {
            mRotateButton = new DelegateButton(":/images/rotate.svg", this, 0, Qt::NoSection);
            mRotateButton->setCursor(Resources::resources()->rotateCursor);
            mRotateButton->setShowProgressIndicator(false);
            mRotateButton->setTransparentToMouseEvent(true);
        }
        result << mRotateButton;
    }

    return result;
}

QList<QGraphicsItem *> SelectionFrame::enclosedGraphicsItems()
{
    QList<QGraphicsItem *> result;
    foreach (GraphicsItemDelegate *d, mEnclosedtems)
    {
        result << d->delegated();
    }

    return result;
}
