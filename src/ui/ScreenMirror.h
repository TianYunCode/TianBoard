/**
 * @file ScreenMirror.h
 * @brief 屏幕镜像部件声明：定时抓取源部件或屏幕区域并缩放显示。
 */

#pragma once

#include <QWidget>

#include <memory>

/**
 * @brief 展示屏上的镜像画面。
 *
 * 界面上黑底居中显示一张按比例缩放的截图。由 ApplicationController 在多屏
 * 模式下创建并交给 DisplayManager 摆放：白板模式下源是一个 QWidget，
 * 桌面模式下则按屏幕矩形抓屏；start()/stop() 控制定时器，抓屏频率来自设置。
 */
class ScreenMirror : public QWidget
{
    Q_OBJECT

public:
    /// 构造空的镜像部件，需随后设置源并 start()。
    ScreenMirror(QWidget *parent = nullptr);
    ~ScreenMirror() override;

    /// 黑底居中绘制最近一次抓到的画面。
    void paintEvent(QPaintEvent *event) override;
    /// 定时抓取新画面并请求重绘。
    void timerEvent(QTimerEvent *event) override;

public slots:

    /// 把镜像源切换为某个部件，立即抓取一帧并重绘。
    void setSourceWidget(QWidget *sourceWidget);

    /// 改为按屏幕矩形抓屏（源部件置空），供桌面镜像使用。
    void setSourceRect(const QRect &pRect);

    /// 按设置中的帧率启动定时抓屏；已在运行时不会重复启动。
    void start();

    /// 停止定时抓屏。
    void stop();

private:
    /// 抓取一帧：有源部件时直接 grab()，否则按记录的矩形抓屏，再等比缩放到自身尺寸。
    void grabPixmap();

    struct Private;
    std::unique_ptr<Private> d;
};
