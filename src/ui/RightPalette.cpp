#include "core/Application.h"
#include "whiteboard/BoardController.h"

#include "ui/RightPalette.h"


struct RightPalette::Private
{
};

RightPalette::RightPalette(QWidget *parent, const char *name):
    DockPalette(eWBDockPaletteType_RIGHT, parent)
    , d(new Private)
{
    setObjectName(name);
    setOrientation(eWBDockOrientation_Right);
    setCollapseWidth(120);
    bool isCollapsed = false;
    if(currentMode() == eWBDockPaletteWidget_BOARD){
        setLastWidth(Settings::settings()->rightLibPaletteBoardModeWidth->get().toInt());
        isCollapsed = Settings::settings()->rightLibPaletteBoardModeIsCollapsed->get().toBool();
    }
    else{
        setLastWidth(Settings::settings()->rightLibPaletteDesktopModeWidth->get().toInt());
        isCollapsed = Settings::settings()->rightLibPaletteDesktopModeIsCollapsed->get().toBool();
    }
    if(isCollapsed)
        resize(0,parentWidget()->height());
    else
        resize(lastWidth(), parentWidget()->height());
}

RightPalette::~RightPalette()
{
}

void RightPalette::mouseMoveEvent(QMouseEvent *event)
{
    if(canResize())
    {
        DockPalette::mouseMoveEvent(event);
    }
}

void RightPalette::resizeEvent(QResizeEvent *event)
{
    int newWidth = width();
    if(currentMode() == eWBDockPaletteWidget_BOARD){
        if(newWidth > collapseWidth())
            Settings::settings()->rightLibPaletteBoardModeWidth->set(newWidth);
        Settings::settings()->rightLibPaletteBoardModeIsCollapsed->set(newWidth == 0);
    }
    else{
        if(newWidth > collapseWidth())
            Settings::settings()->rightLibPaletteDesktopModeWidth->set(newWidth);
        Settings::settings()->rightLibPaletteDesktopModeIsCollapsed->set(newWidth == 0);
    }
    DockPalette::resizeEvent(event);
    emit resized();
}

void RightPalette::updateMaxWidth()
{
    setMaximumWidth((int)(parentWidget()->width() * 0.45));
    setMaximumHeight(parentWidget()->height());
    setMinimumHeight(parentWidget()->height());
}

bool RightPalette::switchMode(eWBDockPaletteWidgetMode mode)
{
    int newModeWidth;
    if(mode == eWBDockPaletteWidget_BOARD){
        setLastWidth(Settings::settings()->rightLibPaletteBoardModeWidth->get().toInt());
        newModeWidth = lastWidth();
        if(Settings::settings()->rightLibPaletteBoardModeIsCollapsed->get().toBool())
            newModeWidth = 0;
    }
    else{
        setLastWidth(Settings::settings()->rightLibPaletteDesktopModeWidth->get().toInt());
        newModeWidth = lastWidth();
        if(Settings::settings()->rightLibPaletteDesktopModeIsCollapsed->get().toBool())
            newModeWidth = 0;
    }
    resize(newModeWidth,height());
    return DockPalette::switchMode(mode);
}
