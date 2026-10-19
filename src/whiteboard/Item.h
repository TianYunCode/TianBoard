#pragma once

#include <QtWidgets>
#include "utils/Enums.h"
#include "whiteboard/GraphicsItemDelegate.h"

class GraphicsScene;
class GraphicsItem;

class Item
{
    protected:
        Item();

    public:
        virtual ~Item();

        enum RenderingQuality
        {
            RenderingQualityNormal = 0, RenderingQualityHigh
        };

        virtual QUuid uuid() const
        {
            return mUuid;
        }

        virtual void setUuid(const QUuid& pUuid)
        {
			mUuid = pUuid;
        }

        virtual RenderingQuality renderingQuality() const
        {
            return mRenderingQuality;
        }

        virtual void setRenderingQuality(RenderingQuality pRenderingQuality)
        {
            mRenderingQuality = pRenderingQuality;
        }

        virtual Item* deepCopy() const = 0;

        virtual void copyItemParameters(Item *copy) const = 0;

        virtual GraphicsScene* scene() 
        {
            return 0;
        }

        virtual QUrl sourceUrl() const
        {
            return mSourceUrl;
        }

        virtual void setSourceUrl(const QUrl& pSourceUrl)
        {
            mSourceUrl = pSourceUrl;
        }

    protected:
        QUuid mUuid;

        RenderingQuality mRenderingQuality;

        QUrl mSourceUrl;
};

class GraphicsItem
{
protected:
    GraphicsItem() : mDelegate(NULL)
    {

    }
    virtual ~GraphicsItem();
    void setDelegate(GraphicsItemDelegate* mDelegate);

public:
    virtual int type() const = 0;

    GraphicsItemDelegate *Delegate() const;

    static void assignZValue(QGraphicsItem*, qreal value);
    static bool isRotatable(QGraphicsItem *item);
    static bool isFlippable(QGraphicsItem *item);
    static bool isLocked(QGraphicsItem *item);
    static QUuid getOwnUuid(QGraphicsItem *item);

    static GraphicsItemDelegate *Delegate(QGraphicsItem *pItem);

    void remove(bool canUndo = true);

    virtual void clearSource(){}

private:
    GraphicsItemDelegate* mDelegate;
};

