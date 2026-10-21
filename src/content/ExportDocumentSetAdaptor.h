/**
 * @file ExportDocumentSetAdaptor.h
 * @brief 把文档树（含子文档）递归打包成 .tbx 文档集。
 */

#pragma once

#include <QtCore>
#include "content/ExportAdaptor.h"
#include "utils/FileSystemUtils.h"
#include "utils/Globals.h"

THIRD_PARTY_WARNINGS_DISABLE
#include "quazip.h"
#include "quazipfile.h"
THIRD_PARTY_WARNINGS_ENABLE

class DocumentProxy;
class DocumentTreeModel;

/**
 * @brief .tbx 导出器：按文档树结构逐层压缩。
 *
 * 每个节点对应包内一个以文档目录名命名的子目录，子文档嵌套在父目录之下，从而在
 * 导入端还原出层级。入口 persist() 从当前选中的树节点或给定 DocumentProxy 出发，
 * 通过 DocumentTreeModel 遍历；与 MetadataDcSubsetAdaptor 无直接关系，仅原样搬运目录。
 */
class ExportDocumentSetAdaptor : public ExportAdaptor
{
    Q_OBJECT

public:
    ExportDocumentSetAdaptor(QObject *parent = 0);
    virtual ~ExportDocumentSetAdaptor();

    /** @brief 菜单文案：“Export to TianBoard Document Set Format”。 */
    virtual QString exportName();
    /** @brief 扩展名 ".tbx"。 */
    virtual QString exportExtention();

    /** @brief 询问保存路径并导出当前选中节点及其子树。 */
    virtual void persist(DocumentProxy *pDocument, const QModelIndex &selectedTreeIndex);
    /** @brief 以 pRootIndex 为根写出 .tbx；失败返回 false。 */
    bool persistData(const QModelIndex &pRootIndex, QString filename);
    /** @brief 递归把一个树节点及其子节点写入 zip，保持目录层级。 */
    bool addDocumentToZip(const QModelIndex &pIndex, DocumentTreeModel *model, QuaZip &zip);

    /** @brief 只有目录节点（可含子文档）才能导出文档集。 */
    virtual bool associatedActionactionAvailableFor(const QModelIndex &selectedIndex);
};
