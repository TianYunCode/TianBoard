#include "ui/LeftPalette.h"
#include "core/Settings.h"


struct LeftPalette::Private
{
};


LeftPalette::LeftPalette(QWidget *parent, const char *name):
    DockPalette(eWBDockPaletteType_LEFT, parent)
    , d(new Private)
{
    setObjectName(name);
    setOrientation(eWBDockOrientation_Left);
    setCollapseWidth(120);

    bool isCollapsed = false;
    if(currentMode() == eWBDockPaletteWidget_BOARD){
        setLastWidth(Settings::settings()->leftLibPaletteBoardModeWidth->get().toInt());
        isCollapsed = Settings::settings()->leftLibPaletteBoardModeIsCollapsed->get().toBool();
    }
    else{
        setLastWidth(Settings::settings()->leftLibPaletteDesktopModeWidth->get().toInt());
        isCollapsed = Settings::settings()->leftLibPaletteDesktopModeIsCollapsed->get().toBool();
    }

    if(isCollapsed)
        resize(0,parentWidget()->height());
    else
        resize(lastWidth(), parentWidget()->height());
}

/**
 * \brief The destructor
 */
LeftPalette::~LeftPalette()
{

}


void LeftPalette::onDocumentSet(DocumentProxy* documentProxy)
{
    Q_UNUSED(documentProxy)
    // the tab zero is forced
    setLastOpenedTabForMode(eWBDockPaletteWidget_BOARD, 0);
}

/**
 * \brief Update the maximum width
 */
void LeftPalette::updateMaxWidth()
{
    setMaximumWidth((int)(parentWidget()->width() * 0.45));
}

/**
 * \brief Handle the resize event
 * @param event as the resize event
 */
void LeftPalette::resizeEvent(QResizeEvent *event)
{
    int newWidth = width();
    if(currentMode() == eWBDockPaletteWidget_BOARD){
        if(newWidth > collapseWidth())
            Settings::settings()->leftLibPaletteBoardModeWidth->set(newWidth);
        Settings::settings()->leftLibPaletteBoardModeIsCollapsed->set(newWidth == 0);
    }
    else{
        if(newWidth > collapseWidth())
            Settings::settings()->leftLibPaletteDesktopModeWidth->set(newWidth);
        Settings::settings()->leftLibPaletteDesktopModeIsCollapsed->set(newWidth == 0);
    }
    DockPalette::resizeEvent(event);
}


bool LeftPalette::switchMode(eWBDockPaletteWidgetMode mode)
{
    int newModeWidth;
    if(mode == eWBDockPaletteWidget_BOARD){
        setLastWidth(Settings::settings()->leftLibPaletteBoardModeWidth->get().toInt());
        newModeWidth = lastWidth();
        if(Settings::settings()->leftLibPaletteBoardModeIsCollapsed->get().toBool())
            newModeWidth = 0;
    }
    else{
        setLastWidth(Settings::settings()->leftLibPaletteDesktopModeWidth->get().toInt());
        newModeWidth = lastWidth();
        if(Settings::settings()->leftLibPaletteDesktopModeIsCollapsed->get().toBool())
            newModeWidth = 0;
    }
    resize(newModeWidth,height());
    return DockPalette::switchMode(mode);
}
