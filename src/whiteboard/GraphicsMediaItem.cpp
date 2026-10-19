#include "whiteboard/GraphicsGroupContainerItem.h"
#include "whiteboard/GraphicsMediaItem.h"
#include "whiteboard/GraphicsMediaItemDelegate.h"
#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsDelegateFrame.h"
#include "model/DocumentProxy.h"
#include "core/Application.h"
#include "whiteboard/BoardController.h"

#include <QGraphicsVideoItem>

bool GraphicsMediaItem::sIsMutedByDefault = false;

GraphicsMediaItem* GraphicsMediaItem::createMediaItem(const QUrl &pMediaFileUrl, QGraphicsItem* parent)
{
    GraphicsMediaItem * mediaItem;

    QString mediaPath = pMediaFileUrl.toString();
    if ("" == mediaPath)
        mediaPath = pMediaFileUrl.toLocalFile();

    if (mediaPath.toLower().contains("videos"))
        mediaItem = new GraphicsVideoItem(pMediaFileUrl, parent);
    else if (mediaPath.toLower().contains("audios"))
        mediaItem = new GraphicsAudioItem(pMediaFileUrl, parent);

    return mediaItem;
}

GraphicsMediaItem::GraphicsMediaItem(const QUrl& pMediaFileUrl, QGraphicsItem *parent)
        : QGraphicsRectItem(parent)
        , mMuted(sIsMutedByDefault)
        , mMutedByUserAction(sIsMutedByDefault)
        , mStopped(false)
        , mMediaFileUrl(pMediaFileUrl)
        , mLinkedImage(NULL)
        , mInitialPos(0)
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

    connect(mMediaObject, &QMediaPlayer::mediaStatusChanged,
            mediaDelegate, &GraphicsMediaItemDelegate::mediaStatusChanged);

    connect(mMediaObject, &QMediaPlayer::playbackStateChanged,
            mediaDelegate, &GraphicsMediaItemDelegate::mediaStateChanged);

    connect(mMediaObject, &QMediaPlayer::positionChanged,
            mediaDelegate, &GraphicsMediaItemDelegate::updateTicker);

    connect(mMediaObject, &QMediaPlayer::durationChanged,
            mediaDelegate, &GraphicsMediaItemDelegate::totalTimeChanged);

    connect(Delegate(), &GraphicsItemDelegate::showOnDisplayChanged,
            this, &GraphicsMediaItem::showOnDisplayChanged);

    connect(mMediaObject, &QMediaPlayer::errorOccurred,
            this, &GraphicsMediaItem::mediaError);
}

GraphicsAudioItem::GraphicsAudioItem(const QUrl &pMediaFileUrl, QGraphicsItem *parent)
    :GraphicsMediaItem(pMediaFileUrl, parent)
{
    haveLinkedImage = false;

    Delegate()->createControls();
    Delegate()->frame()->setOperationMode(GraphicsDelegateFrame::ResizingHorizontally);

    this->setSize(320, 26);
    this->setMinimumSize(QSize(150, 26));
}

GraphicsVideoItem::GraphicsVideoItem(const QUrl &pMediaFileUrl, QGraphicsItem *parent)
    :GraphicsMediaItem(pMediaFileUrl, parent)
{
    haveLinkedImage = true;
    setPlaceholderVisible(true);
    Delegate()->createControls();

    mVideoItem = new QGraphicsVideoItem(this);

    mVideoItem->setData(GraphicsItemData::ItemLayerType, ItemLayerType::Object);
    mVideoItem->setFlag(ItemStacksBehindParent, true);

    /* setVideoSink has to be called only when the video item is visible on the screen,
     * due to a Qt bug (QTBUG-32522). So instead of calling it here, it is called when the
     * active scene has changed, or when the item is first created.
     * */
    mHasVideoOutput = false;

    setMinimumSize(QSize(320, 240));
    setSize(320, 240);

    connect(mVideoItem, &QGraphicsVideoItem::nativeSizeChanged,
            this, &GraphicsVideoItem::videoSizeChanged);

    connect(mMediaObject, &QMediaPlayer::hasVideoChanged,
            this, &GraphicsVideoItem::hasVideoChanged);

    connect(mMediaObject, &QMediaPlayer::playbackStateChanged,
            this, &GraphicsVideoItem::mediaStateChanged);

    connect(mMediaObject, &QMediaPlayer::errorOccurred,
            this, &GraphicsVideoItem::mediaError);

    setAcceptHoverEvents(true);

    update();
}

