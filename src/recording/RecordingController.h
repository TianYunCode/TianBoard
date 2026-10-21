/**
 * @file RecordingController.h
 * @brief 声明屏幕录制的单例控制器，负责状态机、采集源切换、编码器装配与录制菜单动作。
 */

#pragma once

#include <QtGui>

#include "recording/AbstractVideoEncoder.h"

#include "core/ApplicationController.h"

class GraphicsScene;
class WebView;
class RecordingPalette;

/**
 * @brief 屏幕录制的中枢：维护录制状态机，驱动采集、编码、暂停恢复与界面动作。
 *
 * 状态机只有四个状态：Stopped -> Recording <-> Paused -> Stopping -> Stopped，
 * 所有切换都经 setRecordingState() 发出 recordingStateChanged()，界面与调色板据此更新。
 * 采集源随主模式变化：白板模式取 BoardView、网页模式取 WebView、桌面模式取 VirtualDesktop；
 * 白板与网页通过 Paint 事件过滤器按帧率合并重绘区域，桌面则用定时器抓屏。
 * 本类为懒加载单例（instance()），对象挂在 Application::staticMemoryCleaner 下随进程退出回收；
 * 编码器与调色板在首次使用时创建，停止录制或编码结束时 deleteLater。
 * 线程约束：全部逻辑运行在 GUI 线程，waveIn 回调只经排队连接把音频交回编码器。
 */
class RecordingController : public QObject
{
    Q_OBJECT;

private:
    /** @brief 单例构造，只建立信号连接；不启动任何采集。 */
    RecordingController(QObject *pParent = 0);

    /** @brief 析构；正常退出路径上录制已由 applicationAboutToQuit() 收尾。 */
    virtual ~RecordingController();

public:
    /** @brief 取得懒加载单例，首次调用时创建并挂在静态内存清理器下。 */
    static RecordingController *instance();

    /**
     * @brief 过滤采集源控件的 Paint 事件，累积重绘区域。
     * @return 恒返回基类结果，不吞事件。
     * @note 仅在录制中且非抓帧重入（mIsGrabbing）时入队，避免 render() 触发的递归采集。
     */
    virtual bool eventFilter(QObject *obj, QEvent *event);

    /** @brief 枚举系统可用的录音输入设备名，供菜单与设置展示。 */
    virtual QStringList audioRecordingDevices();

    /** @brief 取得（必要时创建）录音设备互斥动作组，含“默认”“不录音”与各设备项。 */
    QList<QAction *> audioRecordingDevicesActions();

    /** @brief 取得（必要时创建）视频尺寸互斥动作组：小、中、完整。 */
    QList<QAction *> videoSizeActions();

    /** @brief 取得发布相关动作列表；当前没有内置发布项，返回空列表。 */
    QList<QAction *> publicationActions();

    /** @brief 录制的四个状态。 */
    enum RecordingState
    {
        Stopped = 0, ///< 未录制或已完全停止。
        Recording,   ///< 正在采集并编码。
        Paused,      ///< 已暂停：不再送帧，waveIn 停止采集。
        Stopping     ///< 正在收尾，等待编码器发出 encodingFinished。
    };

signals:
    /** @brief 录制状态发生变化，参数为新状态。 */
    void recordingStateChanged(RecordingController::RecordingState);

    /** @brief 录制进度（已录制的有效毫秒数，不含暂停时长）。 */
    void recordingProgressChanged(qint64 ms);

public slots:
    /** @brief 开始录制；仅当处于 Stopped 时有效，失败会提示并复位界面。 */
    void start();

    /** @brief 停止录制并让编码器收尾；录制中或暂停中调用有效。 */
    void stop();

    /** @brief 暂停录制；先补送最后一帧，再挂起编码器与音频设备。 */
    void pause();

    /** @brief 从暂停恢复；按暂停时长补偿音视频时间戳并补送一帧。 */
    void unpause();

    /**
     * @brief 显示或隐藏录制调色板。
     * @param visible true 显示，false 隐藏。
     * @note 首次调用会创建调色板、连接动作并定位到画布右下角。
     */
    void toggleRecordingPalette(bool visible);

    /** @brief 开始/停止动作的槽：未录制时 true 启动，否则一律停止。 */
    void recordToggled(bool record);

