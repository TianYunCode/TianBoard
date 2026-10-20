#include "ui/DocumentToolsPalette.h"

#include <QtWidgets>

#include "platform/PlatformUtils.h"

#include "core/Application.h"
#include "ui/MainWindow.h"

struct DocumentToolsPalette::Private
{
};

DocumentToolsPalette::DocumentToolsPalette(QWidget *parent) : ActionPalette(Qt::TopRightCorner, parent), d(new Private)
{
    QList<QAction *> actions;

    // 没有系统虚拟键盘的平台不提供该动作，避免出现无效按钮。
    if (PlatformUtils::hasVirtualKeyboard())
    {
        actions << Application::mainWindow->ui().actionVirtualKeyboard;
    }

    setActions(actions);
    setButtonIconSize(QSize(42, 42));

    adjustSizeAndPosition();
}

DocumentToolsPalette::~DocumentToolsPalette() = default;
