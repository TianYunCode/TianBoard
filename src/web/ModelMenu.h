/**
 * @file ModelMenu.h
 * @brief 声明 ModelMenu：按 QAbstractItemModel 层级动态生成菜单项。
 */

#pragma once

#include <QtWidgets>
#include <QMenu>


/**
 * @brief 模型驱动的菜单：把模型的层级结构映射为子菜单与动作。
 *
 * 通过 aboutToShow 惰性生成，支持限制行数、插入分隔线、用自定义角色决定悬停文本
 * 与分隔符。当前 web 域内没有外部使用者，保留为通用部件。
 */
class ModelMenu : public QMenu {
    Q_OBJECT

public:
    /**
     * @brief 构造菜单并连接 aboutToShow 以惰性填充。
     */
    ModelMenu(QWidget *parent = 0);

    /** @brief 设置数据源模型（不接管所有权）。 */
    void setModel(QAbstractItemModel *model);

    /** @brief 返回数据源模型。 */
    QAbstractItemModel *model() const;

    /** @brief 设置根层最多显示的行数，-1 表示不限制。 */
    void setMaxRows(int max);

    /** @brief 返回根层最大行数。 */
    int maxRows() const;

    /** @brief 设置根层第几项之后插入分隔线，-1 表示不插入。 */
    void setFirstSeparator(int offset);

    /** @brief 返回第一处分隔线的位置。 */
    int firstSeparator() const;

    /** @brief 设置生成菜单的根索引。 */
    void setRootIndex(const QModelIndex &index);

    /** @brief 返回生成菜单的根索引。 */
    QModelIndex rootIndex() const;

    /** @brief 设置悬停信号使用的数据角色。 */
    void setHoverRole(int role);

    /** @brief 返回悬停信号使用的数据角色。 */
    int hoverRole() const;

    /** @brief 设置用于判断分隔行的数据角色，0 表示不启用。 */
    void setSeparatorRole(int role);

    /** @brief 返回分隔行判断角色。 */
    int separatorRole() const;

    /**
     * @brief 创建菜单动作，超长文本按菜单宽度省略。
     * @param icon 动作图标。
     * @param text 动作文本。
     * @param parent 动作的父对象。
     */
    QAction *makeAction(const QIcon &icon, const QString &text, QObject *parent);

protected:
    /** @brief 生成菜单前插入自定义内容的钩子，返回 true 时补一条分隔线。 */
    virtual bool prePopulated();

    /** @brief 生成菜单后的钩子，当前为空实现。 */
    virtual void postPopulated();

    /**
     * @brief 递归按模型层级生成菜单。
     * @param parent 当前层级的父索引。
     * @param max 当前层级最多显示项数，-1 不限制。
     * @param parentMenu 父菜单。
     * @param menu 目标菜单；为空表示需要先创建子菜单。
     */
    void createMenu(const QModelIndex &parent, int max, QMenu *parentMenu = 0, QMenu *menu = 0);

signals:
    /** @brief 某个动作被触发，携带其对应的模型索引。 */
    void activated(const QModelIndex &index);

    /** @brief 悬停到某项，携带该角色的文本。 */
    void hovered(const QString &text);

private slots:
    /** @brief 菜单弹出前填充内容。 */
    void aboutToShow();

    /** @brief 把动作触发转为 activated 信号。 */
    void triggered(QAction *action);

    /** @brief 把动作悬停转为 hovered 信号。 */
    void hovered(QAction *action);

private:
    /** @brief 由模型索引创建带索引数据的动作。 */
    QAction *makeAction(const QModelIndex &index);
    /** @brief 根层最大行数。 */
    int m_maxRows;
    /** @brief 第一处分隔线的位置。 */
    int m_firstSeparator;
    /** @brief 动作文本省略宽度，-1 表示按字体计算一次。 */
    int m_maxWidth;
    /** @brief 悬停文本角色。 */
    int m_hoverRole;
    /** @brief 分隔行判断角色。 */
    int m_separatorRole;
    /** @brief 数据源模型。 */
    QAbstractItemModel *m_model;
    /** @brief 根索引的持久副本。 */
    QPersistentModelIndex m_root;
};
