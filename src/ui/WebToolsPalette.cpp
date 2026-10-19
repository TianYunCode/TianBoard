#include <QtWidgets>

#include "platform/PlatformUtils.h"

#include "core/Application.h"
#include "settings/Settings.h"
#include "core/ApplicationController.h"
#include "core/DisplayManager.h"

#include "ui/MainWindow.h"

#include "ui/WebToolsPalette.h"
#include "ui/Resources.h"
#include "ui/IconButton.h"


struct WebToolsPalette::Private
{
};

WebToolsPalette::WebToolsPalette(QWidget *parent)
    : ActionPalette(Qt::TopRightCorner, parent)
    , d(new Private)
{
    QList<QAction*> actions;

    actions << Application::mainWindow->ui().actionWebCustomCapture;

    actions << Application::mainWindow->ui().actionWebShowHideOnDisplay;

    if (PlatformUtils::hasVirtualKeyboard())
        actions << Application::mainWindow->ui().actionVirtualKeyboard;

    setActions(actions);
    setButtonIconSize(QSize(42, 42));
    adjustSizeAndPosition();
}


WebToolsPalette::~WebToolsPalette()
{
    // NOOP
}