GraphicsMediaItem::~GraphicsMediaItem()
{
    if (mMediaObject) {
        mMediaObject->stop();
        delete mMediaObject;
    }
}

QVariant GraphicsMediaItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if ((change == QGraphicsItem::ItemEnabledChange)
            || (change == QGraphicsItem::ItemSceneChange)
            || (change == QGraphicsItem::ItemVisibleChange))
    {
        if (mMediaObject && (!isEnabled() || !isVisible() || !scene()))
            mMediaObject->pause();
    }
    else if (change == QGraphicsItem::ItemSceneHasChanged)
    {
        if (!scene())
            mMediaObject->stop();
        else {
            QString absoluteMediaFilename;

            if(mMediaFileUrl.toLocalFile().startsWith("audios/") || mMediaFileUrl.toLocalFile().startsWith("videos/"))
                absoluteMediaFilename = scene()->document()->persistencePath() + "/"  + mMediaFileUrl.toLocalFile();
            else
                absoluteMediaFilename = mMediaFileUrl.toLocalFile();

            if (absoluteMediaFilename.length() > 0)
                  mMediaObject->setSource(QUrl::fromLocalFile(absoluteMediaFilename));

        }
    }

    if (Delegate()) {
        QVariant newValue = Delegate()->itemChange(change, value);
        return QGraphicsRectItem::itemChange(change, newValue);
    }

    return QGraphicsRectItem::itemChange(change, value);
}

void GraphicsMediaItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->save();
    //painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    Delegate()->postpaint(painter, option, widget);
    painter->restore();
}


QMediaPlayer::PlaybackState GraphicsMediaItem::playerState() const
{
    return mMediaObject->playbackState();
}

/**
 * @brief Returns true if the video was manually stopped, false otherwise.
 */
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

/**
 * @brief Set the item's minimum size. If the current size is smaller, it will be resized.
 * @param size The new minimum size
 */
