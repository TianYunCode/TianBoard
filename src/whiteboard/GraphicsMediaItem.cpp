#include "whiteboard/GraphicsGroupContainerItem.h"
#include "utils/UserNotices.h"
#include "whiteboard/GraphicsMediaItem.h"
#include "whiteboard/GraphicsMediaItemDelegate.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsDelegateFrame.h"
#include "core/Application.h"
#include "whiteboard/BoardController.h"

#include <QGraphicsVideoItem>

bool GraphicsMediaItem::sIsMutedByDefault = false;

GraphicsMediaItem *GraphicsMediaItem::createMediaItem(const QUrl &pMediaFileUrl, QGraphicsItem *parent)
{
    GraphicsMediaItem *mediaItem = nullptr;

    // 通过路径中的 videos/audios 目录判断媒体种类；无法识别时返回空
    QString mediaPath            = pMediaFileUrl.toString();
    if ("" == mediaPath)
    {
        mediaPath = pMediaFileUrl.toLocalFile();
    }

    if (mediaPath.toLower().contains("videos"))
    {
        mediaItem = new GraphicsVideoItem(pMediaFileUrl, parent);
    }
    else if (mediaPath.toLower().contains("audios"))
    {
        mediaItem = new GraphicsAudioItem(pMediaFileUrl, parent);
    }

    return mediaItem;
}

void GraphicsMediaItem::observeActiveSceneChanges(GraphicsMediaItem *item)
{
    if (!item)
    {
        return;
    }

    QObject::connect(Application::boardController, &BoardController::activeSceneChanged, item, &GraphicsMediaItem::activeSceneChanged);
}

GraphicsMediaItem::GraphicsMediaItem(const QUrl &pMediaFileUrl, QGraphicsItem *parent)
    : QGraphicsRectItem(parent), mMuted(sIsMutedByDefault), mMutedByUserAction(sIsMutedByDefault), mStopped(false), mMediaFileUrl(pMediaFileUrl),
      mLinkedImage(NULL), mInitialPos(0)
{
    mErrorString = "";

    mMediaObject = new QMediaPlayer(this);

    // Qt6 的 QMediaPlayer 不再自己出声，必须显式挂一个 QAudioOutput。
    mAudioOutput = new QAudioOutput(this);
    mMediaObject->setAudioOutput(mAudioOutput);

    mMediaObject->setSource(pMediaFileUrl);
    mAudioOutput->setMuted(mMuted);

    auto *mediaDelegate = new GraphicsMediaItemDelegate(this);
    setDelegate(mediaDelegate);

    setData(GraphicsItemData::itemLayerType, QVariant(itemLayerType::ObjectItem));
    setFlag(ItemIsMovable, true);
    setFlag(ItemSendsGeometryChanges, true);

    connect(mMediaObject, &QMediaPlayer::mediaStatusChanged, mediaDelegate, &GraphicsMediaItemDelegate::mediaStatusChanged);

    connect(mMediaObject, &QMediaPlayer::playbackStateChanged, mediaDelegate, &GraphicsMediaItemDelegate::mediaStateChanged);

    connect(mMediaObject, &QMediaPlayer::positionChanged, mediaDelegate, &GraphicsMediaItemDelegate::updateTicker);

    connect(mMediaObject, &QMediaPlayer::durationChanged, mediaDelegate, &GraphicsMediaItemDelegate::totalTimeChanged);

    connect(Delegate(), &GraphicsItemDelegate::showOnDisplayChanged, this, &GraphicsMediaItem::showOnDisplayChanged);

    connect(mMediaObject, &QMediaPlayer::errorOccurred, this, &GraphicsMediaItem::mediaError);
}

GraphicsAudioItem::GraphicsAudioItem(const QUrl &pMediaFileUrl, QGraphicsItem *parent) : GraphicsMediaItem(pMediaFileUrl, parent)
{
    haveLinkedImage = false;

    Delegate()->createControls();
    // 音频没有画面，只需要水平方向缩放
    Delegate()->frame()->setOperationMode(GraphicsDelegateFrame::ResizingHorizontally);

    this->setSize(320, 26);
    this->setMinimumSize(QSize(150, 26));
}

