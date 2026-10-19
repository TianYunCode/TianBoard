#include "ui/LeftPalette.h"
#include "settings/Settings.h"


struct LeftPalette::Private
{
};


LeftPalette::LeftPalette(QWidget *parent, const char *name):
    DockPalette(DockPaletteType::Left, parent)
    , d(new Private)
{
    setObjectName(name);
    setOrientation(DockOrientation::Left);
    setCollapseWidth(120);

    bool isCollapsed = false;
    if(currentMode() == DockWidgetMode::Board){
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
    setLastOpenedTabForMode(DockWidgetMode::Board, 0);
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
    if(currentMode() == DockWidgetMode::Board){
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


bool LeftPalette::switchMode(DockWidgetMode mode)
{
    int newModeWidth;
    if(mode == DockWidgetMode::Board){
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
