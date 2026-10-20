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
#include "settings/Settings.h"
#include "core/PersistenceManager.h"

#include "model/DocumentProxy.h"

#include "whiteboard/GraphicsWidgetItem.h"

#include "whiteboard/GraphicsTextItem.h"
#include "whiteboard/GraphicsMediaItem.h"
#include "whiteboard/GraphicsGroupContainerItem.h"


#include "utils/FileSystemUtils.h"
#include "whiteboard/DrawingController.h"


DelegateButton::DelegateButton(const QString &fileName, QGraphicsItem *pDelegated, QGraphicsItem *parent,
                               Qt::WindowFrameSection section)
    : QGraphicsSvgItem(fileName, parent),
      mDelegated(pDelegated),
      mIsTransparentToMouseEvent(false),
      mIsPressed(false),
      mProgressTimerId(-1),
      mPressProgres(0),
      mShowProgressIndicator(false),
      mButtonAlignmentSection(section) {
    setAcceptedMouseButtons(Qt::LeftButton);
    setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Control));
    setCacheMode(QGraphicsItem::NoCache); /* 必须 NoCache：抓屏时位图要随窗口刷新，教师屏才能与副屏同步 */
}

DelegateButton::~DelegateButton() {}

void DelegateButton::setFileName(const QString &fileName) {
    // 直接替换共享渲染器，父对象设为自身以便随按钮释放
    QGraphicsSvgItem::setSharedRenderer(new QSvgRenderer(fileName, this));
}

void DelegateButton::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    // 长按 300ms 后才开始显示进度，避免普通点击闪烁
    if (mShowProgressIndicator) {
        QTimer::singleShot(300, this, &DelegateButton::startShowProgress);
    }

    mIsPressed = true;

    // 记录按下时刻，释放时据此区分单击与长按
    mPressedTime = QTime::currentTime();

    event->setAccepted(!mIsTransparentToMouseEvent);
}

void DelegateButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (mShowProgressIndicator && mProgressTimerId != -1) {
        killTimer(mProgressTimerId);
        mPressProgres = 0;
    }

    mIsPressed = false;
    int timeto = qAbs(QTime::currentTime().msecsTo(mPressedTime));

    // 按压时间短于长按阈值算点击，否则算长按
    if (timeto < Settings::longClickInterval) {
        emit clicked();
    } else {
        emit longClicked();
    }

    event->setAccepted(!mIsTransparentToMouseEvent);

    update();
}

void DelegateButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    painter->save();
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    QGraphicsSvgItem::paint(painter, option, widget);
    painter->restore();

    // 按压期间在图标外画一圈白色进度弧，长度按已按压时长增长
    if (mIsPressed && mShowProgressIndicator) {
        QPen pen;
        pen.setBrush(Qt::white);
        pen.setWidth(3);
        painter->save();
        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter->setPen(pen);

        int spanAngle = qMin(mPressProgres, Settings::longClickInterval) * 360 / Settings::longClickInterval;
        painter->drawArc(option->rect.adjusted(pen.width(), pen.width(), -pen.width(), -pen.width()), 16 * 90,
                         -16 * spanAngle);

        painter->restore();
    }
}

void DelegateButton::timerEvent(QTimerEvent *event) {
    // 约 27fps 刷新进度弧，开销很小但足够顺滑
    if (event->timerId() == mProgressTimerId) {
        mPressProgres = qAbs(QTime::currentTime().msecsTo(mPressedTime));
        update();
    }
}

void DelegateButton::startShowProgress() {
    // 延迟到点后按钮可能已经松开，需再次确认仍处于按压状态
    if (mIsPressed) {
        mProgressTimerId = startTimer(37);
    }
}

GraphicsItemDelegate::GraphicsItemDelegate(QGraphicsItem *pDelegated, QObject *parent, GraphicsFlags fls)
    : QObject(parent),
      mDelegated(pDelegated),
      mDeleteButton(NULL),
      mDuplicateButton(NULL),
      mMenuButton(NULL),
      mZOrderUpButton(0),
      mZOrderDownButton(0),
      mMenu(0),
      mLockAction(0),
      mShowOnDisplayAction(0),
      mGotoContentSourceAction(0),
      mFrame(0),
      mFrameWidth(Settings::settings()->objectFrameWidth),
      mAntiScaleRatio(1.0),
      mToolBarItem(NULL),
      mMimeData(NULL) {
    setFrameFlags(fls);
    // 视图缩放变化时所有委托都要重新摆放选择框
    connect(Application::boardController, &BoardController::zoomChanged, this,
            &GraphicsItemDelegate::onZoomChanged);
}

void GraphicsItemDelegate::createControls() {
    // 需要工具栏的项先创建工具栏容器，按钮由子类注入
    if (testFrameFlags(GF_TOOLBAR_USED) && !mToolBarItem)
        mToolBarItem = new GraphicsToolBarItem(mDelegated);

    if (!mFrame) {
        mFrame =
            new GraphicsDelegateFrame(this, QRectF(0, 0, 0, 0), mFrameWidth, testFrameFlags(GF_RESPECT_RATIO),
                                      testFrameFlags(GF_TITLE_BAR_USED));
        // 先隐藏，等项被选中时再显示
        mFrame->hide();
        mFrame->setFlag(QGraphicsItem::ItemIsSelectable, true);
    }

    if (!mDeleteButton) {
        mDeleteButton = new DelegateButton(":/images/close.svg", mDelegated, mFrame, Qt::TopLeftSection);
        mButtons << mDeleteButton;
        connect(mDeleteButton, &DelegateButton::clicked, this, [this]() { remove(); });
        if (testFrameFlags(GF_DUPLICATION_ENABLED)) {
            mDuplicateButton =
                new DelegateButton(":/images/duplicate.svg", mDelegated, mFrame, Qt::TopLeftSection);
            connect(mDuplicateButton, &DelegateButton::clicked, this, &GraphicsItemDelegate::duplicate);
            mButtons << mDuplicateButton;
        }
    }

    if (!mMenuButton) {
        mMenuButton = new DelegateButton(":/images/menu.svg", mDelegated, mFrame, Qt::TopLeftSection);
        connect(mMenuButton, &DelegateButton::clicked, this, &GraphicsItemDelegate::showMenu);
        mButtons << mMenuButton;
    }

    if (!mZOrderUpButton) {
        mZOrderUpButton =
            new DelegateButton(":/images/z_layer_up.svg", mDelegated, mFrame, Qt::BottomLeftSection);
        // 层级按钮支持长按直达最上层/最下层，因此需要进度指示
        mZOrderUpButton->setShowProgressIndicator(true);
        connect(mZOrderUpButton, &DelegateButton::clicked, this, &GraphicsItemDelegate::increaseZLevelUp);
        connect(mZOrderUpButton, &DelegateButton::longClicked, this,
                &GraphicsItemDelegate::increaseZlevelTop);
        mButtons << mZOrderUpButton;
    }

    if (!mZOrderDownButton) {
        mZOrderDownButton =
            new DelegateButton(":/images/z_layer_down.svg", mDelegated, mFrame, Qt::BottomLeftSection);
        mZOrderDownButton->setShowProgressIndicator(true);
        connect(mZOrderDownButton, &DelegateButton::clicked, this, &GraphicsItemDelegate::increaseZLevelDown);
        connect(mZOrderDownButton, &DelegateButton::longClicked, this,
                &GraphicsItemDelegate::increaseZlevelBottom);
        mButtons << mZOrderDownButton;
    }


    buildButtons();

    // 统一的初始隐藏，确保未选中的项不显示任何控件
    foreach (DelegateButton *button, mButtons) {
        button->hide();
        button->setFlag(QGraphicsItem::ItemIsSelectable, true);
    }
}

