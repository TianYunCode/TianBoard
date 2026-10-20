/**
 * @file WindowsMediaVideoEncoder.h
 * @brief 声明基于 Windows Media 的编码器实现，把画面与录音合成单个 WMV 文件。
 */

#pragma once

#include <QtGui>
#include "recording/AbstractVideoEncoder.h"

#include "recording/WindowsMediaFile.h"
#include "recording/WaveRecorder.h"

/**
 * @brief AbstractVideoEncoder 的 Windows 实现：WindowsMediaFile 写视频，WaveRecorder 采音频。
 *
 * 由 RecordingController 在开始录制时创建，编码结束时随 encodingFinished() 回收。
 * start() 会按视频尺寸/码率填充内置 profile 占位符；audio 设备打不开时降级为无声录制。
 * 暂停不关闭 WMV 文件：只停 waveIn 并丢弃暂停期间的视频帧，恢复时由调用方传入暂停时长补偿音轨。
 * 线程约束：除 waveIn 回调外全部在 GUI 线程；两路音频信号都用排队连接回到 GUI 线程再写 writer。
 */
class WindowsMediaVideoEncoder : public AbstractVideoEncoder {
    Q_OBJECT;

public:
    /** @brief 构造编码器，音频默认开启，尚未创建写入器与采集器。 */
    WindowsMediaVideoEncoder(QObject *pParent = 0);

    /** @brief 析构；正常路径上资源已由 stop() 释放。 */
    virtual ~WindowsMediaVideoEncoder();

    /** @brief 初始化音频与视频并开始编码；音频不可用时自动降级为无声。 */
    virtual bool start();

    /** @brief 暂停：停止 waveIn 采集，视频帧在暂停期间被丢弃。 */
    virtual bool pause();

    /** @brief 恢复：按暂停时长推进音频时间戳后重启 waveIn。 */
    virtual bool unpause(long audioTimeOffsetMs = 0);

    /** @brief 先停音频再关视频，成功与否通过 encodingFinished() 发出。 */
    virtual bool stop();

    /** @brief Windows Media 编码支持暂停/恢复。 */
    virtual bool canPause() { return true; };

    /** @brief 追加一帧视频；暂停期间的帧直接丢弃。 */
    virtual void newPixmap(const QImage &pPix, long timestamp);

    /** @brief 记录章节标记，文件关闭时统一写入。 */
    virtual void newChapter(const QString &pLabel, long timestamp);

    /** @brief 输出容器扩展名。 */
    virtual QString videoFileExtension() const { return "wmv"; }

    /** @brief 最近一次失败的错误文本。 */
    virtual QString lastErrorMessage() { return mLastErrorMessage; }

    /** @brief 开关音频；录制中切换会增删到写入器的连接。 */
    virtual void setRecordAudio(bool pRecordAudio);

private slots:
    /** @brief 统计一段 PCM 的峰值电平并发出 audioLevelChanged()。 */
    void processAudioBuffer(const QByteArray &audioData, long timestamp);

private:
    QPointer<WindowsMediaFile> mWMVideo;
    QPointer<WaveRecorder> mWaveRecorder;

    QString mLastErrorMessage;

    bool mRecordAudio;
    bool mIsPaused;

    quint8 mLastAudioLevel;
    bool mIsRecording;
};
