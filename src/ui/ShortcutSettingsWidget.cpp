#include "ui/ShortcutSettingsWidget.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelection>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

#include "core/ShortcutManager.h"

struct ShortcutSettingsWidget::Private
{
    ShortcutManager  *manager{nullptr};        ///< 快捷键注册表，本类不持有所有权。
    QTableView       *table{nullptr};          ///< 命令清单。
    QKeySequenceEdit *sequenceEdit{nullptr};   ///< 新组合键的录入框。
    QPushButton      *applyButton{nullptr};    ///< 把录入框里的组合键绑到选中命令。
    QPushButton      *clearButton{nullptr};    ///< 清掉选中命令的快捷键。
    QPushButton      *resetButton{nullptr};    ///< 选中命令恢复默认。
    QPushButton      *resetAllButton{nullptr}; ///< 全部命令恢复默认。
    QLabel           *statusLabel{nullptr};    ///< 上一次操作的结果说明。
};

namespace
{
    /**
     * @brief 取当前选中行对应的动作。
     * @return 选中组标题行、没有选中或该行未登记时返回空指针。
     */
    QAction *selectedAction(QTableView *table, ShortcutManager *manager)
    {
        const QModelIndex current = table->currentIndex();

        if (!current.isValid())
        {
            return nullptr;
        }

        return manager->actionFor(current.sibling(current.row(), ShortcutManager::CommandColumn).data(ShortcutManager::ActionNameRole).toString());
    }
} // namespace

ShortcutSettingsWidget::ShortcutSettingsWidget(ShortcutManager *manager, QWidget *parent) : QWidget(parent), d(new Private)
{
    d->manager          = manager;

    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *hint        = new QLabel(tr("Pick a command in the list, type the new key combination in the box below, then press Apply."), this);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    d->table = new QTableView(this);
    d->table->setObjectName("shortcutTable");
    d->table->setModel(manager);
    d->table->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->table->setSelectionMode(QAbstractItemView::SingleSelection);
    // 只选不编辑：改绑必须经过 bind()，那里才有冲突检查与落盘。
    d->table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    d->table->setShowGrid(false);
    d->table->setAlternatingRowColors(true);
    d->table->setWordWrap(false);
    d->table->verticalHeader()->setVisible(false);
    d->table->horizontalHeader()->setStretchLastSection(false);
    d->table->horizontalHeader()->setSectionResizeMode(ShortcutManager::CommandColumn, QHeaderView::Stretch);
    d->table->horizontalHeader()->setSectionResizeMode(ShortcutManager::ShortcutColumn, QHeaderView::ResizeToContents);
    d->table->horizontalHeader()->setSectionResizeMode(ShortcutManager::DefaultColumn, QHeaderView::ResizeToContents);
    layout->addWidget(d->table, 1);

    QHBoxLayout *editRow = new QHBoxLayout();
    editRow->addWidget(new QLabel(tr("New shortcut:"), this));
    d->sequenceEdit = new QKeySequenceEdit(this);
    d->sequenceEdit->setObjectName("shortcutSequenceEdit");
    editRow->addWidget(d->sequenceEdit, 1);
    d->applyButton = new QPushButton(tr("Apply"), this);
    d->applyButton->setObjectName("shortcutApplyButton");
    d->clearButton = new QPushButton(tr("Clear"), this);
    d->clearButton->setObjectName("shortcutClearButton");
    editRow->addWidget(d->applyButton);
    editRow->addWidget(d->clearButton);
    layout->addLayout(editRow);

    QHBoxLayout *resetRow = new QHBoxLayout();
    d->resetButton        = new QPushButton(tr("Reset Selected"), this);
    d->resetButton->setObjectName("shortcutResetButton");
    d->resetAllButton = new QPushButton(tr("Reset All"), this);
    d->resetAllButton->setObjectName("shortcutResetAllButton");
    resetRow->addWidget(d->resetButton);
    resetRow->addWidget(d->resetAllButton);
    resetRow->addStretch(1);
    layout->addLayout(resetRow);

    d->statusLabel = new QLabel(this);
    d->statusLabel->setObjectName("shortcutStatusLabel");
    d->statusLabel->setWordWrap(true);
    layout->addWidget(d->statusLabel);

    connect(d->applyButton, &QPushButton::clicked, this, &ShortcutSettingsWidget::applyEditedSequence);
    connect(d->clearButton, &QPushButton::clicked, this, &ShortcutSettingsWidget::clearSelectedSequence);
    connect(d->resetButton, &QPushButton::clicked, this, &ShortcutSettingsWidget::resetSelected);
    connect(d->resetAllButton, &QPushButton::clicked, this, &ShortcutSettingsWidget::resetAll);

    // 选中行一变：把该行的现键填进录入框，并按“是否选中了动作”开关四枚按钮。
    auto syncSelection = [this]
    {
        QAction   *action    = selectedAction(d->table, d->manager);
        const bool hasAction = (action != nullptr);

        if (hasAction)
        {
            d->sequenceEdit->setKeySequence(action->shortcut());
        }
        else
        {
            d->sequenceEdit->clear();
        }

        d->sequenceEdit->setEnabled(hasAction);
        d->applyButton->setEnabled(hasAction);
        d->clearButton->setEnabled(hasAction);
        d->resetButton->setEnabled(hasAction && d->manager->isOverridden(action));
    };

    connect(d->table->selectionModel(), &QItemSelectionModel::selectionChanged, this,
        [syncSelection](const QItemSelection &, const QItemSelection &)
        {
            syncSelection();
        });
    // 选中行没变而绑定变了（应用/恢复之后）也要重算：OverriddenRole 一变，“恢复所选”
    // 该不该亮就得跟着变。
    connect(d->manager, &QAbstractItemModel::dataChanged, this,
        [syncSelection](const QModelIndex &, const QModelIndex &, const QVector<int> &)
        {
            syncSelection();
        });

    syncSelection();
}

