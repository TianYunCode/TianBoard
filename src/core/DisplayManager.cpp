#include "core/DisplayManager.h"

#include "platform/PlatformUtils.h"

#include "core/Application.h"
#include "core/ApplicationController.h"
#include "settings/Settings.h"

#include "whiteboard/BoardView.h"
#include "whiteboard/BoardController.h"

#include "ui/BlackoutWidget.h"


#include "core/VirtualDesktop.h"

DisplayManager::DisplayManager(QObject *parent)
    : QObject(parent),
      mControlScreenIndex(-1),
      mDisplayScreenIndex(-1),
      mControlWidget(0),
      mDisplayWidget(0),
      mDesktopWidget(0) {
    mDesktop = VirtualDesktop::instance();

    mUseMultiScreen = Settings::settings()->appUseMultiscreen->get().toBool();

    initScreenIndexes();

    connect(mDesktop, &VirtualDesktop::resized, this, &DisplayManager::adjustScreens);
    connect(mDesktop, &VirtualDesktop::workAreaResized, this, &DisplayManager::adjustScreens);
}


void DisplayManager::initScreenIndexes() {
    int screenCount = numScreens();

    mScreenIndexesRoles.clear();

    if (screenCount > 0) {
        mControlScreenIndex = mDesktop->primaryScreen();
        if (screenCount > 1 && Settings::settings()->swapControlAndDisplayScreens->get().toBool()) {
            mControlScreenIndex = mControlScreenIndex ^ 1;
        }

        mScreenIndexesRoles << Control;
    } else {
        mControlScreenIndex = -1;
    }

    if (screenCount > 1 && mUseMultiScreen) {
        mDisplayScreenIndex = mControlScreenIndex != 0 ? 0 : 1;
        mScreenIndexesRoles << Display;
    } else {
        mDisplayScreenIndex = -1;
    }

    mPreviousScreenIndexes.clear();

    if (screenCount > 2) {
        for (int i = 2; i < screenCount; i++) {
            if (mControlScreenIndex == i)
                mPreviousScreenIndexes.append(1);
            else
                mPreviousScreenIndexes.append(i);
        }
    }
}

void DisplayManager::swapDisplayScreens(bool swap) {
    initScreenIndexes();

    if (swap && !mPreviousScreenIndexes.isEmpty()) {
        // As it s a really specific ask and we don't have much time to handle it correctly
        // this code handles only the swap between the main display screen and the first previous one
        int displayScreenIndex = mDisplayScreenIndex;
        mDisplayScreenIndex = mPreviousScreenIndexes.at(0);
        mPreviousScreenIndexes.clear();
        mPreviousScreenIndexes.append(displayScreenIndex);
    }

    positionScreens();

    emit screenLayoutChanged();
    emit adjustDisplayViewsRequired();
}


DisplayManager::~DisplayManager() {
    // NOOP
}


int DisplayManager::numScreens() {
    int screenCount = mDesktop->screenCount();
    return screenCount;
}


int DisplayManager::numPreviousViews() {
    return mPreviousScreenIndexes.size();
}


void DisplayManager::setControlWidget(QWidget *pControlWidget) {
    if (hasControl() && pControlWidget && (pControlWidget != mControlWidget))
        mControlWidget = pControlWidget;
}

void DisplayManager::setDesktopWidget(QWidget *pControlWidget) {
    if (pControlWidget && (pControlWidget != mControlWidget))
        mDesktopWidget = pControlWidget;
}

void DisplayManager::setDisplayWidget(QWidget *pDisplayWidget) {
    if (pDisplayWidget && (pDisplayWidget != mDisplayWidget)) {
        if (mDisplayWidget) {
            mDisplayWidget->hide();
            pDisplayWidget->setGeometry(mDisplayWidget->geometry());
            pDisplayWidget->setWindowFlags(mDisplayWidget->windowFlags());
        }
        mDisplayWidget = pDisplayWidget;
        mDisplayWidget->setGeometry(mDesktop->screenGeometry(mDisplayScreenIndex));
        if (Settings::settings()->appUseMultiscreen->get().toBool())
            PlatformUtils::showFullScreen(mDisplayWidget);
    }
}


