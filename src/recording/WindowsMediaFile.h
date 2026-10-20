/**
 * @file WindowsMediaFile.h
 * @brief 声明 Windows Media 容器写入器，负责 WMV 文件、音视频样本与章节标记的落盘。
 */

#pragma once

#include <QtCore>

#ifndef inteface
#define interface struct
#endif
#include <wmsdk.h>
#pragma comment(lib, "wmvcore.lib")

/**
 * @brief 对 IWMWriter 的轻量封装：按 .prx 配置初始化音视频输入流并写入压缩样本。
 *
 * 由 WindowsMediaVideoEncoder 创建：init() 负责 COM 初始化、加载 profile、设置输入属性并开始写文件；
 * close() 结束写入、补写章节标记、释放全部 COM 对象并反初始化 COM。
 * 章节标记不能随帧即时写入（此时文件尚未定型），先缓存在 mChapters，关闭时用元数据编辑器重开文件写入。
 * 线程约束：所有方法都在 GUI 线程调用；音频样本经排队连接从 waveIn 回调线程送回 GUI 线程后再写入。
 */
class WindowsMediaFile : public QObject {
    Q_OBJECT;

public:
    /** @brief 构造写入器，COM 指针置空，不初始化 COM 或文件。 */
    WindowsMediaFile(QObject *pParent = 0);

    /** @brief 析构；不负责关闭文件，调用方应先 close()。 */
    virtual ~WindowsMediaFile();

    /**
     * @brief 初始化 COM、加载 profile、设置输出文件名并开始写入。
     * @param videoFileName 输出文件路径（Windows 分隔符）。
     * @param profileData 自定义 profile 的 XML 文本，尺寸与码率占位符已由调用方替换。
     * @param pFramesPerSecond 帧率，用于计算每帧时长。
     * @param pixelWidth 画面宽度，通常取 4 的倍数。
     * @param pixelHeight 画面高度，通常取 4 的倍数。
     * @param bitsPerPixel 输入位深，编码器固定使用 32。
     * @return 任一步失败返回 false，错误文本见 lastErrorMessage()。
     */
    bool init(const QString &videoFileName, const QString &profileData, int pFramesPerSecond, int pixelWidth,
              int pixelHeight, int bitsPerPixel);

    /**
     * @brief 结束写入、写章节标记并释放 COM 资源。
     * @return EndWriting 失败时返回 false；其余清理步骤失败只记录不中断。
     */
    bool close();

    /**
     * @brief 追加一帧视频。
     * @param pPix 必须是 QImage::Format_RGB32 的整帧图像（与创建的媒体类型一致）。
     * @param msTimeStamp 毫秒时间戳，内部换算成 100 ns 单位的样本时间。
     * @return 分配样本或写入失败返回 false。
     */
    bool appendVideoFrame(const QImage &pPix, long msTimeStamp);

    /** @brief 缓存一个章节标记，实际写入发生在 close()。 */
    void startNewChapter(const QString &pLabel, long timestamp);

    /** @brief 最近一次失败的错误文本。 */
    QString lastErrorMessage() const { return mLastErrorMessage; }

public slots:
    /**
     * @brief 追加一段 PCM 音频样本。
     * @param audioData 原始 PCM 数据，长度不超过分配到的样本缓冲区。
     * @param mstimestamp 毫秒时间戳。
     * @note 连接自 WaveRecorder::newWaveBuffer 的排队连接，运行在 GUI 线程。
     */
    void appendAudioBuffer(const QByteArray &audioData, long mstimestamp);

private:
    /** @brief 按给定尺寸与位深配置视频输入媒体类型，并写回 writer。 */
    bool initVideoStream(int pixelWidth, int pixelHeight, int bitsPerPixel);

    /** @brief 记录错误文本并输出到日志。 */
    void setLastErrorMessage(const QString &error);

    /** @brief 释放 profile、writer、输入属性与内存 DC 等 COM/GDI 资源。 */
    void releaseWMObjects();

    /** @brief 毫秒转 100 ns 样本时间单位，WM 的时间戳基准。 */
    QWORD msToSampleTime(long ms) {
        QWORD qwordMs = ms;
        return (qwordMs * 10000);
    }

    IWMProfile *mWMProfile;
    IWMWriter *mWMWriter;
    IWMWriterPushSink *mPushSink;

    IWMInputMediaProps *mWMInputVideoProps;
    IWMInputMediaProps *mWMInputAudioProps;
    IWMProfileManager *mWMProfileManager;

    HDC mWMhDC;
    DWORD mVideoInputIndex;
    DWORD mAudioInputIndex;
    DWORD mFramesPerSecond;

    QString mLastErrorMessage;

    /** @brief 待写入的章节标记。 */
    struct MarkerInfo {
        QString label;
        long timestamp;
    };

    QList<MarkerInfo> mChapters;

    QString mVideoFileName;
};