void GraphicsItemDelegate::freeControls() {
    QGraphicsScene *controlsScene = delegated()->scene();
    Q_ASSERT(controlsScene);

    if (mFrame) {
        if (controlsScene)
            controlsScene->removeItem(mFrame);
        // 框架的子项（按钮、把手）随框架一起延迟销毁
        mFrame->deleteLater();
        mFrame = nullptr;
    }
    freeButtons();
}

void GraphicsItemDelegate::showControls() {
    // 控件按屏幕像素绘制，需要抵消系统缩放与画板缩放
    mAntiScaleRatio =
        1 / (Application::boardController->systemScaleFactor() * Application::boardController->currentZoom());
    positionHandles();
}

bool GraphicsItemDelegate::controlsExist() const {
    return mFrame && mDeleteButton && mMenuButton && mZOrderUpButton && mZOrderDownButton;
}

GraphicsItemDelegate::~GraphicsItemDelegate() {
    if (Application::boardController)
        disconnect(Application::boardController, &BoardController::zoomChanged, this,
                   &GraphicsItemDelegate::onZoomChanged);
    // 不在此处释放 mMimeData：交给 QDrag 后其所有权已转移，
    // 未交给 QDrag 的情况由下一次 setMimeData() 负责回收。
}


QVariant GraphicsItemDelegate::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value) {
    GraphicsScene *ownerScene = owningScene();
    switch (static_cast<int>(change)) {
        case QGraphicsItem::ItemSelectedHasChanged: {
            if (ownerScene) {
                if (value.toBool()) { // 变为选中
                    // 选中时临时提升到选中层级，避免被其他项遮挡
                    ownerScene->setSelectedZLevel(delegated());
                } else {
                    // 取消选中后恢复原层级并释放选择框，减少常驻图元
                    ownerScene->setOwnZlevel(delegated());
                    freeControls();
                }
            }

        } break;
        case QGraphicsItem::ItemVisibleHasChanged: {
            // 控制类项不参与“在扩展屏显示”，其余项随可见性同步显示状态
            bool shownOnDisplay =
                mDelegated->data(GraphicsItemData::ItemLayerType).toInt() != ItemLayerType::Control;
            showHide(shownOnDisplay);
            break;
        }
        case QGraphicsItem::ItemPositionHasChanged:
        case QGraphicsItem::ItemTransformHasChanged:
        case QGraphicsItem::ItemZValueHasChanged:
            if (!controlsExist()) {
                break;
            }
            // 几何变化后选择框要跟随，并标记文档已修改
            mAntiScaleRatio = 1 / (Application::boardController->systemScaleFactor() *
                                   Application::boardController->currentZoom());
            positionHandles();
            if (ownerScene) {
                ownerScene->setModified(true);
            }
            break;
    }

    return value;
}

GraphicsScene *GraphicsItemDelegate::owningScene() {
    GraphicsScene *castScene = dynamic_cast<GraphicsScene *>(delegated()->scene());

    return castScene;
}

// 主绘制完成后的附加绘制：无控件时用半透明矩形表示选中
void GraphicsItemDelegate::postpaint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                                     QWidget *widget) {
    Q_UNUSED(widget)
    // 控件尚未创建时（如组合），用半透明遮罩表示选中
    if (option->state & QStyle::State_Selected && !controlsExist()) {
        painter->save();
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0x88, 0x88, 0x88, 0x77));
        painter->drawRect(option->rect);

        painter->restore();
    }
}

bool GraphicsItemDelegate::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    mDragStartPosition = event->pos();

    startUndoStep();

    // 未选中时第一次按下只负责选中，不进入拖动
    if (!delegated()->isSelected()) {
        delegated()->setSelected(true);
        return true;
    } else {
        return false;
    }
}

void GraphicsItemDelegate::setMimeData(QMimeData *mimeData) {
    if (mMimeData != mimeData) {
        delete mMimeData; // 上一次按下后没真正拖起来的那份
    }
    mMimeData = mimeData;
}

bool GraphicsItemDelegate::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (mMimeData) {
        QDrag *mDrag = new QDrag(event->widget());
        mDrag->setMimeData(mMimeData);
        if (!mDragPixmap.isNull()) {
            mDrag->setPixmap(mDragPixmap);
            mDrag->setHotSpot(mDragPixmap.rect().center());
        }
        mDrag->exec();
        // QDrag 已经接管 mMimeData（它挂在 event->widget() 下，随控件回收）；
        // 这里必须断开，否则下次 setMimeData 会把 QDrag 拥有的对象再 delete 一次。
        mMimeData = nullptr;
        mDragPixmap = QPixmap();
        return true;
    }
    return false;
}

