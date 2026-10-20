#include "ui/ToolbarButtonGroup.h"

#include <QtWidgets>
#include <QLayout>
#include <QStyleOption>

#include "core/Application.h"
#include "settings/Settings.h"

#include "whiteboard/DrawingController.h"

ToolbarButtonGroup::ToolbarButtonGroup(QToolBar *toolBar, const QList<QAction *> &actions)
    : QWidget(toolBar), mActions(actions), mCurrentIndex(-1), mDisplayLabel(true), mActionGroup(0)
{
    Q_ASSERT(actions.size() > 0);

    // 工具栏原有的第一个按钮只作为尺寸与字体模板，之后由本组按钮取代。
    mToolButton = qobject_cast<QToolButton *>(toolBar->layout()->itemAt(0)->widget());
    Q_ASSERT(mToolButton);

    QVBoxLayout *verticalLayout   = new QVBoxLayout(this);
    QHBoxLayout *horizontalLayout = new QHBoxLayout();
    horizontalLayout->setSpacing(0);
    verticalLayout->addStretch();
    verticalLayout->addLayout(horizontalLayout);
    verticalLayout->addStretch();

    mActionGroup = new QActionGroup(this);
    mActionGroup->setExclusive(true);

    QSize buttonSize;

    int i = 0;

    foreach (QAction *action, actions)
    {
        mActionGroup->addAction(action);

        QToolButton *button = new QToolButton(this);
        mButtons.append(button);
        button->setDefaultAction(action);
        button->setCheckable(true);

        if (i == 0)
        {
            button->setObjectName("buttonGroupLeft");
        }
        else if (i == actions.size() - 1)
        {
            button->setObjectName("buttonGroupRight");
        }
        else
        {
            button->setObjectName("buttonGroupCenter");
        }

        connect(button, &QToolButton::triggered, this, &ToolbarButtonGroup::selected);

        horizontalLayout->addWidget(button);
        mLabel     = action->text();
        buttonSize = button->sizeHint();
        i++;
    }
}

ToolbarButtonGroup::~ToolbarButtonGroup()
{
    // 刻意留空
}

void ToolbarButtonGroup::setIcon(const QIcon &icon, int index)
{
    Q_ASSERT(index < mActions.size());

    foreach (QWidget *widget, mActions.at(index)->associatedWidgets())
    {
        QToolButton *button = qobject_cast<QToolButton *>(widget);
        if (button)
        {
            button->setIcon(icon);
        }
    }
}

void ToolbarButtonGroup::setColor(const QColor &color, int index)
{
    QPixmap pixmap(12, 12);
    pixmap.fill(color);
    QIcon icon(pixmap);
    setIcon(icon, index);
}

void ToolbarButtonGroup::selected(QAction *action)
{
    foreach (QWidget *widget, action->associatedWidgets())
    {
        QToolButton *button = qobject_cast<QToolButton *>(widget);
        if (button)
        {
            int i = 0;
            foreach (QAction *eachAction, mActions)
            {
                if (eachAction == action)
                {
                    setCurrentIndex(i);
                    emit activated(i);
                    break;
                }
                i++;
            }
        }
    }
}

int ToolbarButtonGroup::currentIndex() const
{
    return mCurrentIndex;
}

void ToolbarButtonGroup::setCurrentIndex(int index)
{
    Q_ASSERT(index < mButtons.size());

    if (index != mCurrentIndex)
    {
        for (int i = 0; i < mButtons.size(); i++)
        {
            mButtons.at(i)->setChecked(i == index);
        }
        mCurrentIndex = index;
        emit currentIndexChanged(index);
    }
}

void ToolbarButtonGroup::paintEvent(QPaintEvent *)
{
    QPainter               painter(this);
    QStyleOptionToolButton option;
    QPixmap                emptyPixmap(32, 32);
    emptyPixmap.fill(Qt::transparent);
    QIcon emptyIcon(emptyPixmap);

    option.initFrom(mToolButton);

    option.text   = mLabel;
    option.font   = mToolButton->font();

    int pointSize = mToolButton->font().pointSize();
    if (pointSize > 0)
    {
        option.font.setPointSize(pointSize);
    }
    else
    {
        int pixelSize = mToolButton->font().pixelSize();
        if (pixelSize > 0)
        {
            option.font.setPixelSize(pixelSize);
        }
    }

    option.rect            = rect();
    // 空图标是必需的占位：样式绘制 CE_ToolButtonLabel 时若无图标会改变文字布局。
    option.icon            = emptyIcon;
    option.iconSize        = emptyPixmap.size();
    option.toolButtonStyle = mDisplayLabel ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonIconOnly;

    style()->drawControl(QStyle::CE_ToolButtonLabel, &option, &painter, this);
}

void ToolbarButtonGroup::colorPaletteChanged()
{
    bool isDarkBackground = Settings::settings()->isDarkBackground();

    QList<QColor> colors;

    if (DrawingController::drawingController()->stylusTool() == StylusTool::Pen ||
        DrawingController::drawingController()->stylusTool() == StylusTool::Line)
    {
        colors = Settings::settings()->penColors(isDarkBackground);
    }
    else if (DrawingController::drawingController()->stylusTool() == StylusTool::Marker)
    {
        colors = Settings::settings()->markerColors(isDarkBackground);
    }

    for (int i = 0; i < mButtons.size() && i < colors.size(); i++)
    {
        setColor(colors.at(i), i);
    }
}

void ToolbarButtonGroup::displayText(QVariant display)
{
    mDisplayLabel               = display.toBool();
    // 索引 2 是底部弹簧：显示文字时保留伸展，为按钮下方的文字留出空间。
    QVBoxLayout *verticalLayout = (QVBoxLayout *)layout();
    verticalLayout->setStretch(2, mDisplayLabel ? 1 : 0);
    update();
}
