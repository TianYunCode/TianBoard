/**
 * @file IDesktopAnnotation.h
 * @brief 桌面批注层的对外契约：应用总控器只需要这几件事，不必认识 desktop 的实现类。
 */

#pragma once

#include <QtWidgets>

/**
 * @brief 桌面批注层接口，实现是 desktop/DesktopAnnotationController，由装配点造好装上
 *        （`ApplicationController::installDesktopAnnotationFactory`，见 app/main.cpp）。
 *
 * 详细说明：接口住在 core，因为用它的是 core 的模式总控器；desktop 反过来 include 本文件，
 * 于是 core 不再认识 desktop（拆环手法见 docs/architecture.md 第 7 步）。drawingView() 在
 * 实现里返回 BoardView*，靠 C++ 的协变返回类型顶替这里声明的 QWidget*。
 * 线程约束：只在 GUI 线程使用（它本身就是界面对象）。
 */
class IDesktopAnnotation : public QObject
{
    Q_OBJECT

public:
    explicit IDesktopAnnotation(QObject *parent = nullptr) : QObject(parent)
    {
    }
    ~IDesktopAnnotation() override                 = default;

    /// 透明绘图层，铺满整个虚拟桌面；DisplayManager 拿它做副屏布局。
    virtual QWidget *drawingView()                 = 0;

    /// 进入桌面批注：全屏铺开画布，并切到桌面那一套工具。
    virtual void showWindow()                      = 0;

    /// 退出桌面批注：收起画布与工具栏，并恢复进来之前的画板工具。
    virtual void hideWindow()                      = 0;

    /// 只收起画布与工具栏、不恢复画板工具：退出应用的复用路径。
    virtual void closeWindow()                     = 0;

    /// 按当前视图高度重排浮在画布上的属性面板。
    virtual void relayout()                        = 0;

    /// 同步工具栏上"在副屏显示画板"按钮的可用性。
    virtual void updateShowHideState(bool enabled) = 0;

signals:
    /// 用户在桌面上框选/抓窗/抓屏得到的图像；pageMode 表示要不要按整页插入文档。
    void imageCaptured(const QPixmap &pixmap, bool pageMode);

public slots:
    /// 显示器布局变化：有可用副屏时才亮出副屏按钮。
    virtual void screenLayoutChanged() = 0;
};
