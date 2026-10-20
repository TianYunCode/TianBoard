/**
 * @file ToolWidget.h
 * @brief 浮动网页/交互工具窗口的声明：把场景中的网页对象弹出为独立置顶窗口。
 */

#pragma once


#include <QtWidgets>
#include <QWidget>

#include <memory>

class GraphicsWidgetItem;
class QWidget;
class GraphicsScene;
class QWebEngineView;

/**
 * @brief 承载网页类对象的无边框置顶窗口。
 *
 * 界面上是一个带灰色圆角标题条的小窗口：左上角关闭按钮，可转为页面内容的
 * 对象还有解除固定（pin）按钮，主体是加载对象页面的 QWebEngineView。
 * 由 BoardController 在把场景中的 GraphicsWidgetItem 弹出时创建，关闭后
 * 通过 remove() 隐藏并延迟删除，或者被移回场景。
 */
class ToolWidget : public QWidget {
    Q_OBJECT

public:
    /// 按 URL 推断对象类型（Apple/Web），新建对应图形对象后初始化窗口。
    ToolWidget(const QUrl &pUrl, QWidget *pParent = 0);
    /// 接管一个已存在的图形对象：把它从场景中取出后初始化窗口。
    ToolWidget(GraphicsWidgetItem *pWidget, QWidget *pParent = 0);
    virtual ~ToolWidget();

    /// 隐藏窗口并延迟销毁；会先放弃对图形对象的引用。
    void remove();
    /// 以 pos 为中心移动窗口。
    void centerOn(const QPoint &pos);

    /// 返回内嵌浏览器控件的几何中心（窗口坐标）。
    QPoint naturalCenter() const;

    /// 返回当前承载的图形对象。
    GraphicsWidgetItem *toolWidget() const;

protected:
    /// 创建浏览器、设置边距与固定尺寸并加载对象页面。
    void initialize();
    /// 绘制标题条背景、关闭与解除固定按钮。
    virtual void paintEvent(QPaintEvent *event);

    /// 记录按下位置，判断是否由浏览器吞掉了按下事件。
    virtual void mousePressEvent(QMouseEvent *event);
    /// 拖动窗口主体。
    virtual void mouseMoveEvent(QMouseEvent *event);
    /// 命中关闭/解除固定按钮时执行对应操作。
    virtual void mouseReleaseEvent(QMouseEvent *event);

    /// 转发浏览器控件内的鼠标移动，使拖动窗口在网页上方也生效。
    virtual bool eventFilter(QObject *obj, QEvent *event);

private:
    struct Private;
    std::unique_ptr<Private> d;
};
