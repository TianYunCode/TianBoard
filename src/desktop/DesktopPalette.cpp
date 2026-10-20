#include "desktop/DesktopPalette.h"

#include <QtWidgets>

#include "platform/PlatformUtils.h"

#include "settings/Settings.h"
#include "settings/Setting.h"
#include "core/Application.h"
#include "core/ApplicationController.h"

#include "whiteboard/DrawingController.h"

#include "ui/MainWindow.h"


struct DesktopPalette::Private {
    QAction *mShowHideAction{nullptr};
    QAction *mDisplaySelectAction{nullptr};
    QAction *mMaximizeAction{nullptr};
    QAction *mActionBoard{nullptr};
    QAction *mActionCustomSelect{nullptr};
    QAction *mActionTest{nullptr};

    RightPalette *rightPalette{nullptr};
};

DesktopPalette::DesktopPalette(QWidget *parent, RightPalette *_rightPalette)
    : ActionPalette(Qt::TopLeftCorner, parent), d(new Private) {
    d->rightPalette = _rightPalette;

    QList<QAction *> actions;

    QIcon boardIcon;
    boardIcon.addPixmap(QPixmap(":/images/toolbar/board.png"), QIcon::Normal, QIcon::Off);
    boardIcon.addPixmap(QPixmap(":/images/toolbar/boardOn.png"), QIcon::Normal, QIcon::On);
    d->mActionBoard = new QAction(QIcon(":/images/toolbar/board.png"), tr("Show TianBoard"), this);
    connect(d->mActionBoard, &QAction::triggered, this, [this] { emit boardClick(); });
    actions << d->mActionBoard;

    actions << Application::mainWindow->ui().actionPen;
    actions << Application::mainWindow->ui().actionEraser;
    actions << Application::mainWindow->ui().actionMarker;
    actions << Application::mainWindow->ui().actionSelector;
    actions << Application::mainWindow->ui().actionPointer;

    if (PlatformUtils::hasVirtualKeyboard())
        actions << Application::mainWindow->ui().actionVirtualKeyboard;

    QIcon actionCustomIcon;
    actionCustomIcon.addPixmap(QPixmap(":/images/toolbar/captureScreen.png"), QIcon::Normal, QIcon::Off);
    actionCustomIcon.addPixmap(QPixmap(":/images/toolbar/captureScreenOn.png"), QIcon::Normal, QIcon::On);
    d->mActionCustomSelect =
        new QAction(QIcon(":/images/toolbar/captureScreen.png"), tr("Capture Part of the Screen"), this);
    connect(d->mActionCustomSelect, &QAction::triggered, this, [this] { emit customClick(); });
    actions << d->mActionCustomSelect;

    QIcon displaySelectIcon;
    displaySelectIcon.addPixmap(QPixmap(":/images/toolbar/captureArea.png"), QIcon::Normal, QIcon::Off);
    displaySelectIcon.addPixmap(QPixmap(":/images/toolbar/captureAreaOn.png"), QIcon::Normal, QIcon::On);
    d->mDisplaySelectAction = new QAction(displaySelectIcon, tr("Capture the Screen"), this);
    connect(d->mDisplaySelectAction, &QAction::triggered, this, [this] { emit screenClick(); });
    actions << d->mDisplaySelectAction;

    QIcon showHideIcon;
    showHideIcon.addPixmap(QPixmap(":/images/toolbar/eyeOpened.png"), QIcon::Normal, QIcon::On);
    showHideIcon.addPixmap(QPixmap(":/images/toolbar/eyeClosed.png"), QIcon::Normal, QIcon::Off);
    d->mShowHideAction = new QAction(showHideIcon, "", this);
    d->mShowHideAction->setCheckable(true);

    connect(d->mShowHideAction, &QAction::toggled, this, &DesktopPalette::showHideClick);
    actions << d->mShowHideAction;

    setActions(actions);
    setButtonIconSize(QSize(42, 42));

    adjustSizeAndPosition();

    //  This palette can be minimized
    QIcon maximizeIcon;
    maximizeIcon.addPixmap(QPixmap(":/images/toolbar/stylusTab.png"), QIcon::Normal, QIcon::On);
    d->mMaximizeAction = new QAction(maximizeIcon, tr("Show the stylus palette"), this);
    connect(d->mMaximizeAction, &QAction::triggered, this, [this] { maximizeMe(); });
    connect(this, &FloatingPalette::maximizeStart, this, &DesktopPalette::maximizeMe);
    connect(this, &FloatingPalette::minimizeStart, this, &DesktopPalette::minimizeMe);
    setMinimizePermission(true);

    connect(d->rightPalette, &RightPalette::resized, this, &DesktopPalette::parentResized);
}

DesktopPalette::~DesktopPalette() {}

void DesktopPalette::adjustPosition() {
    QPoint pos = this->pos();
    if (this->pos().y() < 30) {
        pos.setY(30);
        moveInsideParent(pos);
    }
}

void DesktopPalette::disappearForCapture() {
    setWindowOpacity(0.0);
    qApp->processEvents();
}


void DesktopPalette::appear() {
    setWindowOpacity(1.0);
}


