#pragma once

#include <QtWidgets>

#include <QtSvg>
#include <QtSvgWidgets>

#include "utils/Enums.h"
#include "whiteboard/GraphicsItemDelegate.h"

class QGraphicsSceneMouseEvent;
class QGraphicsItem;
class GraphicsCurtainItem;

class GraphicsCurtainItemDelegate : public GraphicsItemDelegate
{
    Q_OBJECT

public:
    GraphicsCurtainItemDelegate(GraphicsCurtainItem* pDelegated, QObject * parent = 0);
    virtual ~GraphicsCurtainItemDelegate();

    virtual bool mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value);
    virtual void positionHandles();

    virtual void init();

public slots:
    virtual void remove(bool checked, bool canUndo = true);

};

