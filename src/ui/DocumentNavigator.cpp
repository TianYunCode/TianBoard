#include <QList>
#include <QPointF>
#include <QPixmap>
#include <QTransform>
#include <QScrollBar>
#include <QFontMetrics>
#include <QGraphicsItem>
#include <QGraphicsPixmapItem>

#include "core/Application.h"
#include "ui/DocumentNavigator.h"
#include "whiteboard/BoardController.h"
#include "model/DocumentController.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/BoardPaletteManager.h"
#include "core/ApplicationController.h"


struct DocumentNavigator::Private {
    /** The scene */
    QGraphicsScene *mScene{nullptr};
    /** The list of current thumbnails with labels*/
    QList<ImgTextThumbnailElement> mThumbsWithLabels;
    /** The current number of columns */
    int mNbColumns{0};
    /** The current thumbnails width */
    int mThumbnailWidth{0};
    /** The current thumbnails minimum width */
    int mThumbnailMinWidth{0};
    /** The selected thumbnail */
    SceneThumbnailNavigPixmap *mSelectedThumbnail{nullptr};
    SceneThumbnailNavigPixmap *mLastClickedThumbnail{nullptr};
    SceneThumbnailNavigPixmap *mDropSource{nullptr};
    SceneThumbnailNavigPixmap *mDropTarget{nullptr};
    QGraphicsRectItem *mDropBar{nullptr};

    int mLongPressInterval{0};
    QTimer mLongPressTimer;
    QPoint mLastPressedMousePos;
};

/**
 * \brief Constructor
 * @param parent as the parent widget
 * @param name as the object name
 */
DocumentNavigator::DocumentNavigator(QWidget *parent, const char *name)
    : QGraphicsView(parent), d(new Private) {
    d->mNbColumns = 1;
    d->mThumbnailMinWidth = 50;
    d->mDropBar = new QGraphicsRectItem();
    d->mLongPressInterval = 350;

    setObjectName(name);
    d->mScene = new QGraphicsScene(this);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setScene(d->mScene);

    d->mThumbnailWidth = width() - 2 * border();

    d->mDropBar->setPen(QPen(Qt::darkGray));
    d->mDropBar->setBrush(QBrush(Qt::lightGray));
    scene()->addItem(d->mDropBar);
    d->mDropBar->hide();

    d->mLongPressTimer.setInterval(d->mLongPressInterval);
    d->mLongPressTimer.setSingleShot(true);

    setFrameShadow(QFrame::Plain);

    connect(Application::boardController, &DocumentContainer::documentThumbnailsUpdated, this,
            &DocumentNavigator::generateThumbnails);
    connect(Application::boardController, &DocumentContainer::documentPageUpdated, this,
            &DocumentNavigator::updateSpecificThumbnail);
    connect(Application::boardController, &BoardController::pageSelectionChanged, this,
            &DocumentNavigator::onScrollToSelectedPage);

    connect(&d->mLongPressTimer, &QTimer::timeout, this, &DocumentNavigator::longPressTimeout,
            Qt::UniqueConnection);

    connect(this, &DocumentNavigator::mousePressAndHoldEventRequired, this,
            &DocumentNavigator::mousePressAndHoldEvent, Qt::UniqueConnection);
}

/**
 * \brief Destructor
 */
DocumentNavigator::~DocumentNavigator() {
    if (NULL != d->mScene) {
        delete d->mScene;
        d->mScene = NULL;
    }
}

/**
 * \brief Generate the thumbnails
 */
void DocumentNavigator::generateThumbnails(DocumentContainer *source) {
    d->mThumbsWithLabels.clear();
    int selectedIndex = -1;
    int lastClickedIndex = -1;
    if (d->mLastClickedThumbnail) {
        lastClickedIndex = d->mLastClickedThumbnail->sceneIndex();
    }

    QList<QGraphicsItem *> graphicsItemList = d->mScene->items();
    for (int i = 0; i < graphicsItemList.size(); i += 1) {
        QGraphicsItem *item = graphicsItemList.at(i);
        if (item->isSelected())
            selectedIndex = i;

        if (item != d->mDropBar) {
            d->mScene->removeItem(item);
            delete item;
            item = NULL;
        }
    }

    for (int i = 0; i < source->selectedDocument()->pageCount(); i++) {
        // TODO: 这里的临时绕法暴露了结构问题
        // source->selectedDocument()->pageCount()  !=   source->pageCount()
        if (i >= source->pageCount() || !source->pageAt(i))
            source->insertThumbPage(i);

        const QPixmap *pix = source->pageAt(i);
        Q_ASSERT(!pix->isNull());
        int pageIndex = DocumentContainer::pageFromSceneIndex(i);

        SceneThumbnailNavigPixmap *pixmapItem =
            new SceneThumbnailNavigPixmap(*pix, source->selectedDocument(), i);

        QString label = tr("Page %0").arg(pageIndex);
        ThumbnailTextItem *labelItem = new ThumbnailTextItem(label);

        ImgTextThumbnailElement thumbWithText(pixmapItem, labelItem);
        thumbWithText.setBorder(border());
        d->mThumbsWithLabels.append(thumbWithText);

        if (lastClickedIndex == i)
            d->mLastClickedThumbnail = pixmapItem;

        d->mScene->addItem(pixmapItem);
        d->mScene->addItem(labelItem);
    }

    if (selectedIndex >= 0 && selectedIndex < d->mThumbsWithLabels.count())
        d->mSelectedThumbnail = d->mThumbsWithLabels.at(selectedIndex).getThumbnail();
    else
        d->mSelectedThumbnail = NULL;

    // Draw the items
    refreshScene();
}

