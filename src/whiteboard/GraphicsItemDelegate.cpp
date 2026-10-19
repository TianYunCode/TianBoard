#include <QtWidgets>
#include <QtSvg>
#include <QtSvgWidgets>
#include <QDrag>

#include "whiteboard/GraphicsItemDelegate.h"
#include "whiteboard/GraphicsMediaItemDelegate.h"
#include "whiteboard/GraphicsDelegateFrame.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsItemUndoCommand.h"
#include "whiteboard/GraphicsItemTransformUndoCommand.h"

#include "whiteboard/BoardController.h"
#include "whiteboard/BoardView.h"

#include "core/Application.h"
#include "core/ApplicationController.h"
#include "core/DisplayManager.h"
#include "core/Settings.h"
#include "core/PersistenceManager.h"

#include "model/DocumentProxy.h"

#include "whiteboard/GraphicsWidgetItem.h"

#include "whiteboard/GraphicsTextItem.h"
#include "whiteboard/GraphicsMediaItem.h"
#include "whiteboard/GraphicsGroupContainerItem.h"

#include "web/WebController.h"

#include "utils/FileSystemUtils.h"
#include "whiteboard/DrawingController.h"


DelegateButton::DelegateButton(const QString & fileName, QGraphicsItem* pDelegated, QGraphicsItem * parent, Qt::WindowFrameSection section)
    : QGraphicsSvgItem(fileName, parent)
    , mDelegated(pDelegated)
    , mIsTransparentToMouseEvent(false)
    , mIsPressed(false)
    , mProgressTimerId(-1)
    , mPressProgres(0)
    , mShowProgressIndicator(false)
    , mButtonAlignmentSection(section)
{
    setAcceptedMouseButtons(Qt::LeftButton);
    setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Control));
    setCacheMode(QGraphicsItem::NoCache); /* 必须 NoCache：抓屏时位图要随窗口刷新，教师屏才能与副屏同步 */
}

DelegateButton::~DelegateButton()
{
    // NOOP
}

void DelegateButton::setFileName(const QString & fileName)
{
    QGraphicsSvgItem::setSharedRenderer(new QSvgRenderer (fileName, this));
}

void DelegateButton::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (mShowProgressIndicator) {
        QTimer::singleShot(300, this, SLOT(startShowProgress()));
    }

    mIsPressed = true;

    // make sure delegate is selected, to avoid control being hidden
    mPressedTime = QTime::currentTime();

    event->setAccepted(!mIsTransparentToMouseEvent);
}

void DelegateButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (mShowProgressIndicator && mProgressTimerId != -1) {
        killTimer(mProgressTimerId);
        mPressProgres = 0;
    }

    mIsPressed = false;
    int timeto = qAbs(QTime::currentTime().msecsTo(mPressedTime));

    if (timeto < Settings::longClickInterval) {
        emit clicked();
    } else {
        emit longClicked();
    }

    event->setAccepted(!mIsTransparentToMouseEvent);

    update();
}

void DelegateButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->save();
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    QGraphicsSvgItem::paint(painter, option, widget);
    painter->restore();

    if (mIsPressed && mShowProgressIndicator) {
        QPen pen;
        pen.setBrush(Qt::white);
        pen.setWidth(3);
        painter->save();
        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter->setPen(pen);

        int spanAngle = qMin(mPressProgres, Settings::longClickInterval) * 360 / Settings::longClickInterval;
        painter->drawArc(option->rect.adjusted(pen.width(), pen.width(), -pen.width(), -pen.width()), 16 * 90, -16 * spanAngle);

        painter->restore();
    }
}

void DelegateButton::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == mProgressTimerId) {
        mPressProgres = qAbs(QTime::currentTime().msecsTo(mPressedTime));
        update();
    }
}

void DelegateButton::startShowProgress()
{
    if (mIsPressed) {
         mProgressTimerId = startTimer(37);
    }
}

GraphicsItemDelegate::GraphicsItemDelegate(QGraphicsItem* pDelegated, QObject * parent, GraphicsFlags fls)
    : QObject(parent)
    , mDelegated(pDelegated)
    , mDeleteButton(NULL)
    , mDuplicateButton(NULL)
    , mMenuButton(NULL)
    , mZOrderUpButton(0)
    , mZOrderDownButton(0)
    , mMenu(0)
    , mLockAction(0)
    , mShowOnDisplayAction(0)
    , mGotoContentSourceAction(0)
    , mFrame(0)
    , mFrameWidth(Settings::settings()->objectFrameWidth)
    , mAntiScaleRatio(1.0)
    , mToolBarItem(NULL)
    , mMimeData(NULL)
{
    setFrameFlags(fls);
    connect(Application::boardController, SIGNAL(zoomChanged(qreal)), this, SLOT(onZoomChanged()));
}

