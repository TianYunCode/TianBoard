#include "recording/RecordingController.h"
#include "core/VirtualDesktop.h"

#include "utils/FileSystemUtils.h"
#include "utils/StringUtils.h"
#include "platform/PlatformUtils.h"

#include "core/Application.h"
#include "core/Settings.h"
#include "core/Setting.h"
#include "core/DisplayManager.h"

#include "whiteboard/BoardController.h"
#include "whiteboard/BoardView.h"
#include "whiteboard/BoardPaletteManager.h"

#include "ui/MainWindow.h"

#include "web/WebController.h"
#include "web/WebView.h"

#include "whiteboard/GraphicsScene.h"

#include "recording/AbstractVideoEncoder.h"

#include "recording/RecordingPalette.h"




// 录屏只有 Windows 一套实现（WindowsMedia 视频 + WaveIn 音频）。其他平台不再引用
// 本仓库里并不存在的 ffmpeg 封装，mVideoEncoder 保持为空，录制入口自动不起作用。
#ifdef Q_OS_WIN
    #include "recording/WindowsMediaVideoEncoder.h"
    #include "recording/WaveRecorder.h"
#endif


RecordingController* RecordingController::sInstance = 0;

unsigned int RecordingController::sBackgroundColor = 0x00000000;  // BBGGRRAA


RecordingController::RecordingController(QObject* pParent)
    : QObject(pParent)
    , mVideoEncoder(0)
    , mIsGrabbing(false)
    , mInitialized(false)
    , mVideoFramesPerSecondAtStart(10)
    , mVideoFrameSizeAtStart(1024, 768)
    , mVideoBitsPerSecondAtStart(1700000)
    , mSourceWidget(0)
    , mSourceScene(0)
    , mScreenGrabingTimerEventID(0)
    , mRecordingProgressTimerEventID(0)
    , mRecordingPalette(0)
    , mRecordingState(Stopped)
    , mApplicationIsClosing(false)
    , mRecordingTimestampOffset(0)
    , mDefaultAudioInputDeviceAction(0)
    , mNoAudioInputDeviceAction(0)
    , mSmallVideoSizeAction(0)
    , mMediumVideoSizeAction(0)
    , mFullVideoSizeAction(0)
{
    connect(Application::applicationController, &ApplicationController::mainModeChanged,
            this, &RecordingController::applicationMainModeChanged);

    connect(Application::applicationController, &ApplicationController::desktopMode,
            this, &RecordingController::applicationDesktopMode);

    connect(Application::webController, &WebController::activeWebPageChanged,
            this, &RecordingController::webActiveWebPageChanged);

    connect(Application::app(), &QApplication::lastWindowClosed,
            this, &RecordingController::applicationAboutToQuit);

}


RecordingController::~RecordingController()
{
    // NOOP
}


void RecordingController::applicationAboutToQuit()
{
    mApplicationIsClosing = true;

    if(mRecordingState == Recording || mRecordingState == Paused)
    {
        stop();
    }
}


void RecordingController::groupActionTriggered(QAction* action)
{
    Q_UNUSED(action);
    updateActionState();
}


void RecordingController::actionToggled(bool checked)
{
    Q_UNUSED(checked);
    updateActionState();
}


void RecordingController::updateActionState()
{
    if (mSmallVideoSizeAction && mSmallVideoSizeAction->isChecked())
        Settings::settings()->recordingVideoSize->set("Small");
    else if (mFullVideoSizeAction && mFullVideoSizeAction->isChecked())
        Settings::settings()->recordingVideoSize->set("Full");
    else
        Settings::settings()->recordingVideoSize->reset();

    Settings::settings()->recordingAudioRecordingDevice->reset();

    if (mDefaultAudioInputDeviceAction && mDefaultAudioInputDeviceAction->isChecked())
         Settings::settings()->recordingAudioRecordingDevice->set("Default");
    else if (mNoAudioInputDeviceAction && mNoAudioInputDeviceAction->isChecked())
         Settings::settings()->recordingAudioRecordingDevice->set("None");
    else
    {
        foreach(QAction* action, mAudioInputDevicesActions)
        {
            if (action->isChecked())
            {
                Settings::settings()->recordingAudioRecordingDevice->set(action->text());
                break;
            }
        }
    }

}


