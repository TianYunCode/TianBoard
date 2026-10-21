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
#include "ui/DocumentController.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/BoardPaletteManager.h"
#include "core/ApplicationController.h"

struct DocumentNavigator::Private
{
    /// @brief 承载缩略图的场景
    QGraphicsScene *mScene{nullptr};
    /// @brief 当前这批带标题的缩略图元素
    QList<ImgTextThumbnailElement> mThumbsWithLabels;
    /// @brief 本次排版算出的列数
    int mNbColumns{0};
    /// @brief 缩略图当前宽度
    int mThumbnailWidth{0};
    /// @brief 缩略图允许的最小宽度
    int mThumbnailMinWidth{0};
    /// @brief 当前选中的那一张
    SceneThumbnailNavigPixmap *mSelectedThumbnail{nullptr};
    SceneThumbnailNavigPixmap *mLastClickedThumbnail{nullptr};
    SceneThumbnailNavigPixmap *mDropSource{nullptr};
    SceneThumbnailNavigPixmap *mDropTarget{nullptr};
    QGraphicsRectItem         *mDropBar{nullptr};

    int    mLongPressInterval{0};
    QTimer mLongPressTimer;
    QPoint mLastPressedMousePos;
};

DocumentNavigator::DocumentNavigator(QWidget *parent, const char *name) : QGraphicsView(parent), d(new Private)
{
    d->mNbColumns         = 1;
    d->mThumbnailMinWidth = 50;
    d->mDropBar           = new QGraphicsRectItem();
    d->mLongPressInterval = 350;

    setObjectName(name);
    d->mScene = new QGraphicsScene(this);
    // 缩略图按列排布，横向滚动没有意义，只保留纵向滚动。
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setScene(d->mScene);

    d->mThumbnailWidth = width() - 2 * border();

    // 拖拽换序时的插入指示条，平时隐藏。
    d->mDropBar->setPen(QPen(Qt::darkGray));
    d->mDropBar->setBrush(QBrush(Qt::lightGray));
    scene()->addItem(d->mDropBar);
    d->mDropBar->hide();

    d->mLongPressTimer.setInterval(d->mLongPressInterval);
    d->mLongPressTimer.setSingleShot(true);

    setFrameShadow(QFrame::Plain);

    connect(Application::boardController, &DocumentContainer::documentThumbnailsUpdated, this, &DocumentNavigator::generateThumbnails);
    connect(Application::boardController, &DocumentContainer::documentPageUpdated, this, &DocumentNavigator::updateSpecificThumbnail);
    connect(Application::boardController, &BoardController::pageSelectionChanged, this, &DocumentNavigator::onScrollToSelectedPage);

    connect(&d->mLongPressTimer, &QTimer::timeout, this, &DocumentNavigator::longPressTimeout, Qt::UniqueConnection);

    connect(this, &DocumentNavigator::mousePressAndHoldEventRequired, this, &DocumentNavigator::mousePressAndHoldEvent, Qt::UniqueConnection);
}

DocumentNavigator::~DocumentNavigator()
{
    if (NULL != d->mScene)
    {
        delete d->mScene;
        d->mScene = NULL;
    }
}

void DocumentNavigator::generateThumbnails(DocumentContainer *source)
{
    d->mThumbsWithLabels.clear();
    int selectedIndex    = -1;
    int lastClickedIndex = -1;
    if (d->mLastClickedThumbnail)
    {
        lastClickedIndex = d->mLastClickedThumbnail->sceneIndex();
    }

    // 重建前清空场景：保留拖拽指示条，其余条目连同标签一起删除。
    QList<QGraphicsItem *> graphicsItemList = d->mScene->items();
    for (int i = 0; i < graphicsItemList.size(); i += 1)
    {
        QGraphicsItem *item = graphicsItemList.at(i);
        if (item->isSelected())
        {
            selectedIndex = i;
        }

        if (item != d->mDropBar)
        {
            d->mScene->removeItem(item);
            delete item;
            item = NULL;
        }
    }

    for (int i = 0; i < source->selectedDocument()->pageCount(); i++)
    {
        // TODO: 这里的临时绕法暴露了结构问题：文档的页数与容器里的页数本应一致，
        // 现在却要各自数一遍，缺页只能就地补。
        if (i >= source->pageCount() || !source->pageAt(i))
        {
            source->insertThumbPage(i);
        }

        const QPixmap *pix = source->pageAt(i);
        Q_ASSERT(!pix->isNull());
        int pageIndex                         = DocumentContainer::pageFromSceneIndex(i);

        SceneThumbnailNavigPixmap *pixmapItem = new SceneThumbnailNavigPixmap(*pix, source->selectedDocument(), i);

        QString            label              = tr("Page %0").arg(pageIndex);
        ThumbnailTextItem *labelItem          = new ThumbnailTextItem(label);

        ImgTextThumbnailElement thumbWithText(pixmapItem, labelItem);
        thumbWithText.setBorder(border());
        d->mThumbsWithLabels.append(thumbWithText);

        if (lastClickedIndex == i)
        {
            d->mLastClickedThumbnail = pixmapItem;
        }

        d->mScene->addItem(pixmapItem);
        d->mScene->addItem(labelItem);
    }

    if (selectedIndex >= 0 && selectedIndex < d->mThumbsWithLabels.count())
    {
        d->mSelectedThumbnail = d->mThumbsWithLabels.at(selectedIndex).getThumbnail();
    }
    else
    {
        d->mSelectedThumbnail = NULL;
    }

    // 这里按场景条目位置重排，确保页码标签与缩略图保持配对。
    refreshScene();
}

