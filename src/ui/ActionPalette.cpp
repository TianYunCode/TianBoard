#include <QLayout>
#include <QAction>

#include "ui/ActionPalette.h"


struct ActionPalette::Private
{
    QList<ActionPaletteButton*> mButtons;
    QButtonGroup* mButtonGroup{nullptr};
    QList<QAction*> mActions;
    QMap<QAction*, ActionPaletteButton*> mMapActionToButton;
    bool mIsClosable{false};
    Qt::ToolButtonStyle mToolButtonStyle{Qt::ToolButtonIconOnly};
    bool mAutoClose{false};
    QSize mButtonSize;
    QPoint mMousePos;
    bool m_customCloseProcessing{false};
};

ActionPalette::ActionPalette(QList<QAction*> actions, Qt::Orientation orientation, QWidget * parent)
    : FloatingPalette(Qt::TopRightCorner, parent)
    , d(new Private)
{
    init(orientation);
    setActions(actions);
}


ActionPalette::ActionPalette(Qt::Orientation orientation, QWidget * parent)
     : FloatingPalette(Qt::TopRightCorner, parent)
     , d(new Private)
{
    init(orientation);
}


ActionPalette::ActionPalette(QWidget * parent)
     : FloatingPalette(Qt::TopRightCorner, parent)
     , d(new Private)
{
    init(Qt::Vertical);
}


ActionPalette::ActionPalette(Qt::Corner corner, QWidget * parent, Qt::Orientation orient)
     : FloatingPalette(corner, parent)
     , d(new Private)
{
    init(orient);
}


void ActionPalette::init(Qt::Orientation orientation)
{
    initButtonState();

    if (orientation == Qt::Horizontal)
        new QHBoxLayout(this);
    else
        new QVBoxLayout(this);

    updateLayout();
}

void ActionPalette::initButtonState()
{
    d->m_customCloseProcessing = false;

    d->mButtonSize = QSize(32, 32);
    d->mIsClosable = false;
    d->mAutoClose = false;
    d->mButtonGroup = 0;
    d->mToolButtonStyle = Qt::ToolButtonIconOnly;
    d->mButtons.clear();
}

void ActionPalette::setAutoClose(bool autoClose)
{
    d->mAutoClose = autoClose;
}

void ActionPalette::setCustomCloseProcessing(bool customCloseProcessing)
{
    d->m_customCloseProcessing = customCloseProcessing;
}

const QList<ActionPaletteButton*>& ActionPalette::buttons() const
{
    return d->mButtons;
}

void ActionPalette::clearButtons()
{
    d->mButtons.clear();
}

QButtonGroup* ActionPalette::buttonGroup() const
{
    return d->mButtonGroup;
}

void ActionPalette::setButtonGroup(QButtonGroup* group)
{
    d->mButtonGroup = group;
}

Qt::ToolButtonStyle ActionPalette::toolButtonStyle() const
{
    return d->mToolButtonStyle;
}

void ActionPalette::appendAction(QAction* action)
{
    d->mActions << action;
}

void ActionPalette::clearActions()
{
    d->mActions.clear();
}

void ActionPalette::clearButtonMap()
{
    d->mMapActionToButton.clear();
}

void ActionPalette::setActions(QList<QAction*> actions)
{
    clearButtonMap();

    foreach(QAction* action, actions)
    {
        addAction(action);
    }

    actionChanged();
}


ActionPaletteButton* ActionPalette::createPaletteButton(QAction* action, QWidget *parent)
{
    ActionPaletteButton* button = new ActionPaletteButton(action, parent);
    button->setIconSize(d->mButtonSize);
    button->setToolButtonStyle(d->mToolButtonStyle);

    if (d->mButtonGroup)
        d->mButtonGroup->addButton(button, d->mButtons.length());

    d->mButtons << button;

    d->mMapActionToButton[action] = button;

    connect(button, &ActionPaletteButton::clicked,
            this, &ActionPalette::buttonClicked);
    connect(action, &QAction::changed,
            this, &ActionPalette::actionChanged);

    return button;
}

void ActionPalette::addAction(QAction* action)
{
    ActionPaletteButton* button = createPaletteButton(action, this);

    layout()->addWidget(button);

    appendAction(action);
}

void ActionPalette::buttonClicked()
{
    if (d->mAutoClose)
    {
        close();
    }
}

QList<QAction*> ActionPalette::actions()
{
    return d->mActions;
}


