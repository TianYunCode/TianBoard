#include "whiteboard/GraphicsGroupContainerItemDelegate.h"

#include <QtWidgets>

#include "whiteboard/GraphicsScene.h"

#include "whiteboard/GraphicsDelegateFrame.h"
#include "whiteboard/GraphicsGroupContainerItem.h"

#include "whiteboard/BoardController.h"

GraphicsGroupContainerItemDelegate::GraphicsGroupContainerItemDelegate(QGraphicsItem *pDelegated, QObject *parent)
    : GraphicsItemDelegate(pDelegated, parent, GF_COMMON | GF_RESPECT_RATIO), mDestroyGroupButton(0)
{
}

GraphicsGroupContainerItem *GraphicsGroupContainerItemDelegate::delegated()
{
    return dynamic_cast<GraphicsGroupContainerItem *>(mDelegated);
}

void GraphicsGroupContainerItemDelegate::decorateMenu(QMenu *menu)
{
    // 组合作为整体只能锁定或整组显示/隐藏，因此菜单项挂在组级委托上
    mLockAction = menu->addAction(tr("Locked"), this, &GraphicsGroupContainerItemDelegate::lock);
    QIcon lockIcon;
    lockIcon.addPixmap(QPixmap(":/images/locked.svg"), QIcon::Normal, QIcon::On);
    lockIcon.addPixmap(QPixmap(":/images/unlocked.svg"), QIcon::Normal, QIcon::Off);
    mLockAction->setIcon(lockIcon);
    mLockAction->setCheckable(true);

    mShowOnDisplayAction = mMenu->addAction(tr("Visible on Extended Screen"), this, &GraphicsGroupContainerItemDelegate::showHide);
    mShowOnDisplayAction->setCheckable(true);

    QIcon showIcon;
    showIcon.addPixmap(QPixmap(":/images/eyeOpened.svg"), QIcon::Normal, QIcon::On);
    showIcon.addPixmap(QPixmap(":/images/eyeClosed.svg"), QIcon::Normal, QIcon::Off);
    mShowOnDisplayAction->setIcon(showIcon);
}

void GraphicsGroupContainerItemDelegate::buildButtons()
{
    // 解散按钮放在左上角，点击后组内子项全部释放
    if (!mDestroyGroupButton)
    {
        mDestroyGroupButton = new DelegateButton(":/images/ungroupItems.svg", mDelegated, mFrame, Qt::TopLeftSection);
        mDestroyGroupButton->setShowProgressIndicator(false);
        connect(mDestroyGroupButton, &DelegateButton::clicked, this, &GraphicsGroupContainerItemDelegate::destroyGroup);
        mButtons << mDestroyGroupButton;
    }

    GraphicsItemDelegate::buildButtons();
}

void GraphicsGroupContainerItemDelegate::freeButtons()
{
    GraphicsItemDelegate::freeButtons();
    mDestroyGroupButton = 0;
    mButtons.clear();
}

bool GraphicsGroupContainerItemDelegate::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event)
    // 组合被点击时先收起正在交互的媒体子项工具栏
    delegated()->deselectCurrentItem();
    return false;
}

bool GraphicsGroupContainerItemDelegate::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event)

    return false;
}

bool GraphicsGroupContainerItemDelegate::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event)

    return false;
}

void GraphicsGroupContainerItemDelegate::destroyGroup()
{
    qDebug() << "Destroying group";
    delegated()->destroy();
}
