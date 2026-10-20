/**
 * @file DesktopPalette.h
 * @brief 声明桌面批注用的浮动工具栏，提供画板、笔、橡皮、标记、截图与副屏显隐按钮。
 */

#pragma once
#include <QtWidgets>
#include <QShowEvent>
#include <QHideEvent>

#include <memory>

#include "ui/ActionPalette.h"
#include "ui/RightPalette.h"

/**
 * @brief 桌面批注工具栏：浮在透明绘图层之上，可最小化成一条标签页。
 *
 * 由 DesktopAnnotationController 创建，父控件是透明绘图层视图，因此不是顶层窗口；
 * 截图前必须真正 hide()，不能靠改窗口透明度隐藏。展开时提供画板/笔/橡皮/标记/选择器/指针、
 * 区域截图、全屏截图与副屏显隐按钮，收起时只留一个“展开笔工具栏”按钮。
 * 显示/隐藏事件会切换笔、标记、橡皮动作的图标（带小箭头版本），供长按展开属性面板。
 * 线程约束：仅在 GUI 线程使用。
 */
class DesktopPalette : public ActionPalette
{
    Q_OBJECT

public:
    /**
     * @brief 构造工具栏并装配全部动作。
     * @param parent 父控件，通常是透明绘图层视图。
     * @param rightPalette 主界面右侧栏，用于贴边时计算避让宽度。
     */
    DesktopPalette(QWidget *parent, RightPalette *rightPalette);

    /** @brief 析构；动作与子控件随 Qt 父子关系回收。 */
    virtual ~DesktopPalette();

    /** @brief 截图前隐藏自身并立即处理事件，确保工具栏不出现在抓取的画面里。 */
    void disappearForCapture();

    /** @brief 截图结束后恢复显示并复位窗口不透明度。 */
    void appear();

    /** @brief 返回指定动作对应按钮在工具栏内的位置，用于判断是否点在小箭头上。 */
    QPoint buttonPos(QAction *action);

signals:
    /** @brief 点击“显示 TianBoard”，请求回到白板。 */
    void boardClick();

    /** @brief 点击“截取屏幕区域”。 */
    void customClick();

    /** @brief 点击“截取窗口”。 */
    void windowClick();

    /** @brief 点击“截取整个屏幕”。 */
    void screenClick();

public slots:
    /** @brief 副屏显隐按钮切换：开/关副屏镜像。 */
    void showHideClick(bool checked);

    /** @brief 更新副屏显隐按钮的选中状态与提示文本。 */
    void updateShowHideState(bool pShowEnabled);

    /** @brief 显示或隐藏副屏显隐按钮（无副屏时隐藏）。 */
    void setShowHideButtonVisible(bool visible);

    /** @brief 显示或隐藏全屏截图按钮。 */
    void setDisplaySelectButtonVisible(bool show);

    /** @brief 收起为单个标签按钮；location 由基类用于贴边定位。 */
    void minimizeMe(eMinimizedLocation location);

    /** @brief 展开全部按钮并发出 maximized()，供控制器重连按钮信号。 */
    void maximizeMe();

    /** @brief 父控件（右侧栏）尺寸变化时保持贴右布局。 */
    void parentResized();

protected:
    /** @brief 显示时把笔/标记/橡皮图标换成带小箭头的版本，并修正贴顶位置。 */
    void showEvent(QShowEvent *event);

    /** @brief 隐藏时把笔/标记/橡皮图标换回普通版本。 */
    void hideEvent(QHideEvent *event);

    /** @brief 贴右定位时需要预留的右侧栏宽度。 */
    virtual int getParentRightOffset();

private:
    /** @brief 避免工具栏贴到屏幕顶边之外。 */
    void adjustPosition();

    struct Private;
    std::unique_ptr<Private> d;

signals:
    /** @brief 请求切换批注工具（历史接口，当前控制器未连接）。 */
    void stylusToolChanged(int tool);
};
