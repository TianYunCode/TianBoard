/**
 * @file QPdfRenderer.h
 * @brief 基于 Qt6::Pdf 的 PDFRenderer 实现声明。
 */

#pragma once

#include "whiteboard/PDFRenderer.h"

#include <QImage>
#include <QtPdf/QPdfDocument>

class QPainter;


/**
 * @brief 以 QPdfDocument 实现 PDFRenderer；替代早期基于 Xpdf 的实现。
 *
 * 内部只缓存“最近一次渲染”的整页位图（页码 + 像素尺寸作键），避免每帧重复
 * 栅格化；渲染尺寸受实现文件中的边长上限约束。实例由 PDFRenderer 的共享表
 * 管理，外部只能 attach/detach。仅在 GUI 线程使用。
 */
class QPdfRenderer : public PDFRenderer {
    Q_OBJECT

public:
    /**
     * @param filename 要加载的 PDF 路径。
     * @param importingFile 是否为导入流程；当前实现不使用该参数。
     */
    QPdfRenderer(const QString &filename, bool importingFile = false);
    ~QPdfRenderer() override;

    /// @brief 文档加载完成且页数大于 0 时为 true。
    bool isValid() const override;

    /// @brief 页数；无效文档返回 0。
    int pageCount() const override;

    /// @brief 指定页尺寸，已按 DPI 从点单位换算为像素。
    QSizeF pageSizeF(int pageNumber) const override;

    /// @brief 始终返回 0：Qt6::Pdf 已在渲染时应用页面旋转，不暴露原始 /Rotate。
    int pageRotation(int pageNumber) const override;

    /// @brief 文档标题元数据。
    QString title() const override;

public slots:
    /**
     * @brief 按画笔当前缩放选择渲染分辨率，并把指定页绘到目标区域。
     * @param p 目标画笔。
     * @param pageNumber 页码，从 1 开始。
     * @param bounds 目标区域（场景坐标）；为空时绘制整页。
     */
    void render(QPainter *p, int pageNumber, const QRectF &bounds = QRectF()) override;

private:
    /// @brief 返回（可能命中的）整页位图，尺寸变化时重新栅格化并更新缓存。
    QImage renderedPage(int pageIndex, const QSize &pixelSize);

    QPdfDocument mDocument; ///< Qt 的 PDF 文档对象

    int mCachedPageIndex;   ///< 缓存对应的页索引（从 0 开始）
    QSize mCachedPixelSize; ///< 缓存位图的像素尺寸
    QImage mCachedImage;    ///< 最近一次渲染的整页位图
};