ShortcutSettingsWidget::~ShortcutSettingsWidget() = default;

void ShortcutSettingsWidget::applyEditedSequence()
{
    QAction *action = selectedAction(d->table, d->manager);

    if (!action)
    {
        return;
    }

    const QKeySequence                sequence = d->sequenceEdit->keySequence();
    const ShortcutManager::BindResult result   = d->manager->bind(action, sequence);

    if (result == ShortcutManager::BindResult::Conflict)
    {
        const QAction *owner = d->manager->actionFor(d->manager->conflictingActionName(sequence, action));
        d->statusLabel->setText(
            tr("%1 is already used by %2.").arg(sequence.toString(QKeySequence::NativeText), owner ? owner->text() : tr("another command")));
    }
    else if (result == ShortcutManager::BindResult::Bound)
    {
        d->statusLabel->setText(tr("%1 is now bound to %2.").arg(action->text(), sequence.toString(QKeySequence::NativeText)));
    }
}

void ShortcutSettingsWidget::clearSelectedSequence()
{
    QAction *action = selectedAction(d->table, d->manager);

    if (!action)
    {
        return;
    }

    if (d->manager->bind(action, QKeySequence()) == ShortcutManager::BindResult::Bound)
    {
        d->statusLabel->setText(tr("%1 no longer has a shortcut.").arg(action->text()));
    }
}

void ShortcutSettingsWidget::resetSelected()
{
    QAction *action = selectedAction(d->table, d->manager);

    if (!action)
    {
        return;
    }

    if (d->manager->resetToDefault(action))
    {
        d->statusLabel->setText(tr("%1 is back to its default shortcut %2.").arg(action->text(), d->manager->defaultText(action)));
    }
}

void ShortcutSettingsWidget::resetAll()
{
    const int changed = restoreAllShortcuts();

    if (changed == 0)
    {
        d->statusLabel->setText(tr("Every command already uses its default shortcut."));
    }
    else
    {
        d->statusLabel->setText(tr("%1 shortcut(s) restored to default.").arg(changed));
    }
}

int ShortcutSettingsWidget::restoreAllShortcuts()
{
    return d->manager->resetAllToDefault();
}