GraphicsVideoItem::GraphicsVideoItem(const QUrl &pMediaFileUrl, QGraphicsItem *parent) : GraphicsMediaItem(pMediaFileUrl, parent)
{
    haveLinkedImage = true;
    setPlaceholderVisible(true);
    Delegate()->createControls();

    mVideoItem = new QGraphicsVideoItem(this);

    mVideoItem->setData(GraphicsItemData::ItemLayerType, ItemLayerType::Object);
    // 画面层垫在父项下面，父项负责边框与占位
    mVideoItem->setFlag(ItemStacksBehindParent, true);

    /* setVideoSink 只能在视频项真正显示在屏幕上时调用（Qt 的已知限制），
     * 因此不在这里设置，而是等到活动场景切换或项首次显示时再设置。
     * */
    mHasVideoOutput = false;

    setMinimumSize(QSize(320, 240));
    setSize(320, 240);

    connect(mVideoItem, &QGraphicsVideoItem::nativeSizeChanged, this, &GraphicsVideoItem::videoSizeChanged);

    connect(mMediaObject, &QMediaPlayer::hasVideoChanged, this, &GraphicsVideoItem::hasVideoChanged);

    connect(mMediaObject, &QMediaPlayer::playbackStateChanged, this, &GraphicsVideoItem::mediaStateChanged);

    connect(mMediaObject, &QMediaPlayer::errorOccurred, this, &GraphicsVideoItem::mediaError);

    setAcceptHoverEvents(true);

    update();
}

GraphicsMediaItem::~GraphicsMediaItem()
{
    // 先停止再删除，避免播放器线程仍在回调已销毁的对象
    if (mMediaObject)
    {
        mMediaObject->stop();
        delete mMediaObject;
    }
}

QVariant GraphicsMediaItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    // 禁用、隐藏或移出场景时暂停播放，避免后台继续出声
    if ((change == QGraphicsItem::ItemEnabledChange) || (change == QGraphicsItem::ItemSceneChange) || (change == QGraphicsItem::ItemVisibleChange))
    {
        if (mMediaObject && (!isEnabled() || !isVisible() || !scene()))
        {
            mMediaObject->pause();
        }
    }
    else if (change == QGraphicsItem::ItemSceneHasChanged)
    {
        if (!scene())
        {
            mMediaObject->stop();
        }
        else
        {
            QString absoluteMediaFilename;

            // 文档内的媒体使用相对路径，需拼接文档持久化目录才能找到文件
            if (mMediaFileUrl.toLocalFile().startsWith("audios/") || mMediaFileUrl.toLocalFile().startsWith("videos/"))
            {
                absoluteMediaFilename = scene()->document()->persistencePath() + "/" + mMediaFileUrl.toLocalFile();
            }
            else
            {
                absoluteMediaFilename = mMediaFileUrl.toLocalFile();
            }

            if (absoluteMediaFilename.length() > 0)
            {
                mMediaObject->setSource(QUrl::fromLocalFile(absoluteMediaFilename));
            }
        }
    }

    if (Delegate())
    {
        QVariant newValue = Delegate()->itemChange(change, value);
        return QGraphicsRectItem::itemChange(change, newValue);
    }

    return QGraphicsRectItem::itemChange(change, value);
}

void GraphicsMediaItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->save();
    Delegate()->postpaint(painter, option, widget);
    painter->restore();
}

QMediaPlayer::PlaybackState GraphicsMediaItem::playerState() const
{
    return mMediaObject->playbackState();
}

// 是否被用户显式停止（区别于自然播放结束）
bool GraphicsMediaItem::isStopped() const
{
    return mStopped;
}

qint64 GraphicsMediaItem::mediaDuration() const
{
    return mMediaObject->duration();
}

qint64 GraphicsMediaItem::mediaPosition() const
{
    return mMediaObject->position();
}

bool GraphicsMediaItem::isMediaSeekable() const
{
    return mMediaObject->isSeekable();
}

// 设置最小尺寸；若当前尺寸更小则立即放大到最小尺寸
void GraphicsMediaItem::setMinimumSize(const QSize &size)
{
    mMinimumSize   = size;

    QSizeF newSize = rect().size();
    int    width   = newSize.width();
    int    height  = newSize.height();

    if (rect().width() < mMinimumSize.width())
    {
        width = mMinimumSize.width();
    }

    if (rect().height() < mMinimumSize.height())
    {
        height = mMinimumSize.height();
    }

    this->setSize(width, height);
}