void RecordingController::setSourceWidget(QWidget* pWidget)
{
    if (mSourceWidget != pWidget)
    {
        if (mSourceWidget == VirtualDesktop::instance())
        {
            killTimer(mScreenGrabingTimerEventID);
            mScreenGrabingTimerEventID = 0;
        }
        else if (mSourceWidget)
        {
            mSourceWidget->removeEventFilter(this);
        }

        mSourceWidget = pWidget;
        mInitialized = false;
        mViewToVideoTransform.reset();
        mLatestCapture.fill(sBackgroundColor);

        if (mSourceWidget)
        {
            QSizeF sourceWidgetSize(mSourceWidget->size());

            if (mSourceWidget == VirtualDesktop::instance())
                sourceWidgetSize = VirtualDesktop::instance()->availableGeometry(Application::applicationController->displayManager()->controleScreenIndex()).size();

            QSizeF videoFrameSize(mVideoFrameSizeAtStart);

            qreal scaleHorizontal = videoFrameSize.width() / sourceWidgetSize.width();
            qreal scaleVertical = videoFrameSize.height() / sourceWidgetSize.height();

            qreal scale = qMin(scaleHorizontal, scaleVertical);

            mViewToVideoTransform.scale(scale, scale);

            BoardView *bv = qobject_cast<BoardView *>(mSourceWidget);

            if (bv)
            {
                connect(Application::boardController, &BoardController::activeSceneChanged, this, &RecordingController::activeSceneChanged);
                connect(Application::boardController, &BoardController::backgroundChanged, this, &RecordingController::sceneBackgroundChanged);
                connect(Application::boardController, &BoardController::controlViewportChanged, this, &RecordingController::activeSceneChanged);

                activeSceneChanged();
            }
            else
            {
                QSizeF scaledWidgetSize = sourceWidgetSize * scale;

                int offsetX = (videoFrameSize.width() - scaledWidgetSize.width()) / 2;
                int offsetY = (videoFrameSize.height() - scaledWidgetSize.height()) / 2;

                mViewToVideoTransform.translate(offsetX / scale, offsetY / scale);

                disconnect(Application::boardController, &BoardController::activeSceneChanged, this, &RecordingController::activeSceneChanged);
                disconnect(Application::boardController, &BoardController::backgroundChanged, this, &RecordingController::sceneBackgroundChanged);
                disconnect(Application::boardController, &BoardController::controlViewportChanged, this, &RecordingController::activeSceneChanged);

                mSourceScene = 0;

                startNextChapter();

                if(mSourceWidget == VirtualDesktop::instance())
                {
                    mScreenGrabingTimerEventID  = startTimer(1000 / mVideoFramesPerSecondAtStart);
                }
                else
                {
                    mSourceWidget->installEventFilter(this);
                }
            }
        }
    }
}


RecordingController* RecordingController::instance()
{
    if(!sInstance)
        sInstance = new RecordingController(Application::staticMemoryCleaner);

    return sInstance;
}


