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
    // 先以外部 parent 创建，addWidget 时布局会把它们重新归属到本部件。
    d->mSpinningWheel = new SpinningWheel(parent);
    d->mLabel = new QLabel(parent);
    d->mLabel->setStyleSheet(QString("QLabel { color: white; background-color: transparent; border: none; "
                                     "font-family: Arial; font-size: 14px }"));

    d->mOriginalAlpha = 255;

    d->mLayout->setContentsMargins(radius() + 15, 4, radius() + 15, 4);

    d->mLayout->addWidget(d->mSpinningWheel);
    d->mLayout->addWidget(d->mLabel);
}

MessageWindow::~MessageWindow() {
    // 刻意留空
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
    // 调用可能发生在耗时操作之前，这里立即重绘并处理挂起事件，
    // 确保消息在界面被阻塞前已经出现在屏幕上。
    repaint();
    QCoreApplication::sendPostedEvents();
}

void MessageWindow::timerEvent(QTimerEvent *event) {
    Q_UNUSED(event);

    // 每 50ms 递减一档，最后十档开始降低背景透明度，形成淡出。
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
