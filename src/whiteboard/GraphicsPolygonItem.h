#pragma once

#include <QtWidgets>

#include "utils/Enums.h"
#include "whiteboard/Item.h"
#include "whiteboard/GraphicsStrokesGroup.h"
#include "whiteboard/GraphicsGroupContainerItem.h"

class Item;
class GraphicsScene;
class GraphicsStroke;

class GraphicsPolygonItem : public QGraphicsPolygonItem, public Item {
public:
    GraphicsPolygonItem(QGraphicsItem *parent = 0);
    GraphicsPolygonItem(const QLineF &line, qreal pWidth);
    GraphicsPolygonItem(const QLineF &pLine, qreal pStartWidth, qreal pEndWidth);
    GraphicsPolygonItem(const QPolygonF &polygon, QGraphicsItem *parent = 0);

    ~GraphicsPolygonItem();

    void initialize();

    void setUuid(const QUuid &pUuid);

    void setStrokesGroup(GraphicsStrokesGroup *group);
    GraphicsStrokesGroup *strokesGroup() const { return mpGroup; }
    void setColor(const QColor &color);

    QColor color() const;

    virtual GraphicsScene *scene();

    inline void subtract(GraphicsPolygonItem *pi) {
        if (boundingRect().intersects(pi->boundingRect())) {
            QPolygonF subtractedPolygon = polygon().subtracted(pi->polygon());

            if (polygon() != subtractedPolygon) {
                mIsNominalLine = false;
                QGraphicsPolygonItem::setPolygon(subtractedPolygon);
            }
        }
    }

    inline void subtractIntersecting(GraphicsPolygonItem *pi) {
        QPolygonF subtractedPolygon = polygon().subtracted(pi->polygon());

        if (polygon() != subtractedPolygon) {
            mIsNominalLine = false;
            QGraphicsPolygonItem::setPolygon(subtractedPolygon);
        }
    }

    enum { Type = GraphicsItemType::PolygonItemType };

    virtual int type() const { return Type; }

    void setPolygon(const QPolygonF pPolygon) {
        mIsNominalLine = false;
        QGraphicsPolygonItem::setPolygon(pPolygon);
    }

    virtual Item *deepCopy() const;

    virtual void copyItemParameters(Item *copy) const;

    QLineF originalLine() { return mOriginalLine; }
    qreal originalWidth() { return mOriginalWidth; }
    bool isNominalLine() { return mIsNominalLine; }

    void setNominalLine(bool isNominalLine) { mIsNominalLine = isNominalLine; }

    QColor colorOnDarkBackground() const { return mColorOnDarkBackground; }

    void setColorOnDarkBackground(QColor pColorOnDarkBackground) {
        mColorOnDarkBackground = pColorOnDarkBackground;
    }

    QColor colorOnLightBackground() const { return mColorOnLightBackground; }

    void setColorOnLightBackground(QColor pColorOnLightBackground) {
        mColorOnLightBackground = pColorOnLightBackground;
    }

    void setStroke(GraphicsStroke *stroke);
    GraphicsStroke *stroke() const;

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
    void clearStroke();

    bool mHasAlpha;

    QLineF mOriginalLine;
    qreal mOriginalWidth;
    bool mIsNominalLine;

    QColor mColorOnDarkBackground;
    QColor mColorOnLightBackground;

    GraphicsStroke *mStroke;
    GraphicsStrokesGroup *mpGroup;
};
