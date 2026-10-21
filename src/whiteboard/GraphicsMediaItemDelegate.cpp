#include <QtWidgets>
#include <QtSvg>
#include <QtSvgWidgets>

#include "whiteboard/GraphicsMediaItem.h"
#include "whiteboard/GraphicsMediaItemDelegate.h"
#include "whiteboard/GraphicsDelegateFrame.h"

#include "whiteboard/GraphicsScene.h"

#include "whiteboard/GraphicsMediaItem.h"

GraphicsMediaItemDelegate::GraphicsMediaItemDelegate(GraphicsMediaItem *pDelegated, QObject *parent)
    : GraphicsItemDelegate(pDelegated, parent, GF_COMMON | GF_RESPECT_RATIO | GF_TOOLBAR_USED), mPlayPauseButton(NULL), mToolBarShowTimer(NULL),
      m_iToolBarShowingInterval(5000)
{
    QPalette palette;
    palette.setBrush(QPalette::Light, Qt::darkGray);

    // 新建媒体项沿用当前的全局静音状态
    if (delegated()->isMuted())
    {
        delegated()->setMute(true);
    }
}

bool GraphicsMediaItemDelegate::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
    // 按下时先浮出工具栏，并重置自动隐藏计时
    mToolBarItem->show();
    positionHandles();

    if (mToolBarShowTimer)
    {
        mToolBarShowTimer->start();
    }

    return GraphicsItemDelegate::mousePressEvent(event);
}

// 显示工具栏；播放中或暂停时启动自动隐藏计时
void GraphicsMediaItemDelegate::showToolBar(bool autohide)
{
    mToolBarItem->show();
    if (mToolBarShowTimer)
    {
        if (delegated()->isPlaying() || delegated()->isPaused())
        {
            mToolBarShowTimer->start();
        }
        else
        {
            mToolBarShowTimer->stop();
        }

        // 停在起始位置时通常还没开始观看，保持工具栏可见
        if (delegated()->mediaPosition() == delegated()->initialPos())
        {
            mToolBarShowTimer->stop();
        }

        // 调用方显式要求常显时不自动隐藏
        if (!autohide)
        {
            mToolBarShowTimer->stop();
        }
    }
}

void GraphicsMediaItemDelegate::hideToolBar()
{
    mToolBarItem->hide();
}

void GraphicsMediaItemDelegate::buildButtons()
{
    if (!mPlayPauseButton)
    {
        // 媒体按钮放在工具栏上，而不是选择框四角
        mPlayPauseButton = new DelegateButton(":/images/play.svg", mDelegated, mToolBarItem, Qt::TitleBarArea);
        connect(mPlayPauseButton, &DelegateButton::clicked, this, &GraphicsMediaItemDelegate::togglePlayPause);

        mStopButton = new DelegateButton(":/images/stop.svg", mDelegated, mToolBarItem, Qt::TitleBarArea);
        connect(mStopButton, &DelegateButton::clicked, delegated(), &GraphicsMediaItem::stop);

        mMediaControl = new DelegateMediaControl(delegated(), mToolBarItem);
        mMediaControl->setFlag(QGraphicsItem::ItemIsSelectable, true);
        GraphicsItem::assignZValue(mMediaControl, delegated()->zValue());

        if (delegated()->isMuted())
        {
            mMuteButton = new DelegateButton(":/images/soundOff.svg", mDelegated, mToolBarItem, Qt::TitleBarArea);
        }
        else
        {
            mMuteButton = new DelegateButton(":/images/soundOn.svg", mDelegated, mToolBarItem, Qt::TitleBarArea);
        }

        connect(mMuteButton, &DelegateButton::clicked, delegated(), &GraphicsMediaItem::toggleMute);
        connect(mMuteButton, &DelegateButton::clicked, this,
            &GraphicsMediaItemDelegate::toggleMute); // 仅用于切换按钮图标

        mToolBarButtons << mPlayPauseButton << mStopButton << mMuteButton;

        mToolBarItem->setItemsOnToolBar(QList<QGraphicsItem *>() << mPlayPauseButton << mStopButton << mMediaControl << mMuteButton);
        mToolBarItem->setVisibleOnBoard(true);
        mToolBarItem->setShifting(false);

        // 有封面/占位的媒体才需要自动隐藏工具栏
        if (!mToolBarShowTimer)
        {
            if (delegated()->hasLinkedImage())
            {
                mToolBarShowTimer = new QTimer();
                mToolBarShowTimer->setInterval(m_iToolBarShowingInterval);
                connect(mToolBarShowTimer, &QTimer::timeout, this, &GraphicsMediaItemDelegate::hideToolBar);
            }
        }

        else
        {
            // 任一控件被操作都重新计时
            connect(mPlayPauseButton, &DelegateButton::clicked, mToolBarShowTimer, qOverload<>(&QTimer::start));

            connect(mStopButton, &DelegateButton::clicked, mToolBarShowTimer, qOverload<>(&QTimer::start));

            connect(mMediaControl, &DelegateMediaControl::used, mToolBarShowTimer, qOverload<>(&QTimer::start));

            connect(mMuteButton, &DelegateButton::clicked, mToolBarShowTimer, qOverload<>(&QTimer::start));
        }

        positionHandles();
    }
}

