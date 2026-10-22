/**
 * @file GraphicsMediaItem.h
 * @brief 声明音频/视频媒体图形项及其工厂方法。
 */

#pragma once

#include <QtWidgets/QGraphicsView>
#include "whiteboard/GraphicsProxyWidget.h"

#include <QAudioOutput>
#include <QMediaPlayer>

#include <QtMultimediaWidgets/QVideoWidget>
#include <QtMultimedia/QVideoFrame>

#include "whiteboard/BoardController.h"
#include "utils/FileSystemUtils.h"

class QGraphicsVideoItem;

/**
 * @brief 媒体图形项的公共基类（音频与视频）。
 *
 * 媒体项内部持有一个 QMediaPlayer 与 QAudioOutput，负责播放、暂停、定位与静音；
 * 视频子类额外持有一个 QGraphicsVideoItem 作为画面层。项本身是可缩放的矩形，
 * 尺寸变化会同步到画面层与播放器输出。
 *
 * 播放器与音频输出由本类创建并拥有，析构时释放；画面层由视频子类创建，作为子项
 * 随本类销毁。媒体项通常作为组合的子项存在，选择框由委托管理。
 */
class GraphicsMediaItem : public QObject, public Item, public GraphicsItem, public QGraphicsRectItem, public ResizableGraphicsItem
{
    Q_OBJECT

public:
    /** @brief 媒体种类。 */
    typedef enum
    {
        mediaType_Video, ///< 视频（带画面）
        mediaType_Audio  ///< 音频（仅控制条）
    } mediaType;

    enum
    {
        Type = GraphicsItemType::MediaItemType
    };

    /** @brief 返回图形项类型标识。 */
    virtual int type() const
    {
        return Type;
    }

    /**
     * @brief 根据文件扩展名创建音频或视频项。
     * @param pMediaFileUrl 媒体文件地址。
     * @param parent 父图形项，可为空。
     * @return 新建的媒体项，调用方负责加入场景并管理生命周期。
     */
    static GraphicsMediaItem *createMediaItem(const QUrl &pMediaFileUrl, QGraphicsItem *parent = 0);

    /**
     * @brief 让一个媒体项跟着"当前活动页"的切换走（切走时自己停播）。
     * @param item 已建好的媒体项；为空时什么都不做。
     * @note 从盘上读回一页时由加载器调用它，而不是自己去连总控器的信号：连谁、怎么连
     * 是画板内部的事，格式层只需要知道"新读进来的媒体要接上这套连线"。
     */
    static void observeActiveSceneChanges(GraphicsMediaItem *item);

    /**
     * @brief 构造媒体项并初始化播放器。
     * @param pMediaFileUrl 媒体文件地址。
     * @param parent 父图形项，可为空。
     */
    GraphicsMediaItem(const QUrl &pMediaFileUrl, QGraphicsItem *parent = 0);
    ~GraphicsMediaItem();

    /** @brief 返回媒体种类，由子类实现。 */
    virtual mediaType getMediaType() const = 0;

    /** @brief 返回所属的 GraphicsScene，失败时返回空。 */
    virtual GraphicsScene *scene();
    /** @brief 是否关联了一张用于替代显示（如封面）的图像。 */
    bool hasLinkedImage() const
    {
        return haveLinkedImage;
    }
    /** @brief 返回媒体文件地址。 */
    virtual QUrl mediaFileUrl() const
    {
        return mMediaFileUrl;
    }
    /** @brief 是否静音。 */
    bool isMuted() const
    {
        return mMuted;
    }
    /** @brief 返回文档记录中的初始播放位置（毫秒）。 */
    qint64 initialPos() const
    {
        return mInitialPos;
    }

    /** @brief 媒体是否支持拖动定位。 */
    bool isMediaSeekable() const;
    /** @brief 返回媒体总时长（毫秒）。 */
    qint64 mediaDuration() const;
    /** @brief 返回当前播放位置（毫秒）。 */
    qint64 mediaPosition() const;

    /** @brief 返回播放器状态。 */
    QMediaPlayer::PlaybackState playerState() const;
    /** @brief 是否正在播放。 */
    bool isPlaying() const
    {
        return (mMediaObject->playbackState() == QMediaPlayer::PlayingState);
    }
    /** @brief 是否已暂停。 */
    bool isPaused() const
    {
        return (mMediaObject->playbackState() == QMediaPlayer::PausedState);
    }
    /** @brief 是否已停止。 */
    bool isStopped() const;

    /** @brief 返回媒体项矩形。 */
    QRectF boundingRect() const;

    /** @brief 返回当前尺寸。 */
    QSizeF size() const
    {
        return rect().size();
    }

    // Setters
    /** @brief 切换媒体文件并重新加载播放器源。 */
    virtual void setMediaFileUrl(QUrl url);
    /** @brief 设置初始播放位置（毫秒），用于文档恢复。 */
    void setInitialPos(qint64 p);
    /** @brief 跳转到指定播放位置（毫秒）。 */
    void setMediaPos(qint64 p);
    /** @brief 设置内容来源地址。 */
    virtual void setSourceUrl(const QUrl &pSourceUrl);
    /** @brief 选中状态变化时同步显示控制条。 */
    void setSelected(bool selected);
    /** @brief 设置最小尺寸，避免缩放到无法操作。 */
    void setMinimumSize(const QSize &size);
    /** @brief 设置 UUID 并写入图形项数据。 */
    void setUuid(const QUuid &pUuid);

