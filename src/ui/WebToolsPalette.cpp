#include <QtWidgets>

#include "platform/PlatformUtils.h"

#include "core/Application.h"
#include "settings/Settings.h"
#include "core/ApplicationController.h"
#include "core/DisplayManager.h"

#include "ui/MainWindow.h"

#include "ui/WebToolsPalette.h"
#include "model/Resources.h"
#include "ui/IconButton.h"

struct WebToolsPalette::Private
{
};

WebToolsPalette::WebToolsPalette(QWidget *parent) : ActionPalette(Qt::TopRightCorner, parent), d(new Private)
{
    QList<QAction *> actions;

    actions << Application::mainWindow->ui().actionWebCustomCapture;

    actions << Application::mainWindow->ui().actionWebShowHideOnDisplay;

    // 虚拟键盘只在支持软键盘的平台出现，避免桌面端多出一个无效按钮。
    if (PlatformUtils::hasVirtualKeyboard())
    {
        actions << Application::mainWindow->ui().actionVirtualKeyboard;
    }

    setActions(actions);
    setButtonIconSize(QSize(42, 42));
    adjustSizeAndPosition();
}

WebToolsPalette::~WebToolsPalette()
{
    // 刻意留空
}
