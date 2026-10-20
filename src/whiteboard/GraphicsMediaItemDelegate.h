/**
 * @file GraphicsMediaItemDelegate.h
 * @brief 声明媒体项的委托，管理播放按钮、静音按钮与控制条。
 */

#pragma once

#include <QtWidgets>
#include <QTimer>
#include <QtMultimedia>

#include "utils/Enums.h"
#include "whiteboard/GraphicsItemDelegate.h"

class QGraphicsSceneMouseEvent;
class QGraphicsItem;

/**
 * @brief 媒体图形项（GraphicsMediaItem）的交互委托。
 *
 * 在通用委托之上构建播放/暂停、停止、静音按钮与底部进度控制条，并把播放器的
 * 状态、时长、进度信号接到控件上。工具栏在鼠标悬停时显示，离开后延时隐藏。
 * 委托不拥有媒体项；控制条与按钮由框架统一释放。
 */
class GraphicsMediaItemDelegate : public GraphicsItemDelegate {
    Q_OBJECT

public:
    /**
     * @brief 构造委托。
     * @param pDelegated 被委托的媒体项，不能为空，所有权不转移。
     * @param parent 父 QObject，可为空。
     */
    GraphicsMediaItemDelegate(GraphicsMediaItem *pDelegated, QObject *parent = 0);
    virtual ~GraphicsMediaItemDelegate();

    /** @brief 摆放选择框、按钮与控制条。 */
    virtual void positionHandles();

    /** @brief 按下时收起/显示工具栏，默认不拦截事件。 */
    bool mousePressEvent(QGraphicsSceneMouseEvent *event);

    /** @brief 显示媒体工具栏，并按 autohide 启动自动隐藏计时。 */
    void showToolBar(bool autohide = true);

public slots:
    /** @brief 切换静音。 */
    void toggleMute();
    /** @brief 播放进度变化时刷新控制条。 */
    void updateTicker(qint64 time);
    /** @brief 切换“在扩展屏显示”时同步画面输出。 */
    virtual void showHide(bool show);

    /** @brief 播放器媒体状态变化（加载完成、结束等）时更新按钮状态。 */
    void mediaStatusChanged(QMediaPlayer::MediaStatus status);
    /** @brief 播放状态变化时更新播放/暂停按钮图标。 */
    void mediaStateChanged(QMediaPlayer::PlaybackState state);

    /** @brief 媒体总时长变化时更新控制条刻度。 */
    // GraphicsMediaItem 在自己的构造函数里连它，和上面几个同组
    void totalTimeChanged(qint64 newTotalTime);

protected slots:
    /** @brief 删除媒体项，并妥善停止播放。 */
    virtual void remove(bool canUndo = true);

    /** @brief 切换播放/暂停。 */
    void togglePlayPause();

    /** @brief 根据播放器状态刷新按钮图标。 */
    void updatePlayPauseState();

    /** @brief 延时结束后隐藏工具栏。 */
    void hideToolBar();


protected:
    /** @brief 创建媒体专用按钮并接入播放器信号。 */
    virtual void buildButtons();

    /** @brief 把被委托项转换为媒体项。 */
    GraphicsMediaItem *delegated();

    DelegateButton *mPlayPauseButton;
    DelegateButton *mStopButton;
    DelegateButton *mMuteButton;
    DelegateMediaControl *mMediaControl;

    QTimer *mToolBarShowTimer;
    int m_iToolBarShowingInterval;
};