bool GraphicsItemDelegate::wheelEvent(QGraphicsSceneWheelEvent *event) {
    Q_UNUSED(event);
    // 选中项吞掉滚轮，避免滚动时误缩放画板
    if (delegated()->isSelected())
        return true;
    else
        return false;
}

bool GraphicsItemDelegate::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    Q_UNUSED(event);

    // 未按 Ctrl 时，释放后只保留当前项选中
    if (delegated()->scene() && delegated()->scene()->selectedItems().count() &&
        event->modifiers() != Qt::ControlModifier) {
        foreach (QGraphicsItem *item, delegated()->scene()->selectedItems()) {
            if (item != delegated()) {
                item->setSelected(false);
            }
        }
    }

    commitUndoStep();

    return true;
}

void GraphicsItemDelegate::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
    Q_UNUSED(event)
}

void GraphicsItemDelegate::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
    Q_UNUSED(event)
}

bool GraphicsItemDelegate::keyPressEvent(QKeyEvent *event) {
    Q_UNUSED(event);
    return true;
}

bool GraphicsItemDelegate::keyReleaseEvent(QKeyEvent *event) {
    Q_UNUSED(event);
    return true;
}

QGraphicsItem *GraphicsItemDelegate::delegated() {
    QGraphicsItem *curDelegate = 0;
    // 若图形项属于组合，则把整个组合当作委托目标，选择框和删除都作用于组合
    if (mDelegated->parentItem() && mDelegated->parentItem()->type() == GraphicsGroupContainerItem::Type) {
        curDelegate = mDelegated->parentItem();
    } else {
        curDelegate = mDelegated;
    }

    return curDelegate;
}

void GraphicsItemDelegate::positionHandles() {
    if (!controlsExist()) {
        return;
    }

    if (mDelegated->isSelected()) {
        // 选中时才构建并显示控件；触控笔处于播放模式时不显示框架，避免遮挡演示
        bool shownOnDisplay =
            mDelegated->data(GraphicsItemData::ItemLayerType).toInt() != ItemLayerType::Control;
        showHide(shownOnDisplay);
        mDelegated->setData(GraphicsItemData::ItemLocked, QVariant(isLocked()));
        updateFrame();

        if (StylusTool::Play != DrawingController::drawingController()->stylusTool())
            mFrame->show();

        updateButtons(true);

        if (mToolBarItem && mToolBarItem->isVisibleOnBoard()) {
            mToolBarItem->positionHandles();
            mToolBarItem->update();
            mToolBarItem->show();
        }
    } else {
        foreach (DelegateButton *button, mButtons)
            button->hide();

        if (mFrame)
            mFrame->hide();

        if (mToolBarItem)
            mToolBarItem->hide();
    }
}

void GraphicsItemDelegate::setZOrderButtonsVisible(bool visible) {
    if (visible) {
        updateFrame();
        updateButtons();

        // 把层级按钮从框架坐标系换算到内容坐标系，再改挂到内容上，
        // 这样按钮不会随框架一起旋转/缩放
        QPointF newUpPoint = mFrame->mapToItem(mDelegated, mZOrderUpButton->pos());
        QPointF newDownPoint = mFrame->mapToItem(mDelegated, mZOrderDownButton->pos());


        mZOrderUpButton->setParentItem(mDelegated);
        mZOrderDownButton->setParentItem(mDelegated);

        mZOrderUpButton->setPos(newUpPoint + QPointF(0, 0));
        mZOrderDownButton->setPos(newDownPoint + QPointF(0, 0));

        mZOrderUpButton->show();
        mZOrderDownButton->show();

    } else {
        mZOrderUpButton->hide();
        mZOrderDownButton->hide();
    }
}

void GraphicsItemDelegate::remove(bool canUndo) {
    GraphicsScene *scene = dynamic_cast<GraphicsScene *>(mDelegated->scene());
    if (scene) {
        // 框架可能已脱离场景，先临时挂回并刷新按钮，确保删除命令记录的是完整状态
        if (mFrame && !mFrame->scene() && mDelegated->scene()) {
            mDelegated->scene()->addItem(mFrame);
            mFrame->setAntiScale(mAntiScaleRatio);
            mFrame->positionHandles();
            updateButtons(true);

            foreach (DelegateButton *button, mButtons) {
                scene->removeItem(button);
            }
            scene->removeItem(mFrame);
        }

        // 含内嵌窗口内容的项必须先移出场景再销毁，否则 removeItem 过程中可能崩溃；
        // 这里仅做类型转换，实际清理在场景删除项时完成
        GraphicsWidgetItem *mDelegated_casted = dynamic_cast<GraphicsWidgetItem *>(mDelegated);

        scene->removeItem(mDelegated);

        if (canUndo) {
            // 删除命令持有该项，撤销时重新插回场景
            GraphicsItemUndoCommand *uc = new GraphicsItemUndoCommand(scene, mDelegated, 0);
            Application::undoStack->push(uc);
        }
    }
}


bool GraphicsItemDelegate::isLocked() const {
    return mDelegated->data(GraphicsItemData::ItemLocked).toBool();
}


void GraphicsItemDelegate::duplicate() {
    Application::boardController->duplicateItem(dynamic_cast<Item *>(delegated()));
}

void GraphicsItemDelegate::increaseZLevelUp() {
    GraphicsScene *curScene = owningScene();
    if (curScene) {
        curScene->changeZLevelTo(delegated(), ZLayerController::up, true);
    }
}
void GraphicsItemDelegate::increaseZlevelTop() {
    GraphicsScene *curScene = owningScene();
    if (curScene) {
        curScene->changeZLevelTo(delegated(), ZLayerController::top, true);
    }
}
void GraphicsItemDelegate::increaseZLevelDown() {
    GraphicsScene *curScene = owningScene();
    if (curScene) {
        curScene->changeZLevelTo(delegated(), ZLayerController::down, true);
    }
}
void GraphicsItemDelegate::increaseZlevelBottom() {
    GraphicsScene *curScene = owningScene();
    if (curScene) {
        curScene->changeZLevelTo(delegated(), ZLayerController::bottom, true);
    }
}

void GraphicsItemDelegate::lock(bool locked) {
    // 组合需要整组锁定，因此递归写入子项
    setLockedRecurs(locked, mDelegated);
    mDelegated->update();

    positionHandles();
    mFrame->positionHandles();
}