void RecordingController::start()
{
    if (mRecordingState == Stopped)
    {
        mInitialized = false;

        QSize recommendedSize(1024, 768);

        int fullBitRate = Settings::settings()->recordingWindowsMediaBitsPerSecond->get().toInt();

        if (mSmallVideoSizeAction && mSmallVideoSizeAction->isChecked())
        {
            recommendedSize = QSize(640, 480);
            mVideoBitsPerSecondAtStart = fullBitRate / 4;
        }
        else if (mMediumVideoSizeAction && mMediumVideoSizeAction->isChecked())
        {
            recommendedSize = QSize(1024, 768);
            mVideoBitsPerSecondAtStart = fullBitRate / 2;
        }
        else if (mFullVideoSizeAction && mFullVideoSizeAction->isChecked())
        {
            recommendedSize = Application::boardController->controlView()->size();
            mVideoBitsPerSecondAtStart = fullBitRate;
        }

        QSize scaledboardSize = Application::boardController->controlView()->size();
        scaledboardSize.scale(recommendedSize, Qt::KeepAspectRatio);

        // Video width/height should be a multiple of 4

        int width = scaledboardSize.width();
        int height = scaledboardSize.height();

        if (width % 4 != 0)
                width = ((width / 4) * 4);

        if (height % 4 != 0)
                height = ((height / 4) * 4);

        mVideoFrameSizeAtStart = QSize(width, height);

        applicationMainModeChanged(Application::applicationController->displayMode());

#ifdef Q_OS_WIN
		mVideoEncoder = new WindowsMediaVideoEncoder(this);  //deleted on stop
#endif

        if (mVideoEncoder)
        {
            connect(mVideoEncoder, &AbstractVideoEncoder::encodingStatus, this, &RecordingController::encodingStatus);
            connect(mVideoEncoder, &AbstractVideoEncoder::encodingFinished, this, &RecordingController::encodingFinished);

            if(mRecordingPalette)
            {
                connect(mVideoEncoder, &AbstractVideoEncoder::audioLevelChanged
                        , mRecordingPalette, &RecordingPalette::audioLevelChanged);
            }

            mVideoEncoder->setRecordAudio(!mNoAudioInputDeviceAction->isChecked());

            QString recordingDevice = "";

            if (!mNoAudioInputDeviceAction->isChecked() && !mDefaultAudioInputDeviceAction->isChecked())
            {
                foreach(QAction* audioDevice, mAudioInputDevicesActions)
                {
                    if (audioDevice->isChecked())
                    {
                        recordingDevice = audioDevice->text();
                        break;
                    }
                }
            }

            mVideoEncoder->setAudioRecordingDevice(recordingDevice);

            mVideoEncoder->setFramesPerSecond(mVideoFramesPerSecondAtStart);
            mVideoEncoder->setVideoSize(mVideoFrameSizeAtStart);
            mVideoEncoder->setVideoBitsPerSecond(mVideoBitsPerSecondAtStart);

            mPartNumber = 0;

            mRecordingPath = Settings::settings()->userRecordingDirectory();

            qDebug() << "mRecordingPath: " << mRecordingPath;

            QString videoFileName;

            //if (mIntranetPublicationAction && mIntranetPublicationAction->isChecked())
            //{
                videoFileName = mRecordingPath + "/" + "Recording-"
                        + QDateTime::currentDateTime().toString("yyyyMMddhhmmss")
                        + "-" + PlatformUtils::computerName() + "." + mVideoEncoder->videoFileExtension();
            //}
            //else
            //{
            //    videoFileName = mRecordingPath + "/" + tr("TianBoard Cast") + "." + mVideoEncoder->videoFileExtension();
            //}

            videoFileName = FileSystemUtils::nextAvailableFileName(videoFileName, " ");

            mVideoEncoder->setVideoFileName(videoFileName);

            mLatestCapture = QImage(mVideoFrameSizeAtStart, QImage::Format_RGB32); //0xffRRGGBB

            mRecordStartTime = QTime::currentTime();

            mRecordingProgressTimerEventID = startTimer(100);

            if(mVideoEncoder->start())
            {
                applicationMainModeChanged(Application::applicationController->displayMode());

                setRecordingState(Recording);

                if (mSourceScene)
                {
                    processScenePaintEvent();
                }
                else if (mSourceWidget)
                {
                    processWidgetPaintEvent();
                }
            }
            else
            {
                Application::showMessage(tr("Failed to start encoder ..."), false);
            }
        }
        else
        {
            Application::showMessage(tr("No screen recording encoder available..."), false);
        }
    }
}

