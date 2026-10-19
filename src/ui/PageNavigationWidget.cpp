#include "ui/PageNavigationWidget.h"
#include "core/Application.h"

#include "whiteboard/BoardController.h"
#include "utils/Globals.h"


/**
 * \brief Constructor
 * @param parent as the parent widget
 * @param name as the object name
 */
PageNavigationWidget::PageNavigationWidget(QWidget *parent, const char *name)
    : DockPaletteWidget(parent),
      mNavigator(NULL),
      mLayout(NULL),
      mHLayout(NULL),
      mPageNbr(NULL),
      mClock(NULL) {
    setObjectName(name);
    setName("PageNavigator");
    setVisibleState(true);

    SET_STYLE_SHEET();

    setIconToRight(QPixmap(":images/general_open.png"));
    setIconToLeft(QPixmap(":images/general_close.png"));

    // Build the gui
    mLayout = new QVBoxLayout(this);
    setLayout(mLayout);

    mNavigator = new DocumentNavigator(this);
    mLayout->addWidget(mNavigator, 1);

    mHLayout = new QHBoxLayout();
    mLayout->addLayout(mHLayout, 0);

    mPageNbr = new QLabel(this);
    mClock = new QLabel(this);
    mHLayout->addWidget(mPageNbr);
    mHLayout->addWidget(mClock);

    // Configure the page number indicator
    mPageNbr->setStyleSheet(QString("QLabel { color: #1296DB; background-color: transparent; border: none; "
                                    "font-family: Arial; font-weight: bold; font-size: 20px }"));
    setPageNumber(0, 0);
    mPageNbr->setAlignment(Qt::AlignHCenter);

    // Configure the clock
    mClock->setStyleSheet(QString("QLabel {color: #1296DB; background-color: transparent; text-align: "
                                  "center; font-family: Arial; font-weight: bold; font-size: 20px}"));
    mTimeFormat = QLocale::system().timeFormat(QLocale::ShortFormat);
    mClock->setAlignment(Qt::AlignHCenter);

    //strip seconds
    mTimeFormat = mTimeFormat.remove(":ss");
    mTimeFormat = mTimeFormat.remove(":s");
    mTimerID = startTimer(1000);
}

/**
 * \brief Destructor
 */
PageNavigationWidget::~PageNavigationWidget() {
    killTimer(mTimerID);

    if (NULL != mClock) {
        delete mClock;
        mClock = NULL;
    }
    if (NULL != mPageNbr) {
        delete mPageNbr;
        mPageNbr = NULL;
    }
    if (NULL != mHLayout) {
        delete mHLayout;
        mHLayout = NULL;
    }
    if (NULL != mLayout) {
        delete mLayout;
        mLayout = NULL;
    }
    if (NULL != mNavigator) {
        delete mNavigator;
        mNavigator = NULL;
    }
}

/**
 * \brief Notify a timer event
 * @param event as the timer event
 */
void PageNavigationWidget::timerEvent(QTimerEvent *event) {
    Q_UNUSED(event);
    updateTime();
}

/**
 * \brief Update the current time
 */
void PageNavigationWidget::updateTime() {
    if (mClock)
        mClock->setText(QLocale::system().toString(QTime::currentTime(), mTimeFormat));
}

/**
 * \brief Set the page number
 * @param current as the current page
 * @param total as the total number of pages
 */
void PageNavigationWidget::setPageNumber(int current, int total) {
    mPageNbr->setText(QString("%1 / %2").arg(current).arg(total));
}

/**
 * \brief Get the custom margin value
 * @return the custom margin value
 */
int PageNavigationWidget::customMargin() {
    return 5;
}

/**
 * \brief Get the border value
 * @return the border value
 */
int PageNavigationWidget::border() {
    return 5;
}