GraphicsMediaItemDelegate::~GraphicsMediaItemDelegate()
{
    // 计时器未指定父对象，需要手动释放
    if (mToolBarShowTimer)
    {
        delete mToolBarShowTimer;
        mToolBarShowTimer = NULL;
    }
}

void GraphicsMediaItemDelegate::positionHandles()
{
    GraphicsItemDelegate::positionHandles();

    GraphicsMediaItem *mediaItem = dynamic_cast<GraphicsMediaItem *>(mDelegated);
    if (mediaItem)
    {
        QRectF toolBarRect = mToolBarItem->rect();

        // 工具栏贴在媒体内容底部，宽度与内容一致
        mToolBarItem->setPos(0, mediaItem->boundingRect().height() - mToolBarItem->rect().height());

        toolBarRect.setWidth(mediaItem->boundingRect().width());
        mToolBarItem->show();

        mToolBarItem->setRect(toolBarRect);
    }

    int toolBarButtonsWidth = 0;
    foreach (DelegateButton *button, mToolBarButtons)
    {
        toolBarButtonsWidth += button->boundingRect().width() + mToolBarItem->getElementsPadding();
    }

    // 控制条占据除按钮之外的全部宽度
    QRectF mediaItemRect = mMediaControl->rect();
    mediaItemRect.setWidth(mediaItem->boundingRect().width() - toolBarButtonsWidth);
    mediaItemRect.setHeight(mToolBarItem->boundingRect().height());
    mMediaControl->setRect(mediaItemRect);

    mToolBarItem->positionHandles();
    mMediaControl->positionHandles();

    if (mediaItem)
    {
        mToolBarItem->show();
    }
}

void GraphicsMediaItemDelegate::remove(bool canUndo)
{
    // 删除前先停止播放，避免撤销栈中留下仍在播放的对象
    if (delegated())
    {
        delegated()->stop();
    }

    GraphicsItemDelegate::remove(canUndo);
}

void GraphicsMediaItemDelegate::toggleMute()
{
    if (delegated()->isMuted())
    {
        mMuteButton->setFileName(":/images/soundOff.svg");
    }
    else
    {
        mMuteButton->setFileName(":/images/soundOn.svg");
    }
}

GraphicsMediaItem *GraphicsMediaItemDelegate::delegated()
{
    return dynamic_cast<GraphicsMediaItem *>(mDelegated);
}

void GraphicsMediaItemDelegate::togglePlayPause()
{
    if (delegated())
    {
        delegated()->togglePlayPause();
    }
}

void GraphicsMediaItemDelegate::mediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    // 可能的状态：UnknownMediaStatus、NoMedia、LoadingMedia、LoadedMedia、
    // StalledMedia、BufferingMedia、BufferedMedia、EndOfMedia、InvalidMedia

    if (status == QMediaPlayer::LoadedMedia)
    {
        mMediaControl->totalTimeChanged(delegated()->mediaDuration());
    }

    // 视频停在开头时，用一次播放+暂停来加载并显示第一帧
    if ((status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia) && delegated()->mediaPosition() == delegated()->initialPos() &&
        !delegated()->isStopped())
    {
        delegated()->play();
        delegated()->pause();
    }

    // 播放到结尾时保持进度条常显，不自动隐藏
    if (status == QMediaPlayer::EndOfMedia)
    {
        showToolBar(false);
    }

    // 多数情况下只需刷新播放/暂停按钮的状态
    updatePlayPauseState();
}

void GraphicsMediaItemDelegate::mediaStateChanged(QMediaPlayer::PlaybackState state)
{
    Q_UNUSED(state);
    // 可能的状态：StoppedState、PlayingState、PausedState

    // 具体处理统一在 updatePlayPauseState 里完成
    updatePlayPauseState();
}

void GraphicsMediaItemDelegate::updatePlayPauseState()
{
    if (delegated()->playerState() == QMediaPlayer::PlayingState)
    {
        mPlayPauseButton->setFileName(":/images/pause.svg");
    }
    else
    {
        mPlayPauseButton->setFileName(":/images/play.svg");
    }
}

void GraphicsMediaItemDelegate::updateTicker(qint64 time)
{
    mMediaControl->totalTimeChanged(delegated()->mediaDuration());
    mMediaControl->updateTicker(time);
}

void GraphicsMediaItemDelegate::totalTimeChanged(qint64 newTotalTime)
{
    mMediaControl->totalTimeChanged(newTotalTime);
}

void GraphicsMediaItemDelegate::showHide(bool show)
{
    QVariant showFlag = QVariant(show ? ItemLayerType::Object : ItemLayerType::Control);
    showHideRecurs(showFlag, mDelegated);
    mDelegated->update();

    emit showOnDisplayChanged(show);
}