void DesktopPalette::showHideClick(bool checked) {
    Application::applicationController->mirroringEnabled(checked);
}


void DesktopPalette::updateShowHideState(bool pShowEnabled) {
    if (d->mShowHideAction)
        d->mShowHideAction->setChecked(pShowEnabled);

    if (d->mShowHideAction->isChecked())
        d->mShowHideAction->setToolTip(tr("Show Board on Secondary Screen"));
    else
        d->mShowHideAction->setToolTip(tr("Show Desktop on Secondary Screen"));

    if (pShowEnabled)
        raise();
}


void DesktopPalette::setShowHideButtonVisible(bool visible) {
    d->mShowHideAction->setVisible(visible);
}


void DesktopPalette::setDisplaySelectButtonVisible(bool visible) {
    d->mDisplaySelectAction->setVisible(visible);
}

//  Called when the palette is near the border and must be minimized
void DesktopPalette::minimizeMe(eMinimizedLocation location) {
    Q_UNUSED(location);
    QList<QAction *> actions;
    clearLayout();

    actions << d->mMaximizeAction;
    setActions(actions);

    adjustSizeAndPosition();
}

//  Called when the user wants to maximize the palette
void DesktopPalette::maximizeMe() {
    QList<QAction *> actions;
    clearLayout();

    actions << d->mActionBoard;
    actions << Application::mainWindow->ui().actionPen;
    actions << Application::mainWindow->ui().actionEraser;
    actions << Application::mainWindow->ui().actionMarker;
    actions << Application::mainWindow->ui().actionSelector;
    actions << Application::mainWindow->ui().actionPointer;
    if (PlatformUtils::hasVirtualKeyboard())
        actions << Application::mainWindow->ui().actionVirtualKeyboard;

    actions << d->mActionCustomSelect;
    actions << d->mDisplaySelectAction;
    actions << d->mShowHideAction;

    setActions(actions);

    adjustSizeAndPosition();

    // Notify that the maximization has been done
    emit maximized();
}

void DesktopPalette::showEvent(QShowEvent *event) {
    Q_UNUSED(event);
    QIcon penIcon;
    QIcon markerIcon;
    QIcon eraserIcon;
    penIcon.addPixmap(QPixmap(":images/stylusPalette/penArrow.png"), QIcon::Normal, QIcon::Off);
    penIcon.addPixmap(QPixmap(":images/stylusPalette/penOnArrow.png"), QIcon::Normal, QIcon::On);
    Application::mainWindow->ui().actionPen->setIcon(penIcon);
    markerIcon.addPixmap(QPixmap(":images/stylusPalette/markerArrow.png"), QIcon::Normal, QIcon::Off);
    markerIcon.addPixmap(QPixmap(":images/stylusPalette/markerOnArrow.png"), QIcon::Normal, QIcon::On);
    Application::mainWindow->ui().actionMarker->setIcon(markerIcon);
    eraserIcon.addPixmap(QPixmap(":images/stylusPalette/eraserArrow.png"), QIcon::Normal, QIcon::Off);
    eraserIcon.addPixmap(QPixmap(":images/stylusPalette/eraserOnArrow.png"), QIcon::Normal, QIcon::On);
    Application::mainWindow->ui().actionEraser->setIcon(eraserIcon);

    adjustPosition();
}

void DesktopPalette::hideEvent(QHideEvent *event) {
    Q_UNUSED(event);
    QIcon penIcon;
    QIcon markerIcon;
    QIcon eraserIcon;
    penIcon.addPixmap(QPixmap(":images/stylusPalette/pen.png"), QIcon::Normal, QIcon::Off);
    penIcon.addPixmap(QPixmap(":images/stylusPalette/penOn.png"), QIcon::Normal, QIcon::On);
    Application::mainWindow->ui().actionPen->setIcon(penIcon);
    markerIcon.addPixmap(QPixmap(":images/stylusPalette/marker.png"), QIcon::Normal, QIcon::Off);
    markerIcon.addPixmap(QPixmap(":images/stylusPalette/markerOn.png"), QIcon::Normal, QIcon::On);
    Application::mainWindow->ui().actionMarker->setIcon(markerIcon);
    eraserIcon.addPixmap(QPixmap(":images/stylusPalette/eraser.png"), QIcon::Normal, QIcon::Off);
    eraserIcon.addPixmap(QPixmap(":images/stylusPalette/eraserOn.png"), QIcon::Normal, QIcon::On);
    Application::mainWindow->ui().actionEraser->setIcon(eraserIcon);
}

QPoint DesktopPalette::buttonPos(QAction *action) {
    QPoint p;

    ActionPaletteButton *pB = getButtonFromAction(action);
    if (NULL != pB) {
        p = pB->pos();
    }

    return p;
}


int DesktopPalette::getParentRightOffset() {
    return d->rightPalette->width();
}

void DesktopPalette::parentResized() {
    QPoint p = pos();
    if (minimizedLocation() == eMinimizedLocation_Right) {
        p.setX(parentWidget()->width() - getParentRightOffset() - width());
    }

    moveInsideParent(p);
}
