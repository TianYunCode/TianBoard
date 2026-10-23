#include <QString>
#include <algorithm>
#include <QCursor>
#include <QGraphicsRectItem>

#include "ui/ThumbnailWidget.h"
#include "ui/RubberBand.h"
#include "ui/MainWindow.h"

#include <QWidget>

#include "settings/Settings.h"
#include "core/Application.h"

#include "model/DocumentProxy.h"
#include "ui/DocumentController.h"

struct ThumbnailWidget::Private
{
    QList<QGraphicsItem *>     mGraphicItems;
    QList<ThumbnailTextItem *> mLabelsItems;
    QPointF                    mMousePressScenePos;
    QPoint                     mMousePressPos;
    QList<QUrl>                mItemsPaths;
    QStringList                mLabels;
    bool                       bSelectionInProgress{false};
    bool                       bCanDrag{false};

    CoreGraphicsScene mThumbnailsScene;

    QString mMimeType;

    QPointF prevMoveMousePos;

    qreal mThumbnailWidth{0};
    qreal mThumbnailHeight{0};
    qreal mSpacing{0};

    Thumbnail            *mLastSelectedThumbnail{nullptr};
    int                   mSelectionSpan{0};
    QRectF                mPrevLassoRect;
    QGraphicsRectItem    *mLassoRectItem{nullptr};
    QSet<QGraphicsItem *> mSelectedThumbnailItems;
    QSet<QGraphicsItem *> mPreviouslyIncrementalSelectedItemsX;
    QSet<QGraphicsItem *> mPreviouslyIncrementalSelectedItemsY;
};

struct Thumbnail::Private
{
    QGraphicsRectItem *mSelectionItem{nullptr};
    bool               mAddedToScene{false};

    int                mColumn{0};
    int                mRow{0};
    ThumbnailTextItem *mLabel{nullptr};
};

struct ThumbnailTextItem::Private
{
    qreal   mWidth{0};
    QString mUnelidedText;
    bool    mIsHighlighted{false};
};

struct SceneThumbnailPixmap::Private
{
    DocumentProxy *mProxy{nullptr};
    int            mSceneIndex{0};
};

struct SceneThumbnailNavigPixmap::Private
{
    bool mEditable{false};
};

struct ThumbnailUI::ThumbnailUIIcon::Private
{
    int mPos{0};
};

ThumbnailWidget::ThumbnailWidget(QWidget *parent) : QGraphicsView(parent), d(new Private)

{
    d->mThumbnailWidth        = Settings::defaultThumbnailWidth;
    d->mSpacing               = Settings::thumbnailSpacing;
    d->mLastSelectedThumbnail = 0;
    d->mSelectionSpan         = 0;
    d->mPrevLassoRect         = QRect();
    d->mLassoRectItem         = 0;

    // 默认允许拖拽
    d->bCanDrag               = true;
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
    setFrameShape(QFrame::NoFrame);
    setScene(&d->mThumbnailsScene);

    setAlignment(Qt::AlignLeft | Qt::AlignTop);

    connect(&d->mThumbnailsScene, &QGraphicsScene::selectionChanged, this, &ThumbnailWidget::sceneSelectionChanged);
}

ThumbnailWidget::~ThumbnailWidget()
{
    disconnect(&d->mThumbnailsScene, &QGraphicsScene::selectionChanged, nullptr, nullptr);
}

qreal ThumbnailWidget::thumbnailWidth()
{
    return d->mThumbnailWidth;
}

void ThumbnailWidget::setBackgroundBrush(const QBrush &brush)
{
    d->mThumbnailsScene.setBackgroundBrush(brush);
}

const QList<QGraphicsItem *> &ThumbnailWidget::graphicItems() const
{
    return d->mGraphicItems;
}

const QList<ThumbnailTextItem *> &ThumbnailWidget::labelsItems() const
{
    return d->mLabelsItems;
}

const QPoint &ThumbnailWidget::mousePressPos() const
{
    return d->mMousePressPos;
}

qreal ThumbnailWidget::spacing()
{
    return d->mSpacing;
}

void ThumbnailWidget::setCanDrag(bool canDrag)
{
    d->bCanDrag = canDrag;
}

void ThumbnailWidget::setThumbnailWidth(qreal pThumbnailWidth)
{
    d->mThumbnailWidth = pThumbnailWidth;

    refreshScene();
}

void ThumbnailWidget::setSpacing(qreal pSpacing)
{
    d->mSpacing = pSpacing;

    refreshScene();
}

void ThumbnailWidget::setGraphicsItems(
    const QList<QGraphicsItem *> &pGraphicsItems, const QList<QUrl> &pItemsPaths, const QStringList pLabels, const QString &pMimeType)
{
    Q_ASSERT(pItemsPaths.count() == pLabels.count());
    d->mGraphicItems = pGraphicsItems;
    d->mItemsPaths   = pItemsPaths;
    d->mMimeType     = pMimeType;
    d->mLabels       = pLabels;

    foreach (QGraphicsItem *it, d->mThumbnailsScene.items())
    {
        d->mThumbnailsScene.removeItem(it, true);
    }

    // 框选矩形也随场景项一起被清掉了，这里同步置空避免悬空指针。
    d->mLassoRectItem = 0;

    foreach (QGraphicsItem *item, pGraphicsItems)
    {
        if (item->scene() != &d->mThumbnailsScene)
        {
            d->mThumbnailsScene.addItem(item);
        }
    }

    d->mLabelsItems.clear();

    foreach (const QString label, pLabels)
    {
        QFontMetrics fm(font());
        // 标题项由场景持有：替换列表时随旧场景项一起删除，或随场景析构。
        ThumbnailTextItem *labelItem = new ThumbnailTextItem(label);

        d->mThumbnailsScene.addItem(labelItem);
        d->mLabelsItems << labelItem;
    }

    refreshScene();

    d->mLastSelectedThumbnail = 0;
}

