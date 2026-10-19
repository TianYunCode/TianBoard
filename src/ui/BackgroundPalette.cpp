#include "ui/BackgroundPalette.h"

#include "ui/MainWindow.h"

struct BackgroundPalette::Private
{
    QVBoxLayout* mVLayout{nullptr};
    QHBoxLayout* mTopLayout{nullptr};
    QHBoxLayout* mBottomLayout{nullptr};

    QSlider* mSlider{nullptr};
    QLabel* mSliderLabel{nullptr};
    ActionPaletteButton* mResetDefaultGridSizeButton{nullptr};
};

BackgroundPalette::BackgroundPalette(QList<QAction*> actions, QWidget * parent)
    : ActionPalette(parent)
    , d(new Private)
{
    init();
    setActions(actions);
}


BackgroundPalette::BackgroundPalette(QWidget * parent)
     : ActionPalette(parent)
     , d(new Private)
{
    init();
}


BackgroundPalette::~BackgroundPalette() = default;


void BackgroundPalette::init()
{
    ActionPalette::clearLayout();
    delete layout();

    initButtonState();

    d->mVLayout = new QVBoxLayout(this);
    d->mTopLayout = new QHBoxLayout();
    d->mBottomLayout = new QHBoxLayout();

    d->mVLayout->addLayout(d->mTopLayout);
    d->mVLayout->addLayout(d->mBottomLayout);

    d->mSlider = new QSlider(Qt::Horizontal);

    d->mSlider->setMinimum(Settings::settings()->minCrossSize);
    d->mSlider->setMaximum(Settings::settings()->maxCrossSize);
    d->mSlider->setSingleStep(2);
    d->mSlider->setTracking(true); // valueChanged() is emitted during movement and not just upon releasing the slider

    d->mSliderLabel = new QLabel(tr("Grid size"));

    d->mResetDefaultGridSizeButton = createPaletteButton(Application::mainWindow->ui().actionDefaultGridSize, this);
    d->mResetDefaultGridSizeButton->setFixedSize(24,24);
    appendAction(Application::mainWindow->ui().actionDefaultGridSize);

    connect(Application::mainWindow->ui().actionDefaultGridSize, SIGNAL(triggered()), this, SLOT(defaultBackgroundGridSize()));

    d->mBottomLayout->addSpacing(16);
    d->mBottomLayout->addWidget(d->mSliderLabel);
    d->mBottomLayout->addWidget(d->mSlider);
    d->mBottomLayout->addWidget(d->mResetDefaultGridSizeButton);
    d->mBottomLayout->addSpacing(16);

    updateLayout();
}

void BackgroundPalette::addAction(QAction* action)
{
    ActionPaletteButton* button = createPaletteButton(action, this);

    d->mTopLayout->addWidget(button);
    appendAction(action);
}

void BackgroundPalette::setActions(QList<QAction*> actions)
{
    clearButtonMap();

    foreach(QAction* action, actions)
    {
        addAction(action);
    }

    actionChanged();
}

void BackgroundPalette::updateLayout()
{
    if (toolButtonStyle() == Qt::ToolButtonIconOnly) {
        d->mVLayout->setContentsMargins (sLayoutContentMargin / 2  + border(), sLayoutContentMargin / 2  + border()
                , sLayoutContentMargin / 2  + border(), sLayoutContentMargin / 2  + border());
    }
    else
    {
        d->mVLayout->setContentsMargins (sLayoutContentMargin  + border(), sLayoutContentMargin  + border()
                , sLayoutContentMargin  + border(), sLayoutContentMargin + border());

    }
   update();
}

void BackgroundPalette::clearLayout()
{
    while(!d->mTopLayout->isEmpty()) {
        QLayoutItem* pItem = d->mTopLayout->itemAt(0);
        QWidget* pW = pItem->widget();
        d->mTopLayout->removeItem(pItem);
        delete pItem;
        d->mTopLayout->removeWidget(pW);
        delete pW;
    }

    delete d->mTopLayout;

    while(!d->mBottomLayout->isEmpty()) {
        QLayoutItem* pItem = d->mBottomLayout->itemAt(0);
        QWidget* pW = pItem->widget();
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

void BackgroundPalette::showEvent(QShowEvent* event)
{
    backgroundChanged();

    d->mSlider->setValue(Application::boardController->activeScene()->backgroundGridSize());
    connect(d->mSlider, SIGNAL(valueChanged(int)),
            this, SLOT(sliderValueChanged(int)));

    QWidget::showEvent(event);
}

void BackgroundPalette::sliderValueChanged(int value)
{
    Application::boardController->activeScene()->setBackgroundGridSize(value);
    Settings::settings()->crossSize = value; // since this function is called (indirectly, by refresh) when we switch scenes, the settings will always have the current scene's cross size.
}

void BackgroundPalette::defaultBackgroundGridSize()
{
    d->mSlider->setValue(Settings::settings()->defaultCrossSize);
    sliderValueChanged(Settings::settings()->defaultCrossSize);
}

void BackgroundPalette::backgroundChanged()
{
    bool dark = Application::boardController->activeScene()->isDarkBackground();

    if (dark)
        d->mSliderLabel->setStyleSheet("QLabel { color : white; }");
    else
        d->mSliderLabel->setStyleSheet("QLabel { color : black; }");
}

void BackgroundPalette::refresh()
{
    backgroundChanged();
    d->mSlider->setValue(Application::boardController->activeScene()->backgroundGridSize());
}
