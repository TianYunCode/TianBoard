#pragma once

#include "whiteboard/GraphicsItemDelegate.h"

class GraphicsGroupContainerItem;

class GraphicsGroupContainerItemDelegate : public GraphicsItemDelegate {
    Q_OBJECT

public:
    GraphicsGroupContainerItemDelegate(QGraphicsItem *pDelegated, QObject *parent = 0);
    GraphicsGroupContainerItem *delegated();

protected:
    virtual void decorateMenu(QMenu *menu);
    virtual void buildButtons();
    virtual void freeButtons();

    virtual bool mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual bool mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual bool mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private slots:
    void destroyGroup();

private:
    DelegateButton *mDestroyGroupButton;
};
