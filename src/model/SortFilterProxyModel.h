/**
 * @file SortFilterProxyModel.h
 * @brief 声明文档树排序代理 SortFilterProxyModel：对 DocumentTreeModel 排序但固定顶层节点。
 */

#pragma once

#include <QSortFilterProxyModel>

/**
 * @brief 文档树排序代理：把排序职责从 DocumentTreeModel 中分离出来。
 *
 * 作为 DocumentTreeView 的视图模型（view model）夹在源模型 DocumentTreeModel 与视图之间，
 * 所有视图索引都必须经它映射回源索引。顶层节点（“我的文档”“回收站”）是固定入口，不参与
 * 排序，lessThan 对它们恒返回 false。由 DocumentController 创建并持有。
 */
class SortFilterProxyModel : public QSortFilterProxyModel
{
public:
    /**
     * @brief 构造代理，关闭动态排序并启用大小写不敏感比较。
     */
    SortFilterProxyModel();

    /**
     * @brief 自定义排序规则：顶层固定节点不参与排序，其余交给基类。
     */
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
};
