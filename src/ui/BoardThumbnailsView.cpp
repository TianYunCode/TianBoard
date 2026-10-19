#include <QList>
#include <QPointF>
#include <QPixmap>
#include <QTransform>
#include <QScrollBar>
#include <QFontMetrics>
#include <QGraphicsItem>
#include <QGraphicsPixmapItem>

#include "core/Application.h"
#include "ui/BoardThumbnailsView.h"
#include "whiteboard/BoardController.h"
#include "content/ThumbnailAdaptor.h"
#include "content/SvgSubsetAdaptor.h"
#include "model/DocumentController.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/BoardPaletteManager.h"
#include "core/ApplicationController.h"
#include "core/PersistenceManager.h"
#include "ui/ThumbnailView.h"

struct BoardThumbnailsView::Private
{
    QList<DraggableThumbnailView*> mThumbnails;
    int mThumbnailWidth{0};
    int mThumbnailMinWidth{0};
    int mMargin{0};
    DraggableThumbnailView* mDropSource{nullptr};
    DraggableThumbnailView* mDropTarget{nullptr};
    QGraphicsRectItem* mDropBar{nullptr};
    int mLongPressInterval{0};
    QTimer mLongPressTimer;
    QPoint mLastPressedMousePos;
};

BoardThumbnailsView::BoardThumbnailsView(QWidget *parent, const char *name)
    : QGraphicsView(parent)
    , d(new Private)
{
    d->mThumbnailMinWidth = 60;
    d->mMargin = 20;
    d->mDropBar = new QGraphicsRectItem(0);
    d->mLongPressInterval = 350;

    setScene(new QGraphicsScene(this));    

    d->mDropBar->setPen(QPen(Qt::darkGray));
    d->mDropBar->setBrush(QBrush(Qt::lightGray));
    scene()->addItem(d->mDropBar);
    d->mDropBar->hide();

    setObjectName(name);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameShadow(QFrame::Plain);

    d->mThumbnailWidth = width() - 2*d->mMargin;

    d->mLongPressTimer.setInterval(d->mLongPressInterval);
    d->mLongPressTimer.setSingleShot(true);

    connect(Application::boardController, SIGNAL(initThumbnailsRequired(DocumentContainer*)), this, SLOT(initThumbnails(DocumentContainer*)), Qt::UniqueConnection);
    connect(Application::boardController, SIGNAL(addThumbnailRequired(DocumentContainer*, int)), this, SLOT(addThumbnail(DocumentContainer*, int)), Qt::UniqueConnection);
    connect(Application::boardController, SIGNAL(moveThumbnailRequired(int, int)), this, SLOT(moveThumbnail(int, int)), Qt::UniqueConnection);
    connect(this, SIGNAL(moveThumbnailRequired(int, int)), this, SLOT(moveThumbnail(int, int)), Qt::UniqueConnection);
    connect(Application::boardController, SIGNAL(updateThumbnailsRequired()), this, SLOT(updateThumbnails()), Qt::UniqueConnection);
    connect(Application::boardController, SIGNAL(removeThumbnailRequired(int)), this, SLOT(removeThumbnail(int)), Qt::UniqueConnection);

    connect(&d->mLongPressTimer, SIGNAL(timeout()), this, SLOT(longPressTimeout()), Qt::UniqueConnection);

    connect(this, SIGNAL(mousePressAndHoldEventRequired(QPoint)), this, SLOT(mousePressAndHoldEvent(QPoint)), Qt::UniqueConnection);

    connect(Application::boardController, SIGNAL(pageSelectionChanged(int)), this, SLOT(ensureVisibleThumbnail(int)), Qt::UniqueConnection);
    connect(Application::boardController, SIGNAL(centerOnThumbnailRequired(int)), this, SLOT(centerOnThumbnail(int)), Qt::UniqueConnection);
}

BoardThumbnailsView::~BoardThumbnailsView() = default;

