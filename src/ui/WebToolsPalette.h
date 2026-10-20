/**
 * @file WebToolsPalette.h
 * @brief 网页模式工具面板的声明：区域截图与投到展示屏等网页专用操作。
 */

#pragma once

#include <memory>

#include "ui/ActionPalette.h"

/**
 * @brief 网页浏览时使用的工具调色板。
 *
 * 界面上是一列大图标按钮（自定义区域截图、在展示屏显示/隐藏，平台支持时
 * 还有虚拟键盘）。由 WebController 在第一次进入网页模式时创建，并登记到
 * BoardPaletteManager 作为当前网页工具面板，之后反复显示/隐藏。
 */
class WebToolsPalette : public ActionPalette
{
    Q_OBJECT

public:
    /// 构造面板并填入网页相关动作。
    WebToolsPalette(QWidget *parent);
    virtual ~WebToolsPalette();

private:
    struct Private;
    std::unique_ptr<Private> d;
};
