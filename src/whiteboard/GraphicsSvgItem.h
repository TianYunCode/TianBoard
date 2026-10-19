#pragma once

#include <QtWidgets>
#include <QtSvg>
#include <QtSvgWidgets>

#include "whiteboard/Item.h"

#include "utils/Enums.h"

class GraphicsItemDelegate;
class GraphicsPixmapItem;

class GraphicsSvgItem: public QGraphicsSvgItem, public Item, public GraphicsItem
{
    public:
        GraphicsSvgItem(const QString& pFile, QGraphicsItem* parent = 0);
        GraphicsSvgItem(const QByteArray& pFileData, QGraphicsItem* parent = 0);

        void init();

        virtual ~GraphicsSvgItem();

        QByteArray fileData() const;

        void setFileData(const QByteArray& pFileData)
        {
            mFileData = pFileData;
        }

        enum { Type = GraphicsItemType::SvgItemType };

        virtual int type() const
        {
            return Type;
        }

        virtual Item* deepCopy() const;

        virtual void copyItemParameters(Item *copy) const;

        virtual void setRenderingQuality(RenderingQuality pRenderingQuality);

        virtual GraphicsScene* scene();

        virtual GraphicsPixmapItem* toPixmapItem() const;

        virtual void setUuid(const QUuid &pUuid);

        virtual void clearSource();

    protected:
        virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
        virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
        virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

        virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

        virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

        QByteArray mFileData;
};

