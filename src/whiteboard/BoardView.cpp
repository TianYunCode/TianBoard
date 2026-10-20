#include "whiteboard/BoardView.h"
#include "core/VirtualDesktop.h"

#include <QtWidgets>
#include <QtXml>
#include <QListView>

#include "whiteboard/DrawingController.h"

#include "utils/GeometryUtils.h"
#include "platform/PlatformUtils.h"

#include "settings/Settings.h"
#include "core/MimeData.h"
#include "core/Application.h"
#include "settings/Setting.h"
#include "core/PersistenceManager.h"
#include "utils/Enums.h"

#include "core/HttpGet.h"

#include "ui/StylusPalette.h"
#include "ui/RubberBand.h"
#include "ui/ToolWidget.h"
#include "ui/Resources.h"
#include "ui/MainWindow.h"
#include "ui/ThumbnailWidget.h"

#include "whiteboard/BoardController.h"
#include "whiteboard/BoardPaletteManager.h"

#include "whiteboard/GraphicsTextItem.h"
#include "whiteboard/GraphicsPixmapItem.h"
#include "whiteboard/GraphicsWidgetItem.h"
#include "whiteboard/GraphicsPDFItem.h"
#include "whiteboard/GraphicsPolygonItem.h"
#include "whiteboard/Item.h"
#include "whiteboard/GraphicsMediaItem.h"
#include "whiteboard/GraphicsSvgItem.h"
#include "whiteboard/GraphicsGroupContainerItem.h"
#include "whiteboard/GraphicsStrokesGroup.h"
#include "whiteboard/GraphicsItemDelegate.h"

#include "model/DocumentProxy.h"

#include "whiteboard/GraphicsRuler.h"
#include "whiteboard/GraphicsCurtainItem.h"
#include "whiteboard/GraphicsCompass.h"
#include "whiteboard/GraphicsCache.h"
#include "whiteboard/GraphicsTriangle.h"
#include "whiteboard/GraphicsProtractor.h"


struct BoardView::Private {
    BoardController *mController{nullptr};

    int mStartLayer{0};
    int mEndLayer{0};
    bool mFilterZIndex{false};

    bool mTabletStylusIsPressed{false};
    bool mUsingTabletEraser{false};

    bool mPendingStylusReleaseEvent{false};

    bool mMouseButtonIsPressed{false};
    QPointF mPreviousPoint;
    QPoint mMouseDownPos;

    bool mPenPressureSensitive{false};
    bool mMarkerPressureSensitive{false};
    bool mUseHighResTabletEvent{false};

    bool mIsCreatingTextZone{false};
    bool mIsCreatingSceneGrabZone{false};

    bool mVirtualKeyboardActive{false};
    bool mOkOnWidget{false};

    bool mWidgetMoved{false};
    QPointF mLastPressedMousePos;
    QGraphicsItem *movingItem{nullptr};
    QMouseEvent *suspendedMousePressEvent{nullptr};

    bool moveRubberBand{false};
    RubberBand *mRubberBand{nullptr};

    QList<QGraphicsItem *> mRubberedItems;
    QSet<QGraphicsItem *> mJustSelectedItems;

    int mLongPressInterval{0};
    QTimer mLongPressTimer;

    bool mIsDragInProgress{false};
    bool mMultipleSelectionIsEnabled{false};
    bool bIsControl{false};
    bool bIsDesktop{false};
    bool mRubberBandInPlayMode{false};
};

BoardView::BoardView(BoardController *pController, QWidget *pParent, bool isControl, bool isDesktop)
    : QGraphicsView(pParent), d(new Private) {
    d->mController = pController;
    d->mIsCreatingTextZone = false;
    d->mIsCreatingSceneGrabZone = false;
    d->mOkOnWidget = false;
    d->suspendedMousePressEvent = NULL;
    d->mLongPressInterval = 1000;
    d->mIsDragInProgress = false;
    d->mMultipleSelectionIsEnabled = false;
    d->bIsControl = isControl;
    d->bIsDesktop = isDesktop;

    init();

    d->mFilterZIndex = false;
    /*
    mFilterZIndex = true;
    mStartLayer = ItemLayerType::FixedBackground;
    mEndLayer = ItemLayerType::Control;
    */


    d->mLongPressTimer.setInterval(d->mLongPressInterval);
    d->mLongPressTimer.setSingleShot(true);
}

BoardView::BoardView(BoardController *pController, int pStartLayer, int pEndLayer, QWidget *pParent,
                     bool isControl, bool isDesktop)
    : QGraphicsView(pParent), d(new Private) {
    d->mController = pController;
    d->suspendedMousePressEvent = NULL;
    d->mLongPressInterval = 1000;
    d->mIsDragInProgress = false;
    d->mMultipleSelectionIsEnabled = false;
    d->bIsControl = isControl;
    d->bIsDesktop = isDesktop;

    init();

    d->mStartLayer = pStartLayer;
    d->mEndLayer = pEndLayer;

    d->mFilterZIndex = true;

    d->mLongPressTimer.setInterval(d->mLongPressInterval);
    d->mLongPressTimer.setSingleShot(true);
}

BoardView::~BoardView() {
    if (d->suspendedMousePressEvent) {
        delete d->suspendedMousePressEvent;
        d->suspendedMousePressEvent = NULL;
    }
}

void BoardView::init() {
    connect(Settings::settings()->boardPenPressureSensitive, SIGNAL(changed(QVariant)), this,
            SLOT(settingChanged(QVariant)));

    connect(Settings::settings()->boardMarkerPressureSensitive, SIGNAL(changed(QVariant)), this,
            SLOT(settingChanged(QVariant)));

    connect(Settings::settings()->boardUseHighResTabletEvent, SIGNAL(changed(QVariant)), this,
            SLOT(settingChanged(QVariant)));

    setOptimizationFlags(QGraphicsView::IndirectPainting |
                         QGraphicsView::DontSavePainterState); // enable BoardView::drawItems filter
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setWindowFlags(Qt::FramelessWindowHint);
    setFrameStyle(QFrame::NoFrame);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAcceptDrops(true);

    d->mTabletStylusIsPressed = false;
    d->mMouseButtonIsPressed = false;
    d->mPendingStylusReleaseEvent = false;

    setCacheMode(QGraphicsView::CacheBackground);

    d->mUsingTabletEraser = false;
    d->mIsCreatingTextZone = false;
    d->mRubberBand = 0;

    d->mVirtualKeyboardActive = false;

    settingChanged(QVariant());

    unsetCursor();

    d->movingItem = NULL;
    d->mWidgetMoved = false;
}

