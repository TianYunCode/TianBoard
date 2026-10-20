#include "ui/DocumentThumbnailWidget.h"

#include "core/Application.h"
#include "core/MimeData.h"
#include "settings/Settings.h"

#include "whiteboard/BoardController.h"

#include "model/DocumentController.h"


struct DocumentThumbnailWidget::Private {
    QGraphicsRectItem *mDropCaretRectItem{nullptr};
    ThumbnailPixmap *mClosestDropItem{nullptr};
    bool mDropIsRight{false};
    bool mDragEnabled{false};
    QTimer *mScrollTimer{nullptr};
    int mScrollMagnitude{0};
};


DocumentThumbnailWidget::DocumentThumbnailWidget(QWidget *parent) : ThumbnailWidget(parent), d(new Private) {
    d->mDragEnabled = true;

    setCanDrag(false);
    d->mScrollTimer = new QTimer(this);
    connect(d->mScrollTimer, &QTimer::timeout, this, &DocumentThumbnailWidget::autoScroll);
}


DocumentThumbnailWidget::~DocumentThumbnailWidget() = default;


void DocumentThumbnailWidget::mouseMoveEvent(QMouseEvent *event) {
    if (!dragEnabled()) {
        event->ignore();
        return;
    }

    if (!(event->buttons() & Qt::LeftButton))
        return;

    if ((event->pos() - mousePressPos()).manhattanLength() < QApplication::startDragDistance())
        return;

    QList<QGraphicsItem *> graphicsItems = items(mousePressPos());

    SceneThumbnailPixmap *sceneItem = 0;

    while (!graphicsItems.isEmpty() && !sceneItem)
        sceneItem = dynamic_cast<SceneThumbnailPixmap *>(graphicsItems.takeFirst());

    if (sceneItem) {
        QDrag *drag = new QDrag(this);
        QList<MimeDataItem> mimeDataItems;
        foreach (QGraphicsItem *item, selectedItems())
            mimeDataItems.append(MimeDataItem(sceneItem->proxy(), graphicItems().indexOf(item)));

        MimeData *mime = new MimeData(mimeDataItems);
        drag->setMimeData(mime);

        drag->setPixmap(sceneItem->pixmap().scaledToWidth(100));
        drag->setHotSpot(QPoint(drag->pixmap().width() / 2, drag->pixmap().height() / 2));

        drag->exec(Qt::MoveAction);
    }

    ThumbnailWidget::mouseMoveEvent(event);
}

void DocumentThumbnailWidget::dragEnterEvent(QDragEnterEvent *event) {
    if (!event->mimeData()->hasFormat(Application::mimeTypeBoardPage)) {
        event->setDropAction(Qt::IgnoreAction);
        event->ignore();
        return;
    }

    ThumbnailWidget::dragEnterEvent(event);
}

void DocumentThumbnailWidget::dragLeaveEvent(QDragLeaveEvent *event) {
    Q_UNUSED(event);
    if (d->mScrollTimer->isActive()) {
        d->mScrollMagnitude = 0;
        d->mScrollTimer->stop();
    }
    deleteDropCaret();
    ThumbnailWidget::dragLeaveEvent(event);
}

void DocumentThumbnailWidget::autoScroll() {
    this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() + d->mScrollMagnitude);
}

void DocumentThumbnailWidget::dragMoveEvent(QDragMoveEvent *event) {
    QRect boundingFrame = frameRect();
    // 靠近可视区上下边缘时启动定时滚动，幅度随距离增大。
    const int SCROLL_DISTANCE = 16;
    int bottomDist = boundingFrame.bottom() - event->pos().y(),
        topDist = boundingFrame.top() - event->pos().y();
    if (qAbs(bottomDist) <= SCROLL_DISTANCE) {
        d->mScrollMagnitude = (SCROLL_DISTANCE - bottomDist) * 4;
        if (verticalScrollBar()->isVisible() && !d->mScrollTimer->isActive())
            d->mScrollTimer->start(100);
    } else if (qAbs(topDist) <= SCROLL_DISTANCE) {
        d->mScrollMagnitude = (-SCROLL_DISTANCE - topDist) * 4;
        if (verticalScrollBar()->isVisible() && !d->mScrollTimer->isActive())
            d->mScrollTimer->start(100);
    } else {
        d->mScrollMagnitude = 0;
        d->mScrollTimer->stop();
    }

    QList<ThumbnailPixmap *> pixmapItems;
    foreach (QGraphicsItem *item, scene()->items(mapToScene(boundingFrame))) {
        ThumbnailPixmap *sceneItem = dynamic_cast<ThumbnailPixmap *>(item);
        if (sceneItem)
            pixmapItems.append(sceneItem);
    }

    int minDistance = 0;
    QGraphicsItem *underlyingItem = itemAt(event->pos());
    d->mClosestDropItem = dynamic_cast<ThumbnailPixmap *>(underlyingItem);

    if (!d->mClosestDropItem) {
        foreach (ThumbnailPixmap *item, pixmapItems) {
            qreal scale = item->transform().m11();
            QPointF itemCenter(item->pos().x() + item->boundingRect().width() * scale / 2,
                               item->pos().y() + item->boundingRect().height() * scale / 2);

            int distance = (itemCenter.toPoint() - mapToScene(event->pos()).toPoint()).manhattanLength();
            if (!d->mClosestDropItem || distance < minDistance) {
                d->mClosestDropItem = item;
                minDistance = distance;
            }
        }
    }

    if (d->mClosestDropItem) {
        qreal scale = d->mClosestDropItem->transform().m11();

        QPointF itemCenter(
            d->mClosestDropItem->pos().x() + d->mClosestDropItem->boundingRect().width() * scale / 2,
            d->mClosestDropItem->pos().y() + d->mClosestDropItem->boundingRect().height() * scale / 2);

        d->mDropIsRight = mapToScene(event->pos()).x() > itemCenter.x();

        if (!d->mDropCaretRectItem && selectedItems().count() < graphicItems().count()) {
            d->mDropCaretRectItem = new QGraphicsRectItem(0);
            scene()->addItem(d->mDropCaretRectItem);
            d->mDropCaretRectItem->setPen(QPen(Qt::darkGray));
            d->mDropCaretRectItem->setBrush(QBrush(Qt::lightGray));
        }

        QRectF dropCaretRect(
            d->mDropIsRight ? d->mClosestDropItem->pos().x() +
                                  d->mClosestDropItem->boundingRect().width() * scale + spacing() / 2 - 1
                            : d->mClosestDropItem->pos().x() - spacing() / 2 - 1,
            d->mClosestDropItem->pos().y(), 3, d->mClosestDropItem->boundingRect().height() * scale);

        if (d->mDropCaretRectItem)
            d->mDropCaretRectItem->setRect(dropCaretRect);
    }

    event->acceptProposedAction();
}


