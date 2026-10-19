#include "recording/RecordingPalette.h"

#include "recording/RecordingController.h"

#include "core/Application.h"

#include "ui/Resources.h"

#include "settings/Settings.h"

#include "ui/MainWindow.h"


struct RecordingPalette::Private
{
    QLabel *mTimerLabel{nullptr};
    VuMeter *mLevelMeter{nullptr};
};

RecordingPalette::RecordingPalette(QWidget *parent)
     : ActionPalette(Qt::Horizontal, parent)
     , d(new Private)
{
    addAction(Application::mainWindow->ui().actionRecordingStart);

    d->mTimerLabel = new QLabel(this);
    d->mTimerLabel->setStyleSheet(QString("QLabel {color: #1296DB; font-size: 14px; font-weight: bold; font-family: Arial; background-color: transparent; border: none}"));
    recordingProgressChanged(0);

    layout()->addWidget(d->mTimerLabel);

    d->mLevelMeter = new VuMeter(this);
    d->mLevelMeter->setMinimumSize(6, 32);

    layout()->addWidget(d->mLevelMeter);

    addAction(Application::mainWindow->ui().actionRecordingConfig);

    foreach(QWidget* menuWidget,  Application::mainWindow->ui().actionRecordingConfig->associatedWidgets())
    {
        QToolButton *tb = qobject_cast<QToolButton*>(menuWidget);

        tb->setIconSize(QSize(16, 16));

        if (tb && !tb->menu())
        {
            tb->setObjectName("buttonMenu");
            tb->setPopupMode(QToolButton::InstantPopup);
            QMenu* menu = new QMenu(this);

            foreach(QAction* audioInputAction, RecordingController::instance()->audioRecordingDevicesActions())
            {
                menu->addAction(audioInputAction);
            }

            menu->addSeparator();

            foreach(QAction* videoSizeAction, RecordingController::instance()->videoSizeActions())
            {
                menu->addAction(videoSizeAction);
            }

            menu->addSeparator();

            QList<QAction*> publication = RecordingController::instance()->publicationActions();

            foreach(QAction* publicationAction, publication)
            {
                menu->addAction(publicationAction);
            }

            tb->setMenu(menu);
        }
    }
}


RecordingPalette::~RecordingPalette()
{
    // NOOP
}


void RecordingPalette::recordingStateChanged(RecordingController::RecordingState state)
{
    if (state == RecordingController::Recording)
    {
        Application::mainWindow->ui().actionRecordingStart->setChecked(true);
        Application::mainWindow->ui().actionRecordingStart->setEnabled(true);

        Application::mainWindow->ui().actionRecordingPause->setChecked(false);
        Application::mainWindow->ui().actionRecordingPause->setEnabled(true);

        //Application::mainWindow->actionRecordingMic->setEnabled(false);

        Application::mainWindow->ui().actionRecordingConfig->setEnabled(false);
    }
    else if (state == RecordingController::Stopped)
    {
        Application::mainWindow->ui().actionRecordingStart->setChecked(false);
        Application::mainWindow->ui().actionRecordingStart->setEnabled(true);

        Application::mainWindow->ui().actionRecordingPause->setChecked(false);
        Application::mainWindow->ui().actionRecordingPause->setEnabled(false);

        //Application::mainWindow->actionRecordingMic->setEnabled(true);
        Application::mainWindow->ui().actionRecordingConfig->setEnabled(true);
    }
    else if (state == RecordingController::Paused)
    {
        Application::mainWindow->ui().actionRecordingStart->setChecked(true);
        Application::mainWindow->ui().actionRecordingStart->setEnabled(true);

        Application::mainWindow->ui().actionRecordingPause->setChecked(true);
        Application::mainWindow->ui().actionRecordingPause->setEnabled(true);

        //Application::mainWindow->actionRecordingMic->setEnabled(false);
        Application::mainWindow->ui().actionRecordingConfig->setEnabled(false);
    }
    else
    {
        Application::mainWindow->ui().actionRecordingStart->setEnabled(false);
        Application::mainWindow->ui().actionRecordingPause->setEnabled(false);
        Application::mainWindow->ui().actionRecordingConfig->setEnabled(false);
    }
}


void RecordingPalette::recordingProgressChanged(qint64 ms)
{
    int min = ms / 60000;
    int seconds = (ms / 1000) % 60;

    d->mTimerLabel->setText(QString("%1:%2").arg(min, 3, 10, QChar(' ')).arg(seconds, 2, 10, QChar('0')));
}


void RecordingPalette::audioLevelChanged(quint8 level)
{
    d->mLevelMeter->setVolume(level);
}


VuMeter::VuMeter(QWidget* pParent)
    : QWidget(pParent)
    , mVolume(0)
{
    // NOOP
}


VuMeter::~VuMeter()
{
    // NOOP
}

void VuMeter::setVolume(quint8 pVolume)
{
    if (mVolume != pVolume)
    {
        mVolume = pVolume;
        update();
    }
}


void VuMeter::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e);

    QPainter painter(this);

    int h = (height() - 8) * mVolume / 255;
    QRectF rect(0, height() - 4 - h, width(), h);

    painter.fillRect(rect, Settings::documentViewLightColor);
}