GraphicsScene *BoardView::scene() {
    return qobject_cast<GraphicsScene *>(QGraphicsView::scene());
}

void BoardView::keyPressEvent(QKeyEvent *event) {
    QApplication::sendEvent(scene(), event);

    if (!event->isAccepted()) {
        switch (event->key()) {
            case Qt::Key_Up:
            case Qt::Key_PageUp:
            case Qt::Key_Left: {
                d->mController->previousScene();
                break;
            }

            case Qt::Key_Down:
            case Qt::Key_PageDown:
            case Qt::Key_Right:
            case Qt::Key_Space: {
                d->mController->nextScene();
                break;
            }

            case Qt::Key_Home: {
                d->mController->firstScene();
                break;
            }
            case Qt::Key_End: {
                d->mController->lastScene();
                break;
            }
            case Qt::Key_Insert: {
                d->mController->addScene();
                break;
            }
            case Qt::Key_Control:
            case Qt::Key_Shift: {
                setMultiselection(true);
            } break;
        }


        if (event->modifiers() & Qt::ControlModifier) // keep only ctrl/cmd keys
        {
            switch (event->key()) {
                case Qt::Key_Plus:
                case Qt::Key_I: {
                    d->mController->zoomIn();
                    event->accept();
                    break;
                }
                case Qt::Key_Minus:
                case Qt::Key_O: {
                    d->mController->zoomOut();
                    event->accept();
                    break;
                }
                case Qt::Key_0: {
                    d->mController->zoomRestore();
                    event->accept();
                    break;
                }
                case Qt::Key_Left: {
                    d->mController->handScroll(-100, 0);
                    event->accept();
                    break;
                }
                case Qt::Key_Right: {
                    d->mController->handScroll(100, 0);
                    event->accept();
                    break;
                }
                case Qt::Key_Up: {
                    d->mController->handScroll(0, -100);
                    event->accept();
                    break;
                }
                case Qt::Key_Down: {
                    d->mController->handScroll(0, 100);
                    event->accept();
                    break;
                }
                default: {
                    // NOOP
                }
            }
        }
    }

    if (event->isAccepted())
        setMultiselection(false);
}


void BoardView::keyReleaseEvent(QKeyEvent *event) {
    if (Qt::Key_Shift == event->key() || Qt::Key_Control == event->key())
        setMultiselection(false);

    QGraphicsView::keyReleaseEvent(event);
}

bool BoardView::event(QEvent *e) {
    if (e->type() == QEvent::Gesture) {
        QGestureEvent *gestureEvent = dynamic_cast<QGestureEvent *>(e);
        if (gestureEvent) {
            QSwipeGesture *swipe = dynamic_cast<QSwipeGesture *>(gestureEvent->gesture(Qt::SwipeGesture));
            if (swipe) {
                if (swipe->horizontalDirection() == QSwipeGesture::Left) {
                    d->mController->previousScene();
                    gestureEvent->setAccepted(swipe, true);
                }

                if (swipe->horizontalDirection() == QSwipeGesture::Right) {
                    d->mController->nextScene();
                    gestureEvent->setAccepted(swipe, true);
                }
            }
        }
    }

    return QGraphicsView::event(e);
}

void BoardView::tabletEvent(QTabletEvent *event) {
    if (!d->mUseHighResTabletEvent) {
        event->setAccepted(false);
        return;
    }

    DrawingController *dc = DrawingController::drawingController();

    QPointF tabletPos = event->pos();
    StylusTool::Enum currentTool = (StylusTool::Enum)dc->stylusTool();

    if (event->type() == QEvent::TabletPress || event->type() == QEvent::TabletEnterProximity) {
        if (event->pointerType() == QPointingDevice::PointerType::Eraser) {
            dc->setStylusTool(StylusTool::Eraser);
            d->mUsingTabletEraser = true;
        } else {
            if (d->mUsingTabletEraser && currentTool == StylusTool::Eraser)
                dc->setStylusTool(dc->latestDrawingTool());

            d->mUsingTabletEraser = false;
        }
    }

    QPointF scenePos = viewportTransform().inverted().map(tabletPos);

    qreal pressure = 1.0;
    if (((currentTool == StylusTool::Pen || currentTool == StylusTool::Line) && d->mPenPressureSensitive) ||
        (currentTool == StylusTool::Marker && d->mMarkerPressureSensitive))
        pressure = event->pressure();
    else {
        event->setAccepted(false);
        return;
    }

    bool acceptEvent = true;

    switch (event->type()) {
        case QEvent::TabletPress: {
            d->mTabletStylusIsPressed = true;
            scene()->inputDevicePress(scenePos, pressure);

            break;
        }
        case QEvent::TabletMove: {
            if (d->mTabletStylusIsPressed)
                scene()->inputDeviceMove(scenePos, pressure);

            acceptEvent = false;

            break;
        }
        case QEvent::TabletRelease: {
            StylusTool::Enum currentTool = (StylusTool::Enum)dc->stylusTool();
            scene()->setToolCursor(currentTool);
            setToolCursor(currentTool);

            scene()->inputDeviceRelease();

            d->mPendingStylusReleaseEvent = false;

            d->mTabletStylusIsPressed = false;
            d->mMouseButtonIsPressed = false;

            break;
        }
        default: {
            //NOOP
        }
    }


    event->setAccepted(acceptEvent);
}

bool BoardView::itemIsLocked(QGraphicsItem *item) {
    if (!item)
        return false;

    return item->data(GraphicsItemData::ItemLocked).toBool();
}

bool BoardView::itemHaveParentWithType(QGraphicsItem *item, int type) {
    if (!item)
        return false;

    if (type == item->type())
        return true;

    return itemHaveParentWithType(item->parentItem(), type);
}

bool BoardView::isBoardItem(QGraphicsItem *item) {
    if ((GraphicsItemType::UserTypesCount > item->type()) && (item->type() > QGraphicsItem::UserType))
        return true;

    return false;
}

bool BoardView::isCppTool(QGraphicsItem *item) {
    return (item->type() == GraphicsItemType::CompassItemType ||
            item->type() == GraphicsItemType::RulerItemType ||
            item->type() == GraphicsItemType::ProtractorItemType ||
            item->type() == GraphicsItemType::TriangleItemType ||
            item->type() == GraphicsItemType::CurtainItemType);
}

