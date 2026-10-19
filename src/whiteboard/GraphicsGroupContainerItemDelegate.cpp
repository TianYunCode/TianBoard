#include "whiteboard/GraphicsGroupContainerItemDelegate.h"

#include <QtWidgets>

#include "whiteboard/GraphicsScene.h"
#include "ui/Resources.h"

#include "whiteboard/GraphicsDelegateFrame.h"
#include "whiteboard/GraphicsGroupContainerItem.h"

#include "whiteboard/BoardController.h"


GraphicsGroupContainerItemDelegate::GraphicsGroupContainerItemDelegate(QGraphicsItem *pDelegated, QObject *parent) :
    GraphicsItemDelegate(pDelegated, parent, GF_COMMON | GF_RESPECT_RATIO), mDestroyGroupButton(0)
{
}

GraphicsGroupContainerItem *GraphicsGroupContainerItemDelegate::delegated()
{
    return dynamic_cast<GraphicsGroupContainerItem*>(mDelegated);
}

void GraphicsGroupContainerItemDelegate::decorateMenu(QMenu *menu)
{
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
    if (!mDestroyGroupButton) {
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
