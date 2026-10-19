#pragma once

#include <QtWidgets>
#include <QTimer>
#include <QtMultimedia>

#include "utils/Enums.h"
#include "whiteboard/GraphicsItemDelegate.h"

class QGraphicsSceneMouseEvent;
class QGraphicsItem;

class GraphicsMediaItemDelegate :  public GraphicsItemDelegate
{
    Q_OBJECT

public:
    GraphicsMediaItemDelegate(GraphicsMediaItem* pDelegated, QObject * parent = 0);
    virtual ~GraphicsMediaItemDelegate();

    virtual void positionHandles();

    bool mousePressEvent(QGraphicsSceneMouseEvent* event);

    void showToolBar(bool autohide = true);

public slots:
    void toggleMute();
    void updateTicker(qint64 time);
    virtual void showHide(bool show);

    void mediaStatusChanged(QMediaPlayer::MediaStatus status);
    void mediaStateChanged(QMediaPlayer::PlaybackState state);

protected slots:
    virtual void remove(bool canUndo = true);

    void togglePlayPause();

    void updatePlayPauseState();

    void totalTimeChanged(qint64 newTotalTime);

    void hideToolBar();


protected:
    virtual void buildButtons();

    GraphicsMediaItem* delegated();

    DelegateButton* mPlayPauseButton;
    DelegateButton* mStopButton;
    DelegateButton* mMuteButton;
    DelegateMediaControl* mMediaControl;

    QTimer* mToolBarShowTimer;
    int m_iToolBarShowingInterval;
};