void GraphicsItemDelegate::createControls()
{
    if (testFrameFlags(GF_TOOLBAR_USED) && !mToolBarItem)
        mToolBarItem = new GraphicsToolBarItem(mDelegated);

    if (!mFrame) {
        mFrame = new GraphicsDelegateFrame(this, QRectF(0, 0, 0, 0), mFrameWidth, testFrameFlags(GF_RESPECT_RATIO), testFrameFlags(GF_TITLE_BAR_USED));
        mFrame->hide();
        mFrame->setFlag(QGraphicsItem::ItemIsSelectable, true);
    }

    if (!mDeleteButton) {
        mDeleteButton = new DelegateButton(":/images/close.svg", mDelegated, mFrame, Qt::TopLeftSection);
        mButtons << mDeleteButton;
        connect(mDeleteButton, SIGNAL(clicked()), this, SLOT(remove()));
        if (testFrameFlags(GF_DUPLICATION_ENABLED)){
            mDuplicateButton = new DelegateButton(":/images/duplicate.svg", mDelegated, mFrame, Qt::TopLeftSection);
            connect(mDuplicateButton, SIGNAL(clicked(bool)), this, SLOT(duplicate()));
            mButtons << mDuplicateButton;
        }
    }

    if (!mMenuButton) {
        mMenuButton = new DelegateButton(":/images/menu.svg", mDelegated, mFrame, Qt::TopLeftSection);
        connect(mMenuButton, SIGNAL(clicked()), this, SLOT(showMenu()));
        mButtons << mMenuButton;
    }

    if (!mZOrderUpButton) {
        mZOrderUpButton = new DelegateButton(":/images/z_layer_up.svg", mDelegated, mFrame, Qt::BottomLeftSection);
        mZOrderUpButton->setShowProgressIndicator(true);
        connect(mZOrderUpButton, SIGNAL(clicked()), this, SLOT(increaseZLevelUp()));
        connect(mZOrderUpButton, SIGNAL(longClicked()), this, SLOT(increaseZlevelTop()));
        mButtons << mZOrderUpButton;
    }

    if (!mZOrderDownButton) {
        mZOrderDownButton = new DelegateButton(":/images/z_layer_down.svg", mDelegated, mFrame, Qt::BottomLeftSection);
        mZOrderDownButton->setShowProgressIndicator(true);
        connect(mZOrderDownButton, SIGNAL(clicked()), this, SLOT(increaseZLevelDown()));
        connect(mZOrderDownButton, SIGNAL(longClicked()), this, SLOT(increaseZlevelBottom()));
        mButtons << mZOrderDownButton;
    }



    buildButtons();

    foreach(DelegateButton* button, mButtons)
    {
        button->hide();
        button->setFlag(QGraphicsItem::ItemIsSelectable, true);
    }
}

void GraphicsItemDelegate::freeControls()
{
    QGraphicsScene *controlsScene = delegated()->scene();
    Q_ASSERT(controlsScene);

    WB_FREE_CONTROL(mFrame, controlsScene);
    freeButtons();
}

void GraphicsItemDelegate::showControls()
{
    mAntiScaleRatio = 1 / (Application::boardController->systemScaleFactor() * Application::boardController->currentZoom());
    positionHandles();
}

bool GraphicsItemDelegate::controlsExist() const
{
    return mFrame
            && mDeleteButton
            && mMenuButton
            && mZOrderUpButton
            && mZOrderDownButton
            ;
}

GraphicsItemDelegate::~GraphicsItemDelegate()
{
    if (Application::boardController)
        disconnect(Application::boardController, SIGNAL(zoomChanged(qreal)), this, SLOT(onZoomChanged()));
    // do not release mMimeData.
    // the mMimeData is owned by QDrag since the setMimeData call as specified in the documentation
}



QVariant GraphicsItemDelegate::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
    GraphicsScene *ownerScene = owningScene();
    switch (static_cast<int>(change)) {

    case QGraphicsItem::ItemSelectedHasChanged : {
        if (ownerScene) {
            if (value.toBool()) { //selected(true)
                ownerScene->setSelectedZLevel(delegated());
            } else {
                ownerScene->setOwnZlevel(delegated());
                freeControls();
            }
        }

    } break;
    case QGraphicsItem::ItemVisibleHasChanged :
    {
        bool shownOnDisplay = mDelegated->data(GraphicsItemData::ItemLayerType).toInt() != ItemLayerType::Control;
        showHide(shownOnDisplay);
        break;
    }
    case QGraphicsItem::ItemPositionHasChanged :
    case QGraphicsItem::ItemTransformHasChanged :
    case QGraphicsItem::ItemZValueHasChanged :
        if (!controlsExist()) {
            break;
        }
        mAntiScaleRatio = 1 / (Application::boardController->systemScaleFactor() * Application::boardController->currentZoom());
        positionHandles();
        if (ownerScene) {
            ownerScene->setModified(true);
        }
        break;
    }

    return value;
}

GraphicsScene *GraphicsItemDelegate::owningScene()
{
    GraphicsScene *castScene = dynamic_cast<GraphicsScene*>(delegated()->scene());

    return castScene;
}

/** Used to render custom data after the main "Paint" operation is finished */
void GraphicsItemDelegate::postpaint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget)
    if (option->state & QStyle::State_Selected && !controlsExist()) {
        painter->save();
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0x88, 0x88, 0x88, 0x77));
        painter->drawRect(option->rect);

        painter->restore();
    }
}

bool GraphicsItemDelegate::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    mDragStartPosition = event->pos();

    startUndoStep();

    if (!delegated()->isSelected())
    {
        delegated()->setSelected(true);
        return true;
    }
    else
    {
        return false;
    }
}

void GraphicsItemDelegate::setMimeData(QMimeData *mimeData)
{
    mMimeData = mimeData;
}

bool GraphicsItemDelegate::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if(mMimeData) {
        QDrag* mDrag = new QDrag(event->widget());
        mDrag->setMimeData(mMimeData);
        if (!mDragPixmap.isNull()) {
            mDrag->setPixmap(mDragPixmap);
            mDrag->setHotSpot(mDragPixmap.rect().center());
        }
        mDrag->exec();
        mDragPixmap = QPixmap();
        return true;
    }
    return false;
}

bool GraphicsItemDelegate::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    Q_UNUSED(event);
    if( delegated()->isSelected() )
        return true;
    else
        return false;
}

bool GraphicsItemDelegate::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);

    //Deselect all the rest selected items if no ctrl key modifier
    if (delegated()->scene()
            && delegated()->scene()->selectedItems().count()
            && event->modifiers() != Qt::ControlModifier) {
        foreach (QGraphicsItem *item, delegated()->scene()->selectedItems()) {
            if (item != delegated()) {
                item->setSelected(false);
            }
        }
    }

    commitUndoStep();

    return true;
}