void ThumbnailWidget::refreshScene()
{
    int nbColumns    = (geometry().width() - d->mSpacing) / (d->mThumbnailWidth + d->mSpacing);

    int labelSpacing = 0;

    if (d->mLabelsItems.size() > 0)
    {
        QFontMetrics fm(d->mLabelsItems.at(0)->font());
        labelSpacing = Settings::thumbnailSpacing + fm.height();
    }
    nbColumns             = qMax(nbColumns, 1);

    qreal thumbnailHeight = d->mThumbnailWidth / Settings::minScreenRatio;

    for (int i = 0; i < d->mGraphicItems.size(); i++)
    {
        QGraphicsItem *item = d->mGraphicItems.at(i);

        qreal scaleWidth    = d->mThumbnailWidth / item->boundingRect().width();
        qreal scaleHeight   = thumbnailHeight / item->boundingRect().height();

        qreal scaleFactor   = qMin(scaleWidth, scaleHeight);

        // 位图缩略图不允许放大，否则显示会糊掉；只按需缩小。
        Thumbnail *pix      = dynamic_cast<Thumbnail *>(item);
        if (pix)
        {
            scaleFactor = qMin(scaleFactor, 1.0);
        }

        QTransform transform;
        transform.scale(scaleFactor, scaleFactor);

        item->setTransform(transform);

        item->setFlag(QGraphicsItem::ItemIsSelectable, true);

        int columnIndex = i % nbColumns;
        int rowIndex    = i / nbColumns;

        if (pix)
        {
            pix->setColumn(columnIndex);
            pix->setRow(rowIndex);
        }

        int     w = item->boundingRect().width();
        int     h = item->boundingRect().height();
        QPointF pos(d->mSpacing + (d->mThumbnailWidth - w * scaleFactor) / 2 + columnIndex * (d->mThumbnailWidth + d->mSpacing),
            d->mSpacing + rowIndex * (thumbnailHeight + d->mSpacing + labelSpacing) + (thumbnailHeight - h * scaleFactor) / 2);

        item->setPos(pos);

        if (d->mLabelsItems.size() > i)
        {
            QFontMetrics fm(d->mLabelsItems.at(i)->font(), this);
            QString      elidedText = fm.elidedText(d->mLabels.at(i), Qt::ElideRight, d->mThumbnailWidth);

            d->mLabelsItems.at(i)->setPlainText(elidedText);
            d->mLabelsItems.at(i)->setWidth(fm.horizontalAdvance(elidedText) + 2 * d->mLabelsItems.at(i)->document()->documentMargin());

            pos.setY(pos.y() + (thumbnailHeight + h * scaleFactor) / 2 + 5);
            qreal labelWidth = fm.horizontalAdvance(elidedText);
            pos.setX(d->mSpacing + (d->mThumbnailWidth - labelWidth) / 2 + columnIndex * (d->mThumbnailWidth + d->mSpacing));
            d->mLabelsItems.at(i)->setPos(pos);
        }
    }

    QScrollBar *vertScrollBar      = verticalScrollBar();
    int         scrollBarThickness = 0;
    if (vertScrollBar && vertScrollBar->isVisible())
    {
        scrollBarThickness = vertScrollBar->width();
    }

    setSceneRect(0, 0, geometry().width() - scrollBarThickness,
        d->mSpacing + ((((d->mGraphicItems.size() - 1) / nbColumns) + 1) * (thumbnailHeight + d->mSpacing + labelSpacing)));
}

QList<QGraphicsItem *> ThumbnailWidget::selectedItems()
{
    QList<QGraphicsItem *> sortedSelectedItems = d->mThumbnailsScene.selectedItems();
    std::sort(sortedSelectedItems.begin(), sortedSelectedItems.end(), thumbnailLessThan);
    return sortedSelectedItems;
}