void DocumentNavigator::onScrollToSelectedPage(int index)
{
    int c = 0;
    foreach (ImgTextThumbnailElement el, d->mThumbsWithLabels)
    {
        if (c == index)
        {
            el.getThumbnail()->setSelected(true);
            d->mSelectedThumbnail = el.getThumbnail();
        }
        else
        {
            el.getThumbnail()->setSelected(false);
        }
        c++;
    }
    if (NULL != d->mSelectedThumbnail)
    {
        ensureVisible(d->mSelectedThumbnail);
    }
}

void DocumentNavigator::updateSpecificThumbnail(int iPage)
{
    const QPixmap             *pix     = Application::boardController->pageAt(iPage);
    SceneThumbnailNavigPixmap *newItem = new SceneThumbnailNavigPixmap(*pix, Application::boardController->selectedDocument(), iPage);

    // 用新条目原位替换旧缩略图，避免整页重建。
    SceneThumbnailNavigPixmap *oldItem = d->mThumbsWithLabels.at(iPage).getThumbnail();
    if (NULL != oldItem)
    {
        d->mScene->removeItem(oldItem);
        d->mScene->addItem(newItem);
        d->mThumbsWithLabels[iPage].setThumbnail(newItem);
        if (d->mLastClickedThumbnail == oldItem)
        {
            d->mLastClickedThumbnail = newItem;
        }
        delete oldItem;
        oldItem = NULL;
    }
}

void DocumentNavigator::refreshScene()
{
    // 缩略图高度按屏幕宽高比换算，保证页面比例一致。
    qreal thumbnailHeight = d->mThumbnailWidth / Settings::minScreenRatio;

    for (int i = 0; i < d->mThumbsWithLabels.size(); i++)
    {
        ImgTextThumbnailElement &item        = d->mThumbsWithLabels[i];
        int                      columnIndex = i % d->mNbColumns;
        int                      rowIndex    = i / d->mNbColumns;
        item.Place(rowIndex, columnIndex, d->mThumbnailWidth, thumbnailHeight);
    }
    scene()->setSceneRect(scene()->itemsBoundingRect());
}

void DocumentNavigator::setNbColumns(int nbColumns)
{
    d->mNbColumns = nbColumns;
}

int DocumentNavigator::nbColumns()
{
    return d->mNbColumns;
}

void DocumentNavigator::setThumbnailMinWidth(int width)
{
    d->mThumbnailMinWidth = width;
}

int DocumentNavigator::thumbnailMinWidth()
{
    return d->mThumbnailMinWidth;
}

int DocumentNavigator::border()
{
    return 10;
}

void DocumentNavigator::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);

    // 视图变窄时不低于最小宽度，避免缩略图小到无法点击。
    d->mThumbnailWidth = (width() > d->mThumbnailMinWidth) ? width() - 2 * border() : d->mThumbnailMinWidth;

    if (d->mSelectedThumbnail)
    {
        ensureVisible(d->mSelectedThumbnail);
    }

    refreshScene();
}

void DocumentNavigator::mousePressEvent(QMouseEvent *event)
{
    QGraphicsView::mousePressEvent(event);

    // 视图自身没有消费该事件（即点在空白或缩略图上）时才启动长按与切页逻辑。
    if (!event->isAccepted())
    {
        d->mLongPressTimer.start();
        d->mLastPressedMousePos  = event->pos();

        d->mLastClickedThumbnail = clickedThumbnail(d->mLastPressedMousePos);

        if (d->mLastClickedThumbnail)
        {
            // 先保存当前页的视图位置与场景状态，再切到目标页并恢复其中心点。
            Application::boardController->persistViewPositionOnCurrentScene();
            Application::boardController->persistCurrentScene();
            Application::boardController->setActiveDocumentScene(d->mLastClickedThumbnail->sceneIndex());
            Application::boardController->centerOn(Application::boardController->activeScene()->lastCenter());
        }
    }
}