void GraphicsItemDelegate::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event)

}

void GraphicsItemDelegate::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Q_UNUSED(event)

}

bool GraphicsItemDelegate::keyPressEvent(QKeyEvent *event)
{
    Q_UNUSED(event);
    return true;
}

bool GraphicsItemDelegate::keyReleaseEvent(QKeyEvent *event)
{
    Q_UNUSED(event);
    return true;
}

QGraphicsItem *GraphicsItemDelegate::delegated()
{
    QGraphicsItem *curDelegate = 0;
    if (mDelegated->parentItem() && mDelegated->parentItem()->type() == GraphicsGroupContainerItem::Type) {
        curDelegate = mDelegated->parentItem(); // considering delegated item as an item's group which contains everything
    } else {
        curDelegate = mDelegated;
    }

    return curDelegate;
}

void GraphicsItemDelegate::positionHandles()
{
    if (!controlsExist()) {
        return;
    }

    if (mDelegated->isSelected()) {
        bool shownOnDisplay = mDelegated->data(GraphicsItemData::ItemLayerType).toInt() != ItemLayerType::Control;
        showHide(shownOnDisplay);
        mDelegated->setData(GraphicsItemData::ItemLocked, QVariant(isLocked()));
        updateFrame();

        if (StylusTool::Play != DrawingController::drawingController()->stylusTool())
            mFrame->show();

        updateButtons(true);

        if (mToolBarItem && mToolBarItem->isVisibleOnBoard())
        {
            mToolBarItem->positionHandles();
            mToolBarItem->update();
            mToolBarItem->show();
        }
    } else {
        foreach(DelegateButton* button, mButtons)
            button->hide();

        if(mFrame)
            mFrame->hide();

        if (mToolBarItem)
            mToolBarItem->hide();
    }
}

void GraphicsItemDelegate::setZOrderButtonsVisible(bool visible)
{
    if (visible) {
        updateFrame();
        updateButtons();

        QPointF newUpPoint = mFrame->mapToItem(mDelegated, mZOrderUpButton->pos());
        QPointF newDownPoint = mFrame->mapToItem(mDelegated, mZOrderDownButton->pos());


        mZOrderUpButton->setParentItem(mDelegated);
        mZOrderDownButton->setParentItem(mDelegated);

        mZOrderUpButton->setPos(newUpPoint + QPointF(0,0));
        mZOrderDownButton->setPos(newDownPoint + QPointF(0,0));

        mZOrderUpButton->show();
        mZOrderDownButton->show();

    } else {
        mZOrderUpButton->hide();
        mZOrderDownButton->hide();
    }
}

void GraphicsItemDelegate::remove(bool canUndo)
{
    GraphicsScene* scene = dynamic_cast<GraphicsScene*>(mDelegated->scene());
    if (scene)
    {
        if (mFrame && !mFrame->scene() && mDelegated->scene())
        {
            mDelegated->scene()->addItem(mFrame);
            mFrame->setAntiScale(mAntiScaleRatio);
            mFrame->positionHandles();
            updateButtons(true);

            foreach(DelegateButton* button, mButtons) {
                scene->removeItem(button);
            }
            scene->removeItem(mFrame);
        }

        /* this is performed because when removing delegated from scene while it contains flash content, segfault happens because of QGraphicsScene::removeItem() */
        GraphicsWidgetItem *mDelegated_casted = dynamic_cast<GraphicsWidgetItem*>(mDelegated);
  /*      if (mDelegated_casted)
            mDelegated_casted->setHtml(QString());*/

        scene->removeItem(mDelegated);

        if (canUndo)
        {
            GraphicsItemUndoCommand *uc = new GraphicsItemUndoCommand(scene, mDelegated, 0);
            Application::undoStack->push(uc);
        }
    }
}


bool GraphicsItemDelegate::isLocked() const
{
    return mDelegated->data(GraphicsItemData::ItemLocked).toBool();
}


void GraphicsItemDelegate::duplicate()
{
    Application::boardController->duplicateItem(dynamic_cast<Item*>(delegated()));
}

void GraphicsItemDelegate::increaseZLevelUp()
{
    GraphicsScene *curScene = owningScene();
    if (curScene) {
        curScene->changeZLevelTo(delegated(), ZLayerController::up, true);
    }
}
void GraphicsItemDelegate::increaseZlevelTop()
{
    GraphicsScene *curScene = owningScene();
    if (curScene) {
        curScene->changeZLevelTo(delegated(), ZLayerController::top, true);
    }
}
void GraphicsItemDelegate::increaseZLevelDown()
{
    GraphicsScene *curScene = owningScene();
    if (curScene) {
        curScene->changeZLevelTo(delegated(), ZLayerController::down, true);
    }
}
void GraphicsItemDelegate::increaseZlevelBottom()
{
    GraphicsScene *curScene = owningScene();
    if (curScene) {
        curScene->changeZLevelTo(delegated(), ZLayerController::bottom, true);
    }
}

void GraphicsItemDelegate::lock(bool locked)
{
    setLockedRecurs(locked, mDelegated);
    mDelegated->update();

    positionHandles();
    mFrame->positionHandles();
}


void GraphicsItemDelegate::setLockedRecurs(const QVariant &pLock, QGraphicsItem *pItem)
{
    pItem->setData(GraphicsItemData::ItemLocked, pLock);
    foreach (QGraphicsItem *insideItem, pItem->childItems())
    {
        setLockedRecurs(pLock, insideItem);
    }
}

void GraphicsItemDelegate::showHide(bool show)
{
    QVariant showFlag = QVariant(show ? ItemLayerType::Object : ItemLayerType::Control);
    showHideRecurs(showFlag, mDelegated);
    mDelegated->update();

    emit showOnDisplayChanged(show);
}

