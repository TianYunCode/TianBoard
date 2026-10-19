#pragma once

#include <QSortFilterProxyModel>
#include "core/PersistenceManager.h"

class SortFilterProxyModel : public QSortFilterProxyModel
{
public:
    SortFilterProxyModel();

    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
};

