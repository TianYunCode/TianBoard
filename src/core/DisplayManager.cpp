#include "core/DisplayManager.h"

namespace
{
    DisplayManager::BlackoutSurfaceFactory gBlackoutFactory;
} // namespace

void DisplayManager::installBlackoutSurfaceFactory(DisplayManager::BlackoutSurfaceFactory factory)
{
    gBlackoutFactory = std::move(factory);
}

#include "platform/PlatformUtils.h"

#include "core/Application.h"
#include "core/ApplicationController.h"
#include "settings/Settings.h"

#include "whiteboard/BoardView.h"
#include "whiteboard/BoardController.h"

#include "platform/VirtualDesktop.h"

DisplayManager::DisplayManager(QObject *parent)
    : QObject(parent), mControlScreenIndex(-1), mDisplayScreenIndex(-1), mControlWidget(0), mDisplayWidget(0), mDesktopWidget(0)
{
    mDesktop        = VirtualDesktop::instance();

    mUseMultiScreen = Settings::settings()->appUseMultiscreen->get().toBool();

    initScreenIndexes();

    connect(mDesktop, &VirtualDesktop::resized, this, &DisplayManager::adjustScreens);
    connect(mDesktop, &VirtualDesktop::workAreaResized, this, &DisplayManager::adjustScreens);
}

void DisplayManager::initScreenIndexes()
{
    int screenCount = numScreens();

    mScreenIndexesRoles.clear();

    if (screenCount > 0)
    {
        mControlScreenIndex = mDesktop->primaryScreen();
        if (screenCount > 1 && Settings::settings()->swapControlAndDisplayScreens->get().toBool())
        {
            mControlScreenIndex = mControlScreenIndex ^ 1;
        }

        mScreenIndexesRoles << Control;
    }
    else
    {
        mControlScreenIndex = -1;
    }

    if (screenCount > 1 && mUseMultiScreen)
    {
        mDisplayScreenIndex = mControlScreenIndex != 0 ? 0 : 1;
        mScreenIndexesRoles << Display;
    }
    else
    {
        mDisplayScreenIndex = -1;
    }

    mPreviousScreenIndexes.clear();

    if (screenCount > 2)
    {
        for (int i = 2; i < screenCount; i++)
        {
            if (mControlScreenIndex == i)
            {
                mPreviousScreenIndexes.append(1);
            }
            else
            {
                mPreviousScreenIndexes.append(i);
            }
        }
    }
}

void DisplayManager::swapDisplayScreens(bool swap)
{
    initScreenIndexes();

    if (swap && !mPreviousScreenIndexes.isEmpty())
    {
        // 交换逻辑只覆盖“主展示屏与第一块历史屏”，更多屏幕的组合不在支持范围内。
        int displayScreenIndex = mDisplayScreenIndex;
        mDisplayScreenIndex    = mPreviousScreenIndexes.at(0);
        mPreviousScreenIndexes.clear();
        mPreviousScreenIndexes.append(displayScreenIndex);
    }

    positionScreens();

    emit screenLayoutChanged();
    emit adjustDisplayViewsRequired();
}

DisplayManager::~DisplayManager()
{
}

int DisplayManager::numScreens()
{
    int screenCount = mDesktop->screenCount();
    return screenCount;
}

int DisplayManager::numPreviousViews()
{
    return mPreviousScreenIndexes.size();
}

void DisplayManager::setControlWidget(QWidget *pControlWidget)
{
    if (hasControl() && pControlWidget && (pControlWidget != mControlWidget))
    {
        mControlWidget = pControlWidget;
    }
}

void DisplayManager::setDesktopWidget(QWidget *pControlWidget)
{
    if (pControlWidget && (pControlWidget != mControlWidget))
    {
        mDesktopWidget = pControlWidget;
    }
}

void DisplayManager::setDisplayWidget(QWidget *pDisplayWidget)
{
    if (pDisplayWidget && (pDisplayWidget != mDisplayWidget))
    {
        if (mDisplayWidget)
        {
            mDisplayWidget->hide();
            pDisplayWidget->setGeometry(mDisplayWidget->geometry());
            pDisplayWidget->setWindowFlags(mDisplayWidget->windowFlags());
        }
        mDisplayWidget = pDisplayWidget;
        mDisplayWidget->setGeometry(mDesktop->screenGeometry(mDisplayScreenIndex));
        if (Settings::settings()->appUseMultiscreen->get().toBool())
        {
            PlatformUtils::showFullScreen(mDisplayWidget);
        }
    }
}