void ThumbnailWidget::mousePressEvent(QMouseEvent *event)
{
    d->mMousePressPos          = event->pos();

    ThumbnailPixmap *sceneItem = dynamic_cast<ThumbnailPixmap *>(itemAt(d->mMousePressPos));
    if (sceneItem == NULL)
    {
        event->ignore();
        return;
    }

    d->mMousePressScenePos                   = mapToScene(d->mMousePressPos);
    QGraphicsItem *underlyingItem            = itemAt(d->mMousePressPos);
    Thumbnail     *previousSelectedThumbnail = d->mLastSelectedThumbnail;

    if (!dynamic_cast<Thumbnail *>(underlyingItem))
    {
        deleteLasso();

        RubberBand   rubberBand(QRubberBand::Rectangle);
        QStyleOption option;
        option.initFrom(&rubberBand);

        d->mPrevLassoRect = QRect();
        d->mLassoRectItem = new QGraphicsRectItem(0);
        scene()->addItem(d->mLassoRectItem);

        // 下面这套取色要跟 Qt 的 qwindowsxpstyle.cpp 保持一致，否则框选矩形和系统高亮色对不上。
        QColor highlight = option.palette.color(QPalette::Active, QPalette::Highlight);
        d->mLassoRectItem->setPen(highlight.darker(120));
        QColor dimHighlight(qMin(highlight.red() / 2 + 110, 255), qMin(highlight.green() / 2 + 110, 255), qMin(highlight.blue() / 2 + 110, 255), 127);
        d->mLassoRectItem->setBrush(dimHighlight);

        d->mLassoRectItem->setZValue(10000);
        d->mLassoRectItem->setRect(QRectF(d->mMousePressScenePos, QSizeF()));

        if (Qt::ControlModifier & event->modifiers() || Qt::ShiftModifier & event->modifiers())
        {
            return;
        }

        d->mSelectedThumbnailItems.clear();
        d->mPreviouslyIncrementalSelectedItemsX.clear();
        d->mPreviouslyIncrementalSelectedItemsY.clear();
        QGraphicsView::mousePressEvent(event);
    }
    else if (Qt::ShiftModifier & event->modifiers())
    {
        if (previousSelectedThumbnail)
        {
            QGraphicsItem *previousSelectedItem = dynamic_cast<QGraphicsItem *>(previousSelectedThumbnail);
            if (previousSelectedItem)
            {
                int index1 = d->mGraphicItems.indexOf(previousSelectedItem);
                int index2 = d->mGraphicItems.indexOf(underlyingItem);
                if (-1 == index2)
                {
                    d->mSelectedThumbnailItems = QSet<QGraphicsItem *>(selectedItems().begin(), selectedItems().end());
                    return;
                }
                d->mSelectionSpan = index2 - index1;
                selectItems(qMin(index1, index2), d->mSelectionSpan < 0 ? -d->mSelectionSpan + 1 : d->mSelectionSpan + 1);
                return;
            }
        }
    }
    else
    {
        d->mLastSelectedThumbnail = dynamic_cast<Thumbnail *>(underlyingItem);
        if (!underlyingItem->isSelected())
        {
            int index = d->mGraphicItems.indexOf(underlyingItem);
            selectItemAt(index, Qt::ControlModifier & event->modifiers());
        }
        else
        {
            QGraphicsView::mousePressEvent(event);
        }
        if (!d->mLastSelectedThumbnail && d->mGraphicItems.count() > 0)
        {
            d->mLastSelectedThumbnail = dynamic_cast<Thumbnail *>(d->mGraphicItems.at(0));
        }
        d->mSelectionSpan = 0;
        return;
    }
}

