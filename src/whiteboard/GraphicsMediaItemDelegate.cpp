#include <QtWidgets>
#include <QtSvg>
#include <QtSvgWidgets>

#include "whiteboard/GraphicsMediaItem.h"
#include "whiteboard/GraphicsMediaItemDelegate.h"
#include "whiteboard/GraphicsDelegateFrame.h"

#include "whiteboard/GraphicsScene.h"

#include "settings/Settings.h"
#include "core/Application.h"
#include "core/ApplicationController.h"
#include "core/DisplayManager.h"

#include "whiteboard/GraphicsMediaItem.h"


GraphicsMediaItemDelegate::GraphicsMediaItemDelegate(GraphicsMediaItem *pDelegated, QObject *parent)
    : GraphicsItemDelegate(pDelegated, parent, GF_COMMON | GF_RESPECT_RATIO | GF_TOOLBAR_USED),
      mPlayPauseButton(NULL),
      mToolBarShowTimer(NULL),
      m_iToolBarShowingInterval(5000) {
    QPalette palette;
    palette.setBrush(QPalette::Light, Qt::darkGray);

    if (delegated()->isMuted())
        delegated()->setMute(true);
}

bool GraphicsMediaItemDelegate::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    Q_UNUSED(event);
    mToolBarItem->show();
    positionHandles();

    if (mToolBarShowTimer)
        mToolBarShowTimer->start();

    return GraphicsItemDelegate::mousePressEvent(event);
}

/**
 * @brief Show the toolbar (play/pause, seek, mute).
 *
 * The toolbar then auto-hides after a set amount of time, if the video is currently
 * playing or is paused.
 */
void GraphicsMediaItemDelegate::showToolBar(bool autohide) {
    mToolBarItem->show();
    if (mToolBarShowTimer) {
        if (delegated()->isPlaying() || delegated()->isPaused())
            mToolBarShowTimer->start();
        else
            mToolBarShowTimer->stop();

        // Don't hide the toolbar if we're at the beginning of the video
        if (delegated()->mediaPosition() == delegated()->initialPos())
            mToolBarShowTimer->stop();

        // Don't hide the toolbar if it was explicitly requested
        if (!autohide)
            mToolBarShowTimer->stop();
    }
}

void GraphicsMediaItemDelegate::hideToolBar() {
    mToolBarItem->hide();
}

void GraphicsMediaItemDelegate::buildButtons() {
    if (!mPlayPauseButton) {
        mPlayPauseButton =
            new DelegateButton(":/images/play.svg", mDelegated, mToolBarItem, Qt::TitleBarArea);
        connect(mPlayPauseButton, &DelegateButton::clicked, this,
                &GraphicsMediaItemDelegate::togglePlayPause);

        mStopButton = new DelegateButton(":/images/stop.svg", mDelegated, mToolBarItem, Qt::TitleBarArea);
        connect(mStopButton, &DelegateButton::clicked, delegated(), &GraphicsMediaItem::stop);

        mMediaControl = new DelegateMediaControl(delegated(), mToolBarItem);
        mMediaControl->setFlag(QGraphicsItem::ItemIsSelectable, true);
        GraphicsItem::assignZValue(mMediaControl, delegated()->zValue());

        if (delegated()->isMuted())
            mMuteButton =
                new DelegateButton(":/images/soundOff.svg", mDelegated, mToolBarItem, Qt::TitleBarArea);
        else
            mMuteButton =
                new DelegateButton(":/images/soundOn.svg", mDelegated, mToolBarItem, Qt::TitleBarArea);

        connect(mMuteButton, &DelegateButton::clicked, delegated(), &GraphicsMediaItem::toggleMute);
        connect(mMuteButton, &DelegateButton::clicked, this,
                &GraphicsMediaItemDelegate::toggleMute); // for changing button image

        mToolBarButtons << mPlayPauseButton << mStopButton << mMuteButton;

        mToolBarItem->setItemsOnToolBar(QList<QGraphicsItem *>()
                                        << mPlayPauseButton << mStopButton << mMediaControl << mMuteButton);
        mToolBarItem->setVisibleOnBoard(true);
        mToolBarItem->setShifting(false);

        if (!mToolBarShowTimer) {
            if (delegated()->hasLinkedImage()) {
                mToolBarShowTimer = new QTimer();
                mToolBarShowTimer->setInterval(m_iToolBarShowingInterval);
                connect(mToolBarShowTimer, &QTimer::timeout, this, &GraphicsMediaItemDelegate::hideToolBar);
            }
        }

        else {
            connect(mPlayPauseButton, &DelegateButton::clicked, mToolBarShowTimer,
                    qOverload<>(&QTimer::start));

            connect(mStopButton, &DelegateButton::clicked, mToolBarShowTimer, qOverload<>(&QTimer::start));

            connect(mMediaControl, &DelegateMediaControl::used, mToolBarShowTimer,
                    qOverload<>(&QTimer::start));

            connect(mMuteButton, &DelegateButton::clicked, mToolBarShowTimer, qOverload<>(&QTimer::start));
        }


        positionHandles();
    }
}

