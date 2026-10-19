#include "model/SortFilterProxyModel.h"
#include "model/DocumentController.h"

SortFilterProxyModel::SortFilterProxyModel() : QSortFilterProxyModel() {
    setDynamicSortFilter(false);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool SortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const {
    DocumentTreeModel *model = dynamic_cast<DocumentTreeModel *>(sourceModel());

    if (model) {
        //if it's a top level folder
        //in other words : myDocuments, models and trash folder
        if (model->isToplevel(left) || model->isToplevel(right)) {
            return false;
        }
    }

    return QSortFilterProxyModel::lessThan(left, right);
}
