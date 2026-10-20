/**
 * @file ExportIwb.h
 * @brief 把文档导出为 IWB 1.0 文件。
 */

#pragma once

#include <QtCore>

#include "content/ExportAdaptor.h"

#include "utils/FileSystemUtils.h"

class DocumentProxy;

/**
 * @brief IWB 导出器：调用 IwbDocumentAdaptor 把 .tbz 内容转换成 .iwb 包。
 *
 * 转换细节与格式兼容性全部由 IwbDocumentAdaptor 承担；本类只负责选路径、等待光标、
 * 状态栏提示，并把转换器累积的错误列表通过 exportWarnings 上报。
 */
class ExportIwb : public ExportAdaptor {
    Q_OBJECT

public:
    ExportIwb(QObject *parent = 0);
    virtual ~ExportIwb();

    /** @brief 菜单文案：“Export to IWB”。 */
    virtual QString exportName();
    /** @brief 扩展名 ".iwb"。 */
    virtual QString exportExtention();
    /** @brief 询问保存路径并执行转换。 */
    virtual void persist(DocumentProxy *pDocument);
    /** @brief 目录节点不可导出。 */
    virtual bool associatedActionactionAvailableFor(const QModelIndex &selectedIndex);
};
