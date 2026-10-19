#pragma once

#include <QtWidgets>
#include <QGraphicsScene>
#include <QGraphicsItem>

class CoreGraphicsScene : public QGraphicsScene
{
    public:
        CoreGraphicsScene(QObject * parent = 0);
        virtual ~CoreGraphicsScene();

        virtual void addItem(QGraphicsItem* item);

        virtual void removeItem(QGraphicsItem* item, bool forceDelete = false);

        virtual bool deleteItem(QGraphicsItem* item);

        void removeItemFromDeletion(QGraphicsItem* item);
        void addItemToDeletion(QGraphicsItem *item);

        bool isModified() const
        {
            return mIsModified;
        }

        void setModified(bool pModified)
        {
            mIsModified = pModified;
        }

    private:
        QSet<QGraphicsItem*> mItemsToDelete;

        bool mIsModified;
};