void GraphicsItemDelegate::showHideRecurs(const QVariant &pShow, QGraphicsItem *pItem)
{
    pItem->setData(GraphicsItemData::ItemLayerType, pShow);
    foreach (QGraphicsItem *insideItem, pItem->childItems()) {
        showHideRecurs(pShow, insideItem);
    }
}

/**
 * @brief Set delegate as background for the scene, replacing any existing background.
 */
void GraphicsItemDelegate::setAsBackground()
{
    GraphicsScene* scene = owningScene();
    QGraphicsItem* item = delegated();

    if (scene && item) {
        startUndoStep();

        item->resetTransform();
        item->setPos(item->sceneBoundingRect().width()/-2., item->sceneBoundingRect().height()/-2.);

        scene->setAsBackgroundObject(item, true);

        GraphicsItemTransformUndoCommand *uc =
                new GraphicsItemTransformUndoCommand(mDelegated,
                                                       mPreviousPosition,
                                                       mPreviousTransform,
                                                       mPreviousZValue,
                                                       mPreviousSize,
                                                       true);
        Application::undoStack->push(uc);
    }
}

void GraphicsItemDelegate::gotoContentSource()
{
    Item* item = dynamic_cast<Item*>(mDelegated);

    if(item && !item->sourceUrl().isEmpty())
    {
        Application::applicationController->showInternet();
        Application::webController->loadUrl(item->sourceUrl());
    }
}

void GraphicsItemDelegate::startUndoStep()
{
    mPreviousPosition = mDelegated->pos();
    mPreviousTransform = mDelegated->transform();
    mPreviousZValue = mDelegated->zValue();
    ResizableGraphicsItem* resizableItem = dynamic_cast<ResizableGraphicsItem*>(mDelegated);

    if (resizableItem)
        mPreviousSize = resizableItem->size();
    else
        mPreviousSize = QSizeF();
}


void GraphicsItemDelegate::commitUndoStep()
{
    ResizableGraphicsItem* resizableItem = dynamic_cast<ResizableGraphicsItem*>(mDelegated);

    if (mDelegated->pos() != mPreviousPosition
        || mDelegated->transform() != mPreviousTransform
        || mDelegated->zValue() != mPreviousZValue
        || (resizableItem && resizableItem->size() != mPreviousSize))
    {
        GraphicsItemTransformUndoCommand *uc =
                new GraphicsItemTransformUndoCommand(mDelegated,
                                                       mPreviousPosition,
                                                       mPreviousTransform,
                                                       mPreviousZValue,
                                                       mPreviousSize);

        Application::undoStack->push(uc);
    }
}

void GraphicsItemDelegate::onZoomChanged()
{
    mAntiScaleRatio = 1 / (Application::boardController->systemScaleFactor() * Application::boardController->currentZoom());

    positionHandles();
}

void GraphicsItemDelegate::buildButtons()
{
}

void GraphicsItemDelegate::freeButtons()
{
   //Previously deleted with the frame
   // Rimplement for some specific behavior
   mButtons.clear();
   mDeleteButton = 0;
   mMenuButton = 0;
   mZOrderUpButton = 0;
   mZOrderDownButton = 0;
}

void GraphicsItemDelegate::decorateMenu(QMenu* menu)
{
    mLockAction = menu->addAction(tr("Locked"), this, SLOT(lock(bool)));
    QIcon lockIcon;
    lockIcon.addPixmap(QPixmap(":/images/locked.svg"), QIcon::Normal, QIcon::On);
    lockIcon.addPixmap(QPixmap(":/images/unlocked.svg"), QIcon::Normal, QIcon::Off);
    mLockAction->setIcon(lockIcon);
    mLockAction->setCheckable(true);

    mShowOnDisplayAction = mMenu->addAction(tr("Visible on Extended Screen"), this, SLOT(showHide(bool)));
    mShowOnDisplayAction->setCheckable(true);

    QIcon showIcon;
    showIcon.addPixmap(QPixmap(":/images/eyeOpened.svg"), QIcon::Normal, QIcon::On);
    showIcon.addPixmap(QPixmap(":/images/eyeClosed.svg"), QIcon::Normal, QIcon::Off);
    mShowOnDisplayAction->setIcon(showIcon);

    if (delegated()->data(GraphicsItemData::ItemCanBeSetAsBackground).toBool()) {
        mSetAsBackgroundAction = mMenu->addAction(tr("Set as background"), this, SLOT(setAsBackground()));
        mSetAsBackgroundAction->setCheckable(false);

        QIcon backgroundIcon;
        backgroundIcon.addPixmap(QPixmap(":/images/setAsBackground.svg"), QIcon::Normal, QIcon::On);
        mSetAsBackgroundAction->setIcon(backgroundIcon);
    }

    if (testFrameFlags(GF_SHOW_CONTENT_SOURCE))
    {
        mGotoContentSourceAction = menu->addAction(tr("Go to Content Source"), this, SLOT(gotoContentSource()));

        QIcon sourceIcon;
        sourceIcon.addPixmap(QPixmap(":/images/toolbar/web.png"), QIcon::Normal, QIcon::On);
        mGotoContentSourceAction->setIcon(sourceIcon);
    }
}

void GraphicsItemDelegate::updateMenuActionState()
{
    if (mLockAction)
        mLockAction->setChecked(isLocked());

    if (mShowOnDisplayAction)
    {
        bool isControl = mDelegated->data(GraphicsItemData::ItemLayerType) == ItemLayerType::Control;
        mShowOnDisplayAction->setChecked(!isControl);
    }

    if (mGotoContentSourceAction)
    {
        Item* item = dynamic_cast<Item*>(mDelegated);
        mGotoContentSourceAction->setEnabled(item && !item->sourceUrl().isEmpty());
    }
}