void DocumentNavigator::onScrollToSelectedPage(int index) {
    int c = 0;
    foreach (ImgTextThumbnailElement el, d->mThumbsWithLabels) {
        if (c == index) {
            el.getThumbnail()->setSelected(true);
            d->mSelectedThumbnail = el.getThumbnail();
        } else {
            el.getThumbnail()->setSelected(false);
        }
        c++;
    }
    if (NULL != d->mSelectedThumbnail)
        ensureVisible(d->mSelectedThumbnail);
}

/**
 * \brief Refresh the given thumbnail
 * @param iPage as the given page related thumbnail
 */
void DocumentNavigator::updateSpecificThumbnail(int iPage) {
    const QPixmap *pix = Application::boardController->pageAt(iPage);
    SceneThumbnailNavigPixmap *newItem =
        new SceneThumbnailNavigPixmap(*pix, Application::boardController->selectedDocument(), iPage);

    // Get the old thumbnail
    SceneThumbnailNavigPixmap *oldItem = d->mThumbsWithLabels.at(iPage).getThumbnail();
    if (NULL != oldItem) {
        d->mScene->removeItem(oldItem);
        d->mScene->addItem(newItem);
        d->mThumbsWithLabels[iPage].setThumbnail(newItem);
        if (d->mLastClickedThumbnail == oldItem)
            d->mLastClickedThumbnail = newItem;
        delete oldItem;
        oldItem = NULL;
    }
}

/**
 * \brief Put the element in the right place in the scene.
 */
void DocumentNavigator::refreshScene() {
    qreal thumbnailHeight = d->mThumbnailWidth / Settings::minScreenRatio;

    for (int i = 0; i < d->mThumbsWithLabels.size(); i++) {
        // Get the item
        ImgTextThumbnailElement &item = d->mThumbsWithLabels[i];
        int columnIndex = i % d->mNbColumns;
        int rowIndex = i / d->mNbColumns;
        item.Place(rowIndex, columnIndex, d->mThumbnailWidth, thumbnailHeight);
    }
    scene()->setSceneRect(scene()->itemsBoundingRect());
}

/**
 * \brief  Set the number of thumbnails columns
 * @param nbColumns as the number of columns
 */
void DocumentNavigator::setNbColumns(int nbColumns) {
    d->mNbColumns = nbColumns;
}

/**
 * \brief Get the number of columns
 * @return the number of thumbnails columns
 */
int DocumentNavigator::nbColumns() {
    return d->mNbColumns;
}

/**
 * \brief Set the thumbnails minimum width
 * @param width as the minimum width
 */
void DocumentNavigator::setThumbnailMinWidth(int width) {
    d->mThumbnailMinWidth = width;
}

/**
 * \brief Get the thumbnails minimum width
 * @return the minimum thumbnails width
 */
int DocumentNavigator::thumbnailMinWidth() {
    return d->mThumbnailMinWidth;
}

/**
 * \brief Get the border size
 * @return the border size in pixels
 */
int DocumentNavigator::border() {
    return 10;
}

/**
 * \brief Handle the resize event
 * @param event as the resize event
 */
void DocumentNavigator::resizeEvent(QResizeEvent *event) {
    Q_UNUSED(event);

    // Update the thumbnails width
    d->mThumbnailWidth = (width() > d->mThumbnailMinWidth) ? width() - 2 * border() : d->mThumbnailMinWidth;

    if (d->mSelectedThumbnail)
        ensureVisible(d->mSelectedThumbnail);

    // Refresh the scene
    refreshScene();
}

/**
 * \brief Handle the mouse press event
 * @param event as the mouse event
 */
void DocumentNavigator::mousePressEvent(QMouseEvent *event) {
    QGraphicsView::mousePressEvent(event);

    if (!event->isAccepted()) {
        d->mLongPressTimer.start();
        d->mLastPressedMousePos = event->pos();

        d->mLastClickedThumbnail = clickedThumbnail(d->mLastPressedMousePos);

        if (d->mLastClickedThumbnail) {
            Application::boardController->persistViewPositionOnCurrentScene();
            Application::boardController->persistCurrentScene();
            Application::boardController->setActiveDocumentScene(d->mLastClickedThumbnail->sceneIndex());
            Application::boardController->centerOn(Application::boardController->activeScene()->lastCenter());
        }
    }
}

