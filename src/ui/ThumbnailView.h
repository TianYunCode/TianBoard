/**
 * @file ThumbnailView.h
 * @brief 文档预览视图的声明：把页面场景按页面比例完整显示。
 */

#pragma once

#include <QGraphicsView>
#include <QLabel>
#include <QHBoxLayout>
#include <QDebug>

#include <memory>

class GraphicsScene;

/**
 * @brief 用于预览单个页面场景的只读图形视图。
 *
 * 构造时把视图缩放到场景的名义尺寸（nominalSize）并铺满可用区域，去掉边框
 * 和滚动条，作为文档列表里的缩略预览。由 DraggableThumbnailView 通过
 * setWidget 嵌入到图形场景中，本身不处理交互。
 */
class ThumbnailView : public QGraphicsView
{
    Q_OBJECT

public:
    /// 绑定场景并按名义尺寸适配视图。
    ThumbnailView(GraphicsScene *scene, QWidget *parent = 0);
    virtual ~ThumbnailView();

private:
    struct Private;
    std::unique_ptr<Private> d;
};
