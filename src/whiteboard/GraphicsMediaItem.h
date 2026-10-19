#pragma once

#include <QtWidgets/QGraphicsView>
#include "whiteboard/GraphicsProxyWidget.h"

#include <QAudioOutput>
#include <QMediaPlayer>

#include <QtMultimediaWidgets/QVideoWidget>
#include <QtMultimedia/QVideoFrame>

#include "core/Application.h"
#include "whiteboard/BoardController.h"
#include "utils/FileSystemUtils.h"

class QGraphicsVideoItem;

class GraphicsMediaItem : public QObject, public Item, public GraphicsItem, public QGraphicsRectItem, public ResizableGraphicsItem
{
    Q_OBJECT

public:
    typedef enum{
        mediaType_Video,
        mediaType_Audio
    } mediaType;

    enum { Type = GraphicsItemType::MediaItemType };

    virtual int type() const
    {
        return Type;
    }

    static GraphicsMediaItem* createMediaItem(const QUrl& pMediaFileUrl, QGraphicsItem* parent = 0);

    GraphicsMediaItem(const QUrl& pMediaFileUrl, QGraphicsItem* parent = 0);
    ~GraphicsMediaItem();

    virtual mediaType getMediaType() const = 0;

    virtual GraphicsScene* scene();
    bool hasLinkedImage() const             { return haveLinkedImage; }
    virtual QUrl mediaFileUrl() const       { return mMediaFileUrl; }
    bool isMuted() const                    { return mMuted; }
    qint64 initialPos() const               { return mInitialPos; }

    bool isMediaSeekable() const;
    qint64 mediaDuration() const;
    qint64 mediaPosition() const;

    QMediaPlayer::PlaybackState playerState() const;
    bool isPlaying() const { return (mMediaObject->playbackState() == QMediaPlayer::PlayingState); }
    bool isPaused() const { return (mMediaObject->playbackState() == QMediaPlayer::PausedState); }
    bool isStopped() const;

    QRectF boundingRect() const;

    QSizeF size() const { return rect().size(); } 

    // Setters
    virtual void setMediaFileUrl(QUrl url);
    void setInitialPos(qint64 p);
    void setMediaPos(qint64 p);
    virtual void setSourceUrl(const QUrl &pSourceUrl);
    void setSelected(bool selected);
    void setMinimumSize(const QSize& size);
    void setUuid(const QUuid &pUuid);

    virtual void copyItemParameters(Item *copy) const;

    virtual void setSize(int width, int height);
    void resize(qreal w, qreal h) { setSize(w, h); }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

public slots:
    void toggleMute();
    void setMute(bool bMute);
    void activeSceneChanged();
    void showOnDisplayChanged(bool shown);

    virtual void play();
    virtual void pause();
    virtual void stop();
    virtual void togglePlayPause();

protected slots:
    void mediaError(QMediaPlayer::Error errorCode, const QString &errorString);

protected:
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);

    virtual void clearSource();

    QMediaPlayer *mMediaObject;

    // Qt6 把音频输出从 QMediaPlayer 里拆出来了，静音/音量都在这个对象上。
    QAudioOutput *mAudioOutput;

    QSize mMinimumSize;

    bool mMuted;
    bool mMutedByUserAction;
    static bool sIsMutedByDefault;
    bool mStopped;

    QUrl mMediaFileUrl;
    QString mMediaSource;

    bool mShouldMove;
    QPointF mMousePressPos;
    QPointF mMouseMovePos;

    bool haveLinkedImage;
    QGraphicsPixmapItem *mLinkedImage;

    qint64 mInitialPos;

    QString mErrorString;
};

class GraphicsAudioItem: public GraphicsMediaItem
{
    Q_OBJECT

public:
    enum { Type = GraphicsItemType::AudioItemType};
    virtual int type() const
    {
        return Type;
    }

    GraphicsAudioItem(const QUrl& pMediaFileUrl, QGraphicsItem *parent = 0);
    mediaType getMediaType() const { return mediaType_Audio; }

    virtual Item* deepCopy() const;
};

class GraphicsVideoItem: public GraphicsMediaItem
{
    Q_OBJECT

public:
    enum { Type = GraphicsItemType::VideoItemType};
    virtual int type() const
    {
        return Type;
    }

    GraphicsVideoItem(const QUrl& pMediaFileUrl, QGraphicsItem *parent = 0);

    mediaType getMediaType() const { return mediaType_Video; }

    void setSize(int width, int height);

    virtual Item* deepCopy() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

public slots:
    void videoSizeChanged(QSizeF newSize);
    void hasVideoChanged(bool hasVideo);
    void mediaStateChanged(QMediaPlayer::PlaybackState state);
    void activeSceneChanged();

protected slots:
    void mediaError(QMediaPlayer::Error errorCode, const QString &errorString);

protected:
    QGraphicsVideoItem *mVideoItem;

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

    void setPlaceholderVisible(bool visible);

    bool mHasVideoOutput;
};

