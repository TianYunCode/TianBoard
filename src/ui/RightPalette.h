/**
 * @file RightPalette.h
 * @brief 右侧停靠调色板的声明：承载功能页、缓存属性与下载列表等标签页。
 */

#pragma once


#include "ui/DockPalette.h"

#include <memory>

/**
 * @brief 停靠在窗口右侧的可折叠面板。
 *
 * 作为 BoardPaletteManager 的容器使用：FeaturesWidget（文档/素材库）、
 * CachePropertiesWidget、DockDownloadWidget 都注册为它的标签页。宽度和是否
 * 折叠按 Board / Desktop 两种模式分别写入设置，切换模式时恢复对应值；
 * 展开时最大宽度被限制为父容器的 45%。
 */
class RightPalette : public DockPalette {
    Q_OBJECT
public:
    /// 构造面板并按当前模式恢复上次的宽度与折叠状态。
    RightPalette(QWidget *parent = 0, const char *name = "RightPalette");
    ~RightPalette();
    /// 切换模式并恢复该模式记录的宽度（折叠时为 0），返回值同基类。
    bool switchMode(DockWidgetMode mode);

signals:
    /// 面板尺寸变化后发出，供外部重新布局相邻部件。
    void resized();

protected:
    /// 按父容器尺寸刷新最大宽度与固定高度。
    void updateMaxWidth();
    /// 只有允许调整大小时才把拖动交给基类，避免折叠状态下被拖开。
    void mouseMoveEvent(QMouseEvent *event);
    /// 尺寸变化时按模式持久化宽度与折叠状态，然后通知基类。
    void resizeEvent(QResizeEvent *event);

private:
    struct Private;
    std::unique_ptr<Private> d;
};