void GraphicsItemDelegate::showMenu()
{
    if (!mMenu)
    {
        mMenu = new QMenu(Application::boardController->controlView());
        decorateMenu(mMenu);
    }

    updateMenuActionState();

    BoardView* cv = Application::boardController->controlView();
    QRect pinPos = cv->mapFromScene(mMenuButton->sceneBoundingRect()).boundingRect();

    mMenu->exec(cv->mapToGlobal(pinPos.bottomRight()));
}

void GraphicsItemDelegate::setLocked(bool pLocked)
{
    Q_ASSERT(mDelegated);

    if (mDelegated) {
        mDelegated->setData(GraphicsItemData::ItemLocked, QVariant(pLocked));
    }
}

void GraphicsItemDelegate::updateFrame()
{
    if (mFrame && !mFrame->scene() && mDelegated->scene())
        mDelegated->scene()->addItem(mFrame);

    mFrame->setAntiScale(mAntiScaleRatio);
    mFrame->positionHandles();
}

void GraphicsItemDelegate::updateButtons(bool showUpdated)
{
    QTransform tr;
    tr.scale(mAntiScaleRatio, mAntiScaleRatio);

    mDeleteButton->setParentItem(mFrame);
    mDeleteButton->setTransform(tr);

    qreal topX = mFrame->rect().left() - mDeleteButton->renderer()->viewBox().width() * mAntiScaleRatio / 2;
    qreal topY = mFrame->rect().top() - mDeleteButton->renderer()->viewBox().height() * mAntiScaleRatio / 2;

    qreal bottomX = mFrame->rect().left() - mDeleteButton->renderer()->viewBox().width() * mAntiScaleRatio / 2;
    qreal bottomY = mFrame->rect().bottom() - mDeleteButton->renderer()->viewBox().height() * mAntiScaleRatio / 2;

    mDeleteButton->setPos(topX, topY);

    if (!mDeleteButton->scene())
    {
        if (mDelegated->scene())
            mDelegated->scene()->addItem(mDeleteButton);
    }

    if (showUpdated)
        mDeleteButton->show();

    int i = 1, j = 0, k = 0, l = 0;
    int frameButtonHeight = mDeleteButton->boundingRect().size().height();
    qreal topXTitleBar = topX + (1.6 * mFrameWidth * mAntiScaleRatio);
    qreal topYTitleBar = topY + frameButtonHeight *mAntiScaleRatio;

#ifndef Q_OS_LINUX
    topYTitleBar += 5;
#endif

    while ((i + j + k + l) < mButtons.size())  {
        DelegateButton* button = mButtons[i + j + k + l];

        if (button->getSection() == Qt::TopLeftSection) {
            button->setParentItem(mFrame);
            button->setPos(topX + (i++ * 1.6 * mFrameWidth * mAntiScaleRatio), topY);
            button->setTransform(tr);
        }
        else if (button->getSection() == Qt::BottomLeftSection) {
            button->setParentItem(mFrame);
            button->setPos(bottomX + (++j * 1.6 * mFrameWidth * mAntiScaleRatio), bottomY);
            button->setTransform(tr);
        }
        else if (button->getSection() == Qt::TitleBarArea){
            button->setParentItem(mFrame);
            button->setPos(topXTitleBar + (k++ * (frameButtonHeight + 5) * mAntiScaleRatio), topYTitleBar);
            button->setTransform(tr);
            button->setTransform(QTransform::fromScale(0.8, 0.8), true);
        }
        else if(button->getSection() == Qt::NoSection){
            ++l;
        }
        if (!button->scene())
        {
            if (mDelegated->scene())
                mDelegated->scene()->addItem(button);
        }
        if (showUpdated) {
            button->show();
            button->setZValue(delegated()->zValue());
        }
    }
}

void GraphicsItemDelegate::setButtonsVisible(bool visible)
{
    foreach(DelegateButton* pButton, mButtons){
        pButton->setVisible(visible);
    }
}

void GraphicsItemDelegate::setFrameFlags(GraphicsFlags pf)
{
    mFlags = pf;

    Q_ASSERT (mDelegated);
    if (!mDelegated) {
        return;
    }

    if (testFrameFlags(GF_FLIPPABLE_ALL_AXIS)) {
        mDelegated->setData(GraphicsItemData::ItemFlippable, QVariant(testFrameFlags(GF_FLIPPABLE_ALL_AXIS)));
    }

    if (testFrameFlags(GF_REVOLVABLE)) {
        mDelegated->setData(GraphicsItemData::ItemRotatable, QVariant(testFrameFlags(GF_REVOLVABLE)));
    }
}

void GraphicsItemDelegate::setFrameFlag(GraphicsFlags pf, bool set)
{
    GraphicsFlags fls = mFlags;
    set ? fls |= pf : fls &= ~pf;

    setFrameFlags(fls);
}

GraphicsToolBarItem::GraphicsToolBarItem(QGraphicsItem * parent) :
    QGraphicsRectItem(parent),
    mShifting(true),
    mVisible(false),
    mMinWidth(200),
    mInitialHeight(26),
    mElementsPadding(2)
{
    QRectF rect = this->rect();
    rect.setHeight(mInitialHeight);
    rect.setWidth(parent->boundingRect().width());
    this->setRect(rect);

    setPen(Qt::NoPen);
    hide();

    update();
}


void GraphicsToolBarItem::positionHandles()
{
    int itemXOffset = 0;
    foreach (QGraphicsItem* item, mItemsOnToolBar)
    {
        item->setPos(itemXOffset, 0);
        itemXOffset += (item->boundingRect().width()+mElementsPadding);
        item->show();
    }
}

void GraphicsToolBarItem::update()
{
    QGraphicsRectItem::update();
}

void GraphicsToolBarItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QPainterPath path;
    path.addRoundedRect(rect(), 10, 10);

    setBrush(QBrush(Settings::paletteColor));

    painter->fillPath(path, brush());
}

MediaTimer::MediaTimer(QGraphicsItem * parent): QGraphicsRectItem(parent)
{
    val        = 0;
    smallPoint = false;
    setNumDigits(6);
}

MediaTimer::~MediaTimer()
{}
void MediaTimer::positionHandles()
{
    digitSpace = smallPoint ? 2 : 1;
    ySegLen    = rect().height()*5/12;
    xSegLen    = ySegLen*2/3;
    segLen     = xSegLen;
    xAdvance   = segLen*(5 + digitSpace)/5;
    xOffset    = (rect().width() - ndigits*xAdvance + segLen/5)/2;
    yOffset    = rect().height() - ySegLen*2;

    setRect(rect().x(), rect().y(), xOffset + xAdvance*ndigits, rect().height());
}

void MediaTimer::drawString(const QString &s, QPainter &p,
                                   QBitArray *newPoints, bool newString)
{
    QPoint  pos;

    for (int i=0;  i<ndigits; i++) {
        pos = QPoint(xOffset + xAdvance*i, yOffset);
        if (newString)
            drawDigit(pos, p, segLen, s[i].toLatin1(), digitStr[i].toLatin1());
        else
            drawDigit(pos, p, segLen, s[i].toLatin1());
        if (newPoints) {
            char newPoint = newPoints->testBit(i) ? '.' : ' ';
            if (newString) {
                char oldPoint = points.testBit(i) ? '.' : ' ';
                drawDigit(pos, p, segLen, newPoint, oldPoint);
            } else {
                drawDigit(pos, p, segLen, newPoint);
            }
        }
    }
    if (newString) {
        digitStr = s;
        digitStr.truncate(ndigits);
        if (newPoints)
            points = *newPoints;
    }
}

void MediaTimer::drawDigit(const QPoint &pos, QPainter &p, int segLen,
                                  char newCh, char oldCh)
{
     char updates[18][2];        // can hold 2 times number of segments, only
                                 // first 9 used if segment table is correct
     int  nErases;
     int  nUpdates;
     const char *segs;
     int  i,j;

     const char erase      = 0;
     const char draw       = 1;
     const char leaveAlone = 2;

     segs = getSegments(oldCh);
     for (nErases=0; segs[nErases] != 99; nErases++) {
         updates[nErases][0] = erase;            // get segments to erase to
         updates[nErases][1] = segs[nErases];    // remove old char
     }
     nUpdates = nErases;
     segs = getSegments(newCh);
     for(i = 0 ; segs[i] != 99 ; i++) {
         for (j=0;  j<nErases; j++)
             if (segs[i] == updates[j][1]) {   // same segment ?
                 updates[j][0] = leaveAlone;     // yes, already on screen
                 break;
             }
         if (j == nErases) {                   // if not already on screen
             updates[nUpdates][0] = draw;
             updates[nUpdates][1] = segs[i];
             nUpdates++;
         }
     }
     for (i=0; i<nUpdates; i++) {
         if (updates[i][0] == draw)
             drawSegment(pos, updates[i][1], p, segLen);
         if (updates[i][0] == erase)
             drawSegment(pos, updates[i][1], p, segLen, true);
     }
}

char MediaTimer::segments [][8] =
        {
              { 0, 1, 2, 4, 5, 6,99, 0},             // 0    0
              { 2, 5,99, 0, 0, 0, 0, 0},             // 1    1
              { 0, 2, 3, 4, 6,99, 0, 0},             // 2    2
              { 0, 2, 3, 5, 6,99, 0, 0},             // 3    3
              { 1, 2, 3, 5,99, 0, 0, 0},             // 4    4
              { 0, 1, 3, 5, 6,99, 0, 0},             // 5    5
              { 0, 1, 3, 4, 5, 6,99, 0},             // 6    6
              { 0, 2, 5,99, 0, 0, 0, 0},             // 7    7
              { 0, 1, 2, 3, 4, 5, 6,99},             // 8    8
              { 0, 1, 2, 3, 5, 6,99, 0},             // 9    9
              { 8, 9,99, 0, 0, 0, 0, 0},             // 10   :
              {99, 0, 0, 0, 0, 0, 0, 0}              // 11   empty
        };

const char* MediaTimer::getSegments(char ch)               // gets list of segments for ch
{
     if (ch >= '0' && ch <= '9')
        return segments[ch - '0'];
     if (ch == ':')
        return segments[10];
     if (ch == ' ')
        return segments[11];

     return NULL;
}

