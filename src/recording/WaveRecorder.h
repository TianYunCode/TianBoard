#pragma once

#include <QtCore>

#include "Windows.h"
#pragma comment(lib, "winmm.lib")

class WaveRecorder : public QObject {
    Q_OBJECT;

public:
    WaveRecorder(QObject *pParent = 0);
    virtual ~WaveRecorder();

    bool init(const QString &waveInDeviceName = QString(""));

    bool start();
    bool stop();

    bool close();

    QString lastErrorMessage() { return mLastErrorMessage; }

    static QStringList waveInDevices();

    // 暂停恢复时把暂停时长补进音频时间戳，避免音画在暂停后错位。
    void skipTime(long milliseconds) { mMsTimeStamp += milliseconds; }

signals:
    // 交出去的是拷贝：驱动线程在 emit 返回后会立刻回收 WAVEHDR，跨线程排队时不能传指针。
    void newWaveBuffer(const QByteArray &audioData, long timestampMs);

private:
    static void CALLBACK waveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1,
                                    DWORD_PTR dwParam2);

    void emitNewWaveBuffer(WAVEHDR *pBuffer);

    void setLastErrorMessage(QString pLastErrorMessage) {
        mLastErrorMessage = pLastErrorMessage;
        qWarning() << mLastErrorMessage;
    }

    bool isRecording() const { return mIsRecording; }

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