void GraphicsItemDelegate::setLockedRecurs(const QVariant &pLock, QGraphicsItem *pItem) {
    pItem->setData(GraphicsItemData::ItemLocked, pLock);
    foreach (QGraphicsItem *insideItem, pItem->childItems()) {
        setLockedRecurs(pLock, insideItem);
    }
}

void GraphicsItemDelegate::showHide(bool show) {
    // 通过图层类型表达“是否在扩展屏显示”：Object 显示，Control 只在控制屏显示
    QVariant showFlag = QVariant(show ? ItemLayerType::Object : ItemLayerType::Control);
    showHideRecurs(showFlag, mDelegated);
    mDelegated->update();

    emit showOnDisplayChanged(show);
}

void GraphicsItemDelegate::showHideRecurs(const QVariant &pShow, QGraphicsItem *pItem) {
    pItem->setData(GraphicsItemData::ItemLayerType, pShow);
    foreach (QGraphicsItem *insideItem, pItem->childItems()) {
        showHideRecurs(pShow, insideItem);
    }
}

// 把当前项铺满页面作为背景，替换已有背景
void GraphicsItemDelegate::setAsBackground() {
    GraphicsScene *scene = owningScene();
    QGraphicsItem *item = delegated();

    if (scene && item) {
        startUndoStep();

        // 背景以页面尺寸为准，重置变换并把内容中心对齐到原点
        item->resetTransform();
        item->setPos(item->sceneBoundingRect().width() / -2., item->sceneBoundingRect().height() / -2.);

        scene->setAsBackgroundObject(item, true);

        // 单独记录一次变换撤销步骤，使“设为背景”可回退
        GraphicsItemTransformUndoCommand *uc = new GraphicsItemTransformUndoCommand(
            mDelegated, mPreviousPosition, mPreviousTransform, mPreviousZValue, mPreviousSize, true);
        Application::undoStack->push(uc);
    }
}

void GraphicsItemDelegate::gotoContentSource() {
    Item *item = dynamic_cast<Item *>(mDelegated);

    if (item && !item->sourceUrl().isEmpty()) {
        Application::applicationController->openInternetUrl(item->sourceUrl());
    }
}

void GraphicsItemDelegate::startUndoStep() {
    // 只记录几何与层级快照；尺寸需项支持 ResizableGraphicsItem 才有意义
    mPreviousPosition = mDelegated->pos();
    mPreviousTransform = mDelegated->transform();
    mPreviousZValue = mDelegated->zValue();
    ResizableGraphicsItem *resizableItem = dynamic_cast<ResizableGraphicsItem *>(mDelegated);

    if (resizableItem)
        mPreviousSize = resizableItem->size();
    else
        mPreviousSize = QSizeF();
}


void GraphicsItemDelegate::commitUndoStep() {
    ResizableGraphicsItem *resizableItem = dynamic_cast<ResizableGraphicsItem *>(mDelegated);

    // 只有确实发生变化才压栈，避免点击也产生空的撤销记录
    if (mDelegated->pos() != mPreviousPosition || mDelegated->transform() != mPreviousTransform ||
        mDelegated->zValue() != mPreviousZValue ||
        (resizableItem && resizableItem->size() != mPreviousSize)) {
        GraphicsItemTransformUndoCommand *uc = new GraphicsItemTransformUndoCommand(
            mDelegated, mPreviousPosition, mPreviousTransform, mPreviousZValue, mPreviousSize);

        Application::undoStack->push(uc);
    }
}

void GraphicsItemDelegate::onZoomChanged() {
    // 缩放后重新计算反缩放系数并刷新框架，保证把手屏幕尺寸不变
    mAntiScaleRatio =
        1 / (Application::boardController->systemScaleFactor() * Application::boardController->currentZoom());

    positionHandles();
}

void GraphicsItemDelegate::buildButtons() {}

void GraphicsItemDelegate::freeButtons() {
    // 按钮已随框架一起删除，这里只需清空指针与列表，避免悬空
    mButtons.clear();
    mDeleteButton = 0;
    mMenuButton = 0;
    mZOrderUpButton = 0;
    mZOrderDownButton = 0;
}

void GraphicsItemDelegate::decorateMenu(QMenu *menu) {
    // 锁定与扩展屏显示是通用菜单项；背景/来源为可选能力，按数据与标志决定是否出现
    mLockAction = menu->addAction(tr("Locked"), this, &GraphicsItemDelegate::lock);
    QIcon lockIcon;
    lockIcon.addPixmap(QPixmap(":/images/locked.svg"), QIcon::Normal, QIcon::On);
    lockIcon.addPixmap(QPixmap(":/images/unlocked.svg"), QIcon::Normal, QIcon::Off);
    mLockAction->setIcon(lockIcon);
    mLockAction->setCheckable(true);

    mShowOnDisplayAction =
        mMenu->addAction(tr("Visible on Extended Screen"), this, &GraphicsItemDelegate::showHide);
    mShowOnDisplayAction->setCheckable(true);

    QIcon showIcon;
    showIcon.addPixmap(QPixmap(":/images/eyeOpened.svg"), QIcon::Normal, QIcon::On);
    showIcon.addPixmap(QPixmap(":/images/eyeClosed.svg"), QIcon::Normal, QIcon::Off);
    mShowOnDisplayAction->setIcon(showIcon);

    if (delegated()->data(GraphicsItemData::ItemCanBeSetAsBackground).toBool()) {
        mSetAsBackgroundAction =
            mMenu->addAction(tr("Set as background"), this, &GraphicsItemDelegate::setAsBackground);
        mSetAsBackgroundAction->setCheckable(false);

        QIcon backgroundIcon;
        backgroundIcon.addPixmap(QPixmap(":/images/setAsBackground.svg"), QIcon::Normal, QIcon::On);
        mSetAsBackgroundAction->setIcon(backgroundIcon);
    }

    if (testFrameFlags(GF_SHOW_CONTENT_SOURCE)) {
        mGotoContentSourceAction =
            menu->addAction(tr("Go to Content Source"), this, &GraphicsItemDelegate::gotoContentSource);

        QIcon sourceIcon;
        sourceIcon.addPixmap(QPixmap(":/images/toolbar/web.png"), QIcon::Normal, QIcon::On);
        mGotoContentSourceAction->setIcon(sourceIcon);
    }
}