    /** @brief 复制位置、尺寸等参数到另一个媒体项。 */
    virtual void copyItemParameters(Item *copy) const;

    /** @brief 设置尺寸并同步画面层。 */
    virtual void setSize(int width, int height);
    /** @brief 缩放入口，转发到 setSize。 */
    void resize(qreal w, qreal h)
    {
        setSize(w, h);
    }

    /** @brief 绘制媒体项背景或占位内容。 */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

public slots:
    /** @brief 切换静音状态。 */
    void toggleMute();
    /** @brief 设置静音。 */
    void setMute(bool bMute);
    /** @brief 场景切换时暂停或恢复播放。 */
    // GraphicsVideoItem 会接管它；连接方拿到的是基类指针，必须靠 vtable 派发
    virtual void activeSceneChanged();
    /** @brief “在扩展屏显示”变化时调整画面输出。 */
    void showOnDisplayChanged(bool shown);

    /** @brief 开始播放。 */
    virtual void play();
    /** @brief 暂停播放。 */
    virtual void pause();
    /** @brief 停止播放并回到起点。 */
    virtual void stop();
    /** @brief 在播放与暂停之间切换。 */
    virtual void togglePlayPause();

protected slots:
    /** @brief 播放器出错时记录错误信息并停止。 */
    void mediaError(QMediaPlayer::Error errorCode, const QString &errorString);

protected:
    /** @brief 位置变化时同步画面层与关联图像。 */
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    /** @brief 按下时记录起点，供拖动逻辑使用。 */
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);

    /** @brief 释放播放器与关联资源。 */
    virtual void clearSource();

    QMediaPlayer *mMediaObject;

    // Qt6 把音频输出从 QMediaPlayer 里拆出来了，静音/音量都在这个对象上。
    QAudioOutput *mAudioOutput;

    QSize mMinimumSize;

    bool        mMuted;
    bool        mMutedByUserAction;
    static bool sIsMutedByDefault;
    bool        mStopped;

    QUrl    mMediaFileUrl;
    QString mMediaSource;

    bool    mShouldMove;
    QPointF mMousePressPos;
    QPointF mMouseMovePos;

    bool                 haveLinkedImage;
    QGraphicsPixmapItem *mLinkedImage;

    qint64 mInitialPos;

    QString mErrorString;
};

/**
 * @brief 音频媒体项：只有控制条与音频输出，没有画面。
 */
class GraphicsAudioItem : public GraphicsMediaItem
{
    Q_OBJECT

public:
    enum
    {
        Type = GraphicsItemType::AudioItemType
    };
    /** @brief 返回图形项类型标识。 */
    virtual int type() const
    {
        return Type;
    }

    /**
     * @brief 构造音频项。
     * @param pMediaFileUrl 音频文件地址。
     * @param parent 父图形项，可为空。
     */
    GraphicsAudioItem(const QUrl &pMediaFileUrl, QGraphicsItem *parent = 0);
    /** @brief 返回音频种类。 */
    mediaType getMediaType() const
    {
        return mediaType_Audio;
    }

    /** @brief 深拷贝音频项。 */
    virtual Item *deepCopy() const;
};

/**
 * @brief 视频媒体项：在音频能力之上增加画面层与占位提示。
 */
class GraphicsVideoItem : public GraphicsMediaItem
{
    Q_OBJECT

public:
    enum
    {
        Type = GraphicsItemType::VideoItemType
    };
    /** @brief 返回图形项类型标识。 */
    virtual int type() const
    {
        return Type;
    }

    /**
     * @brief 构造视频项。
     * @param pMediaFileUrl 视频文件地址。
     * @param parent 父图形项，可为空。
     */
    GraphicsVideoItem(const QUrl &pMediaFileUrl, QGraphicsItem *parent = 0);

    /** @brief 返回视频种类。 */
    mediaType getMediaType() const
    {
        return mediaType_Video;
    }

    /** @brief 设置尺寸并同步画面层。 */
    void setSize(int width, int height);

    /** @brief 深拷贝视频项。 */
    virtual Item *deepCopy() const;

    /** @brief 绘制视频画面或占位内容。 */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

public slots:
    /** @brief 视频原始尺寸变化时按比例调整项尺寸。 */
    void videoSizeChanged(QSizeF newSize);
    /** @brief 是否有视频输出变化时切换占位显示。 */
    void hasVideoChanged(bool hasVideo);
    /** @brief 播放状态变化时刷新占位与画面。 */
    void mediaStateChanged(QMediaPlayer::PlaybackState state);
    /** @brief 场景切换时同步画面输出。 */
    void activeSceneChanged() override;

protected slots:
    /** @brief 播放器出错时记录错误并显示占位。 */
    void mediaError(QMediaPlayer::Error errorCode, const QString &errorString);

protected:
    QGraphicsVideoItem *mVideoItem;

    /** @brief 位置/可见性变化时同步画面层。 */
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);
    /** @brief 悬停进入时显示控制条。 */
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    /** @brief 悬停移动时刷新控制条自动隐藏计时。 */
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
    /** @brief 悬停离开时安排隐藏控制条。 */
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

    /** @brief 显示或隐藏“无视频画面”的占位提示。 */
    void setPlaceholderVisible(bool visible);

    bool mHasVideoOutput;
};