SceneThumbnailNavigPixmap *DocumentNavigator::clickedThumbnail(const QPoint pos) const {
    SceneThumbnailNavigPixmap *clickedThumbnail = NULL;

    QGraphicsItem *clickedItem = itemAt(pos);

    if (clickedItem) {
        clickedThumbnail = dynamic_cast<SceneThumbnailNavigPixmap *>(clickedItem);

        if (!clickedThumbnail) {
            // If we fall here we may have clicked on the label instead of the thumbnail
            ThumbnailTextItem *clickedTextItem = dynamic_cast<ThumbnailTextItem *>(clickedItem);
            if (clickedTextItem) {
                for (int i = 0; i < d->mThumbsWithLabels.size(); i++) {
                    const ImgTextThumbnailElement &el = d->mThumbsWithLabels.at(i);
                    if (el.getCaption() == clickedTextItem) {
                        clickedThumbnail = el.getThumbnail();
                        break;
                    }
                }
            }
        }
    }

    return clickedThumbnail;
}

void DocumentNavigator::mouseReleaseEvent(QMouseEvent *event) {
    event->accept();
    d->mLongPressTimer.stop();
}

void DocumentNavigator::longPressTimeout() {
    if (QApplication::mouseButtons() != Qt::NoButton)
        emit mousePressAndHoldEventRequired();

    d->mLongPressTimer.stop();
}

void DocumentNavigator::mousePressAndHoldEvent() {
    if (d->mLastClickedThumbnail) {
        d->mDropSource = d->mLastClickedThumbnail;
        d->mDropTarget = d->mLastClickedThumbnail;

        QPixmap pixmap = d->mLastClickedThumbnail->pixmap().scaledToWidth(d->mThumbnailWidth / 2);

        QDrag *drag = new QDrag(this);
        drag->setMimeData(new QMimeData());
        drag->setPixmap(pixmap);
        drag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));

        drag->exec();
    }
}

void DocumentNavigator::dragEnterEvent(QDragEnterEvent *event) {
    d->mDropBar->show();

    if (event->source() == this) {
        event->setDropAction(Qt::MoveAction);
        event->accept();
    } else {
        event->acceptProposedAction();
    }
}

void DocumentNavigator::dragMoveEvent(QDragMoveEvent *event) {
    QPointF position = event->pos();

    //autoscroll during drag'n'drop
    QPointF scenePos = mapToScene(position.toPoint());
    int thumbnailHeight = d->mThumbnailWidth / Settings::minScreenRatio;
    QRectF thumbnailArea(0, scenePos.y() - thumbnailHeight / 2, d->mThumbnailWidth, thumbnailHeight);

    ensureVisible(thumbnailArea);

    SceneThumbnailNavigPixmap *item = dynamic_cast<SceneThumbnailNavigPixmap *>(itemAt(position.toPoint()));
    if (item) {
        if (item != d->mDropTarget)
            d->mDropTarget = item;

        qreal scale = item->transform().m11();

        QPointF itemCenter(item->pos().x() +
                               (item->boundingRect().width() - verticalScrollBar()->width()) * scale,
                           item->pos().y() + item->boundingRect().height() * scale / 2);

        bool dropAbove = mapToScene(position.toPoint()).y() < itemCenter.y();
        bool movingUp = d->mDropSource->sceneIndex() > item->sceneIndex();
        qreal y = 0;

        if (movingUp) {
            if (dropAbove) {
                y = item->pos().y() - Settings::thumbnailSpacing / 2;
                if (d->mDropBar->y() != y)
                    d->mDropBar->setRect(
                        QRectF(item->pos().x(), y, d->mThumbnailWidth - verticalScrollBar()->width(), 3));
            }
        } else {
            if (!dropAbove) {
                y = item->pos().y() + item->boundingRect().height() * scale + Settings::thumbnailSpacing / 2;
                if (d->mDropBar->y() != y)
                    d->mDropBar->setRect(
                        QRectF(item->pos().x(), y, d->mThumbnailWidth - verticalScrollBar()->width(), 3));
            }
        }
    }

    event->acceptProposedAction();
}

void DocumentNavigator::dropEvent(QDropEvent *event) {
    Q_UNUSED(event);

    if (d->mDropSource->sceneIndex() != d->mDropTarget->sceneIndex())
        Application::boardController->moveSceneToIndex(d->mDropSource->sceneIndex(),
                                                       d->mDropTarget->sceneIndex());

    d->mDropSource = NULL;
    d->mDropTarget = NULL;
    d->mLastClickedThumbnail = NULL;

    d->mDropBar->setRect(QRectF());
    d->mDropBar->hide();
}
