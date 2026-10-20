/**
 * @file DocumentsWidget.h
 * @brief 提供文档管理界面：左侧文档树、右侧页面缩略图与缩放滑块。
 */

#pragma once

#include <QWidget>

#include <memory>

class QComboBox;
class QSlider;
class QSplitter;
class QToolButton;

class DocumentThumbnailWidget;
class DocumentTreeView;

/**
 * @brief 文档管理界面控件。
 *
 * 界面由可拖动的 QSplitter 分成左右两半：左边是排序控件加文档树，右边是页面
 * 缩略图列表和底部缩放滑块。DocumentController 创建并持有它，作为文档模式的
 * 主视图，并通过下面的访问器取用各个子控件；控件树全部由本类搭建，不再依赖
 * 表单生成的 Ui 类。
 */
class DocumentsWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造完整的文档界面控件树。
     * @param parent 父控件
     */
    DocumentsWidget(QWidget *parent = nullptr);
    ~DocumentsWidget() override;

    /** @brief 左侧文档树。 */
    DocumentTreeView *documentTreeView() const;
    /** @brief 右侧页面缩略图列表。 */
    DocumentThumbnailWidget *thumbnailWidget() const;

    /** @brief 排序方式下拉框。 */
    QComboBox *sortKind() const;
    /** @brief 升序/降序切换按钮。 */
    QToolButton *sortOrder() const;

    /** @brief 分隔文档树与缩略图列表的分割器。 */
    QSplitter *splitter() const;
    /** @brief 缩略图缩放滑块，取值 50–500。 */
    QSlider *documentZoomSlider() const;

private:
    struct Private;
    std::unique_ptr<Private> d;
};
