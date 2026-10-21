/**
 * @file ShortcutManager.h
 * @brief 快捷键注册表与偏好设置“快捷键”页表格模型的声明。
 */

#pragma once

#include <QAbstractTableModel>
#include <QHash>
#include <QKeySequence>
#include <QList>
#include <QString>

class QAction;

/**
 * @brief 登记应用的可绑快捷键动作，读写用户覆盖，并把它摊成一张供界面展示的表。
 *
 * 详细说明：注册时记下每个 QAction 当时的快捷键作为默认值，随即用
 * Shortcut/<objectName> 这一用户配置键读回覆盖并应用；用户在偏好设置里改绑时，
 * 先查冲突再写配置，恢复默认则直接删掉那个键，让动作回落到编译期的默认值。
 * 表格一行要么是分组标题，要么是一个动作；本模型只读，改绑一律走 bind()，
 * 因为只有走这条路才能同时做冲突检查与落盘。
 *
 * 生命周期：进程级单例，由 instance() 首次调用时创建、destroy() 在应用析构时释放；
 * 注册由 MainWindow 构造完成后一次性完成，重复注册同一个动作会被忽略。
 * 线程约束：GUI 线程专用——它直接读写 QAction，且改动会触发视图更新。
 * 快捷键串不参与翻译（按键名不是自然语言），所以它们留在 setupUi 而不在 retranslateUi。
 */
class ShortcutManager : public QAbstractTableModel
{
    Q_OBJECT

public:
    /// @brief 表格列：命令名、当前快捷键、默认快捷键。
    enum Column
    {
        CommandColumn = 0,
        ShortcutColumn,
        DefaultColumn,
        ColumnCount
    };

    /// @brief 分组标识。用它选组、显示时才翻成当前语言的名字，
    /// 这样切换语言后表头仍是新语言，不必重新注册。
    enum class Group
    {
        Common,     ///< 模式切换、剪贴板、退出这类全局动作。
        Board,      ///< 翻页、撤销、清页这些画板动作。
        Tool,       ///< 笔板上的工具。
        LineColor,  ///< 线条粗细、橡皮大小与笔色。
        Background, ///< 稿纸背景与网格。
        Web,        ///< 网页页的导航与缩放。
        Document    ///< 文档库里的新建/导入/导出。
    };

    /// @brief 界面取回某行的动作名、判断该行是否被用户改过的自定义角色。
    enum CustomRole
    {
        ActionNameRole = Qt::UserRole, ///< 值为 QString：动作的 objectName；组标题行没有值。
        OverriddenRole                 ///< 值为 bool：当前快捷键已被用户改过。
    };

    /// @brief 改绑的三种结局，界面负责把它翻成用户看得懂的话。
    enum class BindResult
    {
        Bound,         ///< 已应用并写进用户配置。
        UnknownAction, ///< 这个动作不在这张表里。
        Conflict       ///< 已经被同表里的另一个动作占用。
    };

    /// @brief 返回应用共用那一份注册表，首次调用时创建，父对象是 QCoreApplication。
    static ShortcutManager *instance();
    /// @brief 销毁 instance() 建的那一份并把指针置空。
    static void destroy();

    /// @brief 构造一张空的注册表；自检会自建隔离的实例，不必借用单例。
    explicit ShortcutManager(QObject *parent = nullptr);
    /// @brief 析构。
    ~ShortcutManager() override;

    /// @brief 把一组动作登记进某个分组；已登记过的动作原地跳过。
    void registerActions(Group group, const QList<QAction *> &actions);

    /// @brief 返回分组的显示名，随当前语言变化。
    static QString groupLabel(Group group);

    /// @brief 按 objectName 取回已登记的动作，未登记返回空指针。
    QAction *actionFor(const QString &objectName) const;
    /// @brief 返回全部已登记动作，顺序与表格一致。
    QList<QAction *> actions() const;

    /// @brief 返回某动作的默认快捷键文本（注册那一刻记下的值）。
    QString defaultText(const QAction *action) const;
    /// @brief 判断某动作当前是否带着用户覆盖。
    bool isOverridden(const QAction *action) const;
    /// @brief 返回与该串键冲突的另一个动作；没有冲突返回空串。
    QString conflictingActionName(const QKeySequence &sequence, const QAction *except) const;

    /**
     * @brief 给动作绑上新键：查冲突、改 QAction、写用户配置。
     * @param sequence 新的组合键；空序列表示“明确不绑键”，会原样存进配置，
     *                 重启后也不会偷偷回到默认值。
     */
    BindResult bind(QAction *action, const QKeySequence &sequence);
    /// @brief 删掉用户覆盖并把动作恢复到默认快捷键。
    bool resetToDefault(QAction *action);
    /// @brief 全部恢复默认，返回真正被改动的条数。
    int resetAllToDefault();

    /// @brief 某个动作对应的用户配置键；键名以 Shortcut/ 打头，objectName 做后缀。
    static QString settingKey(const QString &objectName);

    /// @brief 把快捷键文本摊成可比较的规范形态；空串（没绑键）也返回空串。
    static QString normalizedText(const QKeySequence &sequence);

    int      rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int      columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Q_ENUM(BindResult)
    Q_ENUM(Group)

private:
    /// @brief 按 objectName 找到它所在的行；找不到返回 -1。
    int rowOf(const QString &objectName) const;

    /// @brief 表格的一行：action 为空即分组标题行。
    struct Row
    {
        Group    group;  ///< 所属分组。
        QAction *action; ///< 动作指针，不持有所有权。
    };

    QList<Row>              mRows;     ///< 分组标题与动作行混排，与表格行号一一对应。
    QHash<QString, QString> mDefaults; ///< objectName -> 注册时记下的默认快捷键文本。

    static ShortcutManager *sInstance; ///< 进程内唯一实例。
};
