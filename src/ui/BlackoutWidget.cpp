#include <QtWidgets>

#include "ui/BlackoutWidget.h"


struct BlackoutWidget::Private {
    QGridLayout *gridLayout{nullptr};
    QToolButton *iconButton{nullptr};
    QLabel *labelClickToReturn{nullptr};
};

BlackoutWidget::BlackoutWidget(QWidget *parent) : QWidget(parent), d(new Private) {
    setAttribute(Qt::WA_DeleteOnClose);

    if (objectName().isEmpty())
        setObjectName("BlackoutWidget");
    resize(723, 838);

    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(QColor(0, 0, 0, 255), Qt::SolidPattern));
    setPalette(palette);

    d->gridLayout = new QGridLayout(this);
    d->gridLayout->setObjectName("gridLayout");
    d->gridLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding), 0, 1);

    d->iconButton = new QToolButton(this);
    d->iconButton->setObjectName("iconButton");
    d->iconButton->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed));
    d->iconButton->setMinimumSize(QSize(512, 512));
    d->iconButton->setMaximumSize(QSize(512, 512));
    d->iconButton->setStyleSheet("border: none;");
    d->iconButton->setIcon(QIcon(":/images/TianBoardMark.svg"));
    d->iconButton->setIconSize(QSize(512, 512));
    d->iconButton->setText(QString());
    d->gridLayout->addWidget(d->iconButton, 1, 1);

    d->gridLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding), 2, 1);

    d->labelClickToReturn = new QLabel(this);
    d->labelClickToReturn->setObjectName("labelClickToReturn");
    d->labelClickToReturn->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
    QPalette labelPalette;
    labelPalette.setBrush(QPalette::WindowText, QBrush(QColor(255, 255, 255, 255), Qt::SolidPattern));
    labelPalette.setBrush(QPalette::Disabled, QPalette::WindowText,
                          QBrush(QColor(113, 113, 113, 255), Qt::SolidPattern));
    d->labelClickToReturn->setPalette(labelPalette);
    QFont font;
    font.setPointSize(20);
    d->labelClickToReturn->setFont(font);
    d->labelClickToReturn->setAlignment(Qt::AlignCenter);
    d->labelClickToReturn->setText(tr("Click to Return to Application"));
    d->gridLayout->addWidget(d->labelClickToReturn, 3, 1);

    d->gridLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), 1, 0);
    d->gridLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum), 1, 2);

    connect(d->iconButton, &QToolButton::pressed, this, &BlackoutWidget::doActivity);
}

BlackoutWidget::~BlackoutWidget() = default;

void BlackoutWidget::setReturnControlsVisible(bool visible) {
    d->iconButton->setVisible(visible);
    d->labelClickToReturn->setVisible(visible);
}

void BlackoutWidget::mousePressEvent(QMouseEvent *event) {
    Q_UNUSED(event);
    doActivity();
}


void BlackoutWidget::keyPressEvent(QKeyEvent *event) {
    if (!event->isAccepted()) {
        if (event->key() == Qt::Key_B) {
            doActivity();
            event->accept();
        } else {
            event->ignore();
        }
    }
}


void BlackoutWidget::doActivity() {
    emit activity();
}