void GraphicsItemDelegate::updateMenuActionState() {
    // 菜单每次弹出前同步勾选状态，避免显示过期信息
    if (mLockAction)
        mLockAction->setChecked(isLocked());

    if (mShowOnDisplayAction) {
        bool isControl = mDelegated->data(GraphicsItemData::ItemLayerType) == ItemLayerType::Control;
        mShowOnDisplayAction->setChecked(!isControl);
    }

    if (mGotoContentSourceAction) {
        Item *item = dynamic_cast<Item *>(mDelegated);
        mGotoContentSourceAction->setEnabled(item && !item->sourceUrl().isEmpty());
    }
}

void GraphicsItemDelegate::showMenu() {
    if (!mMenu) {
        mMenu = new QMenu(Application::boardController->controlView());
        decorateMenu(mMenu);
    }

    updateMenuActionState();

    // 菜单在控制视图上弹出，锚点为菜单按钮右下角
    BoardView *cv = Application::boardController->controlView();
    QRect pinPos = cv->mapFromScene(mMenuButton->sceneBoundingRect()).boundingRect();

    mMenu->exec(cv->mapToGlobal(pinPos.bottomRight()));
}

void GraphicsItemDelegate::setLocked(bool pLocked) {
    Q_ASSERT(mDelegated);

    if (mDelegated) {
        mDelegated->setData(GraphicsItemData::ItemLocked, QVariant(pLocked));
    }
}

void GraphicsItemDelegate::updateFrame() {
    // 框架可能因取消选中被移出场景，重新显示前要挂回
    if (mFrame && !mFrame->scene() && mDelegated->scene())
        mDelegated->scene()->addItem(mFrame);

    mFrame->setAntiScale(mAntiScaleRatio);
    mFrame->positionHandles();
}

void GraphicsItemDelegate::updateButtons(bool showUpdated) {
    QTransform tr;
    tr.scale(mAntiScaleRatio, mAntiScaleRatio);

    mDeleteButton->setParentItem(mFrame);
    mDeleteButton->setTransform(tr);

    qreal topX = mFrame->rect().left() - mDeleteButton->renderer()->viewBox().width() * mAntiScaleRatio / 2;
    qreal topY = mFrame->rect().top() - mDeleteButton->renderer()->viewBox().height() * mAntiScaleRatio / 2;

    qreal bottomX =
        mFrame->rect().left() - mDeleteButton->renderer()->viewBox().width() * mAntiScaleRatio / 2;
    qreal bottomY =
        mFrame->rect().bottom() - mDeleteButton->renderer()->viewBox().height() * mAntiScaleRatio / 2;

    mDeleteButton->setPos(topX, topY);

    if (!mDeleteButton->scene()) {
        if (mDelegated->scene())
            mDelegated->scene()->addItem(mDeleteButton);
    }

    if (showUpdated)
        mDeleteButton->show();

    // 按贴边区域分组依次排布：i 左上、j 左下、k 标题栏、l 无区域（仅计数）
    int i = 1, j = 0, k = 0, l = 0;
    int frameButtonHeight = mDeleteButton->boundingRect().size().height();
    qreal topXTitleBar = topX + (1.6 * mFrameWidth * mAntiScaleRatio);
    qreal topYTitleBar = topY + frameButtonHeight * mAntiScaleRatio;

    topYTitleBar += 5;

    while ((i + j + k + l) < mButtons.size()) {
        DelegateButton *button = mButtons[i + j + k + l];

        if (button->getSection() == Qt::TopLeftSection) {
            button->setParentItem(mFrame);
            button->setPos(topX + (i++ * 1.6 * mFrameWidth * mAntiScaleRatio), topY);
            button->setTransform(tr);
        } else if (button->getSection() == Qt::BottomLeftSection) {
            button->setParentItem(mFrame);
            button->setPos(bottomX + (++j * 1.6 * mFrameWidth * mAntiScaleRatio), bottomY);
            button->setTransform(tr);
        } else if (button->getSection() == Qt::TitleBarArea) {
            button->setParentItem(mFrame);
            button->setPos(topXTitleBar + (k++ * (frameButtonHeight + 5) * mAntiScaleRatio), topYTitleBar);
            button->setTransform(tr);
            // 标题栏按钮略小，避免与标题文字争空间
            button->setTransform(QTransform::fromScale(0.8, 0.8), true);
        } else if (button->getSection() == Qt::NoSection) {
            ++l;
        }
        if (!button->scene()) {
            if (mDelegated->scene())
                mDelegated->scene()->addItem(button);
        }
        if (showUpdated) {
            button->show();
            // 按钮与内容同层，避免被其他图形项盖住
            button->setZValue(delegated()->zValue());
        }
    }
}

void GraphicsItemDelegate::setButtonsVisible(bool visible) {
    foreach (DelegateButton *pButton, mButtons) {
        pButton->setVisible(visible);
    }
}

void GraphicsItemDelegate::setFrameFlags(GraphicsFlags pf) {
    mFlags = pf;

    Q_ASSERT(mDelegated);
    if (!mDelegated) {
        return;
    }

    // 把可翻转/可旋转能力写回图形项数据，序列化与其他逻辑都读取这两个字段
    if (testFrameFlags(GF_FLIPPABLE_ALL_AXIS)) {
        mDelegated->setData(GraphicsItemData::ItemFlippable, QVariant(testFrameFlags(GF_FLIPPABLE_ALL_AXIS)));
    }

    if (testFrameFlags(GF_REVOLVABLE)) {
        mDelegated->setData(GraphicsItemData::ItemRotatable, QVariant(testFrameFlags(GF_REVOLVABLE)));
    }
}

void GraphicsItemDelegate::setFrameFlag(GraphicsFlags pf, bool set) {
    GraphicsFlags fls = mFlags;
    set ? fls |= pf : fls &= ~pf;

    setFrameFlags(fls);
}

