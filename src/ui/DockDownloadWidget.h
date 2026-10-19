#pragma once

#include <QWidget>
#include <QVBoxLayout>

#include "ui/DockPaletteWidget.h"
#include "ui/DownloadWidget.h"

class DockDownloadWidget : public DockPaletteWidget
{
    Q_OBJECT
public:
    DockDownloadWidget(QWidget* parent=0, const char* name="DockDownloadWidget");
    ~DockDownloadWidget();

    bool visibleInMode(eWBDockPaletteWidgetMode mode)
    {
        return mode == eWBDockPaletteWidget_BOARD;
    }

private:
    QVBoxLayout* mpLayout;
    DownloadWidget* mpDLWidget;
};