void GraphicsMediaItem::setMinimumSize(const QSize& size)
{
    mMinimumSize = size;

    QSizeF newSize = rect().size();
    int width = newSize.width();
    int height = newSize.height();

    if (rect().width() < mMinimumSize.width())
        width = mMinimumSize.width();

    if (rect().height() < mMinimumSize.height())
        height = mMinimumSize.height();

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
    if(selected){
        Delegate()->createControls();
        if (this->getMediaType() == mediaType_Audio)
            Delegate()->frame()->setOperationMode(GraphicsDelegateFrame::ResizingHorizontally);
        else
            Delegate()->frame()->setOperationMode(GraphicsDelegateFrame::Resizing);
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
    //if path is absolute clean duplicated path string
    if (!path.contains(Application::boardController->selectedDocument()->persistencePath()))
        path = Application::boardController->selectedDocument()->persistencePath() + "/" + path;

    if (!FileSystemUtils::deleteFile(path))
        qDebug() << "cannot delete file: " << path;
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
    mMutedByUserAction = mMuted;
    sIsMutedByDefault = mMuted;
}

GraphicsScene* GraphicsMediaItem::scene()
{
    return qobject_cast<GraphicsScene*>(QGraphicsItem::scene());
}


void GraphicsMediaItem::activeSceneChanged()
{
    if (Application::boardController->activeScene() != scene())
        mMediaObject->pause();
}


void GraphicsMediaItem::showOnDisplayChanged(bool shown)
{
    if (!shown) {
        mMuted = true;
        mAudioOutput->setMuted(mMuted);
    }
    else if (!mMutedByUserAction) {
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
    if (!mErrorString.isEmpty()) {
        Application::showMessage("Can't play media: " + mErrorString);
        return;
    }

    if (mMediaObject->playbackState() == QMediaPlayer::StoppedState)
        mMediaObject->play();

    else if (mMediaObject->playbackState() == QMediaPlayer::PlayingState) {

        if ((mMediaObject->duration() - mMediaObject->position()) <= 0) {
            mMediaObject->stop();
            mMediaObject->play();
        }

        else {
            mMediaObject->pause();
            if(scene())
                scene()->setModified(true);
        }
    }

    else if (mMediaObject->playbackState() == QMediaPlayer::PausedState) {
        if ((mMediaObject->duration() - mMediaObject->position()) <= 0)
            mMediaObject->stop();

        mMediaObject->play();
    }

    else  if ( mMediaObject->mediaStatus() == QMediaPlayer::LoadingMedia) {
        mMediaObject->setSource(mediaFileUrl());
        mMediaObject->play();
    }
}

void GraphicsMediaItem::mediaError(QMediaPlayer::Error errorCode, const QString &errorString)
{
    // QMediaPlayer::errorString() isn't very descriptive, so we generate our own message

    switch (errorCode) {
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
            mErrorString = tr("Media error: ")
                + QString::number(static_cast<int>(errorCode)) + " (" + errorString + ")";
    }

    if (!mErrorString.isEmpty() ) {
        Application::showMessage(mErrorString);
        qDebug() << mErrorString;
    }
}

void GraphicsMediaItem::copyItemParameters(Item *copy) const
{
    GraphicsMediaItem *cp = dynamic_cast<GraphicsMediaItem*>(copy);
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

        connect(Application::boardController, &BoardController::activeSceneChanged, cp, &GraphicsMediaItem::activeSceneChanged);

    }
}

void GraphicsMediaItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate()) {
        Delegate()->mousePressEvent(event);
        if (parentItem() && GraphicsGroupContainerItem::Type == parentItem()->type()) {
            GraphicsGroupContainerItem *group = qgraphicsitem_cast<GraphicsGroupContainerItem*>(parentItem());
            if (group) {
                QGraphicsItem *curItem = group->getCurrentItem();
                if (curItem && this != curItem)
                    group->deselectCurrentItem();
                group->setCurrentItem(this);
                this->setSelected(true);
                Delegate()->positionHandles();
            }
        }
    }

    if (parentItem() && parentItem()->type() == GraphicsGroupContainerItem::Type) {
        mShouldMove = false;
        if (!Delegate()->mousePressEvent(event))
            event->accept();
    }
    else {
        mShouldMove = (event->buttons() & Qt::LeftButton);
        mMousePressPos = event->scenePos();
        mMouseMovePos = mMousePressPos;

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

    if (Delegate())
        Delegate()->positionHandles();
    if (scene())
        scene()->setModified(true);
}

Item* GraphicsAudioItem::deepCopy() const
{
    QUrl url = this->mediaFileUrl();
    GraphicsMediaItem *copy = new GraphicsAudioItem(url, parentItem());

    copy->setUuid(this->uuid()); // this is OK for now as long as Widgets are imutable

    copyItemParameters(copy);

    return copy;
}

Item* GraphicsVideoItem::deepCopy() const
{
    QUrl url = this->mediaFileUrl();
    GraphicsMediaItem *copy = new GraphicsVideoItem(url, parentItem());

    copy->setUuid(this->uuid());
    copyItemParameters(copy);

    return copy;
}

void GraphicsVideoItem::setSize(int width, int height)
{
    // Resize the video, then the rest of the Item

    int sizeX = 0;
    int sizeY = 0;

    if (mMinimumSize.width() > width)
        sizeX = mMinimumSize.width();
    else
        sizeX = width;

    if (mMinimumSize.height() > height)
        sizeY = mMinimumSize.height();
    else
        sizeY = height;

    mVideoItem->setSize(QSize(sizeX, sizeY));


    GraphicsMediaItem::setSize(sizeX, sizeY);
}

