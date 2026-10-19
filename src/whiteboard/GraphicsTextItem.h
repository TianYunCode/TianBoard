#pragma once

#include <QtWidgets>
#include "whiteboard/Item.h"
#include "utils/Enums.h"
#include "whiteboard/ResizableGraphicsItem.h"

class GraphicsItemDelegate;
class GraphicsScene;

class GraphicsTextItem : public QGraphicsTextItem, public Item, public ResizableGraphicsItem, public GraphicsItem
{
    Q_OBJECT

    public:
        GraphicsTextItem(QGraphicsItem * parent = 0);
        virtual ~GraphicsTextItem();

        enum { Type = GraphicsItemType::TextItemType };

        virtual int type() const
        {
            return Type;
        }

        virtual Item* deepCopy() const;

        virtual void copyItemParameters(Item *copy) const;

        virtual GraphicsScene* scene();

        virtual QRectF boundingRect() const;
        virtual QPainterPath shape() const;

        void setTextWidth(qreal width);
        void setTextHeight(qreal height);
        qreal textHeight() const;
        qreal pixelsPerPoint() const;

        void contentsChanged();

        virtual void resize(qreal w, qreal h);

        virtual QSizeF size() const;

        static QColor lastUsedTextColor;

        QColor colorOnDarkBackground() const
        {
            return mColorOnDarkBackground;
        }

        void setColorOnDarkBackground(QColor pColorOnDarkBackground)
        {
            mColorOnDarkBackground = pColorOnDarkBackground;
        }

        QColor colorOnLightBackground() const
        {
            return mColorOnLightBackground;
        }

        void setColorOnLightBackground(QColor pColorOnLightBackground)
        {
            mColorOnLightBackground = pColorOnLightBackground;
        }

        virtual void clearSource(){;}
        virtual void setUuid(const QUuid &pUuid);
        void activateTextEditor(bool activate);
        void setSelected(bool selected);
        void recolor();

        QString mTypeTextHereLabel;

    signals:
        void textUndoCommandAdded(GraphicsTextItem *textItem);

    private slots:
        void undoCommandAdded();
        void documentSizeChanged(const QSizeF & newSize);

    private:
        virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
        virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
        virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

        virtual void keyPressEvent(QKeyEvent *event);
        virtual void keyReleaseEvent(QKeyEvent *event);

        virtual void dragMoveEvent(QGraphicsSceneDragDropEvent *event);

        virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

        virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

        qreal mTextHeight;

        int mMultiClickState;
        QTime mLastMousePressTime;

        QColor mColorOnDarkBackground;
        QColor mColorOnLightBackground;
        bool isActivatedTextEditor;
};