void BoardView::handleItemsSelection(QGraphicsItem *item) {
    if (item) {
        if (item->parentItem() && GraphicsGroupContainerItem::Type == d->movingItem->parentItem()->type())
            return;

        if (DelegateButton::Type == item->type())
            return;

        if (QGraphicsSvgItem::Type == item->type())
            return;

        if (GraphicsDelegateFrame::Type == item->type())
            return;

        if (!isMultipleSelectionEnabled()) {
            if ((GraphicsItemType::UserTypesCount > item->type()) &&
                (item->type() > QGraphicsItem::UserType)) {
                scene()->deselectAllItemsExcept(item);
            }
        }
    }
}

bool BoardView::itemShouldReceiveMousePressEvent(QGraphicsItem *item) {
    if (!item)
        return true;

    if (item == scene()->backgroundObject())
        return false;

    StylusTool::Enum currentTool = (StylusTool::Enum)DrawingController::drawingController()->stylusTool();

    switch (item->type()) {
        case GraphicsProtractor::Type:
        case GraphicsRuler::Type:
        case GraphicsTriangle::Type:
        case GraphicsCompass::Type:
        case GraphicsCache::Type:
            return true;
        case GraphicsDelegateFrame::Type:
            if (currentTool == StylusTool::Play)
                return false;
            return true;
        case GraphicsPixmapItem::Type:
        case GraphicsSvgItem::Type:
            if (currentTool == StylusTool::Play)
                return true;
            if (item->isSelected())
                return true;
            else
                return false;
        case DelegateButton::Type:
            return true;

        case GraphicsMediaItem::Type:
        case GraphicsVideoItem::Type:
        case GraphicsAudioItem::Type:
            return false;

        case GraphicsTextItem::Type:
            if (currentTool == StylusTool::Play)
                return true;
            if ((currentTool == StylusTool::Selector) && item->isSelected())
                return true;
            if ((currentTool == StylusTool::Selector) && item->parentItem() &&
                item->parentItem()->isSelected())
                return true;
            if (currentTool != StylusTool::Selector)
                return false;
            break;

        case GraphicsItemType::StrokeItemType:
            if (currentTool == StylusTool::Play || currentTool == StylusTool::Selector)
                return true;
            break;

        case GraphicsGroupContainerItem::Type:
            if (currentTool == StylusTool::Play) {
                d->movingItem = NULL;
                return true;
            }
            return false;
            break;
            //case QWebEngineView::Type:
            //       return true;
        case QGraphicsProxyWidget::Type:
            return false;

        case GraphicsWidgetItem::Type:
            if (currentTool == StylusTool::Selector && item->parentItem() && item->parentItem()->isSelected())
                return true;
            if (currentTool == StylusTool::Selector && item->isSelected())
                return true;
            if (currentTool == StylusTool::Play)
                return true;
            return false;
            break;
    }

    return !isBoardItem(item);
}

bool BoardView::itemShouldReceiveSuspendedMousePressEvent(QGraphicsItem *item) {
    if (!item)
        return false;

    if (item == scene()->backgroundObject())
        return false;

    StylusTool::Enum currentTool = (StylusTool::Enum)DrawingController::drawingController()->stylusTool();

    switch (item->type()) {
        //case QWebEngineView::Type:
        //    return false;
        case GraphicsPixmapItem::Type:
        case GraphicsSvgItem::Type:
        case GraphicsTextItem::Type:
        case GraphicsWidgetItem::Type:
            if (currentTool == StylusTool::Selector && !item->isSelected() && item->parentItem())
                return true;
            if (currentTool == StylusTool::Selector && item->isSelected())
                return true;
            break;

        case DelegateButton::Type:
        case GraphicsMediaItem::Type:
        case GraphicsVideoItem::Type:
        case GraphicsAudioItem::Type:
            return true;
    }

    return false;
}

bool BoardView::itemShouldBeMoved(QGraphicsItem *item) {
    if (!item)
        return false;

    if (item == scene()->backgroundObject())
        return false;

    if (!(d->mMouseButtonIsPressed || d->mTabletStylusIsPressed))
        return false;

    if (d->movingItem->data(GraphicsItemData::ItemLocked).toBool())
        return false;

    if (d->movingItem->parentItem() &&
        GraphicsGroupContainerItem::Type == d->movingItem->parentItem()->type() &&
        !d->movingItem->isSelected() && d->movingItem->parentItem()->isSelected())
        return false;

    StylusTool::Enum currentTool = (StylusTool::Enum)DrawingController::drawingController()->stylusTool();

    switch (item->type()) {
        case GraphicsCurtainItem::Type:
        case GraphicsGroupContainerItem::Type:
            return true;

        case GraphicsWidgetItem::Type:
            if (currentTool == StylusTool::Selector && item->isSelected())
                return false;
            if (currentTool == StylusTool::Play)
                return false;

        case GraphicsSvgItem::Type:
        case GraphicsPixmapItem::Type:
            if (currentTool == StylusTool::Play || !item->isSelected())
                return true;
            if (item->isSelected())
                return false;
        case GraphicsMediaItem::Type:
        case GraphicsVideoItem::Type:
        case GraphicsAudioItem::Type:
            return true;
        case GraphicsStrokesGroup::Type:
            return false;
        case GraphicsTextItem::Type:
            return !item->isSelected();
    }

    return false;
}


QGraphicsItem *BoardView::determineItemToPress(QGraphicsItem *item) {
    if (item) {
        StylusTool::Enum currentTool = (StylusTool::Enum)DrawingController::drawingController()->stylusTool();

        if (StylusTool::Selector == currentTool && item->parentItem() &&
            GraphicsGroupContainerItem::Type == item->parentItem()->type() &&
            !item->parentItem()->isSelected())
            return item->parentItem();

        if (item->parentItem() && GraphicsStrokesGroup::Type == item->parentItem()->type())
            return determineItemToPress(item->parentItem());
    }

    return item;
}

QGraphicsItem *BoardView::determineItemToMove(QGraphicsItem *item) {
    if (item) {
        StylusTool::Enum currentTool = (StylusTool::Enum)DrawingController::drawingController()->stylusTool();

        if ((StylusTool::Play == currentTool) && (GraphicsWidgetItem::Type == item->type()))
            return item;

        if (item->parentItem() && GraphicsGroupContainerItem::Type == item->parentItem()->type()) {
            if (StylusTool::Play == currentTool && item->parentItem()->isSelected())
                return item->parentItem();

            if (GraphicsStrokesGroup::Type == item->type())
                return item->parentItem();

            if (item->parentItem()->isSelected())
                return item;

            if (item->isSelected())
                return NULL;

            return item->parentItem();
        }

        if (item->parentItem() && GraphicsStrokesGroup::Type == item->parentItem()->type())
            return determineItemToMove(item->parentItem());
    }

    return item;
}