GraphicsToolBarItem::GraphicsToolBarItem(QGraphicsItem *parent)
    : QGraphicsRectItem(parent),
      mShifting(true),
      mVisible(false),
      mMinWidth(200),
      mInitialHeight(26),
      mElementsPadding(2) {
    // 初始宽度与父项一致，高度固定
    QRectF rect = this->rect();
    rect.setHeight(mInitialHeight);
    rect.setWidth(parent->boundingRect().width());
    this->setRect(rect);

    setPen(Qt::NoPen);
    hide();

    update();
}


void GraphicsToolBarItem::positionHandles() {
    // 元素从左侧依次水平排列，间距为 mElementsPadding
    int itemXOffset = 0;
    foreach (QGraphicsItem *item, mItemsOnToolBar) {
        item->setPos(itemXOffset, 0);
        itemXOffset += (item->boundingRect().width() + mElementsPadding);
        item->show();
    }
}

void GraphicsToolBarItem::update() {
    QGraphicsRectItem::update();
}

void GraphicsToolBarItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    // 圆角背景使用当前调色板颜色，与画板主题一致
    QPainterPath path;
    path.addRoundedRect(rect(), 10, 10);

    setBrush(QBrush(Settings::paletteColor));

    painter->fillPath(path, brush());
}

MediaTimer::MediaTimer(QGraphicsItem *parent) : QGraphicsRectItem(parent) {
    val = 0;
    smallPoint = false;
    setNumDigits(6);
}

MediaTimer::~MediaTimer() {}
void MediaTimer::positionHandles() {
    // 由高度推导段长与字距，再按位数把整体水平居中
    digitSpace = smallPoint ? 2 : 1;
    ySegLen = rect().height() * 5 / 12;
    xSegLen = ySegLen * 2 / 3;
    segLen = xSegLen;
    xAdvance = segLen * (5 + digitSpace) / 5;
    xOffset = (rect().width() - ndigits * xAdvance + segLen / 5) / 2;
    yOffset = rect().height() - ySegLen * 2;

    setRect(rect().x(), rect().y(), xOffset + xAdvance * ndigits, rect().height());
}

