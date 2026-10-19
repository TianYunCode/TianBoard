#pragma once

#include <QGraphicsRectItem>
#include <QtWidgets>
#include "utils/Enums.h"

#include "whiteboard/GraphicsScene.h"

class DelegateButton;
class GraphicsItemDelegate;

class SelectionFrame : public QObject, public QGraphicsRectItem {
    Q_OBJECT

public:
    enum { om_idle, om_moving, om_rotating } mOperationMode;
    enum { Type = GraphicsItemType::SelectionFrameType };

    SelectionFrame();

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    QRectF boundingRect() const;
    QPainterPath shape() const;

    void setLocalBrush(const QBrush &pBrush) { mLocalBrush = pBrush; }
    QBrush localBrush() const { return mLocalBrush; }
    //    void setEnclosedItems(const QList<GraphicsItemDelegate*> pEncItems) {mEnclosedtems = pEncItems; updateRect();}
    void setEnclosedItems(const QList<QGraphicsItem *> pGraphicsItems);
    void updateRect();
    void updateScale();
    bool isEmpty() const { return this->rect().isEmpty(); }
    virtual int type() const { return Type; }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private slots:
    void setAntiScaleRatio(qreal pAntiscaleRatio) { mAntiscaleRatio = pAntiscaleRatio; }
    void onZoomChanged(qreal pZoom);
    void remove();
    void duplicate();
    void increaseZlevelUp();
    void increaseZlevelTop();
    void increaseZlevelDown();
    void increaseZlevelBottom();
    void groupItems();

private:
    void addSelectionUndo(QList<QGraphicsItem *> items, ZLayerController::moveDestination dest);
    void placeButtons();
    void placeExceptionButton(DelegateButton *pButton, QTransform pTransform);
    void clearButtons();
    inline int adjThickness() const { return mThickness * mAntiscaleRatio; }
    inline GraphicsScene *ubscene();
    void setCursorFromAngle(QString angle);

    QList<QGraphicsItem *> sortedByZ(const QList<QGraphicsItem *> &pItems);
    QList<DelegateButton *> buttonsForFlags(GraphicsFlags fls);

    QList<QGraphicsItem *> enclosedGraphicsItems();

private:
    int mThickness;
    qreal mAntiscaleRatio;
    QList<GraphicsItemDelegate *> mEnclosedtems;
    QBrush mLocalBrush;

    QPointF mPressedPos;
    QPointF mLastMovedPos;
    QPointF mLastTranslateOffset;
    qreal mRotationAngle;

    bool mIsLocked;

    QList<DelegateButton *> mButtons;

    DelegateButton *mDeleteButton;
    DelegateButton *mDuplicateButton;
    DelegateButton *mZOrderUpButton;
    DelegateButton *mZOrderDownButton;
    DelegateButton *mGroupButton;

    DelegateButton *mRotateButton;
};
