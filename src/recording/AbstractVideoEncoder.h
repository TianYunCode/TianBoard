/**
 * @file AbstractVideoEncoder.h
 * @brief 声明录制使用的视频编码器抽象接口，规定启停、暂停、送帧与编码参数。
 */

#pragma once

#include <QtCore>

/**
 * @brief 视频编码器的抽象基类，只约定录制控制器需要的操作，不涉及具体容器格式。
 *
 * RecordingController::start() 创建具体实现（当前为 WindowsMediaVideoEncoder）并以自身为父对象持有；
 * 停止或编码结束后由控制器 deleteLater 回收，本类不自行管理生命周期。
 * 线程约定：start()/stop()/newPixmap()/newChapter() 均在 GUI 线程调用，实现内部不得假设有锁。
 * 暂停语义：只有 canPause() 返回 true 的实现才需要支持 pause()/unpause()，其余实现可直接返回失败。
 */
class AbstractVideoEncoder : public QObject
{
    Q_OBJECT;

public:
    /** @brief 构造编码器，只初始化接口层参数，不打开任何设备或文件。 */
    AbstractVideoEncoder(QObject *pParent = 0);

    /** @brief 析构编码器；不负责结束编码，调用方须先调用 stop()。 */
    virtual ~AbstractVideoEncoder();

    /**
     * @brief 打开输出目标并开始编码。
     * @return 失败返回 false，原因见 lastErrorMessage()。
     * @note 失败时实现可能已部分初始化，调用方仍需 deleteLater 回收。
     */
    virtual bool start() = 0;

    /** @brief 结束编码并关闭输出目标。 @return 失败返回 false。 */
    virtual bool stop()  = 0;

    /** @brief 暂停编码；默认实现不支持。 @return 默认返回 false。 */
    virtual bool pause()
    {
        return false;
    }

    /**
     * @brief 从暂停状态恢复；默认实现不支持。
     * @param audioTimeOffsetMs 本次暂停的毫秒时长，供实现补偿音轨时间戳。
     * @return 默认返回 false。
     */
    virtual bool unpause(long audioTimeOffsetMs = 0)
    {
        Q_UNUSED(audioTimeOffsetMs);
        return false;
    }

    /** @brief 本实现是否支持暂停/恢复。 */
    virtual bool canPause()
    {
        return false;
    }

    /**
     * @brief 送入一帧已渲染好的画面。
     * @param pImage 整帧图像，实现通常要求 QImage::Format_RGB32。
     * @param timestamp 相对录制开始的毫秒时间戳，暂停期间不推进。
     * @note 必须在 GUI 线程调用；编码器实现不保证线程安全。
     */
    virtual void newPixmap(const QImage &pImage, long timestamp) = 0;

    /** @brief 在指定时间戳写入章节标记；默认实现忽略。 */
    virtual void newChapter(const QString &pLabel, long timestamp);

    /**
     * @brief 设置录制帧率，须在 start() 前调用。
     * @param pFps 每秒帧数；小于 1 会被抬到 1——帧率被多处当除数用（帧间隔、
     *             编码器 profile 的 nanoSecondsPerFrame），0 会崩。
     */
    void setFramesPerSecond(int pFps)
    {
        mFramesPerSecond = qMax(1, pFps);
    }

    /** @brief 当前录制帧率。 */
    int framesPerSecond() const
    {
        return mFramesPerSecond;
    }

    /** @brief 输出文件扩展名（不含点），用于拼装录制文件名。 */
    virtual QString videoFileExtension() const = 0;

    /** @brief 设置输出文件路径，须在 start() 前调用。 */
    virtual void setVideoFileName(const QString &pFileName)
    {
        mVideoFileName = pFileName;
    }

    /** @brief 当前输出文件路径。 */
    virtual QString videoFileName()
    {
        return mVideoFileName;
    }

    /** @brief 设置输出画面尺寸，宽高通常取 4 的倍数。 */
    virtual void setVideoSize(const QSize &pSize)
    {
        mVideoSize = pSize;
    }

    /**
     * @brief 本段录制是否真的在采集音频。
     * @note 没有麦克风时录像是照常工作的（音频是可选能力），所以"录制成功"不等于
     * "带音轨"。自检需要一个能问的事实，而不是靠人去猜日志。默认不带。
     */
    virtual bool isCapturingAudio() const
    {
        return false;
    }

    /** @brief 当前输出画面尺寸。 */
    virtual QSize videoSize() const
    {
        return mVideoSize;
    }

    /** @brief 当前视频码率（bit/s）。 */
    virtual long videoBitsPerSecond() const
    {
        return mVideoBitsPerSecond;
    }

    /** @brief 设置视频码率（bit/s），须在 start() 前调用。 */
    virtual void setVideoBitsPerSecond(long pVideoBitsPerSecond)
    {
        mVideoBitsPerSecond = pVideoBitsPerSecond;
    }

    /** @brief 最近一次失败的错误文本；未失败时可能为空。 */
    virtual QString lastErrorMessage() = 0;

    /** @brief 设置录音设备名；空串表示交给实现选择默认设备。 */
    virtual void setAudioRecordingDevice(const QString pAudioRecordingDevice)
    {
        mAudioRecordingDevice = pAudioRecordingDevice;
    }

    /** @brief 当前录音设备名。 */
    virtual QString audioRecordingDevice()
    {
        return mAudioRecordingDevice;
    }

    /** @brief 是否录制音频，须在 start() 前调用；实现可在设备不可用时自行降级为不录音。 */
    virtual void setRecordAudio(bool pRecordAudio) = 0;

signals:
    /** @brief 编码过程中的状态文本，供界面提示使用。 */
    void encodingStatus(const QString &pStatus);

    /** @brief 编码结束；ok 为 false 时错误文本见 lastErrorMessage()。 */
    void encodingFinished(bool ok);

    /** @brief 音频电平（0-255），供 VU 表显示。 */
    void audioLevelChanged(quint8 level);

private:
    int mFramesPerSecond;

    QString mVideoFileName;

    QSize mVideoSize;

    long mVideoBitsPerSecond;

    QString mAudioRecordingDevice;
};
