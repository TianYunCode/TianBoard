/**
 * @file DocumentNavigator.h
 * @brief 提供以图形视图展示文档页缩略图、支持选中与拖拽换序的导航器。
 */

#pragma once

#include <QResizeEvent>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QThread>

#include "model/DocumentProxy.h"
#include "model/DocumentContainer.h"
#include "ui/ThumbnailWidget.h"

#include <memory>

/** 没有选中页时传给滚动槽的下标。 */
#define NO_PAGESELECTED -1


/**
 * @brief 页面缩略图导航视图。
 *
 * 界面上是按列排布的页面缩略图与页码标签。PageNavigationWidget 构造时创建它，
 * 并放在左侧停靠面板的页面导航标签里；视图监听 BoardController 的缩略图更新、
 * 单页更新和当前页变化信号，自行重建场景。长按缩略图可发起拖拽以调整页序，
 * 单击则切换当前页。视图的 QGraphicsScene 归本类所有，析构时删除。
 */
class DocumentNavigator : public QGraphicsView {
    Q_OBJECT
public:
    /**
     * @brief 构造导航视图并连接文档相关信号。
     * @param parent 父控件
     * @param name 对象名
     */
    DocumentNavigator(QWidget *parent = 0, const char *name = "documentNavigator");
    ~DocumentNavigator();

    /**
     * @brief 设置缩略图的列数。
     * @param nbColumns 列数
     * @note 只改成员，需自行调用 refreshScene() 才能重新排布。
     */
    void setNbColumns(int nbColumns);
    /** @brief 当前列数。 */
    int nbColumns();
    /**
     * @brief 设置缩放时允许的最小缩略图宽度。
     * @param width 最小宽度（像素）
     */
    void setThumbnailMinWidth(int width);
    /** @brief 当前最小缩略图宽度。 */
    int thumbnailMinWidth();
    /**
     * @brief 返回视图坐标下某个位置命中的缩略图。
     * @param pos 视图坐标
     * @return 命中的缩略图；点中页码标签时返回其对应缩略图，未命中为 nullptr
     */
    SceneThumbnailNavigPixmap *clickedThumbnail(const QPoint pos) const;

public slots:
    /**
     * @brief 选中并滚动到指定页。
     * @param index 页在场景中的下标
     */
    void onScrollToSelectedPage(int index);
    /**
     * @brief 删除旧场景内容并按源容器重建全部缩略图。
     * @param source 提供每页位图的文档容器
     * @note 若容器缺少某页位图，会先调用 insertThumbPage() 补齐。
     */
    void generateThumbnails(DocumentContainer *source);
    /**
     * @brief 重建单页缩略图。
     * @param iPage 页下标
     */
    void updateSpecificThumbnail(int iPage);

    /** @brief 长按计时结束：鼠标仍按着则发出长按信号。 */
    void longPressTimeout();
    /** @brief 响应长按信号，以被按下的缩略图发起拖拽。 */
    void mousePressAndHoldEvent();

protected:
    virtual void resizeEvent(QResizeEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dragMoveEvent(QDragMoveEvent *event);
    virtual void dropEvent(QDropEvent *event);

signals:
    /** @brief 长按达到阈值，请求开始拖拽。 */
    void mousePressAndHoldEventRequired();

private:
    /** @brief 按当前列数和宽度重新摆放所有缩略图并更新场景矩形。 */
    void refreshScene();
    /** @brief 缩略图区域四周的留白。 */
    int border();

    struct Private;
    std::unique_ptr<Private> d;
};
