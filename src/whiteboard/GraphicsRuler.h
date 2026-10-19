#pragma once

#include <QtWidgets>
#include <QtSvg>
#include <QtSvgWidgets>

#include "utils/Enums.h"
#include "whiteboard/Item.h"
#include "whiteboard/AbstractDrawRuler.h"

class GraphicsScene;

class GraphicsRuler : public AbstractDrawRuler, public QGraphicsRectItem, public Item {
    Q_OBJECT

public:
    GraphicsRuler();
    virtual ~GraphicsRuler();

    enum { Type = GraphicsItemType::RulerItemType };

    virtual int type() const { return Type; }

    virtual Item *deepCopy() const;
    virtual void copyItemParameters(Item *copy) const;

    virtual void StartLine(const QPointF &position, qreal width);
    virtual void DrawLine(const QPointF &position, qreal width);
    virtual void EndLine();

protected:
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *styleOption, QWidget *widget);
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    // Events
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
    void paintGraduations(QPainter *painter);

private:
    bool mResizing;
    bool mRotating;

    void fillBackground(QPainter *painter);
    void paintRotationCenter(QPainter *painter);
    virtual void rotateAroundCenter(qreal angle);

    QGraphicsSvgItem *mRotateSvgItem;
    QGraphicsSvgItem *mResizeSvgItem;

    void updateResizeCursor();
    QCursor resizeCursor() const { return mResizeCursor; }

    virtual QPointF rotationCenter() const;
    virtual QRectF resizeButtonRect() const;
    virtual QRectF closeButtonRect() const;
    virtual QRectF rotateButtonRect() const;
    virtual GraphicsScene *scene() const;

    QCursor mResizeCursor;

    int drawLineDirection;

    // Constants
    static const QRect sDefaultRect;

    static const int sMinLength = 150;   // 3sm
    static const int sMaxLength = 35000; // 700sm

    qreal mStrokeWidth;
};