void BoardView::handleItemMousePress(QMouseEvent *event) {
    d->mLastPressedMousePos = mapToScene(event->pos());

    d->movingItem = determineItemToPress(d->movingItem);
    handleItemsSelection(d->movingItem);

    if (isMultipleSelectionEnabled())
        return;

    if (itemShouldReceiveMousePressEvent(d->movingItem)) {
        QGraphicsView::mousePressEvent(event);

        QGraphicsItem *item =
            determineItemToPress(scene()->itemAt(this->mapToScene(event->localPos().toPoint()), transform()));

        if (item && (item->type() == QGraphicsProxyWidget::Type) && item->parentObject() &&
            item->parentObject()->type() != QGraphicsProxyWidget::Type) {
            QList<QGraphicsItem *> children = item->childItems();

            for (QList<QGraphicsItem *>::iterator it = children.begin(); it != children.end(); ++it)
                if ((*it)->pos().x() < 0 || (*it)->pos().y() < 0)
                    (*it)->setPos(0, item->boundingRect().size().height());
        }
    } else {
        if (d->movingItem) {
            GraphicsItem *graphicsItem = dynamic_cast<GraphicsItem *>(d->movingItem);
            if (graphicsItem)
                graphicsItem->Delegate()->startUndoStep();

            d->movingItem->clearFocus();
        }

        if (d->suspendedMousePressEvent) {
            delete d->suspendedMousePressEvent;
            d->suspendedMousePressEvent = NULL;
        }

        if (itemShouldReceiveSuspendedMousePressEvent(d->movingItem)) {
            d->suspendedMousePressEvent = new QMouseEvent(event->type(), event->pos(), event->button(),
                                                          event->buttons(), event->modifiers());
        }
    }
}

void BoardView::handleItemMouseMove(QMouseEvent *event) {
    d->movingItem = determineItemToMove(d->movingItem);

    if (d->movingItem && itemShouldBeMoved(d->movingItem) &&
        (d->mMouseButtonIsPressed || d->mTabletStylusIsPressed)) {
        QPointF scenePos = mapToScene(event->pos());
        QPointF newPos = d->movingItem->pos() + scenePos - d->mLastPressedMousePos;
        d->movingItem->setPos(newPos);
        d->mLastPressedMousePos = scenePos;
        d->mWidgetMoved = true;
        event->accept();
    } else {
        QPointF posBeforeMove;
        QPointF posAfterMove;

        if (d->movingItem)
            posBeforeMove = d->movingItem->pos();

        QGraphicsView::mouseMoveEvent(event);

        if (d->movingItem)
            posAfterMove = d->movingItem->pos();

        d->mWidgetMoved = ((posAfterMove - posBeforeMove).manhattanLength() != 0);

        if (d->movingItem && d->mWidgetMoved && GraphicsWebWidgetItem::Type == d->movingItem->type())
            d->movingItem->setPos(posBeforeMove);
    }
}

void BoardView::rubberItems() {
    if (d->mRubberBand)
        d->mRubberedItems = items(d->mRubberBand->geometry());

    foreach (QGraphicsItem *item, d->mRubberedItems) {
        if (item->parentItem() && GraphicsGroupContainerItem::Type == item->parentItem()->type())
            d->mRubberedItems.removeOne(item);
    }
}

void BoardView::moveRubberedItems(QPointF movingVector) {
    QRectF invalidateRect = scene()->itemsBoundingRect();

    foreach (QGraphicsItem *item, d->mRubberedItems) {
        if (item->type() == GraphicsWebWidgetItem::Type || item->type() == GraphicsPixmapItem::Type ||
            item->type() == GraphicsMediaItem::Type || item->type() == GraphicsVideoItem::Type ||
            item->type() == GraphicsAudioItem::Type || item->type() == GraphicsSvgItem::Type ||
            item->type() == GraphicsTextItem::Type || item->type() == GraphicsStrokesGroup::Type ||
            item->type() == GraphicsGroupContainerItem::Type) {
            item->setPos(item->pos() + movingVector);
        }
    }

    scene()->invalidate(invalidateRect);
}

void BoardView::setMultiselection(bool enable) {
    d->mMultipleSelectionIsEnabled = enable;
}

bool BoardView::isMultipleSelectionEnabled() {
    return d->mMultipleSelectionIsEnabled;
}

void BoardView::longPressEvent() {
    DrawingController *drawingController = DrawingController::drawingController();
    StylusTool::Enum currentTool = (StylusTool::Enum)DrawingController::drawingController()->stylusTool();

    disconnect(&d->mLongPressTimer, &QTimer::timeout, this, &BoardView::longPressEvent);

    if (StylusTool::Selector == currentTool) {
        drawingController->setStylusTool(StylusTool::Play);
    } else if (currentTool == StylusTool::Play) {
        drawingController->setStylusTool(StylusTool::Selector);
    } else if (StylusTool::Eraser == currentTool) {
        Application::boardController->paletteManager()->toggleErasePalette(true);
    }
}