    /** @brief 暂停动作的槽：录制中 true 暂停，否则恢复。 */
    void pauseToggled(bool pause);

protected:
    /**
     * @brief 设置采集源控件，重建抓帧方式与坐标变换。
     * @param pWidget 白板视图、网页视图、桌面虚拟控件或空指针。
     * @note 切换时会断开旧源的连接与定时器；白板源额外跟随场景切换与背景变化。
     */
    virtual void setSourceWidget(QWidget *pWidget);

    /** @brief 处理抓屏定时器与录制进度定时器事件。 */
    virtual void timerEvent(QTimerEvent *event);

private slots:
    /** @brief 合并控件重绘区域并渲染一帧，随后送编码器。 */
    void processWidgetPaintEvent();

    /** @brief 合并场景重绘区域并按录制上下文渲染一帧。 */
    void processScenePaintEvent();

    /** @brief 场景内容变化时把区域裁剪到视口并入队，按帧率节流触发渲染。 */
    void sceneChanged(const QList<QRectF> &region);

    /** @brief 场景背景变化时强制整帧重绘。 */
    void sceneBackgroundChanged();

    /** @brief 白板活动页切换：重挂场景信号、写章节标记并重绘。 */
    void activeSceneChanged();

    /** @brief 主模式变化时切换采集源（网页或白板）。 */
    void applicationMainModeChanged(ApplicationController::MainMode pMode);

    /** @brief 进入桌面模式时改用 VirtualDesktop 采集，退出时回到主模式对应的源。 */
    void applicationDesktopMode(bool displayed);

    /** @brief 网页模式下活动页面变化时改用新的 WebView 采集。 */
    void webActiveWebPageChanged(QWidget *webView);

    /** @brief 把编码器状态文本转成界面提示。 */
    void encodingStatus(const QString &pStatus);

    /** @brief 编码结束：提示保存位置或错误，回收编码器并回到 Stopped。 */
    void encodingFinished(bool ok);

    /** @brief 应用即将退出：置关闭标志，必要时停止录制。 */
    void applicationAboutToQuit();

    /** @brief 动作组触发后同步设置。 */
    void groupActionTriggered(QAction *);

    /** @brief 发布动作切换后同步设置。 */
    void actionToggled(bool);

    /** @brief 把视频尺寸与录音设备的选择写回设置。 */
    void updateActionState();

private:
    /** @brief 更新状态并在变化时发出 recordingStateChanged()。 */
    void setRecordingState(RecordingState pRecordingState);

    /** @brief 把最近一次采集的画面连同当前时间戳送给编码器。 */
    void sendLatestPixmapToEncoder();

    /** @brief 已录制的有效毫秒数 = 墙钟时长 - 累计暂停时长。 */
    long elapsedRecordingMs();

    static RecordingController *sInstance;

    QPointer<AbstractVideoEncoder> mVideoEncoder;

    QTime mRecordStartTime;

    bool mIsGrabbing;

    QQueue<QRect>  mWidgetRepaintRectQueue;
    QQueue<QRectF> mSceneRepaintRectQueue;

    bool mInitialized;

    QImage mLatestCapture;

    int   mVideoFramesPerSecondAtStart;
    QSize mVideoFrameSizeAtStart;
    long  mVideoBitsPerSecondAtStart;

    static unsigned int sBackgroundColor;

    QWidget *mSourceWidget;

    GraphicsScene *mSourceScene;

    QTransform mViewToVideoTransform;

    int mScreenGrabingTimerEventID;
    int mRecordingProgressTimerEventID;

    int mPartNumber;

    /** @brief 递增分段号并写入“Part N”章节标记。 */
    void startNextChapter();

    RecordingPalette *mRecordingPalette;

    RecordingState mRecordingState;

    bool mApplicationIsClosing;

    QTime mTimeAtPaused;
    long  mRecordingTimestampOffset;

    QAction *mDefaultAudioInputDeviceAction;
    QAction *mNoAudioInputDeviceAction;

    QList<QAction *> mAudioInputDevicesActions;
    QList<QAction *> mVideoSizesActions;

    QAction *mSmallVideoSizeAction;
    QAction *mMediumVideoSizeAction;
    QAction *mFullVideoSizeAction;

    QList<QAction *> mPublicationActions;

    QString mRecordingPath;
};
