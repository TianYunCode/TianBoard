#include "ui/DockPaletteWidget.h"

struct DockPaletteWidget::Private
{
    QPixmap mIconToRight;
    QPixmap mIconToLeft;
    QString mName;

    QVector<DockWidgetMode> mRegisteredModes;

    bool mVisibleState{false};
};

DockPaletteWidget::DockPaletteWidget(QWidget *parent, const char *name) : QWidget(parent), d(new Private)
{
    setObjectName(name);
}

DockPaletteWidget::~DockPaletteWidget()
{
}

QPixmap DockPaletteWidget::iconToRight()
{
    return d->mIconToRight;
}

QPixmap DockPaletteWidget::iconToLeft()
{
    return d->mIconToLeft;
}

QString DockPaletteWidget::name()
{
    return d->mName;
}

void DockPaletteWidget::setName(const QString &name)
{
    d->mName = name;
}

void DockPaletteWidget::setIconToRight(const QPixmap &icon)
{
    d->mIconToRight = icon;
}

void DockPaletteWidget::setIconToLeft(const QPixmap &icon)
{
    d->mIconToLeft = icon;
}

bool DockPaletteWidget::visibleState()
{
    return d->mVisibleState;
}

void DockPaletteWidget::setVisibleState(bool state)
{
    d->mVisibleState = state;
}

void DockPaletteWidget::registerMode(DockWidgetMode mode)
{
    // 页面可能被多次注册同一模式，这里去重，避免模式列表膨胀。
    if (!d->mRegisteredModes.contains(mode))
    {
        d->mRegisteredModes.append(mode);
    }
}

void DockPaletteWidget::slot_changeMode(DockWidgetMode newMode)
{
    // 直接按模式设置可见性；停靠面板仍以 visibleState() 决定是否生成标签。
    this->setVisible(this->visibleInMode(newMode));
}
