#include "recording/WaveRecorder.h"

#include "Mmsystem.h"

WaveRecorder::WaveRecorder(QObject *pParent)
    : QObject(pParent), mIsRecording(false), mMsTimeStamp(0), mBufferLengthInMs(100), mNbChannels(2), mSampleRate(44100), mBitsPerSample(16)
{
    // 刻意留空
}

WaveRecorder::~WaveRecorder()
{
    // 刻意留空
}

bool WaveRecorder::init(const QString &waveInDeviceName)
{
    UINT deviceID = WAVE_MAPPER;

    // 时间戳按“本次录制”清零。start() 会被暂停/恢复复用，不能在那里清零。
    mMsTimeStamp  = 0;

    // 按设备名查找输入通道；名字对不上或为空时保持 WAVE_MAPPER，交给系统选默认设备。
    if (waveInDeviceName.length() > 0)
    {
        int count = waveInGetNumDevs();

        WAVEINCAPS caps;
        for (int i = 0; i < count; i++)
        {
            if (waveInGetDevCaps(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR)
            {
                QString deviceName = QString::fromWCharArray(caps.szPname);

                if (deviceName == waveInDeviceName)
                {
                    deviceID = i;
                    break;
                }
            }
        }
    }

    WAVEFORMATEX format;
    format.cbSize          = 0;
    format.wFormatTag      = WAVE_FORMAT_PCM;
    format.nChannels       = mNbChannels;
    format.wBitsPerSample  = mBitsPerSample;
    format.nSamplesPerSec  = mSampleRate;
    format.nBlockAlign     = format.nChannels * (format.wBitsPerSample / 8);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

    // waveInOpen 的回调参数类型是 DWORD_PTR；原来写成 (DWORD) 在 x64 下会把函数指针
    // 截断成 32 位，驱动回调时直接访问违例。
    if (waveInOpen(&mWaveInDevice, deviceID, &format, (DWORD_PTR)waveInProc, (DWORD_PTR)this, CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
    {
        setLastErrorMessage("Cannot open wave in device ");
        return false;
    }

    int nbBuffers        = 6;
    int sampleBufferSize = 44100 * 2 * 2 * mBufferLengthInMs / 1000; // 44.1 kHz × 双声道 × 16 bit × 缓冲时长

    for (int i = 0; i < nbBuffers; i++)
    {
        WAVEHDR *buffer = new WAVEHDR();
        ZeroMemory(buffer, sizeof(WAVEHDR));

        buffer->lpData = (LPSTR) new BYTE[sampleBufferSize];
        ZeroMemory(buffer->lpData, sampleBufferSize);

        buffer->dwBufferLength = sampleBufferSize;
        buffer->dwFlags        = 0;

        if (waveInPrepareHeader(mWaveInDevice, buffer, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
        {
            setLastErrorMessage("Cannot prepare wave header");
            return false;
        }

        if (waveInAddBuffer(mWaveInDevice, buffer, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
        {
            setLastErrorMessage("Cannot add buffer");
            return false;
        }

        mWaveBuffers << buffer;
    }

    return true;
}

bool WaveRecorder::start()
{
    // 暂停恢复也会走到这里：已有录制起点时不要重置，否则时间戳基准会漂移。
    if (!mIsRecording)
    {
        mRecordingStartTime = QTime::currentTime();
        mIsRecording        = true;
    }

    if (waveInStart(mWaveInDevice) != MMSYSERR_NOERROR)
    {
        setLastErrorMessage("Cannot start wave in device ");
        return false;
    }

    return true;
}

bool WaveRecorder::stop()
{
    mIsRecording = false;

    if (waveInStop(mWaveInDevice) != MMSYSERR_NOERROR)
    {
        setLastErrorMessage("Cannot stop wave in device ");
        return false;
    }

    return true;
}

bool WaveRecorder::close()
{
    // 先 reset 把队列里的缓冲区都标记为完成，紧接着 unprepare/释放才不会失败。
    if (waveInReset(mWaveInDevice) != MMSYSERR_NOERROR)
    {
        setLastErrorMessage("Cannot reset wave in device ");
        return false;
    }

    foreach (WAVEHDR *buffer, mWaveBuffers)
    {
        waveInUnprepareHeader(mWaveInDevice, buffer, sizeof(WAVEHDR));
        delete[] buffer->lpData;
        delete buffer;
    }

    mWaveBuffers.clear();

    if (waveInClose(mWaveInDevice) != MMSYSERR_NOERROR)
    {
        setLastErrorMessage("Cannot close wave in device ");
        return false;
    }

    return true;
}

void CALLBACK WaveRecorder::waveInProc(HWAVEIN waveInDevice, UINT message, DWORD_PTR instance, DWORD_PTR param1, DWORD_PTR param2)
{
    Q_UNUSED(param2);

    if (message == WIM_DATA)
    {
        WaveRecorder *recorder = (WaveRecorder *)instance;
        WAVEHDR      *buffer   = (WAVEHDR *)param1;

        if (recorder && buffer && recorder->isRecording())
        {
            // WIM_DATA 表示这块缓冲区已录满且头部仍标记为 prepared；先 unprepare，
            // 才能重新 prepare 并 AddBuffer，让同一块内存回到采集队列继续循环。
            waveInUnprepareHeader(waveInDevice, buffer, sizeof(WAVEHDR));
            recorder->emitNewWaveBuffer(buffer);

            if (waveInPrepareHeader(waveInDevice, buffer, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
            {
                recorder->setLastErrorMessage("Cannot recycle wave header");
            }

            if (waveInAddBuffer(waveInDevice, buffer, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
            {
                recorder->setLastErrorMessage("Cannot recycle buffer");
            }
        }
    }
}

void WaveRecorder::emitNewWaveBuffer(WAVEHDR *pBuffer)
{
    if (mIsRecording)
    {
        // 拷贝出本块 PCM；时间戳取本块起点，发完后再按实际字节数推进，暂停由 skipTime() 补齐。
        const QByteArray audioData(reinterpret_cast<const char *>(pBuffer->lpData), static_cast<int>(pBuffer->dwBytesRecorded));
        emit             newWaveBuffer(audioData, mMsTimeStamp);

        long bufferMs  = pBuffer->dwBytesRecorded / mNbChannels / (mBitsPerSample / 8) * 1000 / mSampleRate;

        mMsTimeStamp  += bufferMs;
    }
}

QStringList WaveRecorder::waveInDevices()
{
    QStringList devices;

    int count = waveInGetNumDevs();

    WAVEINCAPS caps;
    for (int i = 0; i < count; i++)
    {
        if (waveInGetDevCaps(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR)
        {
            devices << QString::fromWCharArray(caps.szPname);
        }
        else
        {
            qDebug() << "读不到录音设备的能力:" << i;
        }
    }

    // 混音器仅用于调试输出，不是可采集的录音端点，因此不加入返回列表。
    int mixersCount = mixerGetNumDevs();

    for (int i = 0; i < mixersCount; i++)
    {
        MIXERCAPS mixerCaps;

        if (mixerGetDevCaps(i, &mixerCaps, sizeof(mixerCaps)) == MMSYSERR_NOERROR)
        {
            qDebug() << "混音器:" << QString::fromWCharArray(caps.szPname);
        }
    }

    return devices;
}
