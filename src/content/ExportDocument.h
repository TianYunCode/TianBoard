/**
 * @file ExportDocument.h
 * @brief 把单个文档目录打包成 .tbz 文档文件。
 */

#pragma once

#include <QtCore>

#include "content/ExportAdaptor.h"
#include "utils/FileSystemUtils.h"

class DocumentProxy;

/**
 * @brief .tbz 导出器：把 DocumentProxy 的持久化目录整体压缩。
 *
 * 输入是文档在磁盘上的目录（metadata.rdf + pageNNN.svg + 资源），输出是单个 zip 包；
 * 压缩过程通过 ProcessingProgressListener 回报进度。文档目录的结构由
 * PersistenceManager 决定，这里不做任何转换，只做打包。
 */
class ExportDocument : public ExportAdaptor, public ProcessingProgressListener {
    Q_OBJECT

public:
    ExportDocument(QObject *parent = 0);
    virtual ~ExportDocument();

    /** @brief 菜单文案：“Export to TianBoard Format”。 */
    virtual QString exportName();
    /** @brief 扩展名 ".tbz"。 */
    virtual QString exportExtention();
    /** @brief 询问保存路径并写出 .tbz。 */
    virtual void persist(DocumentProxy *pDocument);

    /** @brief 以 UTF-8 文件名编码把文档目录压成 zip；失败返回 false。 */
    virtual bool persistsDocument(DocumentProxy *pDocument, const QString &filename);

    /** @brief 压缩进度回调，把文件名与计数显示到状态栏。 */
    virtual void processing(const QString &pObjectName, int pCurrent, int pTotal);

    /** @brief 目录（catalog）节点不可导出，仅对具体文档返回 true。 */
    virtual bool associatedActionactionAvailableFor(const QModelIndex &selectedIndex);
};
