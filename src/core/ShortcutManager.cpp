#include "core/ShortcutManager.h"

#include <QAction>
#include <QCoreApplication>
#include <QDebug>
#include <QFont>
#include <QStringList>
#include <QVariant>

#include "settings/Settings.h"

namespace
{
    // 用户配置里快捷键键名的前缀；完整键名是前缀 + QAction 的 objectName。
    const char kShortcutKeyPrefix[] = "Shortcut/";
} // namespace

ShortcutManager *ShortcutManager::sInstance = nullptr;

ShortcutManager::ShortcutManager(QObject *parent) : QAbstractTableModel(parent)
{
}

ShortcutManager::~ShortcutManager() = default;

ShortcutManager *ShortcutManager::instance()
{
    if (!sInstance)
    {
        sInstance = new ShortcutManager(QCoreApplication::instance());
    }

    return sInstance;
}

void ShortcutManager::destroy()
{
    delete sInstance;
    sInstance = nullptr;
}

QString ShortcutManager::settingKey(const QString &objectName)
{
    return QString(kShortcutKeyPrefix) + objectName;
}

QString ShortcutManager::normalizedText(const QKeySequence &sequence)
{
    if (sequence.isEmpty())
    {
        return QString();
    }

    // 比较与落盘都用 PortableText：同一串键在各处写法一致，用户配置才能原样读回来。
    return sequence.toString(QKeySequence::PortableText);
}

QString ShortcutManager::groupLabel(Group group)
{
    switch (group)
    {
        case Group::Common:
            return tr("Common");
        case Group::Board:
            return tr("Board");
        case Group::Tool:
            return tr("Tools");
        case Group::LineColor:
            return tr("Line and Color");
        case Group::Background:
            return tr("Background");
        case Group::Web:
            return tr("Web");
        case Group::Document:
            return tr("Documents");
    }

    return QString();
}

void ShortcutManager::registerActions(Group group, const QList<QAction *> &actions)
{
    Settings  *settings = Settings::settings();
    QList<Row> newRows;

    for (QAction *action : actions)
    {
        const QString objectName = action->objectName();

        // objectName 是持久化键名的一部分，没有它这条动作既存不下也读不回。
        if (objectName.isEmpty())
        {
            qWarning() << "ShortcutManager: 没有 objectName 的动作会被忽略:" << action->text();
            continue;
        }

        if (mDefaults.contains(objectName))
        {
            continue;
        }

        mDefaults.insert(objectName, normalizedText(action->shortcut()));
        newRows << Row{group, action};

        // 读回用户覆盖：键存在就照它办，存成空串表示用户明确清掉了这个快捷键。
        const QVariant stored = settings->value(settingKey(objectName));

        if (stored.isValid())
        {
            action->setShortcut(QKeySequence(stored.toString()));
        }
    }

    if (newRows.isEmpty())
    {
        return;
    }

    const int firstRow = mRows.size();
    beginInsertRows(QModelIndex(), firstRow, firstRow + newRows.size());

    mRows.append(Row{group, nullptr});
    mRows.append(newRows);

    endInsertRows();
}

QAction *ShortcutManager::actionFor(const QString &objectName) const
{
    const int row = rowOf(objectName);

    if (row < 0)
    {
        return nullptr;
    }

    return mRows.at(row).action;
}

QList<QAction *> ShortcutManager::actions() const
{
    QList<QAction *> result;

    for (const Row &row : mRows)
    {
        if (row.action)
        {
            result << row.action;
        }
    }

    return result;
}

int ShortcutManager::rowOf(const QString &objectName) const
{
    for (int i = 0; i < mRows.size(); ++i)
    {
        const QAction *action = mRows.at(i).action;

        if (action && action->objectName() == objectName)
        {
            return i;
        }
    }

    return -1;
}

QString ShortcutManager::defaultText(const QAction *action) const
{
    if (!action)
    {
        return QString();
    }

    return mDefaults.value(action->objectName());
}

