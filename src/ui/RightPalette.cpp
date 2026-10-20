#include "core/Application.h"
#include "whiteboard/BoardController.h"

#include "ui/RightPalette.h"


struct RightPalette::Private {};

RightPalette::RightPalette(QWidget *parent, const char *name)
    : DockPalette(DockPaletteType::Right, parent), d(new Private) {
    setObjectName(name);
    setOrientation(DockOrientation::Right);
    setCollapseWidth(120);
    // 宽度与折叠状态按 Board / Desktop 两种模式分别保存，切换模式时各自恢复。
    bool isCollapsed = false;
    if (currentMode() == DockWidgetMode::Board) {
        setLastWidth(Settings::settings()->rightLibPaletteBoardModeWidth->get().toInt());
        isCollapsed = Settings::settings()->rightLibPaletteBoardModeIsCollapsed->get().toBool();
    } else {
        setLastWidth(Settings::settings()->rightLibPaletteDesktopModeWidth->get().toInt());
        isCollapsed = Settings::settings()->rightLibPaletteDesktopModeIsCollapsed->get().toBool();
    }
    if (isCollapsed)
        resize(0, parentWidget()->height());
    else
        resize(lastWidth(), parentWidget()->height());
}

RightPalette::~RightPalette() {}

void RightPalette::mouseMoveEvent(QMouseEvent *event) {
    if (canResize()) {
        DockPalette::mouseMoveEvent(event);
    }
}

void RightPalette::resizeEvent(QResizeEvent *event) {
    // 折叠到 0 时不能把 0 当成新的“上次宽度”存下来，否则下次展开就没有宽度可恢复。
    int newWidth = width();
    if (currentMode() == DockWidgetMode::Board) {
        if (newWidth > collapseWidth())
            Settings::settings()->rightLibPaletteBoardModeWidth->set(newWidth);
        Settings::settings()->rightLibPaletteBoardModeIsCollapsed->set(newWidth == 0);
    } else {
        if (newWidth > collapseWidth())
            Settings::settings()->rightLibPaletteDesktopModeWidth->set(newWidth);
        Settings::settings()->rightLibPaletteDesktopModeIsCollapsed->set(newWidth == 0);
    }
    DockPalette::resizeEvent(event);
    emit resized();
}

void RightPalette::updateMaxWidth() {
    setMaximumWidth((int)(parentWidget()->width() * 0.45));
    setMaximumHeight(parentWidget()->height());
    setMinimumHeight(parentWidget()->height());
}

bool RightPalette::switchMode(DockWidgetMode mode) {
    // 先按目标模式取回宽度（折叠则记为 0），再交给基类完成模式切换。
    int newModeWidth;
    if (mode == DockWidgetMode::Board) {
        setLastWidth(Settings::settings()->rightLibPaletteBoardModeWidth->get().toInt());
        newModeWidth = lastWidth();
        if (Settings::settings()->rightLibPaletteBoardModeIsCollapsed->get().toBool())
            newModeWidth = 0;
    } else {
        setLastWidth(Settings::settings()->rightLibPaletteDesktopModeWidth->get().toInt());
        newModeWidth = lastWidth();
        if (Settings::settings()->rightLibPaletteDesktopModeIsCollapsed->get().toBool())
            newModeWidth = 0;
    }
    resize(newModeWidth, height());
    return DockPalette::switchMode(mode);
}
