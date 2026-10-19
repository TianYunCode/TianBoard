#include "recording/WindowsMediaVideoEncoder.h"

#include <QtGui>

#include "utils/FileSystemUtils.h"

#include "core/Application.h"

WindowsMediaVideoEncoder::WindowsMediaVideoEncoder(QObject *pParent)
    : AbstractVideoEncoder(pParent),
      mWMVideo(0),
      mWaveRecorder(0),
      mRecordAudio(true),
      mLastAudioLevel(0),
      mIsPaused(false) {
    // NOOP
}


WindowsMediaVideoEncoder::~WindowsMediaVideoEncoder() {
    // NOOP
}


bool WindowsMediaVideoEncoder::start() {
    QString profile = FileSystemUtils::readTextFile(":/recording/TianBoard.prx");

    profile.replace("{in.videoWidth}", QString("%1").arg(videoSize().width()));
    profile.replace("{in.videoHeight}", QString("%1").arg(videoSize().height()));
    profile.replace("{in.bitsPerSecond}", QString("%1").arg(videoBitsPerSecond()));
    profile.replace("{in.nanoSecondsPerFrame}", QString("%1").arg(10000000 / framesPerSecond()));

    qDebug() << profile;

    if (mRecordAudio) {
        mWaveRecorder = new WaveRecorder(this);

        bool audioAvailable = mWaveRecorder->init(audioRecordingDevice()) && mWaveRecorder->start();

        if (!audioAvailable) {
            mWaveRecorder->deleteLater();
            mWaveRecorder = 0;
            mRecordAudio = false;
        }
    }

    mWMVideo = new WindowsMediaFile(this);

    if (!mWMVideo->init(videoFileName().replace("/", "\\"), profile, framesPerSecond(), videoSize().width(),
                        videoSize().height(), 32)) {
        mWMVideo->deleteLater();
        return false;
    }

    if (mRecordAudio) {
        connect(mWaveRecorder, &WaveRecorder::newWaveBuffer, mWMVideo, &WindowsMediaFile::appendAudioBuffer,
                Qt::DirectConnection);
        connect(mWaveRecorder, &WaveRecorder::newWaveBuffer, this,
                &WindowsMediaVideoEncoder::processAudioBuffer, Qt::DirectConnection);
    }

    mIsRecording = true;

    return true;
}


bool WindowsMediaVideoEncoder::stop() {
    bool audioOk = true;

    if (mWaveRecorder) {
        disconnect(mWaveRecorder, &WaveRecorder::newWaveBuffer, mWMVideo,
                   &WindowsMediaFile::appendAudioBuffer);
        disconnect(mWaveRecorder, &WaveRecorder::newWaveBuffer, this,
                   &WindowsMediaVideoEncoder::processAudioBuffer);

        mWaveRecorder->stop();
        audioOk = mWaveRecorder->close();
        mLastErrorMessage = mWaveRecorder->lastErrorMessage();
        mWaveRecorder->deleteLater();
        mWaveRecorder = 0;
        emit audioLevelChanged(0);
    }

    bool videoOk = true;

    if (mWMVideo) {
        videoOk = mWMVideo->close();
        mLastErrorMessage = mWMVideo->lastErrorMessage();
        mWMVideo->deleteLater();
        mWMVideo = 0;
    }

    bool ok = audioOk && videoOk;

    emit encodingFinished(ok);

    mIsRecording = false;

    return ok;
}


void WindowsMediaVideoEncoder::newPixmap(const QImage &pPix, long timestamp) {
    if (mWMVideo && !mIsPaused) {
        if (!mWMVideo->appendVideoFrame(pPix, timestamp)) {
            qWarning() << "Error adding new video frame" << mWMVideo->lastErrorMessage();
        }
    }
}


void WindowsMediaVideoEncoder::newChapter(const QString &pLabel, long timestamp) {
    if (mWMVideo)
        mWMVideo->startNewChapter(pLabel, timestamp);
}


void WindowsMediaVideoEncoder::setRecordAudio(bool pRecordAudio) {
    if (mRecordAudio != pRecordAudio) {
        mRecordAudio = pRecordAudio;

        if (mRecordAudio) {
            connect(mWaveRecorder, &WaveRecorder::newWaveBuffer, mWMVideo,
                    &WindowsMediaFile::appendAudioBuffer, Qt::DirectConnection);
        } else {
            disconnect(mWaveRecorder, &WaveRecorder::newWaveBuffer, mWMVideo,
                       &WindowsMediaFile::appendAudioBuffer);
            emit audioLevelChanged(0);
        }
    }
}


void WindowsMediaVideoEncoder::processAudioBuffer(WAVEHDR *waveBuffer, long timestamp) {
    Q_UNUSED(timestamp);

    if (mWaveRecorder && mRecordAudio) {
        long samplesCount = waveBuffer->dwBytesRecorded / 2;
        qint16 *samples = (qint16 *)waveBuffer->lpData;
        quint16 maxRMS = 0;

        for (long i = 0; i < samplesCount; i++) {
            quint8 current = qAbs(samples[i] / 128);

            quint16 currentRMS = current * current;
            maxRMS = qMax(maxRMS, currentRMS);
        }

        quint8 max = sqrt((qreal)maxRMS);

        if (max != mLastAudioLevel) {
            mLastAudioLevel = max;
            emit audioLevelChanged(mLastAudioLevel);
        }
    }
}


bool WindowsMediaVideoEncoder::pause() {
    bool result = true;

    if (!mIsPaused && mIsRecording) {
        if (mWaveRecorder) {
            result = mWaveRecorder->stop();
            emit audioLevelChanged(0);
        }

        mIsPaused = true;
    }

    return result;
}


bool WindowsMediaVideoEncoder::unpause() {
    bool result = true;

    if (mIsPaused && mIsRecording) {
        if (mWaveRecorder) {
            result = mWaveRecorder->start();
        }

        mIsPaused = false;
    }

    return result;
}