SceneThumbnailNavigPixmap *DocumentNavigator::clickedThumbnail(const QPoint pos) const
{
    SceneThumbnailNavigPixmap *clickedThumbnail = NULL;

    QGraphicsItem *clickedItem                  = itemAt(pos);

    if (clickedItem)
    {
        clickedThumbnail = dynamic_cast<SceneThumbnailNavigPixmap *>(clickedItem);

        if (!clickedThumbnail)
        {
            // 点中的可能是页码标签，需要反查它所属的缩略图。
            ThumbnailTextItem *clickedTextItem = dynamic_cast<ThumbnailTextItem *>(clickedItem);
            if (clickedTextItem)
            {
                for (int i = 0; i < d->mThumbsWithLabels.size(); i++)
                {
                    const ImgTextThumbnailElement &el = d->mThumbsWithLabels.at(i);
                    if (el.getCaption() == clickedTextItem)
                    {
                        clickedThumbnail = el.getThumbnail();
                        break;
                    }
                }
            }
        }
    }

    return clickedThumbnail;
}

void DocumentNavigator::mouseReleaseEvent(QMouseEvent *event)
{
    // 松开即取消长按计时，避免拖拽与点击叠加。
    event->accept();
    d->mLongPressTimer.stop();
}

void DocumentNavigator::longPressTimeout()
{
    if (QApplication::mouseButtons() != Qt::NoButton)
    {
        emit mousePressAndHoldEventRequired();
    }

    d->mLongPressTimer.stop();
}

void DocumentNavigator::mousePressAndHoldEvent()
{
    if (d->mLastClickedThumbnail)
    {
        d->mDropSource = d->mLastClickedThumbnail;
        d->mDropTarget = d->mLastClickedThumbnail;

        QPixmap pixmap = d->mLastClickedThumbnail->pixmap().scaledToWidth(d->mThumbnailWidth / 2);

        QDrag *drag    = new QDrag(this);
        drag->setMimeData(new QMimeData());
        drag->setPixmap(pixmap);
        drag->setHotSpot(QPoint(pixmap.width() / 2, pixmap.height() / 2));

        drag->exec();
    }
}

void DocumentNavigator::dragEnterEvent(QDragEnterEvent *event)
{
    d->mDropBar->show();

    if (event->source() == this)
    {
        event->setDropAction(Qt::MoveAction);
        event->accept();
    }
    else
    {
        event->acceptProposedAction();
    }
}

void DocumentNavigator::dragMoveEvent(QDragMoveEvent *event)
{
    QPointF position        = event->position();

    // 拖到上下边缘附近时自动滚动视图。
    QPointF scenePos        = mapToScene(position.toPoint());
    int     thumbnailHeight = d->mThumbnailWidth / Settings::minScreenRatio;
    QRectF  thumbnailArea(0, scenePos.y() - thumbnailHeight / 2, d->mThumbnailWidth, thumbnailHeight);

    ensureVisible(thumbnailArea);

    SceneThumbnailNavigPixmap *item = dynamic_cast<SceneThumbnailNavigPixmap *>(itemAt(position.toPoint()));
    if (item)
    {
        if (item != d->mDropTarget)
        {
            d->mDropTarget = item;
        }

        qreal scale = item->transform().m11();

        QPointF itemCenter(item->pos().x() + (item->boundingRect().width() - verticalScrollBar()->width()) * scale,
            item->pos().y() + item->boundingRect().height() * scale / 2);

        bool  dropAbove = mapToScene(position.toPoint()).y() < itemCenter.y();
        bool  movingUp  = d->mDropSource->sceneIndex() > item->sceneIndex();
        qreal y         = 0;

        if (movingUp)
        {
            if (dropAbove)
            {
                y = item->pos().y() - Settings::thumbnailSpacing / 2;
                if (d->mDropBar->y() != y)
                {
                    d->mDropBar->setRect(QRectF(item->pos().x(), y, d->mThumbnailWidth - verticalScrollBar()->width(), 3));
                }
            }
        }
        else
        {
            if (!dropAbove)
            {
                y = item->pos().y() + item->boundingRect().height() * scale + Settings::thumbnailSpacing / 2;
                if (d->mDropBar->y() != y)
                {
                    d->mDropBar->setRect(QRectF(item->pos().x(), y, d->mThumbnailWidth - verticalScrollBar()->width(), 3));
                }
            }
        }
    }

    event->acceptProposedAction();
}

void DocumentNavigator::dropEvent(QDropEvent *event)
{
    Q_UNUSED(event);

    if (d->mDropSource->sceneIndex() != d->mDropTarget->sceneIndex())
    {
        Application::boardController->moveSceneToIndex(d->mDropSource->sceneIndex(), d->mDropTarget->sceneIndex());
    }

    d->mDropSource           = NULL;
    d->mDropTarget           = NULL;
    d->mLastClickedThumbnail = NULL;

    d->mDropBar->setRect(QRectF());
    d->mDropBar->hide();
}