void ThumbnailWidget::mouseMoveEvent(QMouseEvent *event)
{
    int distance = (d->mMousePressPos - event->pos()).manhattanLength();

    if (0 == (event->buttons() & Qt::LeftButton) || distance < QApplication::startDragDistance())
    {
        return;
    }

    if (d->mLassoRectItem)
    {
        d->bSelectionInProgress        = true;
        int     incrementLassoMinWidth = 2;
        QPointF currentScenePos        = mapToScene(event->pos());
        QRectF  lassoRect(qMin(d->mMousePressScenePos.x(), currentScenePos.x()), qMin(d->mMousePressScenePos.y(), currentScenePos.y()),
            qAbs(d->mMousePressScenePos.x() - currentScenePos.x()), qAbs(d->mMousePressScenePos.y() - currentScenePos.y()));
        if (QPoint() == d->prevMoveMousePos)
        {
            d->prevMoveMousePos = currentScenePos;
        }
        QRectF incrementXSelection(qMin(d->prevMoveMousePos.x(), currentScenePos.x()), qMin(d->mMousePressScenePos.y(), currentScenePos.y()),
            qAbs(d->prevMoveMousePos.x() - currentScenePos.x()) + incrementLassoMinWidth, qAbs(d->mMousePressScenePos.y() - currentScenePos.y()));
        QRectF incrementYSelection(qMin(d->mMousePressScenePos.x(), currentScenePos.x()), qMin(d->prevMoveMousePos.y(), currentScenePos.y()),
            qAbs(d->mMousePressScenePos.x() - currentScenePos.x()), qAbs(d->prevMoveMousePos.y() - currentScenePos.y()) + incrementLassoMinWidth);

        d->prevMoveMousePos = currentScenePos;
        d->mLassoRectItem->setRect(lassoRect);

        QSet<QGraphicsItem *> lassoSelectedThumbnailItems;

        QSet<QGraphicsItem *> toUnset;
        QSet<QGraphicsItem *> toSet;

        // 横向增量框选：只看这次多出来/少掉的那一条竖带
        QSet<QGraphicsItem *> incSelectedItemsX = QSet<QGraphicsItem *>(scene()->items(incrementXSelection, Qt::IntersectsItemBoundingRect).begin(),
            scene()->items(incrementXSelection, Qt::IntersectsItemBoundingRect).end());
        foreach (QGraphicsItem *lassoSelectedItem, incSelectedItemsX)
        {
            if (lassoSelectedItem)
            {
                ThumbnailPixmap *thumbnailItem = dynamic_cast<ThumbnailPixmap *>(lassoSelectedItem);
                if (thumbnailItem)
                {
                    lassoSelectedThumbnailItems += lassoSelectedItem;
                }
            }
        }

        if (lassoRect.width() < d->mPrevLassoRect.width())
        {
            // 框收窄了：上一轮增量选中、这一轮已不在框里的项要取消。差集本身就覆盖
            // 了各种情况，原来外面还套了一层 contains(上一轮整个集合)——QSet 没有
            // "包含一个集合"这种重载，留着只会误导。
            toUnset += d->mPreviouslyIncrementalSelectedItemsX - lassoSelectedThumbnailItems;
        }

        // 累加选择要"这轮框里的 ∪ 上一轮框里的"，必须在覆盖旧值之前算——
        // 原来先赋值再求并集，两个操作数已经是同一个集合，累积选择整个丢掉。
        toSet                                   += lassoSelectedThumbnailItems + d->mPreviouslyIncrementalSelectedItemsX;
        d->mPreviouslyIncrementalSelectedItemsX  = lassoSelectedThumbnailItems;

        lassoSelectedThumbnailItems.clear();

        // 纵向增量框选：同理，只看横带部分

        QSet<QGraphicsItem *> incSelectedItemsY = QSet<QGraphicsItem *>(scene()->items(incrementYSelection, Qt::IntersectsItemBoundingRect).begin(),
            scene()->items(incrementYSelection, Qt::IntersectsItemBoundingRect).end());
        foreach (QGraphicsItem *lassoSelectedItem, incSelectedItemsY)
        {
            if (lassoSelectedItem)
            {
                ThumbnailPixmap *thumbnailItem = dynamic_cast<ThumbnailPixmap *>(lassoSelectedItem);

                if (thumbnailItem)
                {
                    lassoSelectedThumbnailItems += lassoSelectedItem;
                }
            }
        }

        if (lassoRect.height() < d->mPrevLassoRect.height())
        {
            // 框收窄了：上一轮增量选中、这一轮已不在框里的项要取消。差集本身就覆盖
            // 了各种情况，原来外面还套了一层 contains(上一轮整个集合)——QSet 没有
            // "包含一个集合"这种重载，留着只会误导。
            toUnset += d->mPreviouslyIncrementalSelectedItemsY - lassoSelectedThumbnailItems;
        }

        // 累加选择要"这轮框里的 ∪ 上一轮框里的"，必须在覆盖旧值之前算——
        // 原来先赋值再求并集，两个操作数已经是同一个集合，累积选择整个丢掉。
        toSet                                   += lassoSelectedThumbnailItems + d->mPreviouslyIncrementalSelectedItemsY;
        d->mPreviouslyIncrementalSelectedItemsY  = lassoSelectedThumbnailItems;

        toSet                                   -= toUnset;

        foreach (QGraphicsItem *item, toSet)
        {
            item->setSelected(true);
        }

        foreach (QGraphicsItem *item, toUnset)
        {
            item->setSelected(false);
        }

        d->mSelectedThumbnailItems += lassoSelectedThumbnailItems;
        d->mPrevLassoRect           = lassoRect;

        if (Qt::ControlModifier & event->modifiers())
        {
            // 每一项都要选中，包括最后一项（原来上界写成 count() - 1，最后一项永远漏）；
            // 而且 values() 每轮都重建一张临时表，先取一次再遍历。
            const QList<QGraphicsItem *> incremental = d->mSelectedThumbnailItems.values();
            for (QGraphicsItem *item : incremental)
            {
                item->setSelected(true);
            }
        }
    }
    else
    {
        d->bSelectionInProgress = false;
        if (0 == selectedItems().size())
        {
            return;
        }

        if (d->bCanDrag)
        {
            QDrag     *drag = new QDrag(this);
            QMimeData *mime = new QMimeData();

            if (d->mMimeType.length() > 0)
            {
                mime->setData(d->mMimeType,
                    QByteArray()); // 拖放系统只认内置类型，这里让它把我们的自定义 MIME 也登记进来
            }

            drag->setMimeData(mime);

            QList<QUrl> qlElements;

            foreach (QGraphicsItem *item, selectedItems())
            {
                if (d->mGraphicItems.contains(item))
                {
                    if (d->mGraphicItems.indexOf(item) <= d->mItemsPaths.size())
                    {
                        qlElements << d->mItemsPaths.at(d->mGraphicItems.indexOf(item));
                    }
                }
            }

            if (qlElements.size() > 0)
            {
                mime->setUrls(qlElements);
                drag->setMimeData(mime);
                drag->exec(Qt::CopyAction);
            }
        }
    }

    QGraphicsView::mouseMoveEvent(event);
}

void ThumbnailWidget::mouseReleaseEvent(QMouseEvent *event)
{
    d->prevMoveMousePos = QPoint();
    deleteLasso();
    QGraphicsView::mouseReleaseEvent(event);

    // 这里以前套着一个 elapsed() < STARTDRAGTIME(=1000000) 的判断：QElapsedTimer::elapsed()
    // 的单位是毫秒，那个阈值等于 16.6 分钟，条件恒真。查过接收方再决定怎么处理这个"恒真"：
    // DocumentController::pageClicked 把 item 与 index 都 Q_UNUSED 掉，只调
    // pageSelectionChanged() 刷新动作可用性——框选结束正需要这一次刷新。所以既不照字面
    // 去补一个时间阈值（那会把松手后的刷新丢掉），也不改成按位移判断（同理），
    // 而是把恒真的判断删掉、直接发，语义与现在的实际行为一致，且不再谎称有防抖。
    emit mouseClick(itemAt(event->pos()), 0);
}

