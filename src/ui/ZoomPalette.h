/**
 * @file ZoomPalette.h
 * @brief 缩放指示调色板的声明：显示当前缩放倍率并提供“恢复 100%”按钮。
 */

#pragma once

#include "ui/FloatingPalette.h"

#include <memory>

class QPushButton;
class BoardController;

/**
 * @brief 悬浮在右下角的缩放倍率指示器。
 *
 * 界面上是一列两个按钮：当前倍率文本（如 “1.5 x”）与一个带图标的恢复按钮；
 * 点击倍率文本可展开/收起恢复按钮。由 BoardPaletteManager 创建并跟随板面
 * 容器，监听 BoardController 的 zoomChanged/activeSceneChanged 自动刷新；
 * 倍率接近 1 时只短暂显示后自动隐藏，以免遮挡内容。
 */
class ZoomPalette : public FloatingPalette
{
    Q_OBJECT

public:
    /// 构造调色板并连接控制器的缩放信号。
    ZoomPalette(QWidget *parent);
    virtual ~ZoomPalette();

public:
    /**
     * @brief 只有当前倍率接近 1（视为“没缩放”）时才允许隐藏。
     * @note 覆写的是 setVisible 而不是 hide：QWidget::hide()/show() 都不是虚函数，
     *       但它们内部都会走虚的 setVisible —— 覆写 hide() 只对 ZoomPalette* 上的
     *       直接调用有效，经基类指针或 setVisible(false) 来的就会绕过这条判断（以前正是如此）。
     */
    void setVisible(bool visible) override;
    /// 刷新倍率文本与位置，并按倍率决定显示还是延时隐藏。
    void refreshPalette();

private slots:
    /// 展开或收起“恢复 100%”按钮。
    void showHideExtraButton();
    /// 请求控制器把缩放恢复到 100%。
    void goHundred();

private:
    struct Private;
    std::unique_ptr<Private> d;
};