void BoardView::mousePressEvent(QMouseEvent *event) {
    if (!d->bIsControl && !d->bIsDesktop) {
        event->ignore();
        return;
    }

    d->mIsDragInProgress = false;

    if (isAbsurdPoint(event->pos())) {
        event->accept();
        return;
    }

    d->mMouseDownPos = event->pos();
    d->movingItem = scene()->itemAt(this->mapToScene(event->localPos().toPoint()), QTransform());

    if (event->button() == Qt::LeftButton && isInteractive()) {
        int currentTool = (StylusTool::Enum)DrawingController::drawingController()->stylusTool();
        if (!d->mTabletStylusIsPressed)
            d->mMouseButtonIsPressed = true;

        switch (currentTool) {
            case StylusTool::ZoomIn:
                d->mController->zoomIn(mapToScene(event->pos()));
                event->accept();
                break;

            case StylusTool::ZoomOut:
                d->mController->zoomOut(mapToScene(event->pos()));
                event->accept();
                break;

            case StylusTool::Hand:
                viewport()->setCursor(QCursor(Qt::ClosedHandCursor));
                d->mPreviousPoint = event->localPos();
                event->accept();
                break;

            case StylusTool::Selector:
            case StylusTool::Play:
                if (d->bIsDesktop) {
                    event->ignore();
                    return;
                }

                if (scene()->backgroundObject() == d->movingItem)
                    d->movingItem = NULL;

                connect(&d->mLongPressTimer, &QTimer::timeout, this, &BoardView::longPressEvent);
                if (!d->movingItem && !d->mController->cacheIsVisible())
                    d->mLongPressTimer.start();

                handleItemMousePress(event);
                event->accept();
                break;

            case StylusTool::Text: {
                int frameWidth = Settings::settings()->objectFrameWidth;
                QRectF fuzzyRect(0, 0, frameWidth * 4, frameWidth * 4);
                fuzzyRect.moveCenter(mapToScene(d->mMouseDownPos));

                GraphicsTextItem *foundTextItem = 0;
                QListIterator<QGraphicsItem *> it(scene()->items(fuzzyRect));

                while (it.hasNext() && !foundTextItem)
                    foundTextItem = qgraphicsitem_cast<GraphicsTextItem *>(it.next());

                if (foundTextItem) {
                    d->mIsCreatingTextZone = false;
                    QGraphicsView::mousePressEvent(event);
                } else {
                    scene()->deselectAllItems();

                    if (!d->mRubberBand)
                        d->mRubberBand = new RubberBand(QRubberBand::Rectangle, this);
                    d->mRubberBand->setGeometry(QRect(d->mMouseDownPos, QSize()));
                    d->mRubberBand->show();
                    d->mIsCreatingTextZone = true;

                    event->accept();
                }
            } break;

            case StylusTool::Capture:
                scene()->deselectAllItems();

                if (!d->mRubberBand)
                    d->mRubberBand = new RubberBand(QRubberBand::Rectangle, this);

                d->mRubberBand->setGeometry(QRect(d->mMouseDownPos, QSize()));
                d->mRubberBand->show();
                d->mIsCreatingSceneGrabZone = true;

                event->accept();
                break;

            default:
                if (DrawingController::drawingController()->mActiveRuler == NULL) {
                    viewport()->setCursor(QCursor(Qt::BlankCursor));
                }
                if (scene() && !d->mTabletStylusIsPressed) {
                    if (currentTool == StylusTool::Eraser) {
                        connect(&d->mLongPressTimer, &QTimer::timeout, this, &BoardView::longPressEvent);
                        d->mLongPressTimer.start();
                    }
                    scene()->inputDevicePress(
                        mapToScene(GeometryUtils::pointConstrainedInRect(event->pos(), rect())));
                }
                event->accept();
        }
    }
}


void BoardView::mouseMoveEvent(QMouseEvent *event) {
    //    static QTime lastCallTime;
    //    if (!lastCallTime.isNull()) {
    //        qDebug() << "time interval is " << lastCallTime.msecsTo(QTime::currentTime());
    //    }

    //  QTime mouseMoveTime = QTime::currentTime();
    if (!d->mIsDragInProgress && ((mapToScene(event->pos()) - d->mLastPressedMousePos).manhattanLength() <
                                  QApplication::startDragDistance())) {
        return;
    }

    d->mIsDragInProgress = true;
    d->mWidgetMoved = true;
    d->mLongPressTimer.stop();

    if (isAbsurdPoint(event->pos())) {
        event->accept();
        return;
    }

    if ((DrawingController::drawingController()->isDrawingTool()) && !d->mMouseButtonIsPressed)
        QGraphicsView::mouseMoveEvent(event);

    int currentTool = static_cast<int>(DrawingController::drawingController()->stylusTool());
    switch (currentTool) {
        case StylusTool::Hand: {
            if (!d->mMouseButtonIsPressed && !d->mTabletStylusIsPressed) {
                break;
            }
            QPointF eventPosition = event->localPos();
            qreal dx = eventPosition.x() - d->mPreviousPoint.x();
            qreal dy = eventPosition.y() - d->mPreviousPoint.y();
            d->mController->handScroll(dx, dy);
            d->mPreviousPoint = eventPosition;
            event->accept();
        } break;

        case StylusTool::Selector:
        case StylusTool::Play: {
            if (d->bIsDesktop) {
                event->ignore();
                return;
            }

            bool rubberMove = (currentTool != (StylusTool::Play)) &&
                              (d->mMouseButtonIsPressed || d->mTabletStylusIsPressed) && !d->movingItem;

            if (rubberMove) {
                QRect bandRect(d->mMouseDownPos, event->pos());

                bandRect = bandRect.normalized();

                if (!d->mRubberBand) {
                    d->mRubberBand = new RubberBand(QRubberBand::Rectangle, this);
                }
                d->mRubberBand->setGeometry(bandRect);
                d->mRubberBand->show();

                //          QTime startTime = QTime::currentTime();
                //          QTime testTime = QTime::currentTime();
                QList<QGraphicsItem *> rubberItems = items(bandRect);
                //          qDebug() << "==================";
                //          qDebug() << "| ====rubber items" << testTime.msecsTo(QTime::currentTime());
                //          testTime = QTime::currentTime();
                foreach (QGraphicsItem *item, d->mJustSelectedItems) {
                    if (!rubberItems.contains(item)) {
                        item->setSelected(false);
                        d->mJustSelectedItems.remove(item);
                    }
                }
                //          qDebug() << "| ===foreach length" << testTime.msecsTo(QTime::currentTime());
                //          testTime = QTime::currentTime();

                int counter = 0;
                if (currentTool == StylusTool::Selector) {
                    foreach (QGraphicsItem *item, items(bandRect)) {
                        if (item->type() == GraphicsItemType::PolygonItemType && item->parentItem())
                            item = item->parentItem();

                        if (item->type() == GraphicsWebWidgetItem::Type ||
                            item->type() == GraphicsPixmapItem::Type ||
                            item->type() == GraphicsVideoItem::Type ||
                            item->type() == GraphicsAudioItem::Type ||
                            item->type() == GraphicsSvgItem::Type || item->type() == GraphicsTextItem::Type ||
                            item->type() == GraphicsStrokesGroup::Type ||
                            item->type() == GraphicsGroupContainerItem::Type) {
                            if (!d->mJustSelectedItems.contains(item)) {
                                counter++;
                                item->setSelected(true);
                                d->mJustSelectedItems.insert(item);
                            }
                        }
                    }
                }

                //          qDebug() << "| ==selected items count" << counter << Qt::endl
                //                   << "| ==selection time" << testTime.msecsTo(QTime::currentTime()) << Qt::endl
                //                   << "| =elapsed time " << startTime.msecsTo(QTime::currentTime()) << Qt::endl
                //                   << "==================";
                //          QCoreApplication::removePostedEvents(scene(), 0);
            }
            handleItemMouseMove(event);
        } break;

        case StylusTool::Text:
        case StylusTool::Capture: {
            if (d->mRubberBand && (d->mIsCreatingTextZone || d->mIsCreatingSceneGrabZone)) {
                d->mRubberBand->setGeometry(QRect(d->mMouseDownPos, event->pos()).normalized());
                event->accept();
            } else
                QGraphicsView::mouseMoveEvent(event);

        } break;

        default:
            if (!d->mTabletStylusIsPressed && scene()) {
                scene()->inputDeviceMove(
                    mapToScene(GeometryUtils::pointConstrainedInRect(event->pos(), rect())),
                    d->mMouseButtonIsPressed);
            }
            event->accept();
    }

    //  qDebug() << "mouse move time" << mouseMoveTime.msecsTo(QTime::currentTime());
    //  lastCallTime = QTime::currentTime();
}