void MediaTimer::drawSegment(const QPoint &pos, char segmentNo, QPainter &p,
                                    int segLen, bool erase)
{
    Q_UNUSED(erase);

    QPoint ppt;
    QPoint pt = pos;
    int width = segLen/5;

#define LINETO(X,Y) addPoint(a, QPoint(pt.x() + (X),pt.y() + (Y)))
#define LIGHT
#define DARK

    QPolygon a;
    switch (segmentNo) {
    case 0 :
        ppt = pt;
        LIGHT;
        LINETO(segLen - 1,0);
        DARK;
        LINETO(segLen - width - 1,width);
        LINETO(width,width);
        LINETO(0,0);
        break;
    case 1 :
        pt += QPoint(0 , 1);
        ppt = pt;
        LIGHT;
        LINETO(width,width);
        DARK;
        LINETO(width,segLen - width/2 - 2);
        LINETO(0,segLen - 2);
        LIGHT;
        LINETO(0,0);
        break;
    case 2 :
        pt += QPoint(segLen - 1 , 1);
        ppt = pt;
        DARK;
        LINETO(0,segLen - 2);
        LINETO(-width,segLen - width/2 - 2);
        LIGHT;
        LINETO(-width,width);
        LINETO(0,0);
        break;
    case 3 :
        pt += QPoint(0 , segLen);
        ppt = pt;
        LIGHT;
        LINETO(width,-width/2);
        LINETO(segLen - width - 1,-width/2);
        LINETO(segLen - 1,0);
        DARK;
        if (width & 1) {            // adjust for integer division error
            LINETO(segLen - width - 3,width/2 + 1);
            LINETO(width + 2,width/2 + 1);
        } else {
            LINETO(segLen - width - 1,width/2);
            LINETO(width,width/2);
        }
        LINETO(0,0);
        break;
    case 4 :
        pt += QPoint(0 , segLen + 1);
        ppt = pt;
        LIGHT;
        LINETO(width,width/2);
        DARK;
        LINETO(width,segLen - width - 2);
        LINETO(0,segLen - 2);
        LIGHT;
        LINETO(0,0);
        break;
    case 5 :
        pt += QPoint(segLen - 1 , segLen + 1);
        ppt = pt;
        DARK;
        LINETO(0,segLen - 2);
        LINETO(-width,segLen - width - 2);
        LIGHT;
        LINETO(-width,width/2);
        LINETO(0,0);
        break;
    case 6 :
        pt += QPoint(0 , segLen*2);
        ppt = pt;
        LIGHT;
        LINETO(width,-width);
        LINETO(segLen - width - 1,-width);
        LINETO(segLen - 1,0);
        DARK;
        LINETO(0,0);
        break;
    case 7 :
        pt += QPoint(segLen/2 , segLen*2);
        ppt = pt;
        DARK;
        LINETO(width,0);
        LINETO(width,-width);
        LIGHT;
        LINETO(0,-width);
        LINETO(0,0);
        break;
    case 8 :
        pt += QPoint(segLen/2 - width/2 + 1 , segLen/2 + width);
        ppt = pt;
        DARK;
        LINETO(width,0);
        LINETO(width,-width);
        LIGHT;
        LINETO(0,-width);
        LINETO(0,0);
        break;
    case 9 :
        pt += QPoint(segLen/2 - width/2 + 1 , 3*segLen/2 + width);
        ppt = pt;
        DARK;
        LINETO(width,0);
        LINETO(width,-width);
        LIGHT;
        LINETO(0,-width);
        LINETO(0,0);
        break;
    default :
        break;
    }
    // End exact copy
    p.setPen(Qt::white);
    p.setBrush(Qt::white);
    p.drawPolygon(a);
    p.setBrush(Qt::NoBrush);

    pt = pos;

#undef LINETO
#undef LIGHT
#undef DARK
}

void MediaTimer::addPoint(QPolygon &a, const QPoint &p)
{
    uint n = a.size();
    a.resize(n + 1);
    a.setPoint(n, p);
}

void MediaTimer::paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (smallPoint)
        drawString(digitStr, *p, &points, false);
    else
        drawString(digitStr, *p, 0, false);
}

void MediaTimer::internalSetString(const QString& s)
{
    QString buffer;
    int i;
    int len = s.length();
    QBitArray newPoints(ndigits);

    if (!smallPoint) {
        if (len == ndigits)
            buffer = s;
        else
            buffer = s.right(ndigits).rightJustified(ndigits, QLatin1Char(' '));
    } else {
        int  index = -1;
        bool lastWasPoint = true;
        newPoints.clearBit(0);
        for (i=0; i<len; i++) {
            if (s[i] == QLatin1Char('.')) {
                if (lastWasPoint) {           // point already set for digit?
                    if (index == ndigits - 1) // no more digits
                        break;
                    index++;
                    buffer[index] = QLatin1Char(' ');        // 2 points in a row, add space
                }
                newPoints.setBit(index);        // set decimal point
                lastWasPoint = true;
            } else {
                if (index == ndigits - 1)
                    break;
                index++;
                buffer[index] = s[i];
                newPoints.clearBit(index);      // decimal point default off
                lastWasPoint = false;
            }
        }
        if (index < ((int) ndigits) - 1) {
            for(i=index; i>=0; i--) {
                buffer[ndigits - 1 - index + i] = buffer[i];
                newPoints.setBit(ndigits - 1 - index + i,
                                   newPoints.testBit(i));
            }
            for(i=0; i<ndigits-index-1; i++) {
                buffer[i] = QLatin1Char(' ');
                newPoints.clearBit(i);
            }
        }
    }

    if (buffer == digitStr)
        return;

    digitStr = buffer;
    if (smallPoint)
        points = newPoints;
    update();
}

void MediaTimer::display(const QString &s)
{
    val = 0;
    bool ok = false;
    double v = s.toDouble(&ok);
    if (ok)
        val = v;
    internalSetString(s);
}

void MediaTimer::setNumDigits(int numDigits)
{
    if (numDigits > 99) {
        qWarning("QLCDNumber::setNumDigits: Max 99 digits allowed");
        numDigits = 99;
    }
    if (numDigits < 0) {
        qWarning("QLCDNumber::setNumDigits: Min 0 digits allowed");
        numDigits = 0;
    }
    if (digitStr.isNull()) {                  // from constructor
        ndigits = numDigits + numDigits/2 - 1;
        digitStr.fill(QLatin1Char(' '), ndigits);
        points.fill(0, ndigits);
        digitStr[ndigits - 1] = QLatin1Char('0');            // "0" is the default number
    } else {
        if (numDigits == ndigits)             // no change
            return;
        int i;
        int dif;
        if (numDigits > ndigits) {            // expand
            dif = numDigits - ndigits;
            QString buf;
            buf.fill(QLatin1Char(' '), dif);
            digitStr.insert(0, buf);
            points.resize(numDigits);
            for (i=numDigits-1; i>=dif; i--)
                points.setBit(i, points.testBit(i-dif));
            for (i=0; i<dif; i++)
                points.clearBit(i);
        } else {                                        // shrink
            dif = ndigits - numDigits;
            digitStr = digitStr.right(numDigits);
            QBitArray tmpPoints = points;
            points.resize(numDigits);
            for (i=0; i<(int)numDigits; i++)
                points.setBit(i, tmpPoints.testBit(i+dif));
        }
        ndigits = numDigits;
        update();
    }
    positionHandles();
}