void GraphicsMediaItem::setUuid(const QUuid &pUuid)
{
    Item::setUuid(pUuid);
    setData(GraphicsItemData::ItemUuid, QVariant(pUuid));
}

void GraphicsMediaItem::setMediaFileUrl(QUrl url)
{
    mMediaFileUrl = url;
}

void GraphicsMediaItem::setInitialPos(qint64 p)
{
    mInitialPos = p;
}

void GraphicsMediaItem::setMediaPos(qint64 p)
{
    mMediaObject->setPosition(p);
}

void GraphicsMediaItem::setSelected(bool selected)
{
    // 选中时才创建控件，并按媒体种类决定可缩放方向
    if (selected)
    {
        Delegate()->createControls();
        if (this->getMediaType() == mediaType_Audio)
        {
            Delegate()->frame()->setOperationMode(GraphicsDelegateFrame::ResizingHorizontally);
        }
        else
        {
            Delegate()->frame()->setOperationMode(GraphicsDelegateFrame::Resizing);
        }
    }
    QGraphicsRectItem::setSelected(selected);
}

void GraphicsMediaItem::setSourceUrl(const QUrl &pSourceUrl)
{
    Item::setSourceUrl(pSourceUrl);
}

void GraphicsMediaItem::clearSource()
{
    QString path = mediaFileUrl().toLocalFile();
    // 相对路径需要补上文档目录；已经是绝对路径时不要再拼接
    if (!path.contains(Application::boardController->selectedDocument()->persistencePath()))
    {
        path = Application::boardController->selectedDocument()->persistencePath() + "/" + path;
    }

    if (!FileSystemUtils::deleteFile(path))
    {
        qDebug() << "cannot delete file: " << path;
    }
}

void GraphicsMediaItem::toggleMute()
{
    mMuted = !mMuted;
    setMute(mMuted);
}

void GraphicsMediaItem::setMute(bool bMute)
{
    mMuted = bMute;
    mAudioOutput->setMuted(mMuted);
    // 记录用户的选择并作为后续新建媒体项的默认值
    mMutedByUserAction = mMuted;
    sIsMutedByDefault  = mMuted;
}

GraphicsScene *GraphicsMediaItem::scene()
{
    return qobject_cast<GraphicsScene *>(QGraphicsItem::scene());
}

void GraphicsMediaItem::activeSceneChanged()
{
    if (Application::boardController->activeScene() != scene())
    {
        mMediaObject->pause();
    }
}

void GraphicsMediaItem::showOnDisplayChanged(bool shown)
{
    // 只在教师屏显示时静音；用户主动静音的状态在重新显示后保留
    if (!shown)
    {
        mMuted = true;
        mAudioOutput->setMuted(mMuted);
    }
    else if (!mMutedByUserAction)
    {
        mMuted = false;
        mAudioOutput->setMuted(mMuted);
    }
}
void GraphicsMediaItem::play()
{
    mMediaObject->play();
    mStopped = false;
}

void GraphicsMediaItem::pause()
{
    mMediaObject->pause();
    mStopped = false;
}

void GraphicsMediaItem::stop()
{
    mMediaObject->stop();
    mStopped = true;
}

void GraphicsMediaItem::togglePlayPause()
{
    // 载入失败后不再尝试播放，直接把错误提示给用户
    if (!mErrorString.isEmpty())
    {
        UserNotices::presentStatusMessage("Can't play media: " + mErrorString);
        return;
    }

    if (mMediaObject->playbackState() == QMediaPlayer::StoppedState)
    {
        mMediaObject->play();
    }

    else if (mMediaObject->playbackState() == QMediaPlayer::PlayingState)
    {
        // 已到结尾时先回到起点再重新播放，否则直接暂停
        if ((mMediaObject->duration() - mMediaObject->position()) <= 0)
        {
            mMediaObject->stop();
            mMediaObject->play();
        }

        else
        {
            mMediaObject->pause();
            if (scene())
            {
                scene()->setModified(true);
            }
        }
    }

    else if (mMediaObject->playbackState() == QMediaPlayer::PausedState)
    {
        if ((mMediaObject->duration() - mMediaObject->position()) <= 0)
        {
            mMediaObject->stop();
        }

        mMediaObject->play();
    }

    else if (mMediaObject->mediaStatus() == QMediaPlayer::LoadingMedia)
    {
        // 仍处于加载中：重新设置源后再播放
        mMediaObject->setSource(mediaFileUrl());
        mMediaObject->play();
    }
}