bool ShortcutManager::isOverridden(const QAction *action) const
{
    if (!action)
    {
        return false;
    }

    return normalizedText(action->shortcut()) != defaultText(action);
}

QString ShortcutManager::conflictingActionName(const QKeySequence &sequence, const QAction *except) const
{
    const QString wanted = normalizedText(sequence);

    if (wanted.isEmpty())
    {
        return QString();
    }

    for (const Row &row : mRows)
    {
        if (!row.action || row.action == except)
        {
            continue;
        }

        if (normalizedText(row.action->shortcut()) == wanted)
        {
            return row.action->objectName();
        }
    }

    return QString();
}

ShortcutManager::BindResult ShortcutManager::bind(QAction *action, const QKeySequence &sequence)
{
    const int row = action ? rowOf(action->objectName()) : -1;

    if (row < 0)
    {
        return BindResult::UnknownAction;
    }

    const QString conflict = conflictingActionName(sequence, action);

    if (!conflict.isEmpty())
    {
        return BindResult::Conflict;
    }

    action->setShortcut(sequence);

    Settings::settings()->setValue(settingKey(action->objectName()), normalizedText(sequence));
    Settings::settings()->save();

    emit dataChanged(index(row, ShortcutColumn), index(row, DefaultColumn));
    return BindResult::Bound;
}

bool ShortcutManager::resetToDefault(QAction *action)
{
    if (!action)
    {
        return false;
    }

    const int row = rowOf(action->objectName());

    if (row < 0)
    {
        return false;
    }

    Settings::settings()->removeSetting(settingKey(action->objectName()));
    Settings::settings()->save();
    action->setShortcut(QKeySequence(mDefaults.value(action->objectName())));

    emit dataChanged(index(row, ShortcutColumn), index(row, DefaultColumn));
    return true;
}

int ShortcutManager::resetAllToDefault()
{
    QStringList names;

    for (QAction *action : actions())
    {
        if (isOverridden(action))
        {
            names << action->objectName();
        }
    }

    if (names.isEmpty())
    {
        return 0;
    }

    // 整批只做一次落盘：逐条走 resetToDefault 会把用户配置文件同步几十遍。
    Settings *settings = Settings::settings();

    for (const QString &name : names)
    {
        settings->removeSetting(settingKey(name));
        actionFor(name)->setShortcut(QKeySequence(mDefaults.value(name)));
    }

    settings->save();

    emit dataChanged(index(0, ShortcutColumn), index(rowCount() - 1, DefaultColumn));
    return names.size();
}

int ShortcutManager::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : mRows.size();
}

int ShortcutManager::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant ShortcutManager::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= mRows.size())
    {
        return QVariant();
    }

    const Row &row = mRows.at(index.row());

    if (role == ActionNameRole)
    {
        return row.action ? QVariant(row.action->objectName()) : QVariant();
    }

    if (role == OverriddenRole)
    {
        return row.action ? QVariant(isOverridden(row.action)) : QVariant();
    }

    if (role == Qt::FontRole && !row.action)
    {
        QFont bold;
        bold.setBold(true);
        return bold;
    }

    if (role != Qt::DisplayRole)
    {
        return QVariant();
    }

    switch (index.column())
    {
        case CommandColumn:
            return row.action ? QVariant(row.action->text()) : QVariant(groupLabel(row.group));
        case ShortcutColumn:
            return row.action ? QVariant(row.action->shortcut().toString(QKeySequence::NativeText)) : QVariant();
        case DefaultColumn:
            return row.action ? QVariant(defaultText(row.action)) : QVariant();
        default:
            return QVariant();
    }
}

QVariant ShortcutManager::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
    {
        return QVariant();
    }

    switch (section)
    {
        case CommandColumn:
            return tr("Command");
        case ShortcutColumn:
            return tr("Shortcut");
        case DefaultColumn:
            return tr("Default");
        default:
            return QVariant();
    }
}
