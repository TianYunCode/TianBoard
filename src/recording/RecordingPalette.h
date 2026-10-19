#pragma once

#include "ui/ActionPalette.h"
#include "recording/RecordingController.h"

#include <QtGui>
#include <QLabel>

#include <memory>

class VuMeter;

class RecordingPalette : public ActionPalette {
    Q_OBJECT;

public:
    RecordingPalette(QWidget *parent = 0);
    virtual ~RecordingPalette();

public slots:
    void recordingStateChanged(RecordingController::RecordingState);
    void recordingProgressChanged(qint64 ms);
    void audioLevelChanged(quint8 level);

private:
    struct Private;
    std::unique_ptr<Private> d;
};


class VuMeter : public QWidget {
public:
    VuMeter(QWidget *pParent);
    virtual ~VuMeter();

    void setVolume(quint8 pVolume);

protected:
    virtual void paintEvent(QPaintEvent *e);

private:
    quint8 mVolume;
};
