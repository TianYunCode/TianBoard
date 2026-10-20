/**
 * @file DesktopPropertyPalette.h
 * @brief 声明桌面批注的笔、橡皮、标记三个属性面板及其公共基类。
 */

#pragma once

#include <QtWidgets>
#include <QResizeEvent>

#include "ui/PropertyPalette.h"

class RightPalette;

/**
 * @brief 桌面属性面板的公共基类：横向排布，并知道右侧栏宽度用于贴边避让。
 *
 * 由 DesktopAnnotationController 创建，父控件为透明绘图层视图；面板自身不保存工具状态，
 * 选择结果通过 DrawingController 的全局信号回写，然后自动 close()。
 */
class DesktopPropertyPalette : public PropertyPalette {
    Q_OBJECT

public:
    /** @brief 构造面板，记录右侧栏指针用于布局避让。 */
    DesktopPropertyPalette(QWidget *parent, RightPalette *_rightPalette);

private:
    RightPalette *rightPalette;

protected:
    /** @brief 贴右定位时需要预留的右侧栏宽度。 */
    virtual int getParentRightOffset();
};

/**
 * @brief 笔属性面板：颜色与线宽选择，选完即关闭。
 *
 * 按钮来自主窗口的共享动作，面板只负责摆放到桌面工具栏旁；父工具栏收起时断开按钮的释放信号，
 * 展开时重新连接，避免重复触发。
 */
class DesktopPenPalette : public DesktopPropertyPalette {
    Q_OBJECT
public:
    /** @brief 构造面板并连接颜色、线宽选择控件。 */
    DesktopPenPalette(QWidget *parent, RightPalette *rightPalette);
    /** @brief 析构；子控件随父子关系回收。 */
    virtual ~DesktopPenPalette() {}
public slots:
    /** @brief 父工具栏收起时断开按钮释放信号。 */
    void onParentMinimized();
    /** @brief 父工具栏展开时连接按钮释放信号。 */
    void onParentMaximized();

private slots:
    /** @brief 任一按钮释放后关闭面板。 */
    void onButtonReleased();
};

/**
 * @brief 橡皮属性面板：橡皮宽度选择与“清除桌面批注”动作。
 */
class DesktopEraserPalette : public DesktopPropertyPalette {
public:
    /** @brief 构造面板并装配橡皮宽度按钮与清除动作。 */
    DesktopEraserPalette(QWidget *parent, RightPalette *rightPalette);
    /** @brief 析构；子控件随父子关系回收。 */
    virtual ~DesktopEraserPalette() {}
};

/**
 * @brief 标记属性面板：颜色与线宽选择，选完即关闭。
 */
class DesktopMarkerPalette : public DesktopPropertyPalette {
public:
    /** @brief 构造面板并连接颜色、线宽选择控件。 */
    DesktopMarkerPalette(QWidget *parent, RightPalette *rightPalette);
    /** @brief 析构；子控件随父子关系回收。 */
    virtual ~DesktopMarkerPalette() {}
};