void ThumbnailWidget::keyPressEvent(QKeyEvent *event)
{
    if (d->mLastSelectedThumbnail)
    {
        QGraphicsItem *lastSelectedGraphicsItem = dynamic_cast<QGraphicsItem *>(d->mLastSelectedThumbnail);
        if (!lastSelectedGraphicsItem)
        {
            return;
        }
        int startSelectionIndex            = d->mGraphicItems.indexOf(lastSelectedGraphicsItem);
        int previousSelectedThumbnailIndex = startSelectionIndex + d->mSelectionSpan;

        switch (event->key())
        {
            case Qt::Key_Down:
            case Qt::Key_Up:
            {
                if (rowCount() <= 1)
                {
                    break;
                }
                if (Qt::ShiftModifier & event->modifiers())
                {
                    int endSelectionIndex;
                    if (Qt::Key_Down == event->key())
                    {
                        endSelectionIndex = previousSelectedThumbnailIndex + columnCount();
                        if (endSelectionIndex >= d->mGraphicItems.count())
                        {
                            break;
                        }
                    }
                    else
                    {
                        endSelectionIndex = previousSelectedThumbnailIndex - columnCount();
                        if (endSelectionIndex < 0)
                        {
                            break;
                        }
                    }

                    int startIndex    = startSelectionIndex < endSelectionIndex ? startSelectionIndex : endSelectionIndex;
                    int count         = startSelectionIndex < endSelectionIndex ? endSelectionIndex - startSelectionIndex + 1
                                                                                : startSelectionIndex - endSelectionIndex + 1;
                    d->mSelectionSpan = startSelectionIndex < endSelectionIndex ? (count - 1) : -(count - 1);
                    selectItems(startIndex, count);
                }
                else
                {
                    int toSelectIndex;
                    if (Qt::Key_Down == event->key())
                    {
                        toSelectIndex = previousSelectedThumbnailIndex + columnCount();
                        if (toSelectIndex >= d->mGraphicItems.count())
                        {
                            break;
                        }
                    }
                    else
                    {
                        toSelectIndex = previousSelectedThumbnailIndex - columnCount();
                        if (toSelectIndex < 0)
                        {
                            break;
                        }
                    }

                    selectItemAt(toSelectIndex, Qt::ControlModifier & event->modifiers());
                    d->mSelectionSpan = 0;
                }
            }
            break;

            case Qt::Key_Left:
            case Qt::Key_Right:
            {
                QGraphicsItem *previousSelectedItem      = d->mGraphicItems.at(previousSelectedThumbnailIndex);
                Thumbnail     *previousSelectedThumbnail = dynamic_cast<Thumbnail *>(previousSelectedItem);
                if (!previousSelectedThumbnail)
                {
                    break;
                }

                if (Qt::Key_Left == event->key())
                {
                    if (0 == previousSelectedThumbnail->column())
                    {
                        break;
                    }
                }
                else
                {
                    if (previousSelectedThumbnail->column() == columnCount() - 1 || previousSelectedThumbnailIndex == d->mGraphicItems.count() - 1)
                    {
                        break;
                    }
                }

                if (Qt::ShiftModifier & event->modifiers())
                {
                    int endSelectionIndex;
                    if (Qt::Key_Left == event->key())
                    {
                        endSelectionIndex = previousSelectedThumbnailIndex - 1;
                        if (endSelectionIndex < 0)
                        {
                            break;
                        }
                    }
                    else
                    {
                        endSelectionIndex = previousSelectedThumbnailIndex + 1;
                        if (endSelectionIndex >= d->mGraphicItems.count())
                        {
                            break;
                        }
                    }

                    int startIndex    = startSelectionIndex < endSelectionIndex ? startSelectionIndex : endSelectionIndex;
                    int count         = startSelectionIndex < endSelectionIndex ? endSelectionIndex - startSelectionIndex + 1
                                                                                : startSelectionIndex - endSelectionIndex + 1;
                    d->mSelectionSpan = startSelectionIndex < endSelectionIndex ? (count - 1) : -(count - 1);
                    selectItems(startIndex, count);
                }
                else
                {
                    if (Qt::Key_Left == event->key())
                    {
                        selectItemAt(previousSelectedThumbnailIndex - 1, Qt::ControlModifier & event->modifiers());
                    }
                    else
                    {
                        selectItemAt(previousSelectedThumbnailIndex + 1, Qt::ControlModifier & event->modifiers());
                    }

                    d->mSelectionSpan = 0;
                }
            }
            break;

            case Qt::Key_Home:
            {
                if (Qt::ShiftModifier & event->modifiers())
                {
                    d->mSelectionSpan = -startSelectionIndex;
                    selectItems(0, startSelectionIndex + 1);
                }
                else
                {
                    selectItemAt(0, Qt::ControlModifier & event->modifiers());
                    d->mSelectionSpan = 0;
                }
            }
            break;

            case Qt::Key_End:
            {
                if (Qt::ShiftModifier & event->modifiers())
                {
                    d->mSelectionSpan = d->mGraphicItems.count() - startSelectionIndex - 1;
                    selectItems(startSelectionIndex, d->mSelectionSpan + 1);
                }
                else
                {
                    selectItemAt(d->mGraphicItems.count() - 1, Qt::ControlModifier & event->modifiers());
                    d->mSelectionSpan = 0;
                }
            }
            break;
            case Qt::Key_A:
            {
                if (Qt::ControlModifier & event->modifiers())
                {
                    selectAll();
                }
            }
            break;
        }
    }
    QGraphicsView::keyPressEvent(event);
}

