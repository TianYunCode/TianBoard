/**
 * @file WidgetMirror.h
 * @brief 部件镜像的声明：把另一个部件的绘制结果实时渲染到自身。
 */

#pragma once


#include <QWidget>

#include <memory>

/**
 * @brief 轻量级的部件镜像控件。
 *
 * 通过给源部件安装事件过滤器，在源部件重绘时同步刷新自身，并在 paintEvent
 * 中用 QWidget::render 把源部件画到自己的画面上。设计用于在另一个窗口里
 * “照搬”某个部件的显示；当前代码中只有 include，尚未见实例化。
 */
class WidgetMirror : public QWidget {
    Q_OBJECT

public:
    /// 构造镜像并开始监听 sourceWidget 的绘制事件。
    WidgetMirror(QWidget *sourceWidget, QWidget *parent = nullptr);
    ~WidgetMirror() override;

public slots:
    /// 换一个源部件：先摘掉旧源的事件过滤器，再监听新源并刷新。
    void setSourceWidget(QWidget *sourceWidget);

protected:
    /// 源部件重绘时按同一区域刷新自身。
    bool eventFilter(QObject *obj, QEvent *event) override;
    /// 用源部件渲染当前待重绘区域。
    void paintEvent(QPaintEvent *event) override;

private:
    struct Private;
    std::unique_ptr<Private> d;
};
