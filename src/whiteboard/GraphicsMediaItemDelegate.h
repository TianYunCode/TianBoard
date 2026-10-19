#pragma once

#include <QtWidgets>
#include <QTimer>
#include <QtMultimedia>

#include "utils/Enums.h"
#include "whiteboard/GraphicsItemDelegate.h"

class QGraphicsSceneMouseEvent;
class QGraphicsItem;

class GraphicsMediaItemDelegate : public GraphicsItemDelegate {
    Q_OBJECT

public:
    GraphicsMediaItemDelegate(GraphicsMediaItem *pDelegated, QObject *parent = 0);
    virtual ~GraphicsMediaItemDelegate();

    virtual void positionHandles();

    bool mousePressEvent(QGraphicsSceneMouseEvent *event);

    void showToolBar(bool autohide = true);

public slots:
    void toggleMute();
    void updateTicker(qint64 time);
    virtual void showHide(bool show);

    void mediaStatusChanged(QMediaPlayer::MediaStatus status);
    void mediaStateChanged(QMediaPlayer::PlaybackState state);

    // GraphicsMediaItem 在自己的构造函数里连它，和上面几个同组
    void totalTimeChanged(qint64 newTotalTime);

protected slots:
    virtual void remove(bool canUndo = true);

    void togglePlayPause();

    void updatePlayPauseState();

    void hideToolBar();


protected:
    virtual void buildButtons();

    GraphicsMediaItem *delegated();

    DelegateButton *mPlayPauseButton;
    DelegateButton *mStopButton;
    DelegateButton *mMuteButton;
    DelegateMediaControl *mMediaControl;

    QTimer *mToolBarShowTimer;
    int m_iToolBarShowingInterval;
};
