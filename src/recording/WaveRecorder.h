/**
 * @file WaveRecorder.h
 * @brief 声明基于 Windows waveIn 的音频采集器，向编码器提供带毫秒时间戳的 PCM 数据。
 */

#pragma once

#include <QtCore>

#include "Windows.h"
#pragma comment(lib, "winmm.lib")

/**
 * @brief waveIn 音频采集封装：打开设备、循环投递缓冲区，并把录到的 PCM 连同时间戳发出。
 *
 * 生命周期由 WindowsMediaVideoEncoder 管理：start() 里 init() + start()，录音不可用时整体降级；
 * stop() 里 stop() + close() 释放设备与缓冲区。暂停/恢复只调用 stop()/start()，
 * 不重新 init()，因此时间戳的零点仍保持为本次录制的起点，恢复时用 skipTime() 补上暂停时长。
 * 线程约束：init/start/stop/close 在 GUI 线程调用；数据回调 waveInProc 运行在驱动线程，
 * 只有信号 newWaveBuffer 以值拷贝方式跨线程交回 GUI 线程，回调内不碰 Qt 对象状态。
 */
class WaveRecorder : public QObject
{
    Q_OBJECT;

public:
    /** @brief 构造采集器，设置默认 PCM 格式（44.1 kHz、双声道、16 bit），不打开设备。 */
    WaveRecorder(QObject *pParent = 0);

    /** @brief 析构；不自动关闭设备，调用方应先 close()。 */
    virtual ~WaveRecorder();

    /**
     * @brief 打开 waveIn 设备并预投递 6 个缓冲区。
     * @param waveInDeviceName 设备名；空串使用 WAVE_MAPPER 默认设备，找不到同名设备也回退到默认。
     * @return 打开或准备缓冲区失败时返回 false，错误文本见 lastErrorMessage()。
     * @note 会把时间戳清零，表示一次新录制开始；暂停恢复不要调用本函数。
     */
    bool init(const QString &waveInDeviceName = QString(""));

    /** @brief 启动（或从暂停恢复）采集；重复调用只会重启设备。 @return 设备启动失败返回 false。 */
    bool start();

    /** @brief 停止采集但保留已打开的设备和缓冲区，供恢复时复用。 */
    bool stop();

    /** @brief 复位设备、释放所有缓冲区并关闭 waveIn；结束后需重新 init() 才能再录。 */
    bool close();

    /** @brief 最近一次失败的错误文本。 */
    QString lastErrorMessage()
    {
        return mLastErrorMessage;
    }

    /** @brief 枚举系统录音输入设备名。 */
    static QStringList waveInDevices();

    /** @brief 暂停恢复时把暂停时长补进音频时间戳，避免音画在暂停后错位。 */
    void skipTime(long milliseconds)
    {
        mMsTimeStamp += milliseconds;
    }

signals:
    /**
     * @brief 交出一段 PCM 数据及其起始时间戳（毫秒）。
     * @note 数据是拷贝：驱动线程在 emit 返回后会立刻回收 WAVEHDR，跨线程排队时不能传指针。
     */
    void newWaveBuffer(const QByteArray &audioData, long timestampMs);

private:
    /**
     * @brief waveIn 驱动回调：回收已录满的缓冲区，取出数据后重新投递。
     * @warning 运行在驱动线程，不能在此直接操作界面或编码器对象。
     */
    static void CALLBACK waveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

    /**
     * @brief 拷贝缓冲区数据并以当前时间戳发出 newWaveBuffer()，随后推进时间戳。
     * @warning 由 waveInProc() 在驱动线程调用。
     */
    void emitNewWaveBuffer(WAVEHDR *pBuffer);

    void setLastErrorMessage(QString pLastErrorMessage)
    {
        mLastErrorMessage = pLastErrorMessage;
        qWarning() << mLastErrorMessage;
    }

    bool isRecording() const
    {
        return mIsRecording;
    }

    QString mLastErrorMessage;

    HWAVEIN mWaveInDevice;

    QList<WAVEHDR *> mWaveBuffers;

    volatile bool mIsRecording;
    volatile long mMsTimeStamp;

    int mBufferLengthInMs;

    QTime mRecordingStartTime;

    int mNbChannels;
    int mSampleRate;
    int mBitsPerSample;
};
