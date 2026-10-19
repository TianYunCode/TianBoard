#include <QtWidgets>
#include <QtSvg>
#include <QtSvgWidgets>

#include "whiteboard/GraphicsWidgetItemDelegate.h"
#include "whiteboard/GraphicsScene.h"

#include "core/Application.h"
#include "ui/MainWindow.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/BoardView.h"

#include "whiteboard/GraphicsWidgetItem.h"
#include "whiteboard/GraphicsDelegateFrame.h"


GraphicsWidgetItemDelegate::GraphicsWidgetItemDelegate(GraphicsWidgetItem* pDelegated, int widgetType)
    : GraphicsItemDelegate(pDelegated, 0, GF_COMMON
                             | GF_RESPECT_RATIO)
    , freezeAction(0)
    , setAsToolAction(0)
{
    mWidgetType = widgetType;
}


GraphicsWidgetItemDelegate::~GraphicsWidgetItemDelegate()
{
    // NOOP
}


void GraphicsWidgetItemDelegate::pin()
{
    Application::boardController->moveGraphicsWidgetToControlView(delegated());
}


void GraphicsWidgetItemDelegate::updateMenuActionState()
{
    GraphicsItemDelegate::updateMenuActionState();

    if (freezeAction)
        freezeAction->setChecked(delegated()->isFrozen());
}

void GraphicsWidgetItemDelegate::decorateMenu(QMenu* menu)
{
    GraphicsItemDelegate::decorateMenu(menu);

    freezeAction = menu->addAction(tr("Frozen"), this, SLOT(freeze(bool)));

    QIcon freezeIcon;
    freezeIcon.addPixmap(QPixmap(":/images/frozen.svg"), QIcon::Normal, QIcon::On);
    freezeIcon.addPixmap(QPixmap(":/images/unfrozen.svg"), QIcon::Normal, QIcon::Off);
    freezeAction->setIcon(freezeIcon);

    freezeAction->setCheckable(true);

    if (delegated()->canBeTool())
    {
        setAsToolAction = mMenu->addAction(tr("Transform as Tool "), this, SLOT(pin()));
        QIcon pinIcon;
        pinIcon.addPixmap(QPixmap(":/images/unpin.svg"), QIcon::Normal, QIcon::On);
        pinIcon.addPixmap(QPixmap(":/images/pin.svg"), QIcon::Normal, QIcon::Off);
        setAsToolAction->setIcon(pinIcon);
    }
}


void GraphicsWidgetItemDelegate::freeze(bool frozen)
{
    if(frozen)
    {
       delegated()->freeze();
    }
    else
    {
       delegated()->unFreeze();
    }
}


GraphicsWidgetItem* GraphicsWidgetItemDelegate::delegated()
{
    return static_cast<GraphicsWidgetItem*>(mDelegated);
}


void GraphicsWidgetItemDelegate::remove(bool canundo)
{
    delegated()->removeScript();
    GraphicsItemDelegate::remove(canundo);
}