void DisplayManager::setPreviousDisplaysWidgets(QList<BoardView *> pPreviousViews)
{
    mPreviousDisplayWidgets = pPreviousViews;
}

QRect DisplayManager::controlGeometry()
{
    return mDesktop->screenGeometry(mControlScreenIndex);
}

QRect DisplayManager::displayGeometry()
{
    return mDesktop->screenGeometry(mDisplayScreenIndex);
}

void DisplayManager::reinitScreens(bool swap)
{
    Q_UNUSED(swap);
    adjustScreens(-1);
}

void DisplayManager::adjustScreens(int screen)
{
    Q_UNUSED(screen);

    initScreenIndexes();

    positionScreens();

    emit screenLayoutChanged();
}

void DisplayManager::positionScreens()
{
    if (mDesktopWidget && mControlScreenIndex > -1)
    {
        mDesktopWidget->hide();
        mDesktopWidget->setGeometry(mDesktop->screenGeometry(mControlScreenIndex));
    }
    if (mControlWidget && mControlScreenIndex > -1)
    {
        mControlWidget->hide();
        mControlWidget->setGeometry(mDesktop->screenGeometry(mControlScreenIndex));
        PlatformUtils::showFullScreen(mControlWidget);
    }

    if (mDisplayWidget && mDisplayScreenIndex > -1)
    {
        mDisplayWidget->hide();
        mDisplayWidget->setGeometry(mDesktop->screenGeometry(mDisplayScreenIndex));
        PlatformUtils::showFullScreen(mDisplayWidget);
    }
    else if (mDisplayWidget)
    {
        mDisplayWidget->hide();
    }

    for (int wi = mPreviousScreenIndexes.size(); wi < mPreviousDisplayWidgets.size(); wi++)
    {
        mPreviousDisplayWidgets.at(wi)->hide();
    }

    for (int psi = 0; psi < mPreviousScreenIndexes.size(); psi++)
    {
        if (mPreviousDisplayWidgets.size() > psi)
        {
            QWidget *previous = mPreviousDisplayWidgets.at(psi);
            previous->setGeometry(mDesktop->screenGeometry(mPreviousScreenIndexes.at(psi)));
            PlatformUtils::showFullScreen(previous);
        }
    }

    if (mControlWidget && mControlScreenIndex > -1)
    {
        mControlWidget->activateWindow();
    }
}

void DisplayManager::blackout()
{
    if (!gBlackoutFactory)
    {
        qWarning() << "黑屏表面工厂没有装上，多屏黑屏无法铺满（装配点应调用 installBlackoutSurfaceFactory）";
        return;
    }

    QList<int> screenIndexes;

    if (mControlScreenIndex > -1)
    {
        screenIndexes << mControlScreenIndex;
    }

    if (mDisplayScreenIndex > -1)
    {
        screenIndexes << mDisplayScreenIndex;
    }

    screenIndexes << mPreviousScreenIndexes;

    for (int i = 0; i < screenIndexes.size(); i++)
    {
        int screenIndex = screenIndexes.at(i);

        mBlackoutWidgets << gBlackoutFactory(screenIndex == mControlScreenIndex, mDesktop->screenGeometry(screenIndex),
            [this]
            {
                unBlackout();
            });
    }

    for (QWidget *surface : mBlackoutWidgets)
    {
        PlatformUtils::showFullScreen(surface);
    }
}

void DisplayManager::unBlackout()
{
    while (!mBlackoutWidgets.isEmpty())
    {
        // 控件带 Qt::WA_DeleteOnClose，close() 之后由 Qt 释放。
        mBlackoutWidgets.takeFirst()->close();
    }

    Application::boardController->freezeWebWidgets(false);
}

void DisplayManager::setRoleToScreen(DisplayRole role, int screenIndex)
{
    Q_UNUSED(role);
    Q_UNUSED(screenIndex);
}

void DisplayManager::setUseMultiScreen(bool pUse)
{
    mUseMultiScreen = pUse;
}
