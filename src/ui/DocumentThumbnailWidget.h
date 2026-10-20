/**
 * @file DocumentThumbnailWidget.h
 * @brief 提供文档页缩略图列表，支持拖拽换序与拖入外部页面。
 */

#pragma once

#include "ui/ThumbnailWidget.h"

#include <memory>

class GraphicsScene;


/**
 * @brief 文档页缩略图视图，DocumentsWidget 右侧的页面列表。
 *
 * 由 DocumentsWidget 创建并长期持有；内容由 DocumentController 通过
 * setGraphicsItems() 填充。支持把页面拖到别处，也接受来自同一文档的页面拖放：
 * 拖拽过程中在目标位置画一条灰色插入线，靠近边缘时自动滚动，释放后通过
 * sceneDropped() 通知控制器移动页面。关闭拖拽时只保留选择行为。
 */
class DocumentThumbnailWidget : public ThumbnailWidget {
    Q_OBJECT

public:
    /**
     * @brief 构造缩略图视图，默认允许拖拽。
     * @param parent 父控件
     */
    DocumentThumbnailWidget(QWidget *parent);
    virtual ~DocumentThumbnailWidget();

    /**
     * @brief 是否允许从本视图拖出页面。
     * @param enabled 新状态
     */
    void setDragEnabled(bool enabled);
    /** @brief 当前是否允许拖出页面。 */
    bool dragEnabled() const;

    /**
     * @brief 高亮并选中指定下标的缩略图。
     * @param index 页在视图中的下标
     * @note 下标越界时仅尝试按原样调用 selectItemAt()。
     */
    void hightlightItem(int index);

public slots:
    /**
     * @brief 替换视图中的缩略图集合。
     * @param pGraphicsItems 图形项
     * @param pItemPaths 每项对应的文件路径
     * @param pLabels 每项的文字标签
     * @param pMimeType 拖拽使用的 MIME 类型
     * @note 先删除拖放插入线，再交给基类重建内容。
     */
    virtual void setGraphicsItems(const QList<QGraphicsItem *> &pGraphicsItems, const QList<QUrl> &pItemPaths,
                                  const QStringList pLabels = QStringList(),
                                  const QString &pMimeType = QString(""));

signals:
    /**
     * @brief 页面被拖放到新位置。
     * @param proxy 页面所属文档
     * @param source 源下标
     * @param target 目标下标
     */
    void sceneDropped(DocumentProxy *proxy, int source, int target);

private slots:
    /** @brief 定时器回调：按当前幅度滚动垂直滚动条。 */
    void autoScroll();

protected:
    virtual void mouseMoveEvent(QMouseEvent *event);

    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dragLeaveEvent(QDragLeaveEvent *event);
    virtual void dragMoveEvent(QDragMoveEvent *event);
    virtual void dropEvent(QDropEvent *event);

private:
    /** @brief 从场景中删除拖放插入线。 */
    void deleteDropCaret();

    struct Private;
    std::unique_ptr<Private> d;
};
