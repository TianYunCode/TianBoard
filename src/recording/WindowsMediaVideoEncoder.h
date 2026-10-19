#pragma once

#include <QtGui>
#include "recording/AbstractVideoEncoder.h"

#include "recording/WindowsMediaFile.h"
#include "recording/WaveRecorder.h"

class WindowsMediaVideoEncoder : public AbstractVideoEncoder
{
    Q_OBJECT;

    public:
        WindowsMediaVideoEncoder(QObject* pParent = 0);

        virtual ~WindowsMediaVideoEncoder();

        virtual bool start();
        virtual bool pause();
        virtual bool unpause();
        virtual bool stop();

        virtual bool canPause() { return true;};

        virtual void newPixmap(const QImage& pPix, long timestamp);
        virtual void newChapter(const QString& pLabel, long timestamp);

        virtual QString videoFileExtension() const
        {
            return "wmv";
        }

        virtual QString lastErrorMessage()
        {
            return mLastErrorMessage;
        }

        virtual void setRecordAudio(bool pRecordAudio);

    private slots:
        void processAudioBuffer(WAVEHDR*, long);

    private:
        QPointer<WindowsMediaFile> mWMVideo;
        QPointer<WaveRecorder> mWaveRecorder;

        QString mLastErrorMessage;

        bool mRecordAudio;
        bool mIsPaused;

        quint8 mLastAudioLevel;
        bool mIsRecording;

};