void BoardView::mouseReleaseEvent(QMouseEvent *event) {
    StylusTool::Enum currentTool = (StylusTool::Enum)DrawingController::drawingController()->stylusTool();

    setToolCursor(currentTool);
    // first/ propagate device release to the scene
    if (scene())
        scene()->inputDeviceRelease();

    if (currentTool == StylusTool::Selector) {
        if (d->bIsDesktop) {
            event->ignore();
            return;
        }

        GraphicsItem *graphicsItem = dynamic_cast<GraphicsItem *>(d->movingItem);
        if (graphicsItem)
            graphicsItem->Delegate()->commitUndoStep();

        bool bReleaseIsNeed = true;
        if (d->movingItem != determineItemToPress(scene()->itemAt(
                                 this->mapToScene(event->localPos().toPoint()), QTransform()))) {
            d->movingItem = NULL;
            bReleaseIsNeed = false;
        }
        if (d->mWidgetMoved) {
            d->mWidgetMoved = false;
            d->movingItem = NULL;
        } else if (d->movingItem &&
                   (!isCppTool(d->movingItem) || GraphicsCurtainItem::Type == d->movingItem->type())) {
            if (d->suspendedMousePressEvent) {
                QGraphicsView::mousePressEvent(
                    d->suspendedMousePressEvent); // suspendedMousePressEvent is deleted by old Qt event loop
                d->movingItem = NULL;
                delete d->suspendedMousePressEvent;
                d->suspendedMousePressEvent = NULL;
                bReleaseIsNeed = true;
            } else {
                if (isBoardItem(d->movingItem) && DelegateButton::Type != d->movingItem->type() &&
                    GraphicsDelegateFrame::Type != d->movingItem->type() &&
                    GraphicsCache::Type != d->movingItem->type() &&
                    //QGraphicsView::Type != movingItem->type() && // for W3C widgets as Tools.
                    !(!isMultipleSelectionEnabled() && d->movingItem->parentItem() &&
                      GraphicsWidgetItem::Type == d->movingItem->type() &&
                      GraphicsGroupContainerItem::Type == d->movingItem->parentItem()->type())) {
                    bReleaseIsNeed = false;
                    if (d->movingItem->isSelected() && isMultipleSelectionEnabled())
                        d->movingItem->setSelected(false);
                    else if (d->movingItem->parentItem() && d->movingItem->parentItem()->isSelected() &&
                             isMultipleSelectionEnabled())
                        d->movingItem->parentItem()->setSelected(false);
                    else {
                        if (d->movingItem->isSelected())
                            bReleaseIsNeed = true;

                        GraphicsTextItem *textItem = dynamic_cast<GraphicsTextItem *>(d->movingItem);
                        GraphicsMediaItem *movieItem = dynamic_cast<GraphicsMediaItem *>(d->movingItem);
                        if (textItem)
                            textItem->setSelected(true);
                        else if (movieItem)
                            movieItem->setSelected(true);
                        else
                            d->movingItem->setSelected(true);
                    }
                }
            }
        } else
            bReleaseIsNeed = true;

        if (bReleaseIsNeed) {
            QGraphicsView::mouseReleaseEvent(event);
        }
    } else if (currentTool == StylusTool::Text) {
        bool bReleaseIsNeed = true;
        if (d->movingItem != determineItemToPress(scene()->itemAt(
                                 this->mapToScene(event->localPos().toPoint()), QTransform()))) {
            d->movingItem = NULL;
            bReleaseIsNeed = false;
        }

        GraphicsItem *graphicsItem = dynamic_cast<GraphicsItem *>(d->movingItem);
        if (graphicsItem)
            graphicsItem->Delegate()->commitUndoStep();

        if (d->mWidgetMoved) {
            d->mWidgetMoved = false;
            d->movingItem = NULL;
            if (scene() && d->mRubberBand && d->mIsCreatingTextZone) {
                QRect rubberRect = d->mRubberBand->geometry();

                GraphicsTextItem *textItem = scene()->addTextHtml("", mapToScene(rubberRect.topLeft()));
                event->accept();

                DrawingController::drawingController()->setStylusTool(StylusTool::Selector);

                textItem->setTextInteractionFlags(Qt::TextEditorInteraction);
                textItem->setSelected(true);
                textItem->setTextWidth(0);
                textItem->setFocus();
            }
        } else if (d->movingItem &&
                   (!isCppTool(d->movingItem) || GraphicsCurtainItem::Type == d->movingItem->type())) {
            if (d->suspendedMousePressEvent) {
                QGraphicsView::mousePressEvent(
                    d->suspendedMousePressEvent); // suspendedMousePressEvent is deleted by old Qt event loop
                d->movingItem = NULL;
                delete d->suspendedMousePressEvent;
                d->suspendedMousePressEvent = NULL;
                bReleaseIsNeed = true;
            } else {
                if (isBoardItem(d->movingItem) && DelegateButton::Type != d->movingItem->type() &&
                    QGraphicsSvgItem::Type != d->movingItem->type() &&
                    GraphicsDelegateFrame::Type != d->movingItem->type() &&
                    GraphicsCache::Type != d->movingItem->type() &&
                    //QGraphicsView::Type != movingItem->type() && // for W3C widgets as Tools.
                    !(!isMultipleSelectionEnabled() && d->movingItem->parentItem() &&
                      GraphicsWidgetItem::Type == d->movingItem->type() &&
                      GraphicsGroupContainerItem::Type == d->movingItem->parentItem()->type())) {
                    bReleaseIsNeed = false;
                    if (d->movingItem->isSelected() && isMultipleSelectionEnabled())
                        d->movingItem->setSelected(false);
                    else if (d->movingItem->parentItem() && d->movingItem->parentItem()->isSelected() &&
                             isMultipleSelectionEnabled())
                        d->movingItem->parentItem()->setSelected(false);
                    else {
                        if (d->movingItem->isSelected())
                            bReleaseIsNeed = true;

                        d->movingItem->setSelected(true);
                    }
                }
            }
        } else
            bReleaseIsNeed = true;

        if (bReleaseIsNeed) {
            QGraphicsView::mouseReleaseEvent(event);
        }
    } else if (currentTool == StylusTool::Play) {
        if (d->bIsDesktop) {
            event->ignore();
            return;
        }

        if (d->mWidgetMoved) {
            d->movingItem->setSelected(false);
            d->movingItem = NULL;
            d->mWidgetMoved = false;
        } else {
            if (d->suspendedMousePressEvent) {
                QGraphicsView::mousePressEvent(
                    d->suspendedMousePressEvent); // suspendedMousePressEvent is deleted by old Qt event loop
                d->movingItem = NULL;
                delete d->suspendedMousePressEvent;
                d->suspendedMousePressEvent = NULL;
            }
        }
        QGraphicsView::mouseReleaseEvent(event);
    } else if (currentTool == StylusTool::Capture) {
        if (scene() && d->mRubberBand && d->mIsCreatingSceneGrabZone &&
            d->mRubberBand->geometry().width() > 16) {
            QRect rect = d->mRubberBand->geometry();
            QPointF sceneTopLeft = mapToScene(rect.topLeft());
            QPointF sceneBottomRight = mapToScene(rect.bottomRight());
            QRectF sceneRect(sceneTopLeft, sceneBottomRight);

            d->mController->grabScene(sceneRect);

            event->accept();
        } else {
            QGraphicsView::mouseReleaseEvent(event);
        }

        d->mIsCreatingSceneGrabZone = false;
    } else {
        if (d->mPendingStylusReleaseEvent || d->mMouseButtonIsPressed) {
            event->accept();
        }
    }


    if (d->mRubberBand) {
        d->mRubberBand->hide();
        delete d->mRubberBand;
        d->mRubberBand = NULL;
    }

    d->mMouseButtonIsPressed = false;
    d->mPendingStylusReleaseEvent = false;
    d->mTabletStylusIsPressed = false;
    d->movingItem = NULL;

    d->mLongPressTimer.stop();
    emit mouseReleased();
}

