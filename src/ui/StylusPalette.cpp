#include "ui/StylusPalette.h"

#include <QtWidgets>

#include "ui/MainWindow.h"

#include "core/Application.h"
#include "core/Settings.h"
#include "core/ApplicationController.h"

#include "whiteboard/DrawingController.h"

#include "platform/PlatformUtils.h"


struct StylusPalette::Private
{
    int mLastSelectedId{-1};
};

StylusPalette::StylusPalette(QWidget *parent, Qt::Orientation orient)
    : ActionPalette(Qt::TopLeftCorner, parent, orient)
    , d(new Private)
{
    QList<QAction*> actions;

    actions << Application::mainWindow->ui().actionPen;
    actions << Application::mainWindow->ui().actionEraser;
    actions << Application::mainWindow->ui().actionMarker;
    actions << Application::mainWindow->ui().actionSelector;
    actions << Application::mainWindow->ui().actionPlay;

    actions << Application::mainWindow->ui().actionHand;
    actions << Application::mainWindow->ui().actionZoomIn;
    actions << Application::mainWindow->ui().actionZoomOut;

    actions << Application::mainWindow->ui().actionPointer;
    actions << Application::mainWindow->ui().actionLine;
    actions << Application::mainWindow->ui().actionText;
    actions << Application::mainWindow->ui().actionCapture;

    if(PlatformUtils::hasVirtualKeyboard())
        actions << Application::mainWindow->ui().actionVirtualKeyboard;

    setActions(actions);
    setButtonIconSize(QSize(42, 42));

    if(!PlatformUtils::hasVirtualKeyboard())
    {
        groupActions();
    }
    else
    {
        // VirtualKeyboard action is not in group
        // So, groupping all buttons, except last
        setButtonGroup(new QButtonGroup(this));
        for(int i=0; i < buttons().size()-1; i++)
        {
            buttonGroup()->addButton(buttons()[i], i);
        }
        connect(buttonGroup(), &QButtonGroup::idClicked, this, &ActionPalette::buttonGroupClicked);
    }

    adjustSizeAndPosition();

    initPosition();

    foreach(const ActionPaletteButton* button, buttons())
    {
        connect(button, SIGNAL(doubleClicked()), this, SLOT(stylusToolDoubleClicked()));
    }

}

void StylusPalette::initPosition()
{
    QWidget* pParentW = parentWidget();
    if(!pParentW) 
		return ;

    setCustomPositionFlag(true);

    QPoint pos;
    int parentWidth = pParentW->width();
    int parentHeight = pParentW->height();

    if(Settings::settings()->appToolBarOrientationVertical->get().toBool()){
        int posX = border();
        int posY = (parentHeight / 2) - (height() / 2);
        pos.setX(posX);
        pos.setY(posY);
    }
    else {
        int posX = (parentWidth / 2) - (width() / 2);
        int posY = parentHeight - border() - height();
        pos.setX(posX);
        pos.setY(posY);
    }
    moveInsideParent(pos);
}

StylusPalette::~StylusPalette()
{

}

void StylusPalette::stylusToolDoubleClicked()
{
    emit stylusToolDoubleClicked(buttonGroup()->checkedId());
}