void GraphicsMediaItem::mediaError(QMediaPlayer::Error errorCode, const QString &errorString)
{
    // QMediaPlayer::errorString() 信息有限，这里生成更明确的提示
    switch (errorCode)
    {
        case QMediaPlayer::NoError:
            mErrorString = "";
            break;
        case QMediaPlayer::ResourceError:
            mErrorString = tr("Media resource couldn't be resolved");
            break;
        case QMediaPlayer::FormatError:
            mErrorString = tr("Unsupported media format");
            break;
        default:
            mErrorString = tr("Media error: ") + QString::number(static_cast<int>(errorCode)) + " (" + errorString + ")";
    }

    if (!mErrorString.isEmpty())
    {
        UserNotices::presentStatusMessage(mErrorString);
        qDebug() << mErrorString;
    }
}

void GraphicsMediaItem::copyItemParameters(Item *copy) const
{
    GraphicsMediaItem *cp = dynamic_cast<GraphicsMediaItem *>(copy);
    if (cp)
    {
        cp->setPos(this->pos());
        cp->setTransform(this->transform());
        cp->setFlag(QGraphicsItem::ItemIsMovable, true);
        cp->setFlag(QGraphicsItem::ItemIsSelectable, true);
        cp->setData(GraphicsItemData::ItemLayerType, this->data(GraphicsItemData::ItemLayerType));
        cp->setData(GraphicsItemData::ItemLocked, this->data(GraphicsItemData::ItemLocked));
        cp->setSourceUrl(this->sourceUrl());
        cp->setSize(rect().width(), rect().height());

        cp->setZValue(this->zValue());

        // 副本也需要跟随活动场景切换而暂停
        connect(Application::boardController, &BoardController::activeSceneChanged, cp, &GraphicsMediaItem::activeSceneChanged);
    }
}

void GraphicsMediaItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate())
    {
        Delegate()->mousePressEvent(event);
        // 组合内的媒体项：按下时把它记为组合的当前交互项，并单独选中
        if (parentItem() && GraphicsGroupContainerItem::Type == parentItem()->type())
        {
            GraphicsGroupContainerItem *group = qgraphicsitem_cast<GraphicsGroupContainerItem *>(parentItem());
            if (group)
            {
                QGraphicsItem *curItem = group->getCurrentItem();
                if (curItem && this != curItem)
                {
                    group->deselectCurrentItem();
                }
                group->setCurrentItem(this);
                this->setSelected(true);
                Delegate()->positionHandles();
            }
        }
    }

    if (parentItem() && parentItem()->type() == GraphicsGroupContainerItem::Type)
    {
        // 组合内的媒体项由组合负责移动，自身不拖动
        mShouldMove = false;
        if (!Delegate()->mousePressEvent(event))
        {
            event->accept();
        }
    }
    else
    {
        mShouldMove    = (event->buttons() & Qt::LeftButton);
        mMousePressPos = event->scenePos();
        mMouseMovePos  = mMousePressPos;

        event->accept();
        setSelected(true);
    }
    QGraphicsRectItem::mousePressEvent(event);
}

QRectF GraphicsMediaItem::boundingRect() const
{
    return rect();
}

void GraphicsMediaItem::setSize(int width, int height)
{
    QRectF r = rect();
    r.setWidth(width);
    r.setHeight(height);
    setRect(r);

    // 尺寸变化后按钮与控制条要重新摆放
    if (Delegate())
    {
        Delegate()->positionHandles();
    }
    if (scene())
    {
        scene()->setModified(true);
    }
}

Item *GraphicsAudioItem::deepCopy() const
{
    QUrl               url  = this->mediaFileUrl();
    GraphicsMediaItem *copy = new GraphicsAudioItem(url, parentItem());

    // 沿用原 UUID，保持副本与源项的对应关系
    copy->setUuid(this->uuid());

    copyItemParameters(copy);

    return copy;
}

Item *GraphicsVideoItem::deepCopy() const
{
    QUrl               url  = this->mediaFileUrl();
    GraphicsMediaItem *copy = new GraphicsVideoItem(url, parentItem());

    copy->setUuid(this->uuid());
    copyItemParameters(copy);

    return copy;
}