void BoardThumbnailsView::moveThumbnail(int from, int to)
{
    d->mThumbnails.move(from, to);

    updateThumbnailsPos();
}

void BoardThumbnailsView::updateThumbnails()
{
    updateThumbnailsPos();
}

void BoardThumbnailsView::removeThumbnail(int i)
{
    DraggableThumbnailView* item = d->mThumbnails.at(i);

    scene()->removeItem(item->pageNumber());
    scene()->removeItem(item);

    d->mThumbnails.removeAt(i);

    updateThumbnailsPos();
}

DraggableThumbnailView* BoardThumbnailsView::createThumbnail(DocumentContainer* source, int i)
{
    Application::showMessage(tr("Loading page (%1/%2)").arg(i+1).arg(source->selectedDocument()->pageCount()));

    GraphicsScene* pageScene = PersistenceManager::persistenceManager()->loadDocumentScene(source->selectedDocument(), i);
    ThumbnailView* pageView = new ThumbnailView(pageScene);

    return new DraggableThumbnailView(pageView, source->selectedDocument(), i);
}

void BoardThumbnailsView::addThumbnail(DocumentContainer* source, int i)
{
    DraggableThumbnailView* item = createThumbnail(source, i);
    d->mThumbnails.insert(i, item);

    scene()->addItem(item);
    scene()->addItem(item->pageNumber());

    updateThumbnailsPos();
}

void BoardThumbnailsView::clearThumbnails()
{
    for(int i = 0; i < d->mThumbnails.size(); i++)
    {
        scene()->removeItem(d->mThumbnails.at(i)->pageNumber());
        scene()->removeItem(d->mThumbnails.at(i));
        d->mThumbnails.at(i)->deleteLater();
    }

    d->mThumbnails.clear();
}

void BoardThumbnailsView::initThumbnails(DocumentContainer* source)
{
    clearThumbnails();

    for(int i = 0; i < source->selectedDocument()->pageCount(); i++)
    {
        d->mThumbnails.append(createThumbnail(source, i));

        scene()->addItem(d->mThumbnails.last());
        scene()->addItem(d->mThumbnails.last()->pageNumber());
    }

    updateThumbnailsPos();
}

void BoardThumbnailsView::centerOnThumbnail(int index)
{
    centerOn(d->mThumbnails.at(index));
}

void BoardThumbnailsView::ensureVisibleThumbnail(int index)
{
    ensureVisible(d->mThumbnails.at(index));
}

void BoardThumbnailsView::updateThumbnailsPos()
{    
    qreal thumbnailHeight = d->mThumbnailWidth / Settings::minScreenRatio;

    for (int i=0; i < d->mThumbnails.length(); i++)
    {
        d->mThumbnails.at(i)->setSceneIndex(i);
        d->mThumbnails.at(i)->setPageNumber(i);
        d->mThumbnails.at(i)->updatePos(d->mThumbnailWidth, thumbnailHeight);
    }

    scene()->setSceneRect(0, 0, scene()->itemsBoundingRect().size().width() - verticalScrollBar()->width(), scene()->itemsBoundingRect().size().height());

    update();
}

void BoardThumbnailsView::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);

    // Update the thumbnails width
    d->mThumbnailWidth = (width() > d->mThumbnailMinWidth) ? width() - verticalScrollBar()->width() - 2*d->mMargin : d->mThumbnailMinWidth;

    // Refresh the scene
    updateThumbnailsPos();

    emit Application::boardController->centerOnThumbnailRequired(Application::boardController->activeSceneIndex());
}

void BoardThumbnailsView::mousePressEvent(QMouseEvent *event)
{
    QGraphicsView::mousePressEvent(event);

    if (!event->isAccepted())
    {
        d->mLongPressTimer.start();
        d->mLastPressedMousePos = event->pos();

        DraggableThumbnailView* item = dynamic_cast<DraggableThumbnailView*>(itemAt(event->pos()));

        if (item)
        {
            Application::boardController->persistViewPositionOnCurrentScene();
            Application::boardController->persistCurrentScene();
            Application::boardController->setActiveDocumentScene(item->sceneIndex());
            Application::boardController->centerOn(Application::boardController->activeScene()->lastCenter());
        }
    }
}

