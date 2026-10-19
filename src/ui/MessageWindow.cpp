#include "ui/MessageWindow.h"

#include "ui/SpinningWheel.h"


struct MessageWindow::Private {
    QHBoxLayout *mLayout{nullptr};
    SpinningWheel *mSpinningWheel{nullptr};
    QLabel *mLabel{nullptr};
    QBasicTimer mTimer;

    int mOriginalAlpha{0};
    int mFadingStep{0};

    int mTimerID{0};
};

MessageWindow::MessageWindow(QWidget *parent)
    : FloatingPalette(Qt::BottomLeftCorner, parent), d(new Private) {
    d->mTimerID = -1;

    d->mLayout = new QHBoxLayout(this);
    d->mSpinningWheel = new SpinningWheel(parent);
    d->mLabel = new QLabel(parent);
    d->mLabel->setStyleSheet(QString("QLabel { color: white; background-color: transparent; border: none; "
                                     "font-family: Arial; font-size: 14px }"));

    d->mOriginalAlpha = 255;

    d->mLayout->setContentsMargins(radius() + 15, 4, radius() + 15, 4);

#ifdef Q_OS_OSX
    d->mLayout->setContentsMargins(radius() + 15, 8, radius() + 15, 10);
#endif

    d->mLayout->addWidget(d->mSpinningWheel);
    d->mLayout->addWidget(d->mLabel);
}

MessageWindow::~MessageWindow() {
    // NOOP
}

void MessageWindow::showMessage(const QString &message, bool showSpinningWheel) {
    d->mTimer.stop();

    d->mLabel->setText(message);

    setBackgroundAlpha(d->mOriginalAlpha);
    d->mFadingStep = 25;

    if (showSpinningWheel) {
        d->mSpinningWheel->show();
        d->mSpinningWheel->startAnimation();
    } else {
        d->mSpinningWheel->hide();
        d->mSpinningWheel->stopAnimation();
        d->mTimer.start(50, this);
    }
    adjustSizeAndPosition();

    show();
    // showMessage may have been called from the GUI thread, so make sure the message window is drawn right now
    repaint();
    // I mean it, *right now*, also on Mac
    QCoreApplication::sendPostedEvents();
}

void MessageWindow::timerEvent(QTimerEvent *event) {
    Q_UNUSED(event);

    d->mFadingStep--;

    if (d->mFadingStep < 1) {
        hide();
        d->mTimer.stop();
        d->mSpinningWheel->stopAnimation();
    } else if (d->mFadingStep < 10) {
        setBackgroundAlpha(d->mOriginalAlpha / 10 * d->mFadingStep);

        update();
    }
}

void MessageWindow::hideMessage() {
    d->mFadingStep = 0;
    hide();
    QCoreApplication::sendPostedEvents();
}