void ThumbnailWidget::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event);

    if (0 == selectedItems().count() && d->mGraphicItems.count() > 0 && Qt::TabFocusReason == event->reason())
    {
        selectItemAt(0);
        d->mSelectionSpan = 0;
    }
}

void ThumbnailWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);

    refreshScene();

    emit resized();
}

void ThumbnailWidget::sceneSelectionChanged()
{
    emit selectionChanged();
}

void ThumbnailWidget::selectItemAt(int pIndex, bool extend)
{
    QGraphicsItem *itemToSelect = 0;

    if (pIndex >= 0 && pIndex < d->mGraphicItems.size())
    {
        itemToSelect = d->mGraphicItems.at(pIndex);
    }

    foreach (QGraphicsItem *item, items())
    {
        if (item == itemToSelect)
        {
            d->mLastSelectedThumbnail = dynamic_cast<Thumbnail *>(item);
            item->setSelected(true);
            ensureVisible(item);
        }
        else if (!extend)
        {
            item->setSelected(false);
        }
    }
}

void ThumbnailWidget::unselectItemAt(int pIndex)
{
    if (pIndex >= 0 && pIndex < d->mGraphicItems.size())
    {
        QGraphicsItem *itemToUnselect = d->mGraphicItems.at(pIndex);
        itemToUnselect->setSelected(false);
    }
}

void ThumbnailWidget::selectItems(int startIndex, int count)
{
    for (int i = 0; i < d->mGraphicItems.count(); i++)
    {
        d->mGraphicItems.at(i)->setSelected(i >= startIndex && i < startIndex + count);
    }
}

void ThumbnailWidget::selectAll()
{
    foreach (QGraphicsItem *item, d->mGraphicItems)
    {
        item->setSelected(true);
    }
}

int ThumbnailWidget::rowCount() const
{
    Thumbnail *lastThumbnail = dynamic_cast<Thumbnail *>(d->mGraphicItems.last());
    return lastThumbnail ? lastThumbnail->row() + 1 : 0;
}

int ThumbnailWidget::columnCount() const
{
    Thumbnail *lastThumbnail = dynamic_cast<Thumbnail *>(d->mGraphicItems.last());
    if (!lastThumbnail)
    {
        return 0;
    }
    int lastRow    = lastThumbnail->row();
    int lastColumn = lastThumbnail->column();
    return lastRow > 0 ? (d->mGraphicItems.count() - lastColumn - 1) / lastRow : d->mGraphicItems.count();
}

void ThumbnailWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    QGraphicsItem *item = itemAt(event->pos());

    if (item)
    {
        int  index = d->mGraphicItems.indexOf(item);
        emit mouseDoubleClick(item, index);
    }
}

bool ThumbnailWidget::thumbnailLessThan(QGraphicsItem *item1, QGraphicsItem *item2)
{
    Thumbnail *thumbnail1 = dynamic_cast<Thumbnail *>(item1);
    Thumbnail *thumbnail2 = dynamic_cast<Thumbnail *>(item2);
    if (thumbnail1 && thumbnail2)
    {
        if (thumbnail1->row() != thumbnail2->row())
        {
            return thumbnail1->row() < thumbnail2->row();
        }
        else
        {
            return thumbnail1->column() < thumbnail2->column();
        }
    }
    return false;
}

void ThumbnailWidget::deleteLasso()
{
    if (d->mLassoRectItem && scene())
    {
        scene()->removeItem(d->mLassoRectItem);
        delete d->mLassoRectItem;
        d->mLassoRectItem = 0;
    }
}

Thumbnail::Thumbnail() : d(new Private)
{
    d->mSelectionItem = new QGraphicsRectItem(0, 0, 0, 0);
    d->mSelectionItem->setPen(QPen(Settings::treeViewBackgroundColor, 8));
}

Thumbnail::~Thumbnail()
{
    if (d->mSelectionItem && !d->mAddedToScene)
    {
        delete d->mSelectionItem;
    }
}

void Thumbnail::itemChange(QGraphicsItem *item, QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
    Q_UNUSED(value);

    if ((change == QGraphicsItem::ItemSelectedHasChanged || change == QGraphicsItem::ItemTransformHasChanged ||
            change == QGraphicsItem::ItemPositionHasChanged) &&
        item->scene())
    {
        if (item->isSelected())
        {
            if (!d->mSelectionItem->scene())
            {
                item->scene()->addItem(d->mSelectionItem);
                d->mSelectionItem->setZValue(item->zValue() - 1);
                d->mAddedToScene = true;
            }

            d->mSelectionItem->setRect(item->sceneBoundingRect().x() - 5, item->sceneBoundingRect().y() - 5, item->sceneBoundingRect().width() + 10,
                item->sceneBoundingRect().height() + 10);

            d->mSelectionItem->show();
        }
        else
        {
            d->mSelectionItem->hide();
        }
    }
}

int Thumbnail::column()
{
    return d->mColumn;
}

void Thumbnail::setColumn(int column)
{
    d->mColumn = column;
}

int Thumbnail::row()
{
    return d->mRow;
}

void Thumbnail::setRow(int row)
{
    d->mRow = row;
}

ThumbnailTextItem *Thumbnail::label()
{
    return d->mLabel;
}