ActionPalette::~ActionPalette()
{
    qDeleteAll(d->mButtons.begin(), d->mButtons.end());
    d->mButtons.clear();
}


void ActionPalette::setButtonIconSize(const QSize& size)
{
    foreach(QToolButton* button, d->mButtons)
        button->setIconSize(size);

    d->mButtonSize = size;
}


void ActionPalette::groupActions()
{
    setButtonGroup(new QButtonGroup(this));
    int i = 0;
    foreach(QToolButton* button, d->mButtons)
    {
        d->mButtonGroup->addButton(button, i);
        ++i;
    }

    connect(d->mButtonGroup, &QButtonGroup::idClicked, this, &ActionPalette::buttonGroupClicked);
}


void ActionPalette::setToolButtonStyle(Qt::ToolButtonStyle tbs)
{
    foreach(QToolButton* button, d->mButtons)
        button->setToolButtonStyle(tbs);

    d->mToolButtonStyle = tbs;

    updateLayout();

}

void ActionPalette::updateLayout()
{
    if (d->mToolButtonStyle == Qt::ToolButtonIconOnly)
    {
        layout()->setContentsMargins (sLayoutContentMargin / 2  + border(), sLayoutContentMargin / 2  + border()
                , sLayoutContentMargin / 2  + border(), sLayoutContentMargin / 2  + border());
    }
    else
    {
        layout()->setContentsMargins (sLayoutContentMargin  + border(), sLayoutContentMargin  + border()
                , sLayoutContentMargin  + border(), sLayoutContentMargin + border());

    }
   update();
}


void ActionPalette::setClosable(bool pClosable)
{
    d->mIsClosable = pClosable;

    updateLayout();
}


int ActionPalette::border()
{
    if (d->mIsClosable)
        return 10;
    else
        return 5;
}


void ActionPalette::paintEvent(QPaintEvent *event)
{
    FloatingPalette::paintEvent(event);

    if (d->mIsClosable)
    {
        QPainter painter(this);
        painter.drawPixmap(0, 0, QPixmap(":/images/close.svg"));
    }
}


void ActionPalette::close()
{
    if(!d->m_customCloseProcessing)
        hide();

    emit closed();
}


void ActionPalette::mouseReleaseEvent(QMouseEvent * event)
{
    if (d->mIsClosable && event->pos().x() >= 0 && event->pos().x() < QPixmap(":/images/close.svg").width()
        && event->pos().y() >= 0 && event->pos().y() < QPixmap(":/images/close.svg").height())
    {
        event->accept();
        close();
    }

    FloatingPalette::mouseReleaseEvent(event);
}


void ActionPalette::actionChanged()
{
    for(int i = 0; i < d->mActions.length() && i < d->mButtons.length(); i++)
    {
        d->mButtons.at(i)->setVisible(d->mActions.at(i)->isVisible());
    }
}

void ActionPalette::clearLayout()
{
    QLayout* pLayout = layout();
    if(NULL != pLayout)
    {
        while(!pLayout->isEmpty())
        {
            QLayoutItem* pItem = pLayout->itemAt(0);
            QWidget* pW = pItem->widget();
            pLayout->removeItem(pItem);
            delete pItem;
            pLayout->removeWidget(pW);
            delete pW;
        }

        clearActions();
        clearButtons();
    }
}

ActionPaletteButton::ActionPaletteButton(QAction* action, QWidget * parent)
    : QToolButton(parent)
{
    setIconSize(QSize(32, 32));
    setDefaultAction(action);
    setStyleSheet(QString("QToolButton {color: white; font-weight: bold; font-family: Arial; background-color: transparent; border: none}"));

    setFocusPolicy(Qt::NoFocus);

    setObjectName("actionPaletteButton");
}


ActionPaletteButton::~ActionPaletteButton()
{

}


void ActionPaletteButton::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);

    emit doubleClicked();
}

QSize ActionPalette::buttonSize()
{
    return d->mButtonSize;
}

/**
 * \brief Returns the button related to the given action
 * @param action as the given action
 */
ActionPaletteButton* ActionPalette::getButtonFromAction(QAction *action)
{
    ActionPaletteButton* pButton = NULL;

    pButton = d->mMapActionToButton.value(action);

    return pButton;
}

bool ActionPaletteButton::hitButton(const QPoint &pos) const
{
    Q_UNUSED(pos);
    return true;
}
