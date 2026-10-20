/**
 * @file StylusPalette.h
 * @brief 触控笔工具面板的声明：集中放置绘图、选择、缩放等常用工具按钮。
 */

#pragma once


#include <QButtonGroup>

#include <memory>

#include "ui/ActionPalette.h"

/**
 * @brief 触控笔工具栏调色板。
 *
 * 界面上是一排（竖向或横向）大图标按钮：笔、橡皮、荧光笔、选择、播放、
 * 抓手、放大、缩小、激光笔、画线、文本、截图，平台支持时再加虚拟键盘。
 * 由 BoardPaletteManager 创建并随工具栏方向（设置中的 appToolBarOrientationVertical）
 * 停靠在容器左侧中部或底部居中；双击某个工具按钮会发出工具 id，由控制器
 * 打开对应的属性面板。
 */
class StylusPalette : public ActionPalette {
    Q_OBJECT

public:
    /// 构造面板：从主窗口界面取得动作、按平台决定是否加入虚拟键盘并完成分组。
    StylusPalette(QWidget *parent = 0, Qt::Orientation orient = Qt::Vertical);
    virtual ~StylusPalette();

    /// 按工具栏方向把面板摆到父容器左侧中部或底部居中。
    void initPosition();

private slots:
    /// 双击按钮的槽：转发当前选中按钮的 id。
    void stylusToolDoubleClicked();

private:
    struct Private;
    std::unique_ptr<Private> d;

signals:
    /// 某个工具按钮被双击，携带其在按钮组中的 id（未分组时为 -1）。
    void stylusToolDoubleClicked(int tool);
};