void Thumbnail::setLabel(ThumbnailTextItem *label)
{
    d->mLabel = label;
}

ThumbnailTextItem::ThumbnailTextItem(int index) : QGraphicsTextItem(tr("Page %0").arg(index + 1)), d(new Private)
{
    d->mWidth         = 0;
    d->mUnelidedText  = toPlainText();
    d->mIsHighlighted = false;
}

ThumbnailTextItem::ThumbnailTextItem(const QString &text) : QGraphicsTextItem(text), d(new Private)
{
    d->mWidth         = 0;
    d->mUnelidedText  = text;
    d->mIsHighlighted = false;

    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
}

ThumbnailTextItem::~ThumbnailTextItem() = default;

QRectF ThumbnailTextItem::boundingRect() const
{
    return QRectF(QPointF(0.0, 0.0), QSize(d->mWidth, QFontMetricsF(font()).height() + 5));
}

void ThumbnailTextItem::setWidth(qreal pWidth)
{
    if (d->mWidth != pWidth)
    {
        prepareGeometryChange();
        d->mWidth = pWidth;
        computeText();
    }
}

qreal ThumbnailTextItem::width()
{
    return d->mWidth;
}

void ThumbnailTextItem::highlight()
{
    if (!d->mIsHighlighted)
    {
        d->mIsHighlighted = true;
        computeText();
    }
}

void ThumbnailTextItem::computeText()
{
    QFontMetricsF fm(font());
    QString       elidedText = fm.elidedText(d->mUnelidedText, Qt::ElideRight, d->mWidth);

    if (d->mIsHighlighted)
    {
        setHtml("<span style=\"color: #6682b5\">" + elidedText + "</span>");
    }
    else
    {
        setPlainText(elidedText);
    }
}

SceneThumbnailPixmap::SceneThumbnailPixmap(const QPixmap &pix, DocumentProxy *proxy, int pSceneIndex) : ThumbnailPixmap(pix), d(new Private)
{
    d->mProxy      = proxy;
    d->mSceneIndex = pSceneIndex;
}

SceneThumbnailPixmap::~SceneThumbnailPixmap() = default;

DocumentProxy *SceneThumbnailPixmap::proxy()
{
    return d->mProxy;
}

int SceneThumbnailPixmap::sceneIndex()
{
    return d->mSceneIndex;
}

void SceneThumbnailPixmap::highlight()
{
}

SceneThumbnailNavigPixmap::SceneThumbnailNavigPixmap(const QPixmap &pix, DocumentProxy *proxy, int pSceneIndex)
    : SceneThumbnailPixmap(pix, proxy, pSceneIndex), d(new Private)
{
    d->mEditable = false;

    if (0 <= DocumentContainer::pageFromSceneIndex(pSceneIndex))
    {
        setAcceptHoverEvents(true);
        setFlag(QGraphicsItem::ItemIsSelectable, true);
    }
}

SceneThumbnailNavigPixmap::~SceneThumbnailNavigPixmap()
{
}

bool SceneThumbnailNavigPixmap::editable()
{
    return d->mEditable;
}

bool SceneThumbnailNavigPixmap::deletable()
{
    return proxy()->pageCount() > 1;
}

bool SceneThumbnailNavigPixmap::movableUp()
{
    return sceneIndex() > 0;
}

bool SceneThumbnailNavigPixmap::movableDown()
{
    return sceneIndex() < (proxy()->pageCount() - 1);
}

void SceneThumbnailNavigPixmap::showUI()
{
    setEditable(true);
}

void SceneThumbnailNavigPixmap::hideUI()
{
    setEditable(false);
}

void SceneThumbnailNavigPixmap::setEditable(bool editable)
{
    d->mEditable = editable;
}

void SceneThumbnailNavigPixmap::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    event->accept();
    showUI();
    update();
}

void SceneThumbnailNavigPixmap::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    event->accept();
    hideUI();
    update();
}

void SceneThumbnailNavigPixmap::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    SceneThumbnailPixmap::paint(painter, option, widget);
    using namespace ThumbnailUI;

    if (editable())
    {
        if (deletable())
        {
            draw(painter, *getIcon("close"));
        }
        else
        {
            draw(painter, *getIcon("closeDisabled"));
        }

        draw(painter, *getIcon("duplicate"));
    }
}

void SceneThumbnailNavigPixmap::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF p = event->pos();

    using namespace ThumbnailUI;

    if (triggered(p.y()))
    {
        if (deletable() && getIcon("close")->triggered(p.x()))
        {
            event->accept();
            deletePage();
        }
        else if (getIcon("duplicate")->triggered(p.x()))
        {
            event->accept();
            duplicatePage();
        }
        else
        {
            event->ignore();
        }
        /*
        else if(movableUp() && getIcon("moveUp")->triggered(p.x()))
        {
            event->accept();
            moveUpPage();
        }
        else if (movableDown() && getIcon("moveDown")->triggered(p.x()))
        {
            event->accept();
            moveDownPage();
        }
        */
    }
    else
    {
        event->ignore();
    }
}

void SceneThumbnailNavigPixmap::deletePage()
{
    if (Application::mainWindow->yesNoQuestion(
            QObject::tr("Remove Page"), QObject::tr("Are you sure you want to remove 1 page from the selected document '%0'?")
                                            .arg(proxy()->metaData(Settings::documentName).toString())))
    {
        BoardController::instance()->deleteScene(sceneIndex());
    }
}

