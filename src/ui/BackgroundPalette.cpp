#include "ui/BackgroundPalette.h"
#include "core/Application.h"

#include "ui/MainWindow.h"

struct BackgroundPalette::Private
{
    QVBoxLayout *mVLayout{nullptr};
    QHBoxLayout *mTopLayout{nullptr};
    QHBoxLayout *mBottomLayout{nullptr};

    QSlider             *mSlider{nullptr};
    QLabel              *mSliderLabel{nullptr};
    ActionPaletteButton *mResetDefaultGridSizeButton{nullptr};
};

BackgroundPalette::BackgroundPalette(QList<QAction *> actions, QWidget *parent) : ActionPalette(parent), d(new Private)
{
    init();
    setActions(actions);
}

BackgroundPalette::BackgroundPalette(QWidget *parent) : ActionPalette(parent), d(new Private)
{
    init();
}

BackgroundPalette::~BackgroundPalette() = default;

void BackgroundPalette::init()
{
    // 基类构造时已经建立了通用按钮布局，这里整套换掉，改用上下两行的专用布局。
    ActionPalette::clearLayout();
    delete layout();

    initButtonState();

    d->mVLayout      = new QVBoxLayout(this);
    d->mTopLayout    = new QHBoxLayout();
    d->mBottomLayout = new QHBoxLayout();

    d->mVLayout->addLayout(d->mTopLayout);
    d->mVLayout->addLayout(d->mBottomLayout);

    d->mSlider = new QSlider(Qt::Horizontal);

    d->mSlider->setMinimum(Settings::settings()->minCrossSize);
    d->mSlider->setMaximum(Settings::settings()->maxCrossSize);
    d->mSlider->setSingleStep(2);
    // 开启跟踪，拖动过程中就持续发出 valueChanged()，而不是等松开滑块才发。
    d->mSlider->setTracking(true);

    d->mSliderLabel                = new QLabel(tr("Grid size"));

    d->mResetDefaultGridSizeButton = createPaletteButton(Application::mainWindow->ui().actionDefaultGridSize, this);
    d->mResetDefaultGridSizeButton->setFixedSize(24, 24);
    appendAction(Application::mainWindow->ui().actionDefaultGridSize);

    connect(Application::mainWindow->ui().actionDefaultGridSize, &QAction::triggered, this, &BackgroundPalette::defaultBackgroundGridSize);

    d->mBottomLayout->addSpacing(16);
    d->mBottomLayout->addWidget(d->mSliderLabel);
    d->mBottomLayout->addWidget(d->mSlider);
    d->mBottomLayout->addWidget(d->mResetDefaultGridSizeButton);
    d->mBottomLayout->addSpacing(16);

    updateLayout();
}

void BackgroundPalette::addAction(QAction *action)
{
    ActionPaletteButton *button = createPaletteButton(action, this);

    d->mTopLayout->addWidget(button);
    appendAction(action);
}

void BackgroundPalette::setActions(QList<QAction *> actions)
{
    clearButtonMap();

    foreach (QAction *action, actions)
    {
        addAction(action);
    }

    actionChanged();
}

void BackgroundPalette::updateLayout()
{
    if (toolButtonStyle() == Qt::ToolButtonIconOnly)
    {
        d->mVLayout->setContentsMargins(sLayoutContentMargin / 2 + border(), sLayoutContentMargin / 2 + border(), sLayoutContentMargin / 2 + border(),
            sLayoutContentMargin / 2 + border());
    }
    else
    {
        d->mVLayout->setContentsMargins(
            sLayoutContentMargin + border(), sLayoutContentMargin + border(), sLayoutContentMargin + border(), sLayoutContentMargin + border());
    }
    update();
}

void BackgroundPalette::clearLayout()
{
    while (!d->mTopLayout->isEmpty())
    {
        QLayoutItem *pItem = d->mTopLayout->itemAt(0);
        QWidget     *pW    = pItem->widget();
        d->mTopLayout->removeItem(pItem);
        delete pItem;
        d->mTopLayout->removeWidget(pW);
        delete pW;
    }

    delete d->mTopLayout;

    while (!d->mBottomLayout->isEmpty())
    {
        QLayoutItem *pItem = d->mBottomLayout->itemAt(0);
        QWidget     *pW    = pItem->widget();
        d->mBottomLayout->removeItem(pItem);
        delete pItem;
        d->mBottomLayout->removeWidget(pW);
        delete pW;
    }

    delete d->mBottomLayout;

    delete d->mVLayout;

    clearActions();
    clearButtons();
}

void BackgroundPalette::showEvent(QShowEvent *event)
{
    backgroundChanged();

    d->mSlider->setValue(BoardController::instance()->activeScene()->backgroundGridSize());
    // 每次显示都重新连接；Qt 会忽略同一对信号的重复连接，不会造成多次回调。
    connect(d->mSlider, &QSlider::valueChanged, this, &BackgroundPalette::sliderValueChanged);

    QWidget::showEvent(event);
}

void BackgroundPalette::sliderValueChanged(int value)
{
    BoardController::instance()->activeScene()->setBackgroundGridSize(value);
    // 切换场景时会经由 refresh() 间接回到这里，所以设置里保存的始终是当前场景的值。
    Settings::settings()->crossSize = value;
}

void BackgroundPalette::defaultBackgroundGridSize()
{
    d->mSlider->setValue(Settings::settings()->defaultCrossSize);
    sliderValueChanged(Settings::settings()->defaultCrossSize);
}

void BackgroundPalette::backgroundChanged()
{
    bool dark = BoardController::instance()->activeScene()->isDarkBackground();

    if (dark)
    {
        d->mSliderLabel->setStyleSheet("QLabel { color : white; }");
    }
    else
    {
        d->mSliderLabel->setStyleSheet("QLabel { color : black; }");
    }
}

void BackgroundPalette::refresh()
{
    backgroundChanged();
    d->mSlider->setValue(BoardController::instance()->activeScene()->backgroundGridSize());
}
