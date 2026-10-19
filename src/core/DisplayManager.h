#pragma once

#include <QtWidgets>

class BlackoutWidget;
class BoardView;
class VirtualDesktop;

class DisplayManager : public QObject
{
    Q_OBJECT

    public:
        DisplayManager(QObject *parent = 0);
        virtual ~DisplayManager();

        int numScreens();

        int numPreviousViews();

        void setControlWidget(QWidget* pControlWidget);

        void setDisplayWidget(QWidget* pDisplayWidget);

        void setDesktopWidget(QWidget* pControlWidget);

        void setPreviousDisplaysWidgets(QList<BoardView*> pPreviousViews);

        bool hasControl()
        {
            return mControlScreenIndex > -1;
        }

        bool hasDisplay()
        {
            return mDisplayScreenIndex > -1;
        }

        bool hasPrevious()
        {
            return !mPreviousScreenIndexes.isEmpty();
        }

        enum DisplayRole
        {
            None = 0, Control, Display, Previous1, Previous2, Previous3, Previous4, Previous5
        };

        bool useMultiScreen() { return mUseMultiScreen; }

        void setUseMultiScreen(bool pUse);

        int controleScreenIndex()
        {
            return mControlScreenIndex;
        }

        int displayScreenIndex() const
        {
            return mDisplayScreenIndex;
        }

        QRect controlGeometry();
        QRect displayGeometry();

   signals:
        void screenLayoutChanged();
        void adjustDisplayViewsRequired();

   public slots:
        void reinitScreens(bool bswap);

        void adjustScreens(int screen);

        void blackout();

        void unBlackout();

        void setRoleToScreen(DisplayRole role, int screenIndex);

        void swapDisplayScreens(bool swap);

    private:
        void positionScreens();

        void initScreenIndexes();

        int mControlScreenIndex;

        int mDisplayScreenIndex;

        QList<int> mPreviousScreenIndexes;

        VirtualDesktop* mDesktop;

        QWidget* mControlWidget;

        QWidget* mDisplayWidget;

        QWidget *mDesktopWidget;

        QList<BoardView*> mPreviousDisplayWidgets;

        QList<BlackoutWidget*> mBlackoutWidgets;

        QList<DisplayRole> mScreenIndexesRoles;

        bool mUseMultiScreen;

};