void MediaTimer::drawString(const QString &s, QPainter &p, QBitArray *newPoints, bool newString) {
    QPoint pos;

    // 逐位绘制；newString 为 true 时还会重绘上一位的差异（用于小数点变化）
    for (int i = 0; i < ndigits; i++) {
        pos = QPoint(xOffset + xAdvance * i, yOffset);
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

void MediaTimer::drawDigit(const QPoint &pos, QPainter &p, int segLen, char newCh, char oldCh) {
    char updates[18][2]; // 容量为段数的两倍；段表正确时实际只会用到前 9 个
    int nErases;
    int nUpdates;
    const char *segs;
    int i, j;

    const char erase = 0;
    const char draw = 1;
    const char leaveAlone = 2;

    // 新旧字符做差集：只擦除消失的段、只绘制新增的段，重叠段保持不动
    segs = getSegments(oldCh);
    for (nErases = 0; segs[nErases] != 99; nErases++) {
        updates[nErases][0] = erase;
        updates[nErases][1] = segs[nErases];
    }
    nUpdates = nErases;
    segs = getSegments(newCh);
    for (i = 0; segs[i] != 99; i++) {
        for (j = 0; j < nErases; j++)
            if (segs[i] == updates[j][1]) {
                updates[j][0] = leaveAlone;
                break;
            }
        if (j == nErases) {
            updates[nUpdates][0] = draw;
            updates[nUpdates][1] = segs[i];
            nUpdates++;
        }
    }
    for (i = 0; i < nUpdates; i++) {
        if (updates[i][0] == draw)
            drawSegment(pos, updates[i][1], p, segLen);
        if (updates[i][0] == erase)
            drawSegment(pos, updates[i][1], p, segLen, true);
    }
}

char MediaTimer::segments[][8] = {
    {0, 1, 2, 4, 5, 6, 99, 0}, // 0    0
    {2, 5, 99, 0, 0, 0, 0, 0}, // 1    1
    {0, 2, 3, 4, 6, 99, 0, 0}, // 2    2
    {0, 2, 3, 5, 6, 99, 0, 0}, // 3    3
    {1, 2, 3, 5, 99, 0, 0, 0}, // 4    4
    {0, 1, 3, 5, 6, 99, 0, 0}, // 5    5
    {0, 1, 3, 4, 5, 6, 99, 0}, // 6    6
    {0, 2, 5, 99, 0, 0, 0, 0}, // 7    7
    {0, 1, 2, 3, 4, 5, 6, 99}, // 8    8
    {0, 1, 2, 3, 5, 6, 99, 0}, // 9    9
    {8, 9, 99, 0, 0, 0, 0, 0}, // 10   :
    {99, 0, 0, 0, 0, 0, 0, 0}  // 11   empty
};

const char *MediaTimer::getSegments(char ch) // 返回字符 ch 对应的段号列表
{
    // 返回以 99 结尾的段号表；空格表示空位，冒号占用中间两个点
    if (ch >= '0' && ch <= '9')
        return segments[ch - '0'];
    if (ch == ':')
        return segments[10];
    if (ch == ' ')
        return segments[11];

    return NULL;
}

void MediaTimer::drawSegment(const QPoint &pos, char segmentNo, QPainter &p, int segLen, bool erase) {
    Q_UNUSED(erase);

    QPoint ppt;
    QPoint pt = pos;
    int width = segLen / 5;

#define LINETO(X, Y) addPoint(a, QPoint(pt.x() + (X), pt.y() + (Y)))
#define LIGHT
#define DARK

    QPolygon a;
    // 每个 case 用相对坐标拼出一个段的四边形；LIGHT/DARK 是空宏，仅用于标明起止笔
    switch (segmentNo) {
        case 0:
            ppt = pt;
            LIGHT;
            LINETO(segLen - 1, 0);
            DARK;
            LINETO(segLen - width - 1, width);
            LINETO(width, width);
            LINETO(0, 0);
            break;
        case 1:
            pt += QPoint(0, 1);
            ppt = pt;
            LIGHT;
            LINETO(width, width);
            DARK;
            LINETO(width, segLen - width / 2 - 2);
            LINETO(0, segLen - 2);
            LIGHT;
            LINETO(0, 0);
            break;
        case 2:
            pt += QPoint(segLen - 1, 1);
            ppt = pt;
            DARK;
            LINETO(0, segLen - 2);
            LINETO(-width, segLen - width / 2 - 2);
            LIGHT;
            LINETO(-width, width);
            LINETO(0, 0);
            break;
        case 3:
            pt += QPoint(0, segLen);
            ppt = pt;
            LIGHT;
            LINETO(width, -width / 2);
            LINETO(segLen - width - 1, -width / 2);
            LINETO(segLen - 1, 0);
            DARK;
            if (width & 1) { // 修正整数除法带来的误差
                LINETO(segLen - width - 3, width / 2 + 1);
                LINETO(width + 2, width / 2 + 1);
            } else {
                LINETO(segLen - width - 1, width / 2);
                LINETO(width, width / 2);
            }
            LINETO(0, 0);
            break;
        case 4:
            pt += QPoint(0, segLen + 1);
            ppt = pt;
            LIGHT;
            LINETO(width, width / 2);
            DARK;
            LINETO(width, segLen - width - 2);
            LINETO(0, segLen - 2);
            LIGHT;
            LINETO(0, 0);
            break;
        case 5:
            pt += QPoint(segLen - 1, segLen + 1);
            ppt = pt;
            DARK;
            LINETO(0, segLen - 2);
            LINETO(-width, segLen - width - 2);
            LIGHT;
            LINETO(-width, width / 2);
            LINETO(0, 0);
            break;
        case 6:
            pt += QPoint(0, segLen * 2);
            ppt = pt;
            LIGHT;
            LINETO(width, -width);
            LINETO(segLen - width - 1, -width);
            LINETO(segLen - 1, 0);
            DARK;
            LINETO(0, 0);
            break;
        case 7:
            pt += QPoint(segLen / 2, segLen * 2);
            ppt = pt;
            DARK;
            LINETO(width, 0);
            LINETO(width, -width);
            LIGHT;
            LINETO(0, -width);
            LINETO(0, 0);
            break;
        case 8:
            pt += QPoint(segLen / 2 - width / 2 + 1, segLen / 2 + width);
            ppt = pt;
            DARK;
            LINETO(width, 0);
            LINETO(width, -width);
            LIGHT;
            LINETO(0, -width);
            LINETO(0, 0);
            break;
        case 9:
            pt += QPoint(segLen / 2 - width / 2 + 1, 3 * segLen / 2 + width);
            ppt = pt;
            DARK;
            LINETO(width, 0);
            LINETO(width, -width);
            LIGHT;
            LINETO(0, -width);
            LINETO(0, 0);
            break;
        default:
            break;
    }
    // 段轮廓拼完后统一填充为白色
    p.setPen(Qt::white);
    p.setBrush(Qt::white);
    p.drawPolygon(a);
    p.setBrush(Qt::NoBrush);

    pt = pos;

#undef LINETO
#undef LIGHT
#undef DARK
}

void MediaTimer::addPoint(QPolygon &a, const QPoint &p) {
    uint n = a.size();
    a.resize(n + 1);
    a.setPoint(n, p);
}

void MediaTimer::paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (smallPoint)
        drawString(digitStr, *p, &points, false);
    else
        drawString(digitStr, *p, 0, false);
}

void MediaTimer::internalSetString(const QString &s) {
    QString buffer;
    int i;
    int len = s.length();
    QBitArray newPoints(ndigits);

    if (!smallPoint) {
        // 不显示小数点时直接右对齐截断
        if (len == ndigits)
            buffer = s;
        else
            buffer = s.right(ndigits).rightJustified(ndigits, QLatin1Char(' '));
    } else {
        // 小数点单独占一位，连续两个小数点时补一个空格占位
        int index = -1;
        bool lastWasPoint = true;
        newPoints.clearBit(0);
        for (i = 0; i < len; i++) {
            if (s[i] == QLatin1Char('.')) {
                if (lastWasPoint) {
                    if (index == ndigits - 1)
                        break;
                    index++;
                    buffer[index] = QLatin1Char(' ');
                }
                newPoints.setBit(index);
                lastWasPoint = true;
            } else {
                if (index == ndigits - 1)
                    break;
                index++;
                buffer[index] = s[i];
                newPoints.clearBit(index);
                lastWasPoint = false;
            }
        }
        // 内容不足位数时整体右移，前面补空格
        if (index < ((int)ndigits) - 1) {
            for (i = index; i >= 0; i--) {
                buffer[ndigits - 1 - index + i] = buffer[i];
                newPoints.setBit(ndigits - 1 - index + i, newPoints.testBit(i));
            }
            for (i = 0; i < ndigits - index - 1; i++) {
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

void MediaTimer::display(const QString &s) {
    // val 仅保存数值形式，绘制仍使用字符串，以便兼容冒号与空格
    val = 0;
    bool ok = false;
    double v = s.toDouble(&ok);
    if (ok)
        val = v;
    internalSetString(s);
}

void MediaTimer::setNumDigits(int numDigits) {
    if (numDigits > 99) {
        qWarning("QLCDNumber::setNumDigits: Max 99 digits allowed");
        numDigits = 99;
    }
    if (numDigits < 0) {
        qWarning("QLCDNumber::setNumDigits: Min 0 digits allowed");
        numDigits = 0;
    }
    if (digitStr.isNull()) { // 仅在构造阶段成立
        // 构造阶段：按 1.5 倍位数预留空间（含分隔符位置）
        ndigits = numDigits + numDigits / 2 - 1;
        digitStr.fill(QLatin1Char(' '), ndigits);
        points.fill(0, ndigits);
        digitStr[ndigits - 1] = QLatin1Char('0'); // 默认显示 0
    } else {
        if (numDigits == ndigits) // 位数未变化
            return;
        int i;
        int dif;
        if (numDigits > ndigits) { // 扩展：内容右移，左侧补空
            dif = numDigits - ndigits;
            QString buf;
            buf.fill(QLatin1Char(' '), dif);
            digitStr.insert(0, buf);
            points.resize(numDigits);
            for (i = numDigits - 1; i >= dif; i--)
                points.setBit(i, points.testBit(i - dif));
            for (i = 0; i < dif; i++)
                points.clearBit(i);
        } else { // 收缩：保留右侧低位
            dif = ndigits - numDigits;
            digitStr = digitStr.right(numDigits);
            QBitArray tmpPoints = points;
            points.resize(numDigits);
            for (i = 0; i < (int)numDigits; i++)
                points.setBit(i, tmpPoints.testBit(i + dif));
        }
        ndigits = numDigits;
        update();
    }
    positionHandles();
}

DelegateMediaControl::DelegateMediaControl(GraphicsMediaItem *pDelegated, QGraphicsItem *parent)
    : QGraphicsRectItem(parent),
      mDelegate(pDelegated),
      mDisplayCurrentTime(false),
      mCurrentTimeInMs(0),
      mTotalTimeInMs(0),
      mStartWidth(200),
      mSeecAreaBorderHeight(0) {
    setAcceptedMouseButtons(Qt::LeftButton);
    setBrush(QBrush(Qt::white));
    setPen(Qt::NoPen);
    setData(GraphicsItemData::ItemLayerType, QVariant(ItemLayerType::Control));

    // 数字计时器是控制条的子项，随控制条一起销毁
    lcdTimer = new MediaTimer(this);

    update();
}


void DelegateMediaControl::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QPainterPath path;

    // 进度槽为圆角胶囊，按当前时间比例计算滑块位置
    path.addRoundedRect(mSeecArea, mSeecArea.height() / 2, mSeecArea.height() / 2);
    painter->fillPath(path, brush());

    qreal frameWidth = mSeecArea.height() / 2;
    int position = frameWidth;

    if (mTotalTimeInMs > 0) {
        position = frameWidth + ((mSeecArea.width() - (2 * frameWidth)) / mTotalTimeInMs) * mCurrentTimeInMs;
    }

    int clearance = 2;
    int radius = frameWidth - clearance;
    QRectF r(position - radius, clearance + mSeecAreaBorderHeight, radius * 2, radius * 2);

    painter->setBrush(Settings::documentViewLightColor);
    painter->setPen(Qt::black);
    painter->drawEllipse(r);
}


QPainterPath DelegateMediaControl::shape() const {
    // 命中区域只覆盖进度条，避免遮挡上方媒体画面
    QPainterPath path;
    path.addRoundedRect(mSeecArea, mSeecArea.height() / 2, mSeecArea.height() / 2);
    return path;
}

void DelegateMediaControl::positionHandles() {
    QTime tTotal;
    tTotal = tTotal.addMSecs(mTotalTimeInMs);
    mLCDTimerArea.setHeight(parentItem()->boundingRect().height());
    int digitsCount = 2;
    int timerWidth = mLCDTimerArea.height();

    mDisplayFormat = "ss";


    // 至少显示分与秒；超过一小时再补上小时位
    mDisplayFormat = "mm:" + mDisplayFormat;
    digitsCount += 3;
    timerWidth += mLCDTimerArea.height();

    if (tTotal.hour() > 0) {
        mDisplayFormat = "hh:" + mDisplayFormat;
        digitsCount += 3;
        timerWidth += mLCDTimerArea.height();
    }

    lcdTimer->setNumDigits(digitsCount);

    mLCDTimerArea.setWidth(timerWidth);
    lcdTimer->setRect(mLCDTimerArea);

    // 计时器贴右侧，进度槽占据剩余宽度
    lcdTimer->setPos(rect().width() - mLCDTimerArea.width(), 0);

    mSeecAreaBorderHeight = rect().height() / 20;
    mSeecArea.setWidth(rect().width() - mLCDTimerArea.width() - 2);
    mSeecArea.setHeight(rect().height() - 2 * mSeecAreaBorderHeight);
    mSeecArea.setY(mSeecAreaBorderHeight);
}

void DelegateMediaControl::update() {
    QTime tCurrent;
    tCurrent = tCurrent.addMSecs(mCurrentTimeInMs < 0 ? 0 : mCurrentTimeInMs);


    lcdTimer->display(tCurrent.toString(mDisplayFormat));

    QGraphicsRectItem::update();
}

void DelegateMediaControl::updateTicker(qint64 time) {
    mCurrentTimeInMs = time;
    update();
}


void DelegateMediaControl::totalTimeChanged(qint64 newTotalTime) {
    // 总时长变化可能改变显示格式（是否出现小时位），需要重新布局
    if (mTotalTimeInMs != newTotalTime) {
        mTotalTimeInMs = newTotalTime;
        positionHandles();
        update();
    }
}


void DelegateMediaControl::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    // 用左右各偏移半个胶囊高度的方式排除圆角外侧的点击
    qreal frameWidth = mSeecArea.height() / 2;
    if (boundingRect().contains(event->pos() - QPointF(frameWidth, 0)) &&
        boundingRect().contains(event->pos() + QPointF(frameWidth, 0))) {
        mDisplayCurrentTime = true;
        seekToMousePos(event->pos());
        this->update();
        event->accept();
        emit used();
    }
}

void DelegateMediaControl::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    qreal frameWidth = mSeecArea.height() / 2;
    if (boundingRect().contains(event->pos() - QPointF(frameWidth, 0)) &&
        boundingRect().contains(event->pos() + QPointF(frameWidth, 0))) {
        seekToMousePos(event->pos());
        this->update();
        event->accept();
        emit used();
    }
}

void DelegateMediaControl::seekToMousePos(QPointF mousePos) {
    qreal minX, length;
    qreal frameWidth = rect().height() / 2;

    minX = frameWidth;
    length = mSeecArea.width() - mSeecArea.height();

    // 把鼠标位置夹在可拖动范围内，再按比例换算成播放时间
    qreal mouseX = mousePos.x();
    if (mouseX >= (mSeecArea.width() - mSeecArea.height() / 2))
        mouseX = mSeecArea.width() - mSeecArea.height() / 2;

    if (mTotalTimeInMs > 0 && length > 0 && mDelegate && mDelegate->isMediaSeekable()) {
        qint64 tickPos = (mTotalTimeInMs / length) * (mouseX - minX);
        mDelegate->setMediaPos(tickPos);

        // 立即刷新数字，部分平台上播放器回调有延迟
        updateTicker(tickPos);
    }
    emit used();
}

void DelegateMediaControl::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    mDisplayCurrentTime = false;
    this->update();
    event->accept();
}