void GraphicsVideoItem::setSize(int width, int height)
{
    // 先确定不小于最小尺寸的实际尺寸，再同步到画面层与基类
    int sizeX = 0;
    int sizeY = 0;

    if (mMinimumSize.width() > width)
    {
        sizeX = mMinimumSize.width();
    }
    else
    {
        sizeX = width;
    }

    if (mMinimumSize.height() > height)
    {
        sizeY = mMinimumSize.height();
    }
    else
    {
        sizeY = height;
    }

    mVideoItem->setSize(QSize(sizeX, sizeY));

    GraphicsMediaItem::setSize(sizeX, sizeY);
}

void GraphicsVideoItem::videoSizeChanged(QSizeF newSize)
{
    /* 不同平台/解码后端提供视频原始尺寸的时机不同（加载完成或开始播放时），
     * 因此必须在这个信号到来时再调整一次尺寸。
     */

    // 停止或播放结束时上报的新尺寸为 (0, 0)，此时不应把项缩成零

    if (newSize != QSizeF(0, 0))
    {
        this->setSize(newSize.width(), newSize.height());
    }

    else // 保持工具栏可见，避免停止后控件消失
    {
        Delegate()->showToolBar(false);
    }
}

void GraphicsVideoItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // 去掉选中虚框，选中效果由委托的选择框负责，避免双框
    QStyleOptionGraphicsItem styleOption  = QStyleOptionGraphicsItem(*option);
    styleOption.state                    &= ~QStyle::State_Selected;

    QGraphicsRectItem::paint(painter, &styleOption, widget);
    GraphicsMediaItem::paint(painter, option, widget);
}

QVariant GraphicsVideoItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    // 首次可见且位于当前活动场景时才绑定视频输出，满足 setVideoSink 的调用时机要求
    if (change == QGraphicsItem::ItemVisibleChange && value.toBool() && !mHasVideoOutput && Application::app()->boardController &&
        Application::app()->boardController->activeScene() == scene())
    {
        mMediaObject->setVideoSink(mVideoItem->videoSink());
        mHasVideoOutput = true;
    }

    return GraphicsMediaItem::itemChange(change, value);
}

void GraphicsVideoItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    // 鼠标进入时浮出进度条
    Delegate()->showToolBar();
    QGraphicsRectItem::hoverEnterEvent(event);
}

void GraphicsVideoItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    // 移动时刷新自动隐藏计时
    Delegate()->showToolBar();
    QGraphicsRectItem::hoverMoveEvent(event);
}

void GraphicsVideoItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    QGraphicsRectItem::hoverLeaveEvent(event);
}

void GraphicsVideoItem::hasVideoChanged(bool hasVideo)
{
    // 各平台上报时机不一致：有的在开始播放和结束时都上报，有的结束时不上报，
    // 因此占位显示还要依赖 mediaStateChanged 兜底
    setPlaceholderVisible(!hasVideo);
}

void GraphicsVideoItem::mediaStateChanged(QMediaPlayer::PlaybackState state)
{
    setPlaceholderVisible((state == QMediaPlayer::StoppedState));
}

void GraphicsVideoItem::activeSceneChanged()
{
    // 切页后重新评估占位显示，避免占位被错误隐藏
    setPlaceholderVisible(!mErrorString.isEmpty());

    // 若视频可见且尚未建立输出，则在成为活动场景时补上 setVideoSink
    if (!mHasVideoOutput && Application::boardController->activeScene() == scene())
    {
        mMediaObject->setSource(mMediaFileUrl);
        mMediaObject->setVideoSink(mVideoItem->videoSink());
        mHasVideoOutput = true;
    }

    GraphicsMediaItem::activeSceneChanged();
}

void GraphicsVideoItem::mediaError(QMediaPlayer::Error errorCode, const QString &errorString)
{
    Q_UNUSED(errorString);
    // 出错时显示黑色占位，提示用户此处本应有画面
    setPlaceholderVisible(errorCode != QMediaPlayer::NoError);
}

// 用黑底白框的矩形代替无画面时的视频区域
void GraphicsVideoItem::setPlaceholderVisible(bool visible)
{
    if (visible)
    {
        setBrush(QColor(Qt::black));
        setPen(QColor(Qt::white));
    }
    else
    {
        setBrush(QColor(Qt::transparent));
        setPen(QColor(Qt::transparent));
    }
}
