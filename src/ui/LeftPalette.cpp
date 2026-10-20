#include "ui/LeftPalette.h"
#include "settings/Settings.h"

struct LeftPalette::Private
{
};

LeftPalette::LeftPalette(QWidget *parent, const char *name) : DockPalette(DockPaletteType::Left, parent), d(new Private)
{
    setObjectName(name);
    setOrientation(DockOrientation::Left);
    setCollapseWidth(120);

    // 构造时 currentMode() 固定为 Board，这里按模式读取宽度与折叠状态。
    bool isCollapsed = false;
    if (currentMode() == DockWidgetMode::Board)
    {
        setLastWidth(Settings::settings()->leftLibPaletteBoardModeWidth->get().toInt());
        isCollapsed = Settings::settings()->leftLibPaletteBoardModeIsCollapsed->get().toBool();
    }
    else
    {
        setLastWidth(Settings::settings()->leftLibPaletteDesktopModeWidth->get().toInt());
        isCollapsed = Settings::settings()->leftLibPaletteDesktopModeIsCollapsed->get().toBool();
    }

    // 折叠状态用 0 宽度表示，恢复时使用上次保存的宽度。
    if (isCollapsed)
    {
        resize(0, parentWidget()->height());
    }
    else
    {
        resize(lastWidth(), parentWidget()->height());
    }
}

LeftPalette::~LeftPalette()
{
}

void LeftPalette::onDocumentSet(DocumentProxy *documentProxy)
{
    Q_UNUSED(documentProxy)
    // 新文档总是从第一个标签页开始，因此强制记住下标 0。
    setLastOpenedTabForMode(DockWidgetMode::Board, 0);
}

void LeftPalette::updateMaxWidth()
{
    // 限制在容器宽度的 45% 以内，避免面板挤占整个画布。
    setMaximumWidth((int)(parentWidget()->width() * 0.45));
}

void LeftPalette::resizeEvent(QResizeEvent *event)
{
    int newWidth = width();
    // 只在宽度大于折叠阈值时记录宽度，收到 0 时记录“已折叠”。
    if (currentMode() == DockWidgetMode::Board)
    {
        if (newWidth > collapseWidth())
        {
            Settings::settings()->leftLibPaletteBoardModeWidth->set(newWidth);
        }
        Settings::settings()->leftLibPaletteBoardModeIsCollapsed->set(newWidth == 0);
    }
    else
    {
        if (newWidth > collapseWidth())
        {
            Settings::settings()->leftLibPaletteDesktopModeWidth->set(newWidth);
        }
        Settings::settings()->leftLibPaletteDesktopModeIsCollapsed->set(newWidth == 0);
    }
    DockPalette::resizeEvent(event);
}

bool LeftPalette::switchMode(DockWidgetMode mode)
{
    // 切模式前先恢复该模式自己的宽度与折叠状态，再交给基类增删标签。
    int newModeWidth;
    if (mode == DockWidgetMode::Board)
    {
        setLastWidth(Settings::settings()->leftLibPaletteBoardModeWidth->get().toInt());
        newModeWidth = lastWidth();
        if (Settings::settings()->leftLibPaletteBoardModeIsCollapsed->get().toBool())
        {
            newModeWidth = 0;
        }
    }
    else
    {
        setLastWidth(Settings::settings()->leftLibPaletteDesktopModeWidth->get().toInt());
        newModeWidth = lastWidth();
        if (Settings::settings()->leftLibPaletteDesktopModeIsCollapsed->get().toBool())
        {
            newModeWidth = 0;
        }
    }
    resize(newModeWidth, height());
    return DockPalette::switchMode(mode);
}
