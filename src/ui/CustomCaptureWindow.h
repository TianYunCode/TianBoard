/**
 * @file CustomCaptureWindow.h
 * @brief 提供整屏快照上框选区域的自定义截图窗口。
 */

#pragma once

#include <QtWidgets>
#include <QDialog>
#include <QRubberBand>

#include <memory>


/**
 * @brief 自定义区域截图对话框：显示整屏截图并让用户拖拽框选。
 *
 * 由截图流程调用 execute() 以模态方式打开；窗口无边框、铺满鼠标所在屏幕，
 * 背景是传入的整屏位图，拖动时用 RubberBand 显示选区。确认后调用方通过
 * getSelectedPixmap() 取回裁剪结果。对象由调用方创建并销毁。
 */
class CustomCaptureWindow : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief 构造无边框、初始完全透明的截图窗口。
     * @param parent 父控件
     */
    CustomCaptureWindow(QWidget *parent = 0);
    virtual ~CustomCaptureWindow();

    /**
     * @brief 在鼠标所在屏幕上打开窗口并进入模态循环。
     * @param pScreenPixmap 整屏快照，绘制为窗口背景
     * @return exec() 的结果；用户确认选择时为 Accepted
     * @note 调用前应保证 pScreenPixmap 与屏幕几何一致，选区按窗口坐标裁剪。
     */
    int execute(const QPixmap &pScreenPixmap);

    /**
     * @brief 返回当前选区对应的图像。
     * @return 裁剪后的位图；尚未创建选区时返回空位图
     */
    QPixmap getSelectedPixmap();

protected:
    virtual void showEvent(QShowEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void paintEvent(QPaintEvent *event);

private:
    struct Private;
    std::unique_ptr<Private> d;
};