GraphicsMediaItemDelegate::~GraphicsMediaItemDelegate() {
    if (mToolBarShowTimer) {
        delete mToolBarShowTimer;
        mToolBarShowTimer = NULL;
    }
}

void GraphicsMediaItemDelegate::positionHandles() {
    GraphicsItemDelegate::positionHandles();

    GraphicsMediaItem *mediaItem = dynamic_cast<GraphicsMediaItem *>(mDelegated);
    if (mediaItem) {
        QRectF toolBarRect = mToolBarItem->rect();

        mToolBarItem->setPos(0, mediaItem->boundingRect().height() - mToolBarItem->rect().height());

        toolBarRect.setWidth(mediaItem->boundingRect().width());
        mToolBarItem->show();

        mToolBarItem->setRect(toolBarRect);
    }

    int toolBarButtonsWidth = 0;
    foreach (DelegateButton *button, mToolBarButtons)
        toolBarButtonsWidth += button->boundingRect().width() + mToolBarItem->getElementsPadding();

    QRectF mediaItemRect = mMediaControl->rect();
    mediaItemRect.setWidth(mediaItem->boundingRect().width() - toolBarButtonsWidth);
    mediaItemRect.setHeight(mToolBarItem->boundingRect().height());
    mMediaControl->setRect(mediaItemRect);

    mToolBarItem->positionHandles();
    mMediaControl->positionHandles();

    if (mediaItem)
        mToolBarItem->show();
}

void GraphicsMediaItemDelegate::remove(bool canUndo) {
    if (delegated())
        delegated()->stop();

    GraphicsItemDelegate::remove(canUndo);
}


void GraphicsMediaItemDelegate::toggleMute() {
    if (delegated()->isMuted())
        mMuteButton->setFileName(":/images/soundOff.svg");
    else
        mMuteButton->setFileName(":/images/soundOn.svg");
}


GraphicsMediaItem *GraphicsMediaItemDelegate::delegated() {
    return dynamic_cast<GraphicsMediaItem *>(mDelegated);
}

void GraphicsMediaItemDelegate::togglePlayPause() {
    if (delegated())
        delegated()->togglePlayPause();
}

void GraphicsMediaItemDelegate::mediaStatusChanged(QMediaPlayer::MediaStatus status) {
    // Possible statuses are: UnknownMediaStatus, NoMedia, LoadingMedia, LoadedMedia,
    // StalledMedia, BufferingMedia, BufferedMedia, EndOfMedia, InvalidMedia

    if (status == QMediaPlayer::LoadedMedia)
        mMediaControl->totalTimeChanged(delegated()->mediaDuration());

    // At the beginning of the video, play/pause to load and display the first frame
    if ((status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia) &&
        delegated()->mediaPosition() == delegated()->initialPos() && !delegated()->isStopped()) {
        delegated()->play();
        delegated()->pause();
    }

    // At the end of the video, make sure the progress bar doesn't autohide
    if (status == QMediaPlayer::EndOfMedia)
        showToolBar(false);


    // in most cases, the only necessary action is to update the play/pause state
    updatePlayPauseState();
}

void GraphicsMediaItemDelegate::mediaStateChanged(QMediaPlayer::PlaybackState state) {
    Q_UNUSED(state);
    // Possible states are StoppedState, PlayingState and PausedState

    // updatePlayPauseState handles this functionality
    updatePlayPauseState();
}


void GraphicsMediaItemDelegate::updatePlayPauseState() {
    if (delegated()->playerState() == QMediaPlayer::PlayingState)
        mPlayPauseButton->setFileName(":/images/pause.svg");
    else
        mPlayPauseButton->setFileName(":/images/play.svg");
}


void GraphicsMediaItemDelegate::updateTicker(qint64 time) {
    mMediaControl->totalTimeChanged(delegated()->mediaDuration());
    mMediaControl->updateTicker(time);
}


void GraphicsMediaItemDelegate::totalTimeChanged(qint64 newTotalTime) {
    mMediaControl->totalTimeChanged(newTotalTime);
}

void GraphicsMediaItemDelegate::showHide(bool show) {
    QVariant showFlag = QVariant(show ? ItemLayerType::Object : ItemLayerType::Control);
    showHideRecurs(showFlag, mDelegated);
    mDelegated->update();

    emit showOnDisplayChanged(show);
}