void BoardThumbnailsView::mouseMoveEvent(QMouseEvent *event)
{
    QGraphicsView::mouseMoveEvent(event);
}

void BoardThumbnailsView::longPressTimeout()
{
    if (QApplication::mouseButtons() != Qt::NoButton)
        emit mousePressAndHoldEventRequired(d->mLastPressedMousePos);

    d->mLongPressTimer.stop();
}

void BoardThumbnailsView::mousePressAndHoldEvent(QPoint pos)
{
    DraggableThumbnailView* item = dynamic_cast<DraggableThumbnailView*>(itemAt(pos));
    if (item)
    {
        d->mDropSource = item;
        d->mDropTarget = item;

        QPixmap pixmap = item->widget()->grab().scaledToWidth(d->mThumbnailWidth/2);

        QDrag *drag = new QDrag(this);
        drag->setMimeData(new QMimeData());
        drag->setPixmap(pixmap);
        drag->setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));

        drag->exec();
    }   
}

void BoardThumbnailsView::mouseReleaseEvent(QMouseEvent *event)
{
    d->mLongPressTimer.stop();

    QGraphicsView::mouseReleaseEvent(event);
}

void BoardThumbnailsView::dragEnterEvent(QDragEnterEvent *event)
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

void BoardThumbnailsView::dragMoveEvent(QDragMoveEvent *event)
{        
    QPointF position = event->pos();

    //autoscroll during drag'n'drop
    QPointF scenePos = mapToScene(position.toPoint());
    int thumbnailHeight = d->mThumbnailWidth / Settings::minScreenRatio;
    QRectF thumbnailArea(0, scenePos.y() - thumbnailHeight/2, d->mThumbnailWidth, thumbnailHeight);

    ensureVisible(thumbnailArea);

    DraggableThumbnailView* item = dynamic_cast<DraggableThumbnailView*>(itemAt(position.toPoint()));
    if (item)
    {
        if (item != d->mDropTarget)
            d->mDropTarget = item;

        qreal scale = item->transform().m11();

        QPointF itemCenter(item->pos().x() + (item->boundingRect().width()-verticalScrollBar()->width()) * scale,
                           item->pos().y() + item->boundingRect().height() * scale / 2);

        bool dropAbove = mapToScene(position.toPoint()).y() < itemCenter.y();
        bool movingUp = d->mDropSource->sceneIndex() > item->sceneIndex();
        qreal y = 0;

        if (movingUp)
        {
            if (dropAbove)
            {
                y = item->pos().y() - Settings::thumbnailSpacing / 2;
                if (d->mDropBar->y() != y)
                    d->mDropBar->setRect(QRectF(item->pos().x(), y, d->mThumbnailWidth-verticalScrollBar()->width(), 3));
            }
        }
        else
        {
            if (!dropAbove)
            {
                y = item->pos().y() + item->boundingRect().height() * scale + Settings::thumbnailSpacing / 2;
                if (d->mDropBar->y() != y)
                    d->mDropBar->setRect(QRectF(item->pos().x(), y, d->mThumbnailWidth-verticalScrollBar()->width(), 3));
            }
        }
    }

    event->acceptProposedAction();
}

void BoardThumbnailsView::dropEvent(QDropEvent *event)
{
    Q_UNUSED(event);

    if (d->mDropSource->sceneIndex() != d->mDropTarget->sceneIndex())
        Application::boardController->moveSceneToIndex(d->mDropSource->sceneIndex(), d->mDropTarget->sceneIndex());

    d->mDropSource = NULL;
    d->mDropTarget = NULL;

    d->mDropBar->setRect(QRectF());
    d->mDropBar->hide();
}
