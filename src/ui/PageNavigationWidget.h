#pragma once

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QTimerEvent>
#include <QLabel>
#include <QString>

#include "ui/BoardThumbnailsView.h"
#include "ui/DocumentNavigator.h"
#include "ui/DockPaletteWidget.h"
#include "model/DocumentProxy.h"

class PageNavigationWidget : public DockPaletteWidget
{
    Q_OBJECT
public:
    PageNavigationWidget(QWidget* parent=0, const char* name="PageNavigationWidget");
    ~PageNavigationWidget();

    bool visibleInMode(eWBDockPaletteWidgetMode mode)
    {
        return mode == eWBDockPaletteWidget_BOARD;
    }

signals:
    void resizeRequest(QResizeEvent* event);

public slots:
    void setPageNumber(int current, int total);

protected:
    virtual void timerEvent(QTimerEvent *event);


private:
    void updateTime();
    int customMargin();
    int border();

    DocumentNavigator* mNavigator;
    QVBoxLayout* mLayout;
    QHBoxLayout* mHLayout;
    QLabel* mPageNbr;
    QLabel* mClock;
    QString mTimeFormat;
    int mTimerID;

};