DelegateMediaControl::DelegateMediaControl(GraphicsMediaItem* pDelegated, QGraphicsItem * parent)
    : QGraphicsRectItem(parent)
    , mDelegate(pDelegated)
    , mDisplayCurrentTime(false)
    , mCurrentTimeInMs(0)
    , mTotalTimeInMs(0)
    , mStartWidth(200)
    , mSeecAreaBorderHeight(0)
{
    setAcceptedMouseButtons(Qt::LeftButton);
    setBrush(QBrush(Qt::white));
    setPen(Qt::NoPen);
    setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Control));

    lcdTimer = new MediaTimer(this);

    update();
}


void DelegateMediaControl::paint(QPainter *painter,
        const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QPainterPath path;

    path.addRoundedRect(mSeecArea, mSeecArea.height()/2, mSeecArea.height()/2);
    painter->fillPath(path, brush());

    qreal frameWidth = mSeecArea.height() / 2;
    int position = frameWidth;

    if (mTotalTimeInMs > 0)
    {
        position = frameWidth + ((mSeecArea.width() - (2 * frameWidth)) / mTotalTimeInMs) * mCurrentTimeInMs;
    }

    int clearance = 2;
    int radius = frameWidth-clearance;
    QRectF r(position - radius, clearance+mSeecAreaBorderHeight, radius * 2, radius * 2);

    painter->setBrush(Settings::documentViewLightColor);
    painter->setPen(Qt::black);
    painter->drawEllipse(r);
}


QPainterPath DelegateMediaControl::shape() const
{
    QPainterPath path;
    path.addRoundedRect(mSeecArea, mSeecArea.height()/2, mSeecArea.height()/2);
    return path;
}

void DelegateMediaControl::positionHandles()
{
    QTime tTotal;
    tTotal = tTotal.addMSecs(mTotalTimeInMs);
    mLCDTimerArea.setHeight(parentItem()->boundingRect().height());
    int digitsCount = 2;
    int timerWidth = mLCDTimerArea.height();

    mDisplayFormat = "ss";


    //Explanation at least the second and minutes are diplayed
    mDisplayFormat = "mm:" + mDisplayFormat;
    digitsCount += 3;
    timerWidth += mLCDTimerArea.height();

    if (tTotal.hour() > 0)
    {
        mDisplayFormat = "hh:" + mDisplayFormat;
        digitsCount += 3;
        timerWidth += mLCDTimerArea.height();
    }

    lcdTimer->setNumDigits(digitsCount);

    mLCDTimerArea.setWidth(timerWidth);
    lcdTimer->setRect(mLCDTimerArea);

    lcdTimer->setPos(rect().width() - mLCDTimerArea.width(), 0);

    mSeecAreaBorderHeight = rect().height()/20;
    mSeecArea.setWidth(rect().width()-mLCDTimerArea.width()-2);
    mSeecArea.setHeight(rect().height()-2*mSeecAreaBorderHeight);
    mSeecArea.setY(mSeecAreaBorderHeight);
}

void DelegateMediaControl::update()
{
    QTime tCurrent;
    tCurrent = tCurrent.addMSecs(mCurrentTimeInMs < 0 ? 0 : mCurrentTimeInMs);


    lcdTimer->display(tCurrent.toString(mDisplayFormat));

    QGraphicsRectItem::update();
}

void DelegateMediaControl::updateTicker(qint64 time )
{
    mCurrentTimeInMs = time;
    update();
}


void DelegateMediaControl::totalTimeChanged(qint64 newTotalTime)
{
    if (mTotalTimeInMs != newTotalTime)
    {
        mTotalTimeInMs = newTotalTime;
        positionHandles();
        update();
    }
}


void DelegateMediaControl::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
   qreal frameWidth =  mSeecArea.height()/2;
    if (boundingRect().contains(event->pos() - QPointF(frameWidth,0))
        && boundingRect().contains(event->pos() + QPointF(frameWidth,0)))
    {
        mDisplayCurrentTime = true;
        seekToMousePos(event->pos());
        this->update();
        event->accept();
        emit used();
    }
}

void DelegateMediaControl::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    qreal frameWidth = mSeecArea.height() / 2;
    if (boundingRect().contains(event->pos() - QPointF(frameWidth,0))
        && boundingRect().contains(event->pos() + QPointF(frameWidth,0)))
    {
        seekToMousePos(event->pos());
        this->update();
        event->accept();
        emit used();
    }
}

void DelegateMediaControl::seekToMousePos(QPointF mousePos)
{
    qreal minX, length;
    qreal frameWidth = rect().height() / 2;

    minX = frameWidth;
    length = mSeecArea.width() - mSeecArea.height();

    qreal mouseX = mousePos.x();
    if (mouseX >= (mSeecArea.width() - mSeecArea.height()/2))
        mouseX = mSeecArea.width() - mSeecArea.height()/2;

    if (mTotalTimeInMs > 0 && length > 0 && mDelegate
        && mDelegate->isMediaSeekable())
    {
        qint64 tickPos = (mTotalTimeInMs/length)* (mouseX - minX);
        mDelegate->setMediaPos(tickPos);

        //OSX is a bit lazy
        updateTicker(tickPos);
    }
    emit used();
}

void DelegateMediaControl::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    mDisplayCurrentTime = false;
    this->update();
    event->accept();
}