void DisplayManager::setPreviousDisplaysWidgets(QList<BoardView *> pPreviousViews) {
    mPreviousDisplayWidgets = pPreviousViews;
}


QRect DisplayManager::controlGeometry() {
    return mDesktop->screenGeometry(mControlScreenIndex);
}

QRect DisplayManager::displayGeometry() {
    return mDesktop->screenGeometry(mDisplayScreenIndex);
}

void DisplayManager::reinitScreens(bool swap) {
    Q_UNUSED(swap);
    adjustScreens(-1);
}

void DisplayManager::adjustScreens(int screen) {
    Q_UNUSED(screen);

    initScreenIndexes();

    positionScreens();

    emit screenLayoutChanged();
}


void DisplayManager::positionScreens() {
    if (mDesktopWidget && mControlScreenIndex > -1) {
        mDesktopWidget->hide();
        mDesktopWidget->setGeometry(mDesktop->screenGeometry(mControlScreenIndex));
    }
    if (mControlWidget && mControlScreenIndex > -1) {
        mControlWidget->hide();
        mControlWidget->setGeometry(mDesktop->screenGeometry(mControlScreenIndex));
        PlatformUtils::showFullScreen(mControlWidget);
    }

    if (mDisplayWidget && mDisplayScreenIndex > -1) {
        mDisplayWidget->hide();
        mDisplayWidget->setGeometry(mDesktop->screenGeometry(mDisplayScreenIndex));
        PlatformUtils::showFullScreen(mDisplayWidget);
    } else if (mDisplayWidget) {
        mDisplayWidget->hide();
    }

    for (int wi = mPreviousScreenIndexes.size(); wi < mPreviousDisplayWidgets.size(); wi++) {
        mPreviousDisplayWidgets.at(wi)->hide();
    }

    for (int psi = 0; psi < mPreviousScreenIndexes.size(); psi++) {
        if (mPreviousDisplayWidgets.size() > psi) {
            QWidget *previous = mPreviousDisplayWidgets.at(psi);
            previous->setGeometry(mDesktop->screenGeometry(mPreviousScreenIndexes.at(psi)));
            PlatformUtils::showFullScreen(previous);
        }
    }

    if (mControlWidget && mControlScreenIndex > -1)
        mControlWidget->activateWindow();
}


void DisplayManager::blackout() {
    QList<int> screenIndexes;

    if (mControlScreenIndex > -1)
        screenIndexes << mControlScreenIndex;

    if (mDisplayScreenIndex > -1)
        screenIndexes << mDisplayScreenIndex;

    screenIndexes << mPreviousScreenIndexes;

    for (int i = 0; i < screenIndexes.size(); i++) {
        int screenIndex = screenIndexes.at(i);

        BlackoutWidget *blackoutWidget = new BlackoutWidget(); //deleted in DisplayManager::unBlackout
        connect(blackoutWidget, &BlackoutWidget::activity, this, &DisplayManager::unBlackout);

        // display logo on main screen
        blackoutWidget->setReturnControlsVisible(screenIndex == mControlScreenIndex);

        blackoutWidget->setGeometry(mDesktop->screenGeometry(screenIndex));

        mBlackoutWidgets << blackoutWidget;
    }


    foreach (BlackoutWidget *blackoutWidget, mBlackoutWidgets) {
        PlatformUtils::showFullScreen(blackoutWidget);
    }
}

void DisplayManager::unBlackout() {
    while (!mBlackoutWidgets.isEmpty()) {
        // the widget is also destroyed thanks to its Qt::WA_DeleteOnClose attribute
        mBlackoutWidgets.takeFirst()->close();
    }


    Application::boardController->freezeWebWidgets(false);
}


void DisplayManager::setRoleToScreen(DisplayRole role, int screenIndex) {
    Q_UNUSED(role);
    Q_UNUSED(screenIndex);
}


void DisplayManager::setUseMultiScreen(bool pUse) {
    mUseMultiScreen = pUse;
}
