#pragma once

#include <QtWidgets>

#include "whiteboard/GraphicsItemDelegate.h"
#include "whiteboard/GraphicsWidgetItem.h"

class GraphicsWidgetItemDelegate : public GraphicsItemDelegate
{
    Q_OBJECT

    public:
        GraphicsWidgetItemDelegate(GraphicsWidgetItem* pDelegated, int widgetType = 0);
        virtual ~GraphicsWidgetItemDelegate();

    protected:
        virtual void decorateMenu(QMenu* menu);
        virtual void updateMenuActionState();
        virtual void remove(bool canundo);

    private slots:
        void freeze(bool frozeon);
        void pin();

    private:
        int mWidgetType;

        GraphicsWidgetItem* delegated();

        QAction* freezeAction;
        QAction* setAsToolAction;
};