void GraphicsVideoItem::videoSizeChanged(QSizeF newSize)
{
    /* Depending on the platform/video backend, video size information becomes
     * available at different times (either when the file is loaded, or when
     * playback begins), so this slot is needed to resize the video item as
     * soon as the information is available.
     */


    // We don't want the video item to resize when the video is stopped or finished;
    // and in those cases, the new size is reported as (0, 0).

    if (newSize != QSizeF(0,0))
        this->setSize(newSize.width(), newSize.height());

    else // Make sure the toolbar doesn't disappear
        Delegate()->showToolBar(false);
}

void GraphicsVideoItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // When selected, a QGraphicsRectItem is drawn with a dashed line border. We don't want this
    QStyleOptionGraphicsItem styleOption = QStyleOptionGraphicsItem(*option);
    styleOption.state &= ~QStyle::State_Selected;

    QGraphicsRectItem::paint(painter, &styleOption, widget);
    GraphicsMediaItem::paint(painter, option, widget);

}

QVariant GraphicsVideoItem::itemChange(GraphicsItemChange change, const QVariant &value) {
    if (change == QGraphicsItem::ItemVisibleChange
            && value.toBool()
            && !mHasVideoOutput
            && Application::app()->boardController
            && Application::app()->boardController->activeScene() == scene())
    {
        //qDebug() << "Item change, setting video output";

        mMediaObject->setVideoSink(mVideoItem->videoSink());
        mHasVideoOutput = true;
    }

    return GraphicsMediaItem::itemChange(change, value);
}

void GraphicsVideoItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    // Display the seek bar
    Delegate()->showToolBar();
    QGraphicsRectItem::hoverEnterEvent(event);
}

void GraphicsVideoItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    Delegate()->showToolBar();
    QGraphicsRectItem::hoverMoveEvent(event);
}

void GraphicsVideoItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    QGraphicsRectItem::hoverLeaveEvent(event);
}

void GraphicsVideoItem::hasVideoChanged(bool hasVideo)
{
    // On Linux, this is called (with hasVideo == true) when the video is first played
    // and when it finishes (hasVideo == false). But on Windows and OS X, it isn't called when
    // the video finishes, so those platforms require another solution to showing/hiding the
    // placeholder black rectangle.

    setPlaceholderVisible(!hasVideo);
}

void GraphicsVideoItem::mediaStateChanged(QMediaPlayer::PlaybackState state)
{

#if defined(Q_OS_OSX) || defined(Q_OS_WIN)
    setPlaceholderVisible((state == QMediaPlayer::StoppedState));
#else
    Q_UNUSED(state);
#endif

}

void GraphicsVideoItem::activeSceneChanged()
{
    //qDebug() << "Active scene changed";

    // Update the visibility of the placeholder, to prevent it being hidden when switching pages
    setPlaceholderVisible(!mErrorString.isEmpty());

    // Call setVideoSink, if the video is visible and if it hasn't been called already
    if (!mHasVideoOutput && Application::boardController->activeScene() == scene()) {
        //qDebug() << "setting video output";
        mMediaObject->setSource(mMediaFileUrl);
        mMediaObject->setVideoSink(mVideoItem->videoSink());
        mHasVideoOutput = true;
    }

    GraphicsMediaItem::activeSceneChanged();
}

void GraphicsVideoItem::mediaError(QMediaPlayer::Error errorCode, const QString &errorString)
{
    Q_UNUSED(errorString);
    setPlaceholderVisible(errorCode != QMediaPlayer::NoError);
}

/**
 * @brief Set the brush and fill to display a black rectangle
 * @param visible If true, a black rectangle is displayed in place of the video
 *
 * Depending on platforms, when a video is finished or stopped, the video might be
 * removed altogether. To avoid just having the controls bar visible at that point,
 * we can display a "fake" black video frame in its place using this method.
 */
void GraphicsVideoItem::setPlaceholderVisible(bool visible)
{
    if (visible) {
        setBrush(QColor(Qt::black));
        setPen(QColor(Qt::white));
    }
    else {
        setBrush(QColor(Qt::transparent));
        setPen(QColor(Qt::transparent));
    }

}
