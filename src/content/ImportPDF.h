/**
 * @file ImportPDF.h
 * @brief 把 PDF 的每一页作为背景导入。
 */

#pragma once

#include <QtGui>
#include "content/ImportAdaptor.h"

class DocumentProxy;

/**
 * @brief PDF 按页导入器。
 *
 * 借助 PDFRenderer 缓存同一 PDF 的渲染器，为每一页构造 GraphicsPDFItem；
 * 放置到场景时把首页作为背景并同步页面标称尺寸，后续页由调用方翻页放置。
 */
class ImportPDF : public PageBasedImportAdaptor
{
    Q_OBJECT;

public:
    ImportPDF(QObject *parent = 0);
    virtual ~ImportPDF();

    /** @brief 支持 "pdf"。 */
    virtual QStringList supportedExtentions();
    /** @brief 文件对话框过滤器 “Portable Document Format (*.pdf)”。 */
    virtual QString importFileFilter();

    /** @brief 为 PDF 每页创建一个 GraphicsPDFItem；文件无效时返回空列表。 */
    virtual QList<GraphicsItem *> import(const QUuid &uuid, const QString &filePath);
    /** @brief 把 PDF 页设为背景对象，并以其尺寸作为页面尺寸。 */
    virtual void placeImportedItemToScene(GraphicsScene *scene, GraphicsItem *item);
    /** @brief PDF 渲染缓存位于对象目录，返回该目录名以便拷贝。 */
    virtual const QString &folderToCopy();

private:
    /// 渲染 PDF 使用的 DPI，构造时取当前屏幕物理 DPI。
    int dpi;
};