void BoardView::forcedTabletRelease() {
    if (d->mMouseButtonIsPressed || d->mTabletStylusIsPressed || d->mPendingStylusReleaseEvent) {
        qDebug() << "dirty mouse/tablet state:";
        qDebug() << "mMouseButtonIsPressed =" << d->mMouseButtonIsPressed;
        qDebug() << "mTabletStylusIsPressed = " << d->mTabletStylusIsPressed;
        qDebug() << "mPendingStylusReleaseEvent" << d->mPendingStylusReleaseEvent;
        qDebug() << "forcing device release";

        scene()->inputDeviceRelease();

        d->mMouseButtonIsPressed = false;
        d->mTabletStylusIsPressed = false;
        d->mPendingStylusReleaseEvent = false;
    }
}

void BoardView::mouseDoubleClickEvent(QMouseEvent *event) {
    // We don't want a double click, we want two clicks
    mousePressEvent(event);
}

void BoardView::wheelEvent(QWheelEvent *wheelEvent) {
    QList<QGraphicsItem *> selItemsList = scene()->selectedItems();
    // if NO have selected items, than no need process mouse wheel. just exist
    if (selItemsList.count() > 0) {
        // only one selected item possible, so we will work with first item only
        QGraphicsItem *selItem = selItemsList[0];

        // get items list under mouse cursor
        QPointF scenePos = mapToScene(wheelEvent->position().toPoint());
        QList<QGraphicsItem *> itemsList = scene()->items(scenePos);

        bool isSelectedAndMouseHower = itemsList.contains(selItem);
        if (isSelectedAndMouseHower) {
            QGraphicsView::wheelEvent(wheelEvent);
            wheelEvent->accept();
        }
    }
}

void BoardView::leaveEvent(QEvent *event) {
    if (scene())
        scene()->leaveEvent(event);

    d->mJustSelectedItems.clear();

    QGraphicsView::leaveEvent(event);
}

bool BoardView::shouldDisplayItem(QGraphicsItem *item) {
    bool ok;
    int itemLayerType = item->data(GraphicsItemData::ItemLayerType).toInt(&ok);
    return (ok && (itemLayerType >= d->mStartLayer && itemLayerType <= d->mEndLayer));
}

void BoardView::drawItems(QPainter *painter, int numItems, QGraphicsItem *items[],
                          const QStyleOptionGraphicsItem options[]) {
    if (!d->mFilterZIndex)
        QGraphicsView::drawItems(painter, numItems, items, options);
    else {
        int count = 0;

        QGraphicsItem **itemsFiltered = new QGraphicsItem *[numItems];
        QStyleOptionGraphicsItem *optionsFiltered = new QStyleOptionGraphicsItem[numItems];

        for (int i = 0; i < numItems; i++) {
            if (shouldDisplayItem(items[i])) {
                itemsFiltered[count] = items[i];
                optionsFiltered[count] = options[i];
                count++;
            }
        }

        QGraphicsView::drawItems(painter, count, itemsFiltered, optionsFiltered);

        delete[] optionsFiltered;
        delete[] itemsFiltered;
    }
}


void BoardView::dragMoveEvent(QDragMoveEvent *event) {
    QGraphicsView::dragMoveEvent(event);
    event->acceptProposedAction();
}

void BoardView::dropEvent(QDropEvent *event) {
    QGraphicsItem *onItem = itemAt(event->pos().x(), event->pos().y());
    if (onItem && onItem->type() == GraphicsWidgetItem::Type) {
        QGraphicsView::dropEvent(event);
    } else {
        if (!event->source() || qobject_cast<ThumbnailWidget *>(event->source()) ||
            qobject_cast<QWebEngineView *>(event->source()) || qobject_cast<QListView *>(event->source())) {
            d->mController->processMimeData(event->mimeData(), mapToScene(event->pos()));
            event->acceptProposedAction();
        }
    }
    // 事件处理期间库条目可能已被模型删除，这里要防住
    //Qt base classes
    if (event->dropAction() == Qt::MoveAction) {
        event->setDropAction(Qt::CopyAction);
    }
}

