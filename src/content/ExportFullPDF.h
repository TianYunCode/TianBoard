/**
 * @file ExportFullPDF.h
 * @brief 导出 PDF：先渲染批注层，有 PDF 背景时再尝试与背景合并。
 */

#pragma once

#include <QtCore>
#include "content/ExportAdaptor.h"
#include "content/ExportPDF.h"

class DocumentProxy;

/**
 * @brief “完整 PDF”导出器。
 *
 * 与 ExportPDF 的区别：背景为 PDF 的页面理论上应保留矢量背景并与批注合并，
 * 因此先把批注导出到 *_overlay.pdf，再调用外部合并器；合并器不可用时退化为
 * 纯批注 PDF（即 ExportPDF 的结果）。本工程没接那个合并器，所以只有退化路径：
 * _overlay 文件直接重命名成最终文件。
 */
class ExportFullPDF : public ExportAdaptor
{
    Q_OBJECT;

public:
    ExportFullPDF(QObject *parent = 0);
    virtual ~ExportFullPDF();

    /** @brief 菜单文案：“Export to PDF”。 */
    virtual QString exportName();
    /** @brief 扩展名 ".pdf"。 */
    virtual QString exportExtention();
    /** @brief 询问保存路径并导出。 */
    virtual void persist(DocumentProxy *pDocument, const QModelIndex &selectedTreeIndex);
    /** @brief 目录节点不可导出。 */
    virtual bool associatedActionactionAvailableFor(const QModelIndex &selectedIndex);

    /** @brief 生成 overlay 并在无 PDF 背景时直接改名为目标文件；当前总是返回 true。 */
    virtual bool persistsDocument(DocumentProxy *pDocument, const QString &filename);

protected:
    /** @brief 用 mSimpleExporter 把批注渲染到 overlayName。 */
    void saveOverlayPdf(DocumentProxy *pDocumentProxy, const QString &filename);

private:
    /// 屏幕像素到 PDF 点（1/72 英寸）的换算系数。
    float      mScaleFactor;
    ExportPDF *mSimpleExporter;
};
