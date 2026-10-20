/**
 * @file DockDownloadWidget.h
 * @brief 提供右侧停靠面板中承载下载列表的标签页。
 */

#pragma once

#include <QWidget>
#include <QVBoxLayout>

#include "ui/DockPaletteWidget.h"
#include "ui/DownloadWidget.h"

/**
 * @brief 下载面板：把 DownloadWidget 包成右侧 DockPalette 的一个标签页。
 *
 * BoardPaletteManager 创建并注册到 RightPalette，初始隐藏；第一次开始下载时由
 * 调色板管理器显示该标签页。只在 Board 模式可见，内部下载列表由 DownloadWidget
 * 直接连接 DownloadManager 维护。
 */
class DockDownloadWidget : public DockPaletteWidget
{
    Q_OBJECT
public:
    /**
     * @brief 构造面板并嵌入 DownloadWidget。
     * @param parent 父控件
     * @param name 对象名
     */
    DockDownloadWidget(QWidget *parent = 0, const char *name = "DockDownloadWidget");
    ~DockDownloadWidget();

    /**
     * @brief 该面板只在 Board 模式显示。
     * @param mode 当前界面模式
     * @return mode 为 Board 时返回 true
     */
    bool visibleInMode(DockWidgetMode mode)
    {
        return mode == DockWidgetMode::Board;
    }

private:
    QVBoxLayout    *mpLayout;
    DownloadWidget *mpDLWidget;
};