void DocumentThumbnailWidget::dropEvent(QDropEvent *event) {
    if (d->mScrollTimer->isActive()) {
        d->mScrollMagnitude = 0;
        d->mScrollTimer->stop();
    }
    deleteDropCaret();

    if (d->mClosestDropItem) {
        int targetIndex = d->mDropIsRight ? graphicItems().indexOf(d->mClosestDropItem) + 1
                                          : graphicItems().indexOf(d->mClosestDropItem);

        QList<MimeDataItem> mimeDataItems;
        if (event->mimeData()->hasFormat(Application::mimeTypeBoardPage)) {
            const MimeData *mimeData = qobject_cast<const MimeData *>(event->mimeData());
            if (mimeData)
                mimeDataItems = mimeData->items();
        }

        if (1 == mimeDataItems.count() &&
            (mimeDataItems.at(0).sceneIndex() == graphicItems().indexOf(d->mClosestDropItem) ||
             targetIndex == mimeDataItems.at(0).sceneIndex() ||
             targetIndex == mimeDataItems.at(0).sceneIndex() + 1)) {
            return;
        }

        int sourceIndexOffset = 0;
        int actualTargetIndex = targetIndex;
        // 从后往前处理多选拖放：每移动一页，后方元素的目标下标就要相应前移。
        for (int i = mimeDataItems.count() - 1; i >= 0; i--) {
            MimeDataItem sourceItem = mimeDataItems.at(i);
            int actualSourceIndex = sourceItem.sceneIndex();
            if (sourceItem.sceneIndex() >= targetIndex)
                actualSourceIndex += sourceIndexOffset;

            if (sourceItem.sceneIndex() < targetIndex) {
                if (actualSourceIndex != actualTargetIndex - 1)
                    emit sceneDropped(sourceItem.documentProxy(), actualSourceIndex, actualTargetIndex - 1);
                actualTargetIndex -= 1;
            } else {
                if (actualSourceIndex != actualTargetIndex)
                    emit sceneDropped(sourceItem.documentProxy(), actualSourceIndex, actualTargetIndex);
                sourceIndexOffset += 1;
            }
        }
    }
    ThumbnailWidget::dropEvent(event);
}

void DocumentThumbnailWidget::deleteDropCaret() {
    if (d->mDropCaretRectItem && scene()) {
        scene()->removeItem(d->mDropCaretRectItem);
        delete d->mDropCaretRectItem;
        d->mDropCaretRectItem = 0;
    }
}

void DocumentThumbnailWidget::setGraphicsItems(const QList<QGraphicsItem *> &pGraphicsItems,
                                               const QList<QUrl> &pItemPaths, const QStringList pLabels,
                                               const QString &pMimeType) {
    deleteDropCaret();

    ThumbnailWidget::setGraphicsItems(pGraphicsItems, pItemPaths, pLabels, pMimeType);
}

void DocumentThumbnailWidget::setDragEnabled(bool enabled) {
    d->mDragEnabled = enabled;
}

bool DocumentThumbnailWidget::dragEnabled() const {
    return d->mDragEnabled;
}

void DocumentThumbnailWidget::hightlightItem(int index) {
    if (0 <= index && index < labelsItems().length()) {
        labelsItems().at(index)->highlight();
    }
    if (0 <= index && index < graphicItems().length()) {
        SceneThumbnailPixmap *thumbnail = dynamic_cast<SceneThumbnailPixmap *>(graphicItems().at(index));
        if (thumbnail)
            thumbnail->highlight();
    }

    selectItemAt(index);
}
