#include "model/SortFilterProxyModel.h"
#include "model/DocumentTreeModel.h"

SortFilterProxyModel::SortFilterProxyModel() : QSortFilterProxyModel()
{
    setDynamicSortFilter(false);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool SortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    DocumentTreeModel *model = dynamic_cast<DocumentTreeModel *>(sourceModel());

    if (model)
    {
        // 顶层是“我的文档/回收站”这类固定入口，顺序不能随排序变化；
        // 任一侧是顶层时直接视为相等，保持原有相对位置。
        if (model->isToplevel(left) || model->isToplevel(right))
        {
            return false;
        }
    }

    return QSortFilterProxyModel::lessThan(left, right);
}