void RecordingController::pause()
{
    if (mVideoEncoder && mRecordingState == Recording && mVideoEncoder->canPause())
    {
        sendLatestPixmapToEncoder();

        mTimeAtPaused = QTime::currentTime();

        if (mVideoEncoder->pause())
        {
            setRecordingState(Paused);
        }
    }
}


void RecordingController::unpause()
{
    if (mVideoEncoder && mRecordingState == Paused && mVideoEncoder->canPause())
    {
        if (mVideoEncoder->unpause())
        {
             mRecordingTimestampOffset += mTimeAtPaused.msecsTo(QTime::currentTime());
             sendLatestPixmapToEncoder();

             setRecordingState(Recording);
        }
    }
}


void RecordingController::stop()
{
    if ((mRecordingState == Recording || mRecordingState == Paused) && mVideoEncoder)
    {
        if (mScreenGrabingTimerEventID != 0)
        {
            killTimer(mScreenGrabingTimerEventID);
            mScreenGrabingTimerEventID = 0;
        }

        if (mRecordingProgressTimerEventID != 0)
            killTimer(mRecordingProgressTimerEventID);

        sendLatestPixmapToEncoder();

        setRecordingState(Stopping);

        mVideoEncoder->stop();
    }
}


bool RecordingController::eventFilter(QObject *obj, QEvent *event)
{
    if (mRecordingState == Recording && !mIsGrabbing && event->type() == QEvent::Paint)
    {
        QPaintEvent *paintEvent = static_cast<QPaintEvent*>(event);

        mWidgetRepaintRectQueue.enqueue(paintEvent->rect());

        if (mWidgetRepaintRectQueue.length() == 1)
            QTimer::singleShot(1000.0 / mVideoFramesPerSecondAtStart, this, &RecordingController::processWidgetPaintEvent);
     }

    return QObject::eventFilter(obj, event);
}


void RecordingController::processWidgetPaintEvent()
{
    if(mRecordingState != Recording)
        return;

    QRect repaintRect;

    if (!mInitialized)
    {
        mWidgetRepaintRectQueue.clear();
        repaintRect = mSourceWidget->geometry();

        mLatestCapture.fill(sBackgroundColor);

        mInitialized = true;
    }
    else
    {
        while(mWidgetRepaintRectQueue.size() > 0)
        {
            repaintRect = repaintRect.united(mWidgetRepaintRectQueue.dequeue());
        }
    }

    if (!repaintRect.isNull())
    {
        mIsGrabbing = true;

        QPainter p(&mLatestCapture);
        p.setTransform(mViewToVideoTransform);
        p.setRenderHints(QPainter::Antialiasing);
        p.setRenderHints(QPainter::SmoothPixmapTransform);

        mSourceWidget->render(&p, repaintRect.topLeft(), QRegion(repaintRect), QWidget::DrawChildren);

        mIsGrabbing = false;

        sendLatestPixmapToEncoder();
    }
}


void RecordingController::activeSceneChanged()
{
    if (mSourceScene)
    {
        disconnect(mSourceScene, &QGraphicsScene::changed,
                this, &RecordingController::sceneChanged);
    }

    mSourceScene = Application::boardController->activeScene();

    connect(mSourceScene, &QGraphicsScene::changed,
        this, &RecordingController::sceneChanged);

    mInitialized = false;

    startNextChapter();

    BoardView *bv = qobject_cast<BoardView*>(mSourceWidget);
    if (bv)
    {
        QRectF viewportRect = bv->mapToScene(bv->geometry()).boundingRect();
        mSceneRepaintRectQueue.enqueue(viewportRect);
    }

    processScenePaintEvent();
}