void BoardView::resizeEvent(QResizeEvent *event) {
    const qreal maxWidth = width() * 10;
    const qreal maxHeight = height() * 10;

    setSceneRect(-(maxWidth / 2), -(maxHeight / 2), maxWidth, maxHeight);
    centerOn(0, 0);

    emit resized(event);
}

void BoardView::drawBackground(QPainter *painter, const QRectF &rect) {
    if (testAttribute(Qt::WA_TranslucentBackground)) {
        QGraphicsView::drawBackground(painter, rect);
        return;
    }

    bool darkBackground = scene() && scene()->isDarkBackground();

    if (darkBackground) {
        painter->fillRect(rect, QBrush(QColor(Qt::black)));
    } else {
        painter->fillRect(rect, QBrush(QColor(Qt::white)));
    }

    if (transform().m11() > 0.5) {
        QColor bgCrossColor;

        if (darkBackground)
            bgCrossColor = QColor(Settings::settings()->boardCrossColorDarkBackground->get().toString());
        else
            bgCrossColor = QColor(Settings::settings()->boardCrossColorLightBackground->get().toString());

        if (transform().m11() < 0.7) {
            int alpha = 255 * transform().m11() / 2;
            bgCrossColor.setAlpha(alpha); // fade the crossing on small zooms
        }

        qreal gridSize = scene()->backgroundGridSize();

        painter->setPen(bgCrossColor);

        if (scene() && scene()->pageBackground() == PageBackground::crossed) {
            qreal firstY = ((int)(rect.y() / gridSize)) * gridSize;

            for (qreal yPos = firstY; yPos < rect.y() + rect.height(); yPos += gridSize) {
                painter->drawLine(rect.x(), yPos, rect.x() + rect.width(), yPos);
            }

            qreal firstX = ((int)(rect.x() / gridSize)) * gridSize;

            for (qreal xPos = firstX; xPos < rect.x() + rect.width(); xPos += gridSize) {
                painter->drawLine(xPos, rect.y(), xPos, rect.y() + rect.height());
            }
        }

        if (scene() && scene()->pageBackground() == PageBackground::ruled) {
            qreal firstY = ((int)(rect.y() / gridSize)) * gridSize;

            for (qreal yPos = firstY; yPos < rect.y() + rect.height(); yPos += gridSize) {
                painter->drawLine(rect.x(), yPos, rect.x() + rect.width(), yPos);
            }
        }
    }

    if (!d->mFilterZIndex && scene()) {
        QSize pageNominalSize = scene()->nominalSize();

        if (pageNominalSize.isValid()) {
            qreal penWidth = 8.0 / transform().m11();

            QRectF pageRect(pageNominalSize.width() / -2, pageNominalSize.height() / -2,
                            pageNominalSize.width(), pageNominalSize.height());

            pageRect.adjust(-penWidth / 2, -penWidth / 2, penWidth / 2, penWidth / 2);

            QColor docSizeColor;

            if (darkBackground)
                docSizeColor = Settings::documentSizeMarkColorDarkBackground;
            else
                docSizeColor = Settings::documentSizeMarkColorLightBackground;

            QPen pen(docSizeColor);
            pen.setWidth(penWidth);
            painter->setPen(pen);
            painter->drawRect(pageRect);
        }
    }
}

void BoardView::settingChanged(QVariant newValue) {
    Q_UNUSED(newValue);

    d->mPenPressureSensitive = Settings::settings()->boardPenPressureSensitive->get().toBool();
    d->mMarkerPressureSensitive = Settings::settings()->boardMarkerPressureSensitive->get().toBool();
    d->mUseHighResTabletEvent = Settings::settings()->boardUseHighResTabletEvent->get().toBool();
}

void BoardView::virtualKeyboardActivated(bool b) {
    PlatformUtils::setWindowNonActivableFlag(this, b);
    d->mVirtualKeyboardActive = b;
    setInteractive(!b);
}


// Apple remote desktop sends funny events when the transmission is bad

bool BoardView::isAbsurdPoint(QPoint point) {
    VirtualDesktop *desktop = VirtualDesktop::instance();
    bool isValidPoint = false;

    for (int i = 0; i < desktop->numScreens(); i++) {
        QRect screenRect = desktop->screenGeometry(i);
        isValidPoint = isValidPoint || screenRect.contains(mapToGlobal(point));
    }

    return !isValidPoint;
}

void BoardView::focusOutEvent(QFocusEvent *event) {
    Q_UNUSED(event);
}

void BoardView::setToolCursor(int tool) {
    QWidget *controlViewport = viewport();
    switch (tool) {
        case StylusTool::Pen:
            controlViewport->setCursor(Resources::resources()->penCursor);
            break;
        case StylusTool::Eraser:
            controlViewport->setCursor(Resources::resources()->eraserCursor);
            break;
        case StylusTool::Marker:
            controlViewport->setCursor(Resources::resources()->markerCursor);
            break;
        case StylusTool::Pointer:
            controlViewport->setCursor(Resources::resources()->pointerCursor);
            break;
        case StylusTool::Hand:
            controlViewport->setCursor(Resources::resources()->handCursor);
            break;
        case StylusTool::ZoomIn:
            controlViewport->setCursor(Resources::resources()->zoomInCursor);
            break;
        case StylusTool::ZoomOut:
            controlViewport->setCursor(Resources::resources()->zoomOutCursor);
            break;
        case StylusTool::Selector:
            controlViewport->setCursor(Resources::resources()->arrowCursor);
            break;
        case StylusTool::Play:
            controlViewport->setCursor(Resources::resources()->playCursor);
            break;
        case StylusTool::Line:
            controlViewport->setCursor(Resources::resources()->penCursor);
            break;
        case StylusTool::Text:
            controlViewport->setCursor(Resources::resources()->textCursor);
            break;
        case StylusTool::Capture:
            controlViewport->setCursor(Resources::resources()->penCursor);
            break;
        default:
            Q_ASSERT(false);
            //failsafe
            controlViewport->setCursor(Resources::resources()->penCursor);
    }
}


bool BoardView::hasSelectedParents(QGraphicsItem *item) {
    if (item->isSelected())
        return true;
    if (item->parentItem() == NULL)
        return false;
    return hasSelectedParents(item->parentItem());
}
