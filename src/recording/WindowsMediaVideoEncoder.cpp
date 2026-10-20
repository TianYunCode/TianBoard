#include "recording/WindowsMediaVideoEncoder.h"

#include <QtGui>

#include "utils/FileSystemUtils.h"

WindowsMediaVideoEncoder::WindowsMediaVideoEncoder(QObject *pParent)
    : AbstractVideoEncoder(pParent), mWMVideo(0), mWaveRecorder(0), mRecordAudio(true), mLastAudioLevel(0), mIsPaused(false)
{
    // 刻意留空
}

WindowsMediaVideoEncoder::~WindowsMediaVideoEncoder()
{
    // 刻意留空
}

bool WindowsMediaVideoEncoder::start()
{
    // profile 是带占位符的 WMV 配置模板，按本次录制的尺寸/码率/帧时长替换后再交给 WM 写入器。
    QString profile = FileSystemUtils::readTextFile(":/recording/TianBoard.prx");

    profile.replace("{in.videoWidth}", QString("%1").arg(videoSize().width()));
    profile.replace("{in.videoHeight}", QString("%1").arg(videoSize().height()));
    profile.replace("{in.bitsPerSecond}", QString("%1").arg(videoBitsPerSecond()));
    profile.replace("{in.nanoSecondsPerFrame}", QString("%1").arg(10000000 / framesPerSecond()));

    qDebug() << profile;

    // 音频是可选能力：设备打不开或启动失败时只关掉录音，视频仍照常录。
    if (mRecordAudio)
    {
        mWaveRecorder       = new WaveRecorder(this);

        bool audioAvailable = mWaveRecorder->init(audioRecordingDevice()) && mWaveRecorder->start();

        if (!audioAvailable)
        {
            mWaveRecorder->deleteLater();
            mWaveRecorder = 0;
            mRecordAudio  = false;
        }
    }

    mWMVideo = new WindowsMediaFile(this);

    if (!mWMVideo->init(videoFileName().replace("/", "\\"), profile, framesPerSecond(), videoSize().width(), videoSize().height(), 32))
    {
        mWMVideo->deleteLater();
        // 视频打不开时音频已经 init+start 过了，不收尾的话麦克风会一直采集到进程退出。
        if (mWaveRecorder)
        {
            mWaveRecorder->stop();
            mWaveRecorder->close();
            mWaveRecorder->deleteLater();
            mWaveRecorder = nullptr;
        }
        return false;
    }

    if (mRecordAudio)
    {
        // waveIn 回调在驱动线程里跑；IWMWriter 只能在 GUI 线程写（视频帧也在那里写）。
        // 队列连接 + QByteArray 拷贝把音频数据搬回同一线程，避免两个线程无锁操作同一个 writer。
        connect(mWaveRecorder, &WaveRecorder::newWaveBuffer, mWMVideo, &WindowsMediaFile::appendAudioBuffer, Qt::QueuedConnection);
        connect(mWaveRecorder, &WaveRecorder::newWaveBuffer, this, &WindowsMediaVideoEncoder::processAudioBuffer, Qt::QueuedConnection);
    }

    mIsRecording = true;

    return true;
}

bool WindowsMediaVideoEncoder::stop()
{
    bool audioOk = true;

    // 先断音频连接并关闭 waveIn，确保最后几块 PCM 已进入 writer，再结束视频写入。
    if (mWaveRecorder)
    {
        disconnect(mWaveRecorder, &WaveRecorder::newWaveBuffer, mWMVideo, &WindowsMediaFile::appendAudioBuffer);
        disconnect(mWaveRecorder, &WaveRecorder::newWaveBuffer, this, &WindowsMediaVideoEncoder::processAudioBuffer);

        mWaveRecorder->stop();
        audioOk           = mWaveRecorder->close();
        mLastErrorMessage = mWaveRecorder->lastErrorMessage();
        mWaveRecorder->deleteLater();
        mWaveRecorder = 0;
        emit audioLevelChanged(0);
    }

    bool videoOk = true;

    if (mWMVideo)
    {
        videoOk           = mWMVideo->close();
        mLastErrorMessage = mWMVideo->lastErrorMessage();
        mWMVideo->deleteLater();
        mWMVideo = 0;
    }

    bool ok = audioOk && videoOk;

    emit encodingFinished(ok);

    mIsRecording = false;

    return ok;
}

void WindowsMediaVideoEncoder::newPixmap(const QImage &pPix, long timestamp)
{
    // 暂停期间控制器仍可能触发重绘，这里直接丢弃，避免把暂停画面写进视频。
    if (mWMVideo && !mIsPaused)
    {
        if (!mWMVideo->appendVideoFrame(pPix, timestamp))
        {
            qWarning() << "Error adding new video frame" << mWMVideo->lastErrorMessage();
        }
    }
}

void WindowsMediaVideoEncoder::newChapter(const QString &pLabel, long timestamp)
{
    if (mWMVideo)
    {
        mWMVideo->startNewChapter(pLabel, timestamp);
    }
}

void WindowsMediaVideoEncoder::setRecordAudio(bool pRecordAudio)
{
    if (mRecordAudio != pRecordAudio)
    {
        mRecordAudio = pRecordAudio;

        if (mRecordAudio)
        {
            connect(mWaveRecorder, &WaveRecorder::newWaveBuffer, mWMVideo, &WindowsMediaFile::appendAudioBuffer, Qt::DirectConnection);
        }
        else
        {
            disconnect(mWaveRecorder, &WaveRecorder::newWaveBuffer, mWMVideo, &WindowsMediaFile::appendAudioBuffer);
            emit audioLevelChanged(0);
        }
    }
}

void WindowsMediaVideoEncoder::processAudioBuffer(const QByteArray &audioData, long timestamp)
{
    Q_UNUSED(timestamp);

    if (mWaveRecorder && mRecordAudio)
    {
        // 按 16 bit 样本逐点处理：绝对值除以 128 归一到 0-255，取本块最大能量再开方作为 VU 表电平。
        const long    samplesCount = audioData.size() / 2;
        const qint16 *samples      = reinterpret_cast<const qint16 *>(audioData.constData());
        quint16       maxRMS       = 0;

        for (long i = 0; i < samplesCount; i++)
        {
            quint8 current     = qAbs(samples[i] / 128);

            quint16 currentRMS = current * current;
            maxRMS             = qMax(maxRMS, currentRMS);
        }

        quint8 max = sqrt((qreal)maxRMS);

        if (max != mLastAudioLevel)
        {
            mLastAudioLevel = max;
            emit audioLevelChanged(mLastAudioLevel);
        }
    }
}

bool WindowsMediaVideoEncoder::pause()
{
    bool result = true;

    if (!mIsPaused && mIsRecording)
    {
        if (mWaveRecorder)
        {
            result = mWaveRecorder->stop();
            emit audioLevelChanged(0);
        }

        mIsPaused = true;
    }

    return result;
}

bool WindowsMediaVideoEncoder::unpause(long audioTimeOffsetMs)
{
    bool result = true;

    if (mIsPaused && mIsRecording)
    {
        if (mWaveRecorder)
        {
            // 视频时间戳会跳过暂停时长，音频也要跟着跳，否则恢复后音画错位。
            mWaveRecorder->skipTime(audioTimeOffsetMs);
            result = mWaveRecorder->start();
        }

        mIsPaused = false;
    }

    return result;
}