void RecordingController::sceneBackgroundChanged()
{
    BoardView *bv = qobject_cast<BoardView*>(mSourceWidget);

    if (bv)
    {
        mInitialized = false;
    }

    processScenePaintEvent();
}


long RecordingController::elapsedRecordingMs()
{
    QTime now = QTime::currentTime();
    long msFromStart = mRecordStartTime.msecsTo(now);

    return msFromStart - mRecordingTimestampOffset;
}


void RecordingController::startNextChapter()
{
    if (mVideoEncoder)
    {
        //punch chapter in
        ++mPartNumber;
        mVideoEncoder->newChapter(tr("Part %1").arg(mPartNumber), elapsedRecordingMs());
    }
}


void RecordingController::sceneChanged(const QList<QRectF> & region)
{
    if(mRecordingState != Recording)
        return;

    bool shouldRepaint = (mSceneRepaintRectQueue.length() == 0);

    BoardView *bv = qobject_cast<BoardView *>(mSourceWidget);
    if (bv)
    {
        QRectF viewportRect = bv->mapToScene(QRect(0, 0, bv->width(), bv->height())).boundingRect();
        foreach(const QRectF rect, region)
        {
            QRectF maxRect = rect.intersected(viewportRect);
            mSceneRepaintRectQueue.enqueue(maxRect);
        }

        if (shouldRepaint)
            QTimer::singleShot(1000.0 / mVideoFramesPerSecondAtStart, this, &RecordingController::processScenePaintEvent);

    }
}


void RecordingController::processScenePaintEvent()
{
    if(mRecordingState != Recording)
        return;

    BoardView *bv = qobject_cast<BoardView *>(mSourceWidget);

    if(!bv)
        return;

    QRectF repaintRect;

    if (!mInitialized)
    {
        mSceneRepaintRectQueue.clear();
        repaintRect = bv->mapToScene(QRect(0, 0, bv->width(), bv->height())).boundingRect();

        if (bv->scene()->isDarkBackground())
                mLatestCapture.fill(Qt::black);
        else
                mLatestCapture.fill(Qt::white);

        mLatestCapture.fill(Qt::white);

        mInitialized = true;
    }
    else
    {
        while(mSceneRepaintRectQueue.size() > 0)
        {
            repaintRect = repaintRect.united(mSceneRepaintRectQueue.dequeue());
        }
    }

    if (!repaintRect.isNull())
    {
        GraphicsScene *scene = bv->scene();

        QPainter p(&mLatestCapture);

        p.setTransform(mViewToVideoTransform);
        p.setTransform(bv->viewportTransform(), true);

        p.setRenderHints(QPainter::Antialiasing);
        p.setRenderHints(QPainter::SmoothPixmapTransform);

        repaintRect.adjust(-1, -1, 1, 1);

        p.setClipRect(repaintRect);

        if (scene->isDarkBackground())
            p.fillRect(repaintRect, Qt::black);
        else
            p.fillRect(repaintRect, Qt::white);

        scene->setRenderingContext(GraphicsScene::Recording);

        scene->render(&p, repaintRect, repaintRect);

        scene->setRenderingContext(GraphicsScene::Screen);

        sendLatestPixmapToEncoder();
    }
}


void RecordingController::applicationMainModeChanged(ApplicationController::MainMode pMode)
{
    if (pMode == ApplicationController::Internet)
    {
        setSourceWidget(Application::webController->controlView());
    }
    else
    {
        setSourceWidget(Application::boardController->controlView());
    }
}


void RecordingController::applicationDesktopMode(bool displayed)
{
    Q_UNUSED(displayed);

    if (displayed)
    {
        setSourceWidget(VirtualDesktop::instance());
    }
    else
    {
        applicationMainModeChanged(Application::applicationController->displayMode());
    }
}


void RecordingController::webActiveWebPageChanged(WebView* pWebView)
{
    if(Application::applicationController->displayMode() == ApplicationController::Internet)
    {
         setSourceWidget(pWebView);
    }
}


