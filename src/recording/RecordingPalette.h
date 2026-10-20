/**
 * @file RecordingPalette.h
 * @brief 声明录制期间显示的浮动调色板与音量电平表。
 */

#pragma once

#include "ui/ActionPalette.h"
#include "recording/RecordingController.h"

#include <QtGui>
#include <QLabel>

#include <memory>

class VuMeter;

/**
 * @brief 录制控制条：开始/暂停按钮、计时文本、VU 表与录制配置菜单。
 *
 * 由 RecordingController::toggleRecordingPalette() 懒加载创建，父窗口为 MainWindow，
 * 固定在画布右下角；生命周期与主窗口一致，不随单次录制销毁。
 * 调色板不持有录制状态，所有状态来自 RecordingController 的信号：
 * recordingStateChanged() 更新按钮可用性，recordingProgressChanged() 刷新计时，
 * audioLevelChanged() 驱动 VU 表。所有槽都在 GUI 线程执行。
 */
class RecordingPalette : public ActionPalette {
    Q_OBJECT;

public:
    /**
     * @brief 构造调色板并装配开始按钮、计时标签、VU 表与配置菜单。
     * @param parent 父窗口，通常为 MainWindow。
     */
    RecordingPalette(QWidget *parent = 0);

    /** @brief 析构；子控件随 Qt 父子关系回收。 */
    virtual ~RecordingPalette();

public slots:
    /** @brief 按录制状态切换开始/暂停/配置按钮的选中与可用状态。 */
    void recordingStateChanged(RecordingController::RecordingState);

    /** @brief 把录制毫秒数格式化成“分:秒”显示。 */
    void recordingProgressChanged(qint64 ms);

    /** @brief 更新 VU 表音量（0-255）。 */
    void audioLevelChanged(quint8 level);

private:
    struct Private;
    std::unique_ptr<Private> d;
};


/**
 * @brief 简易音量电平表，用一条填充矩形表示当前音频电平。
 *
 * 由 RecordingPalette 创建并随其销毁；只保存 0-255 的归一化音量，
 * 绘制时按控件高度换算，无音频时显示为空。仅在 GUI 线程使用。
 */
class VuMeter : public QWidget {
public:
    /** @brief 构造电平表，初始音量为 0。 */
    VuMeter(QWidget *pParent);

    /** @brief 析构；无额外资源。 */
    virtual ~VuMeter();

    /** @brief 设置音量（0-255）；值变化时才请求重绘。 */
    void setVolume(quint8 pVolume);

protected:
    /** @brief 在控件底部按当前音量绘制电平条。 */
    virtual void paintEvent(QPaintEvent *e);

private:
    quint8 mVolume;
};