void SceneThumbnailNavigPixmap::duplicatePage()
{
    BoardController::instance()->duplicateScene(sceneIndex());
}

void SceneThumbnailNavigPixmap::moveUpPage()
{
    if (sceneIndex() != 0)
    {
        BoardController::instance()->moveSceneToIndex(sceneIndex(), sceneIndex() - 1);
    }
}

void SceneThumbnailNavigPixmap::moveDownPage()
{
    if (sceneIndex() < BoardController::instance()->selectedDocument()->pageCount() - 1)
    {
        BoardController::instance()->moveSceneToIndex(sceneIndex(), sceneIndex() + 1);
    }
}

void ImgTextThumbnailElement::Place(int row, int col, qreal width, qreal height)
{
    int labelSpacing = 0;
    if (this->caption)
    {
        QFontMetrics fm(this->caption->font());
        labelSpacing = Settings::thumbnailSpacing + fm.height();
    }
    if (this->thumbnail)
    {
        int w                  = this->thumbnail->boundingRect().width();
        int h                  = this->thumbnail->boundingRect().height();

        qreal      scaleWidth  = width / w;
        qreal      scaleHeight = height / h;
        qreal      scaleFactor = qMin(scaleWidth, scaleHeight);
        Thumbnail *pix         = dynamic_cast<Thumbnail *>(this->thumbnail);

        QTransform transform;
        transform.scale(scaleFactor, scaleFactor);

        // Apply the scaling
        this->thumbnail->setTransform(transform);
        this->thumbnail->setFlag(QGraphicsItem::ItemIsSelectable, true);

        if (pix)
        {
            pix->setColumn(col);
            pix->setRow(row);
        }

        QPointF pos((width - w * scaleFactor) / 2, row * (height + labelSpacing) + (height - h * scaleFactor) / 2);

        this->thumbnail->setPos(pos);

        if (this->caption)
        {
            QFontMetrics fm(this->caption->font());
            QString      elidedText = fm.elidedText(this->caption->toPlainText(), Qt::ElideRight, width);

            this->caption->setPlainText(elidedText);
            this->caption->setWidth(fm.horizontalAdvance(elidedText) + 2 * this->caption->document()->documentMargin());
            pos.setY(pos.y() + (height + h * scaleFactor) / 2 + 5); // What is this 5 ??
            qreal labelWidth = fm.horizontalAdvance(elidedText);
            pos.setX((width - labelWidth) / 2 + col * (width + border));
            this->caption->setPos(pos);
        }
    }
}

ThumbnailUI::ThumbnailUIIcon::ThumbnailUIIcon(const QString &filename, int pos)
    : QPixmap(QSize(ThumbnailUI::ICONSIZE, ThumbnailUI::ICONSIZE)), d(new Private)
{
    d->mPos = pos;

    QSvgRenderer svgRenderer(filename);
    QPainter     painter;
    fill(Qt::transparent);
    painter.begin(this);
    svgRenderer.render(&painter);
    painter.end();
}

ThumbnailUI::ThumbnailUIIcon::~ThumbnailUIIcon() = default;

int ThumbnailUI::ThumbnailUIIcon::pos() const
{
    return d->mPos;
}

bool ThumbnailUI::ThumbnailUIIcon::triggered(qreal x) const
{
    using namespace ThumbnailUI;
    return (x >= pos() * (ICONSIZE + ICONSPACING) && x <= (pos() + 1) * ICONSIZE + pos() * ICONSPACING);
}

ThumbnailUI::ThumbnailUIIcon *ThumbnailUI::addIcon(const QString &thumbnailIcon, int pos)
{
    QString          thumbnailIconPath = ":images/" + thumbnailIcon + ".svg";
    ThumbnailUIIcon *newIcon           = new ThumbnailUIIcon(thumbnailIconPath, pos);

    using namespace ThumbnailUI::_private;
    if (!newIcon)
    {
        qDebug() << "加不上图标，检查这个路径:" + thumbnailIconPath;
    }
    else
    {
        catalog.insert(thumbnailIcon, newIcon);
    }

    return newIcon;
}

ThumbnailUI::ThumbnailUIIcon *ThumbnailUI::getIcon(const QString &thumbnailIcon)
{
    using namespace ThumbnailUI::_private;
    if (!catalog.contains(thumbnailIcon))
    {
        qDebug() << "取不到图标，检查这个路径 ':images/" + thumbnailIcon + ".svg'";
    }

    return catalog.value(thumbnailIcon, NULL);
}

void ThumbnailUI::draw(QPainter *painter, const ThumbnailUIIcon &thumbnailIcon)
{
    using namespace ThumbnailUI;
    painter->drawPixmap(thumbnailIcon.pos() * (ICONSIZE + ICONSPACING), 0, ICONSIZE, ICONSIZE, thumbnailIcon);
}

void ThumbnailUI::_private::initCatalog()
{
    using namespace ThumbnailUI;
    using namespace ThumbnailUI::_private;

    addIcon("close", 0);
    addIcon("closeDisabled", 0);

    addIcon("duplicate", 1);

    addIcon("moveUp", 2);
    addIcon("moveUpDisabled", 2);

    addIcon("moveDown", 3);
    addIcon("moveDownDisabled", 3);
}

bool ThumbnailUI::triggered(qreal y)
{
    return (y >= 0 && y <= ThumbnailUI::ICONSIZE);
}