void RecordingController::encodingStatus(const QString& pStatus)
{
    Application::showMessage(pStatus, true);
}


void RecordingController::encodingFinished(bool ok)
{
    if (mVideoEncoder)
    {
        if (ok)
        {
            if (!mApplicationIsClosing)
            {
                QString location;

                if (mRecordingPath == QStandardPaths::writableLocation(QStandardPaths::DesktopLocation))
                    location = tr("on your desktop ...");
                else
                {
                    QDir dir(mRecordingPath);
                    location = tr("in folder %1").arg(mRecordingPath);
                }

                Application::showMessage(tr("Screen recording created %1").arg(location), false);

            }
        }
        else
        {
            qWarning() << mVideoEncoder->lastErrorMessage();

            Application::showMessage(tr("Screen recording error (%1)").arg(mVideoEncoder->lastErrorMessage()), false);
        }

        mVideoEncoder->deleteLater();

        setRecordingState(Stopped);
    }
}


void RecordingController::sendLatestPixmapToEncoder()
{
    if (mVideoEncoder)
        mVideoEncoder->newPixmap(mLatestCapture, elapsedRecordingMs());
}

void RecordingController::timerEvent(QTimerEvent *event)
{
    if (mRecordingState == Recording
            && event->timerId() == mScreenGrabingTimerEventID
            && mSourceWidget == VirtualDesktop::instance())
    {
        VirtualDesktop *dtop = VirtualDesktop::instance();
        QRect dtopRect = dtop->screenGeometry(Application::controlScreenIndex());
        QScreen * screen = Application::controlScreen();

        QPixmap desktop = VirtualDesktop::grabScreen(screen, dtopRect);

        {
            QPainter p(&mLatestCapture);

            if (!mInitialized)
            {
                mLatestCapture.fill(sBackgroundColor);
                mInitialized = true;
            }

            QRectF targetRect = mViewToVideoTransform.mapRect(QRectF(0, 0, desktop.width(), desktop.height()));

            p.setRenderHints(QPainter::Antialiasing);
            p.setRenderHints(QPainter::SmoothPixmapTransform);
            p.drawPixmap(targetRect.left(), targetRect.top(), desktop.scaled(targetRect.width(), targetRect.height(),  Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }

        sendLatestPixmapToEncoder();
    }

    if (mRecordingProgressTimerEventID == event->timerId() && mRecordingState == Recording)
    {
        emit recordingProgressChanged(elapsedRecordingMs());
    }
}


QStringList RecordingController::audioRecordingDevices()
{
    QStringList devices;

#ifdef Q_OS_WIN
    devices = WaveRecorder::waveInDevices();
#endif

    return devices;
}


void RecordingController::recordToggled(bool record)
{
    if ((mRecordingState == Stopped) && record)
        start();
    else
        stop();
}

void RecordingController::pauseToggled(bool paused)
{
    if ((mRecordingState == Recording) && paused)
        pause();
    else
        unpause();
}


void RecordingController::toggleRecordingPalette(bool visible)
{
    if(!mRecordingPalette)
    {
        mRecordingPalette = new RecordingPalette(Application::mainWindow);

        mRecordingPalette->adjustSizeAndPosition();
        mRecordingPalette->setCustomPosition(true);

        int left = Application::boardController->controlView()->width() * 0.75
                   - mRecordingPalette->width() / 2;

        int top = Application::boardController->controlView()->height()
                   - mRecordingPalette->height() - Settings::boardMargin;

        QPoint controlViewPoint(left, top);
        QPoint mainWindowsPoint = Application::boardController->controlView()->mapTo(Application::mainWindow, controlViewPoint);

        mRecordingPalette->move(mainWindowsPoint);

        connect(Application::mainWindow->ui().actionRecordingStart, &QAction::triggered
             , this, &RecordingController::recordToggled);

        connect(Application::mainWindow->ui().actionRecordingPause, &QAction::toggled
             , this, &RecordingController::pauseToggled);

        connect(this, &RecordingController::recordingStateChanged
                , mRecordingPalette, &RecordingPalette::recordingStateChanged);
        connect(this, &RecordingController::recordingProgressChanged
                , mRecordingPalette, &RecordingPalette::recordingProgressChanged);
    }

    mRecordingPalette->setVisible(visible);
}


void RecordingController::setRecordingState(RecordingState pRecordingState)
{
    if(mRecordingState != pRecordingState)
    {
        mRecordingState = pRecordingState;
        emit recordingStateChanged(mRecordingState);
    }
}


QList<QAction*> RecordingController::audioRecordingDevicesActions()
{
    if (mAudioInputDevicesActions.length() == 0)
    {
        QString settingsDevice = Settings::settings()->recordingAudioRecordingDevice->get().toString();

        mDefaultAudioInputDeviceAction = new QAction(tr("Default Audio Input"), this);
        QAction *checkedAction = mDefaultAudioInputDeviceAction;

        mNoAudioInputDeviceAction = new QAction(tr("No Audio Recording"), this);

        if (settingsDevice == "None")
            checkedAction = mNoAudioInputDeviceAction;

        mAudioInputDevicesActions << mNoAudioInputDeviceAction;
        mAudioInputDevicesActions << mDefaultAudioInputDeviceAction;

        foreach(QString audioDevice, audioRecordingDevices())
        {
            QAction* act = new QAction(audioDevice, this);
            act->setCheckable(true);
            mAudioInputDevicesActions << act;
            if (settingsDevice == audioDevice)
                checkedAction = act;
        }

        QActionGroup* audioInputActionGroup = new QActionGroup(this);
        audioInputActionGroup->setExclusive(true);

        foreach(QAction* action, mAudioInputDevicesActions)
        {
            audioInputActionGroup->addAction(action);
            action->setCheckable(true);
        }
        checkedAction->setChecked(true);

        connect(audioInputActionGroup, qOverload<QAction*>(&QActionGroup::triggered), this, &RecordingController::groupActionTriggered);
    }

    return mAudioInputDevicesActions;

}


QList<QAction*> RecordingController::videoSizeActions()
{
    if (mVideoSizesActions.length() == 0)
    {
        mSmallVideoSizeAction = new QAction(tr("Small"), this);
        mMediumVideoSizeAction = new QAction(tr("Medium"), this);
        mFullVideoSizeAction = new QAction(tr("Full"), this);

        mVideoSizesActions << mSmallVideoSizeAction;
        mVideoSizesActions << mMediumVideoSizeAction;
        mVideoSizesActions << mFullVideoSizeAction;

        QActionGroup* videoSizeActionGroup = new QActionGroup(this);
        videoSizeActionGroup->setExclusive(true);

        foreach(QAction* videoSizeAction, mVideoSizesActions)
        {
            videoSizeAction->setCheckable(true);
            videoSizeActionGroup->addAction(videoSizeAction);
        }

        QString videoSize = Settings::settings()->recordingVideoSize->get().toString();

        if (videoSize == "Small")
            mSmallVideoSizeAction->setChecked(true);
        else if (videoSize == "Full")
            mFullVideoSizeAction->setChecked(true);
        else
            mMediumVideoSizeAction->setChecked(true);

        connect(videoSizeActionGroup, qOverload<QAction*>(&QActionGroup::triggered), this, &RecordingController::groupActionTriggered);
    }

    return mVideoSizesActions;
}


QList<QAction*> RecordingController::publicationActions()
{
    if (mPublicationActions.length() == 0)
    {
        foreach(QAction* publicationAction, mPublicationActions)
        {
            connect(publicationAction, &QAction::toggled, this, &RecordingController::actionToggled);
        }
    }

    return mPublicationActions;
}



