#include <QtWidgets>
#include <QtSvg>
#include <QtSvgWidgets>

#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsDelegateFrame.h"
#include "whiteboard/GraphicsCurtainItemDelegate.h"
#include "whiteboard/GraphicsCurtainItem.h"


GraphicsCurtainItemDelegate::GraphicsCurtainItemDelegate(GraphicsCurtainItem *pDelegated, QObject *parent)
    : GraphicsItemDelegate(pDelegated, parent, GF_MENU_SPECIFIED | GF_DUPLICATION_ENABLED) {}

GraphicsCurtainItemDelegate::~GraphicsCurtainItemDelegate() {}

void GraphicsCurtainItemDelegate::init() {
    if (!mFrame) {
        createControls();
        // 幕布只有被选中时才应显示选择框，初始化阶段先隐藏
        mFrame->hide();
        // 幕布是控制类对象，不参与普通对象的层级上下移动
        mZOrderUpButton->hide();
        mZOrderDownButton->hide();
    }
}


bool GraphicsCurtainItemDelegate::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    Q_UNUSED(event);

    // 记录按下前的状态，便于拖动结束后整体撤销
    startUndoStep();

    return false;
}


QVariant GraphicsCurtainItemDelegate::itemChange(QGraphicsItem::GraphicsItemChange change,
                                                 const QVariant &value) {
    if (change == QGraphicsItem::ItemVisibleHasChanged) {
        // 幕布的显示/隐藏不标记文档为已修改，避免仅切换幕布就触发保存提示
    }

    return GraphicsItemDelegate::itemChange(change, value);
}

void GraphicsCurtainItemDelegate::positionHandles() {
    GraphicsItemDelegate::positionHandles();
    // 基类摆放按钮时会重新设置可见性，这里再次强制隐藏
    if (mZOrderUpButton)
        mZOrderUpButton->hide();
    if (mZOrderDownButton)
        mZOrderDownButton->hide();
}

void GraphicsCurtainItemDelegate::remove(bool checked, bool canUndo) {
    Q_UNUSED(checked);
    Q_UNUSED(canUndo);

    GraphicsCurtainItem *curtain = dynamic_cast<GraphicsCurtainItem *>(mDelegated);

    if (curtain) {
        // 幕布不销毁，仅隐藏并通知场景，之后仍可重新显示
        curtain->setVisible(false);
        curtain->triggerRemovedSignal();
    }
}
