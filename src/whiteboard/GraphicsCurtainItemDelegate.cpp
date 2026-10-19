#include <QtWidgets>
#include <QtSvg>
#include <QtSvgWidgets>

#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsDelegateFrame.h"
#include "whiteboard/GraphicsCurtainItemDelegate.h"
#include "whiteboard/GraphicsCurtainItem.h"


GraphicsCurtainItemDelegate::GraphicsCurtainItemDelegate(GraphicsCurtainItem* pDelegated, QObject * parent)
    : GraphicsItemDelegate(pDelegated, parent, GF_MENU_SPECIFIED | GF_DUPLICATION_ENABLED)
{
    //NOOP
}


GraphicsCurtainItemDelegate::~GraphicsCurtainItemDelegate()
{
    //NOOP
}

void GraphicsCurtainItemDelegate::init()
{
    if(!mFrame){
        createControls();
        mFrame->hide();
        mZOrderUpButton->hide();
        mZOrderDownButton->hide();
    }
}


bool GraphicsCurtainItemDelegate::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);

    startUndoStep();

    //if (!mDelegated->isSelected())
    //{
    //    mDelegated->setSelected(true);
    //    positionHandles();

    //    return true;
    //}
    //else
    {
        return false;
    }

}


QVariant GraphicsCurtainItemDelegate::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
    if (change == QGraphicsItem::ItemVisibleHasChanged)
    {
        //GraphicsScene* ownerScene = qobject_cast<GraphicsScene*>(mDelegated->scene());
        //if(ownerScene)
        //    ownerScene->setModified(true);
    }

    return GraphicsItemDelegate::itemChange(change, value);
}

void GraphicsCurtainItemDelegate::positionHandles()
{
    GraphicsItemDelegate::positionHandles();
    if(mZOrderUpButton)
        mZOrderUpButton->hide();
    if(mZOrderDownButton)
        mZOrderDownButton->hide();
}

void GraphicsCurtainItemDelegate::remove(bool checked, bool canUndo)
{
    Q_UNUSED(checked);
    Q_UNUSED(canUndo);

    GraphicsCurtainItem *curtain = dynamic_cast<GraphicsCurtainItem*>(mDelegated);

    if (curtain)
    {
        curtain->setVisible(false);
        curtain->triggerRemovedSignal();
    }
}

