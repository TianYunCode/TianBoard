#include "ui/ZoomPalette.h"

#include <QtWidgets>
#include <QPushButton>
#include <math.h>

#include "core/Application.h"

#include "whiteboard/BoardController.h"
#include "whiteboard/BoardView.h"

struct ZoomPalette::Private
{
    BoardController *mBoardController{nullptr};
    QPushButton     *mCurrentZoomButton{nullptr};
    QPushButton     *mHundredButton{nullptr};
    QPushButton     *mShowAllButton{nullptr};

    bool mIsExpanded{true};
};

ZoomPalette::ZoomPalette(QWidget *parent) : FloatingPalette(Qt::BottomRightCorner, parent), d(new Private)
{
    d->mBoardController   = Application::boardController;
    QLayout *layout       = new QVBoxLayout(this);
    d->mCurrentZoomButton = new QPushButton(parent);
    d->mCurrentZoomButton->setStyleSheet(QString("QPushButton { color: white; background-color: transparent; border: none; font-family: "
                                                 "Arial; font-weight: bold; font-size: 20px }"));
    d->mCurrentZoomButton->setFocusPolicy(Qt::NoFocus);
    connect(d->mCurrentZoomButton, &QPushButton::clicked, this, &ZoomPalette::showHideExtraButton);
    connect(d->mBoardController, &BoardController::zoomChanged, this, &ZoomPalette::refreshPalette);
    connect(d->mBoardController, &BoardController::activeSceneChanged, this, &ZoomPalette::refreshPalette);

    d->mHundredButton = new QPushButton(parent);
    d->mHundredButton->setStyleSheet(QString("QPushButton { color: white; background-color: transparent; border: none; font-family: "
                                             "Arial; font-weight: bold; font-size: 20px }"));
    d->mHundredButton->setFocusPolicy(Qt::NoFocus);
    d->mHundredButton->setIcon(QIcon(":/images/stylusPalette/restoreZoom.png"));
    d->mHundredButton->setIconSize(QSize(42, 42));
    connect(d->mHundredButton, &QPushButton::clicked, this, &ZoomPalette::goHundred);

    layout->setContentsMargins(radius() + 15, 4, radius() + 15, 4);
    layout->addWidget(d->mHundredButton);
    layout->addWidget(d->mCurrentZoomButton);
    hide();
    refreshPalette();
}

ZoomPalette::~ZoomPalette()
{
    delete d->mCurrentZoomButton;
}

void ZoomPalette::showHideExtraButton()
{
    if (d->mIsExpanded)
    {
        d->mHundredButton->setVisible(false);
        d->mIsExpanded = false;
        adjustSizeAndPosition(false);
    }
    else
    {
        d->mHundredButton->setVisible(true);
        d->mIsExpanded = true;
        adjustSizeAndPosition(true);
    }
}

void ZoomPalette::goHundred()
{
    d->mBoardController->zoomRestore();
}

void ZoomPalette::hide()
{
    qreal currentZoomFactor = d->mBoardController->currentZoom();

    // 这个槽也挂在“无缩放后延时隐藏”上，用户此时若已重新缩放就不该隐藏。
    bool showAsNoZoom       = (currentZoomFactor > 0.9 && currentZoomFactor < 1.1);
    if (showAsNoZoom)
    {
        FloatingPalette::hide();
    }
}

void ZoomPalette::refreshPalette()
{
    qreal currentZoomFactor = d->mBoardController->currentZoom();
    // 与 hide() 使用同一判定：0.9~1.1 视为“没有缩放”，显示成整数 1x。
    bool showAsNoZoom       = (currentZoomFactor > 0.9 && currentZoomFactor < 1.1);
    if (showAsNoZoom)
    {
        currentZoomFactor = 1;
    }
    QString stringFactor = tr("%1 x").arg(currentZoomFactor, 0, 'f', 1);

    d->mCurrentZoomButton->setText(stringFactor);
    adjustSizeAndPosition();
    // 无缩放时先显示 500ms 告知用户，再自动隐藏，避免长期占用画面角落。
    if (showAsNoZoom)
    {
        QTimer::singleShot(500, this, &ZoomPalette::hide);
    }
    else
    {
        show();
        raise();
    }
}
