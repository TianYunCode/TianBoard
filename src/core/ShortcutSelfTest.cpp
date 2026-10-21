#include "core/ShortcutSelfTest.h"

#include <QAction>
#include <QDebug>
#include <QHash>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QPushButton>
#include <QTableView>
#include <QStringList>
#include <QVariant>

#include "core/Application.h"
#include "core/ShortcutManager.h"
#include "settings/Settings.h"
#include "ui/MainWindow.h"
#include "ui/ShortcutSettingsWidget.h"

namespace
{
    /// @brief 自检专用动作名的前缀：写进用户配置也只落在这几个键上，收尾统一删掉。
    const QString kTestKeyPrefix = QStringLiteral("selftestShortcut");

    /**
     * @brief 造一个带默认快捷键的动作，父对象交给调用方的作用域对象。
     * @param parent 所有权归属，自检结束时随它一起销毁。
     * @param name 后缀，最终 objectName 是前缀 + 它。
     * @param sequence 默认快捷键文本，空串表示这个动作出厂不绑键。
     */
    QAction *makeAction(QObject *parent, const QString &name, const QString &sequence)
    {
        QAction *action = new QAction(parent);
        action->setObjectName(kTestKeyPrefix + name);
        action->setText(name);

        if (!sequence.isEmpty())
        {
            action->setShortcut(QKeySequence(sequence));
        }

        return action;
    }

    /// @brief 读某个动作在用户配置里的原始值；键不存在返回无效 QVariant。
    QVariant storedOverride(const QAction *action)
    {
        return Settings::settings()->value(ShortcutManager::settingKey(action->objectName()));
    }
} // namespace

namespace ShortcutSelfTest
{
    int run()
    {
        QStringList failures;
        QObject     scope;

        // ---------- 一、应用真实那一份：装配完整、默认值还在、互不撞键 ----------
        ShortcutManager *live = ShortcutManager::instance();

        if (live->actions().isEmpty())
        {
            failures << QStringLiteral("主窗口一个动作都没登记进快捷键注册表");
        }

        const QStringList mustBeRegistered = {QStringLiteral("actionUndo"), QStringLiteral("actionRedo"), QStringLiteral("actionBack"),
            QStringLiteral("actionForward"), QStringLiteral("actionPen"), QStringLiteral("actionEraser"), QStringLiteral("actionSnap"),
            QStringLiteral("actionCopy"), QStringLiteral("actionDelete")};

        for (const QString &objectName : mustBeRegistered)
        {
            if (!live->actionFor(objectName))
            {
                failures << QStringLiteral("注册表里找不到 %1").arg(objectName);
            }
        }

        // 默认值是从 setupUi 那一段捕获的；这里抽查几条，能看出“搬位置搬丢了”这类事故。
        struct ExpectedDefault
        {
            const char *objectName;
            const char *sequence;
        };

        static const ExpectedDefault expectedDefaults[] = {
            {"actionUndo",    "Ctrl+Z"},
            {"actionRedo",    "Ctrl+Y"},
            {"actionBack",    "PgUp"  },
            {"actionForward", "PgDown"},
            {"actionPen",     "Ctrl+I"},
            {"actionSnap",    "Ctrl+S"},
            {"actionDelete",  "Del"   },
        };

        for (const ExpectedDefault &expected : expectedDefaults)
        {
            const QString objectName = QString::fromLatin1(expected.objectName);
            QAction      *action     = live->actionFor(objectName);

            if (!action)
            {
                continue;
            }

            const QString actual = ShortcutManager::normalizedText(action->shortcut());

            if (actual != QString::fromLatin1(expected.sequence))
            {
                failures << QStringLiteral("%1 的默认快捷键是“%2”，期望“%3”").arg(objectName, actual, QString::fromLatin1(expected.sequence));
            }

            if (live->isOverridden(action))
            {
                failures << QStringLiteral("%1 刚装配完就被判定成用户改过，说明默认值捕获与实际值不一致").arg(objectName);
            }
        }

        // 出厂默认之间不许撞键：撞了的话，改绑界面上永远有两条互相报冲突，
        // 而且先收到键的那个动作是未定的。
        QHash<QString, QString> keyOwner;

        for (QAction *action : live->actions())
        {
            const QString key = ShortcutManager::normalizedText(action->shortcut());

            if (key.isEmpty())
            {
                continue;
            }

            if (keyOwner.contains(key))
            {
                failures << QStringLiteral("%1 与 %2 抢同一个快捷键“%3”").arg(keyOwner.value(key), action->objectName(), key);
            }
            else
            {
                keyOwner.insert(key, action->objectName());
            }
        }

        // objectName 是持久化键名，重复登记会让两条动作共用同一个配置键。
        QHash<QString, int> nameCount;

        for (QAction *action : live->actions())
        {
            nameCount[action->objectName()] = nameCount.value(action->objectName()) + 1;
        }

        for (auto it = nameCount.constBegin(); it != nameCount.constEnd(); ++it)
        {
            if (it.value() > 1)
            {
                failures << QStringLiteral("%1 被登记了 %2 次").arg(it.key()).arg(it.value());
            }
        }

        // 偏好设置对话框在主窗口装配时就建好了（Application::exec 里 new PreferencesController），
        // 顺着主窗口验一遍“快捷键那一页真在界面树里、清单真架在应用共用注册表上”——
        // 对话框是主窗口的子控件，从 qApp 找是找不到的。
        QList<ShortcutSettingsWidget *> pages;

        if (Application::mainWindow)
        {
            pages = Application::mainWindow->findChildren<ShortcutSettingsWidget *>();
        }

        if (pages.size() != 1)
        {
            failures << QStringLiteral("主窗口下找到 %1 个快捷键页，期望偏好设置对话框里那 1 个").arg(pages.size());
        }
        else
        {
            QTableView *liveView = pages.at(0)->findChild<QTableView *>(QStringLiteral("shortcutTable"));

            if (!liveView || liveView->model() != live)
            {
                failures << QStringLiteral("对话框里的快捷键页没架在应用共用注册表上");
            }
            else if (liveView->model()->rowCount() <= live->actions().size())
            {
                failures << QStringLiteral("快捷键页的清单只有 %1 行，装不下 %2 条命令加分组标题")
                                .arg(liveView->model()->rowCount())
                                .arg(live->actions().size());
            }
        }

        // ---------- 二、自建一张表，跑改绑 → 落盘 → 读回 → 恢复默认 ----------
        {
            ShortcutManager table;
            QAction        *press = makeAction(&scope, QStringLiteral("A"), QStringLiteral("Ctrl+Z"));
            QAction        *swap  = makeAction(&scope, QStringLiteral("B"), QStringLiteral("Ctrl+X"));
            QAction        *spare = makeAction(&scope, QStringLiteral("C"), QString());

            table.registerActions(ShortcutManager::Group::Board, {press, swap, spare});

            // 表格形状：一行组标题 + 三行动作，列数与枚举一致。
            if (table.rowCount() != 4)
            {
                failures << QStringLiteral("自建注册表行数 %1，期望 4（1 行组标题 + 3 行动作）").arg(table.rowCount());
            }

            if (table.columnCount() != ShortcutManager::ColumnCount)
            {
                failures << QStringLiteral("自建注册表列数 %1，期望 %2").arg(table.columnCount()).arg(ShortcutManager::ColumnCount);
            }

            if (table.data(table.index(0, ShortcutManager::CommandColumn), Qt::DisplayRole).toString() !=
                table.groupLabel(ShortcutManager::Group::Board))
            {
                failures << QStringLiteral("第一行不是组标题");
            }

            if (table.data(table.index(0, ShortcutManager::CommandColumn), ShortcutManager::ActionNameRole).isValid())
            {
                failures << QStringLiteral("组标题行不该带 objectName");
            }

            if (table.data(table.index(1, ShortcutManager::ShortcutColumn), ShortcutManager::ActionNameRole).toString() != press->objectName())
            {
                failures << QStringLiteral("ActionNameRole 取回的 objectName 不对");
            }

            // 模型只读：改绑必须走 bind()，否则冲突检查与落盘都会被视图绕过。
            if (table.flags(table.index(1, ShortcutManager::ShortcutColumn)) & Qt::ItemIsEditable)
            {
                failures << QStringLiteral("快捷键列被标成可编辑");
            }

            if (table.data(table.index(1, ShortcutManager::DefaultColumn), Qt::DisplayRole).toString() != QStringLiteral("Ctrl+Z"))
            {
                failures << QStringLiteral("默认列没显示注册那一刻记下的快捷键");
            }

            // 改绑：动作、配置、表格三方都要跟上。
            const QKeySequence fresh(QStringLiteral("Ctrl+Shift+Q"));

            if (table.bind(press, fresh) != ShortcutManager::BindResult::Bound)
            {
                failures << QStringLiteral("绑一个新键就被拒了");
            }

            if (press->shortcut() != fresh)
            {
                failures << QStringLiteral("改绑后 QAction 上的快捷键没变");
            }

            if (storedOverride(press).toString() != ShortcutManager::normalizedText(fresh))
            {
                failures << QStringLiteral("改绑没写进用户配置");
            }

            if (!table.isOverridden(press))
            {
                failures << QStringLiteral("已经改绑的动作被判定成没改过");
            }

            if (!table.data(table.index(1, ShortcutManager::ShortcutColumn), ShortcutManager::OverriddenRole).toBool())
            {
                failures << QStringLiteral("改绑之后 OverriddenRole 仍是 false");
            }

            if (table.defaultText(press) != QStringLiteral("Ctrl+Z"))
            {
                failures << QStringLiteral("改绑把默认值也改写了");
            }

            // 撞键：把另一条动作绑到刚占用的键上，必须整笔拒绝。
            if (table.bind(swap, fresh) != ShortcutManager::BindResult::Conflict)
            {
                failures << QStringLiteral("撞键的改绑被接受了");
            }

            if (swap->shortcut() != QKeySequence(QStringLiteral("Ctrl+X")))
            {
                failures << QStringLiteral("撞键被拒之后动作的快捷键仍被改掉了");
            }

            if (storedOverride(swap).isValid())
            {
                failures << QStringLiteral("撞键被拒之后仍写了一个配置键");
            }

            // 明确清空：空串要能存下来，重启后不许偷偷回到默认。
            if (table.bind(swap, QString()) != ShortcutManager::BindResult::Bound)
            {
                failures << QStringLiteral("清空快捷键被拒了");
            }

            if (!swap->shortcut().isEmpty())
            {
                failures << QStringLiteral("清空之后 QAction 仍带着快捷键");
            }

            const QVariant cleared = storedOverride(swap);

            if (!cleared.isValid() || !cleared.toString().isEmpty())
            {
                failures << QStringLiteral("清空之后配置里不是“存在且为空”");
            }

            // 模拟重启：同名动作换一份新的默认值注册，覆盖必须照旧生效。
            {
                ShortcutManager reloaded;
                QAction        *pressAgain = makeAction(&scope, QStringLiteral("A"), QStringLiteral("Ctrl+Z"));
                QAction        *swapAgain  = makeAction(&scope, QStringLiteral("B"), QStringLiteral("Ctrl+X"));
                reloaded.registerActions(ShortcutManager::Group::Board, {pressAgain, swapAgain});

                if (pressAgain->shortcut() != fresh)
                {
                    failures << QStringLiteral("重启后没把用户改的键读回来");
                }

                if (!swapAgain->shortcut().isEmpty())
                {
                    failures << QStringLiteral("用户明确清空的快捷键在重启后又回来了");
                }

                if (reloaded.defaultText(pressAgain) != QStringLiteral("Ctrl+Z"))
                {
                    failures << QStringLiteral("重启后默认值列不对");
                }
            }

            // 恢复默认：配置键要彻底删掉，动作回到注册那一刻的值。
            if (!table.resetToDefault(press))
            {
                failures << QStringLiteral("恢复默认失败");
            }

            if (press->shortcut() != QKeySequence(QStringLiteral("Ctrl+Z")))
            {
                failures << QStringLiteral("恢复默认后动作没回到默认键");
            }

            if (storedOverride(press).isValid())
            {
                failures << QStringLiteral("恢复默认后配置键还留在文件里");
            }

            if (table.isOverridden(press))
            {
                failures << QStringLiteral("恢复默认后仍被判定成改过");
            }

            // 此时只有 swap 带着“清空”这一条覆盖，计数必须是 1。
            const int changed = table.resetAllToDefault();

            if (changed != 1)
            {
                failures << QStringLiteral("全部恢复默认改了 %1 条，期望 1 条").arg(changed);
            }

            if (swap->shortcut() != QKeySequence(QStringLiteral("Ctrl+X")))
            {
                failures << QStringLiteral("全部恢复默认之后被清空的动作没回到默认");
            }
        }

        // ---------- 三、快捷键页本身：选中、应用、冲突、清除、恢复默认 ----------
        {
            ShortcutManager pageTable;
            QAction        *alpha = makeAction(&scope, QStringLiteral("D"), QStringLiteral("Ctrl+Z"));
            QAction        *beta  = makeAction(&scope, QStringLiteral("E"), QStringLiteral("Ctrl+X"));
            pageTable.registerActions(ShortcutManager::Group::Tool, {alpha, beta});

            ShortcutSettingsWidget page(&pageTable);

            QTableView       *view     = page.findChild<QTableView *>(QStringLiteral("shortcutTable"));
            QKeySequenceEdit *edit     = page.findChild<QKeySequenceEdit *>(QStringLiteral("shortcutSequenceEdit"));
            QPushButton      *apply    = page.findChild<QPushButton *>(QStringLiteral("shortcutApplyButton"));
            QPushButton      *clear    = page.findChild<QPushButton *>(QStringLiteral("shortcutClearButton"));
            QPushButton      *reset    = page.findChild<QPushButton *>(QStringLiteral("shortcutResetButton"));
            QPushButton      *resetAll = page.findChild<QPushButton *>(QStringLiteral("shortcutResetAllButton"));
            QLabel           *status   = page.findChild<QLabel *>(QStringLiteral("shortcutStatusLabel"));

            if (!view || !edit || !apply || !clear || !reset || !resetAll || !status)
            {
                failures << QStringLiteral("快捷键页上少了控件，自检没法继续");
            }
            else if (view->model() != &pageTable)
            {
                failures << QStringLiteral("快捷键页的清单没架在注入的注册表上");
            }
            else
            {
                // 选中第一行动作：录入框跟上现键，未改过的动作不给“恢复所选”。
                view->setCurrentIndex(pageTable.index(1, ShortcutManager::CommandColumn));

                if (edit->keySequence() != QKeySequence(QStringLiteral("Ctrl+Z")))
                {
                    failures << QStringLiteral("选中一行后录入框没填上当前快捷键");
                }

                if (!apply->isEnabled() || !clear->isEnabled())
                {
                    failures << QStringLiteral("选中了动作，应用/清除却还是禁用的");
                }

                if (reset->isEnabled())
                {
                    failures << QStringLiteral("没改过的动作也能点“恢复所选”");
                }

                edit->setKeySequence(QKeySequence(QStringLiteral("Ctrl+Shift+7")));
                QMetaObject::invokeMethod(&page, "applyEditedSequence", Qt::DirectConnection);

                if (alpha->shortcut() != QKeySequence(QStringLiteral("Ctrl+Shift+7")))
                {
                    failures << QStringLiteral("页面上的“应用”没真的改绑");
                }

                if (!reset->isEnabled())
                {
                    failures << QStringLiteral("改过之后“恢复所选”仍然禁用");
                }

                if (storedOverride(alpha).toString() != QStringLiteral("Ctrl+Shift+7"))
                {
                    failures << QStringLiteral("页面上改绑之后配置没跟上");
                }

                // 冲突：给第二行动作输入同一个组合键，页面必须拒绝并给出说明。
                view->setCurrentIndex(pageTable.index(2, ShortcutManager::CommandColumn));
                edit->setKeySequence(QKeySequence(QStringLiteral("Ctrl+Shift+7")));
                QMetaObject::invokeMethod(&page, "applyEditedSequence", Qt::DirectConnection);

                if (beta->shortcut() != QKeySequence(QStringLiteral("Ctrl+X")))
                {
                    failures << QStringLiteral("冲突的改绑在页面上被接受了");
                }

                if (status->text().isEmpty())
                {
                    failures << QStringLiteral("拒绝冲突改绑之后页面没有说明");
                }

                // 恢复所选：alpha 回到默认。
                view->setCurrentIndex(pageTable.index(1, ShortcutManager::CommandColumn));
                QMetaObject::invokeMethod(&page, "resetSelected", Qt::DirectConnection);

                if (alpha->shortcut() != QKeySequence(QStringLiteral("Ctrl+Z")) || storedOverride(alpha).isValid())
                {
                    failures << QStringLiteral("页面上的“恢复所选”没回到默认");
                }

                // 清除：beta 变成“明确不绑键”。
                view->setCurrentIndex(pageTable.index(2, ShortcutManager::CommandColumn));
                QMetaObject::invokeMethod(&page, "clearSelectedSequence", Qt::DirectConnection);

                if (!beta->shortcut().isEmpty())
                {
                    failures << QStringLiteral("页面上的“清除”没生效");
                }

                // 页面级全部恢复：只剩 beta 这一条覆盖，计数必须是 1。
                if (page.restoreAllShortcuts() != 1)
                {
                    failures << QStringLiteral("页面的全部恢复默认少算或多算了条数");
                }

                if (beta->shortcut() != QKeySequence(QStringLiteral("Ctrl+X")))
                {
                    failures << QStringLiteral("页面的全部恢复默认没把被清空的键找回来");
                }

                // 按钮那条路：再点一次“全部恢复默认”，此时已无覆盖，必须给一句说明。
                QMetaObject::invokeMethod(&page, "resetAll", Qt::DirectConnection);

                if (status->text().isEmpty())
                {
                    failures << QStringLiteral("页面上的全部恢复默认没有给出说明");
                }
            }
        }

        // 收尾：把自检碰过的配置键全部删掉，别把痕迹留给真实用户。
        Settings *settings = Settings::settings();

        for (const QString &suffix : {QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("C"), QStringLiteral("D"), QStringLiteral("E")})
        {
            settings->removeSetting(ShortcutManager::settingKey(kTestKeyPrefix + suffix));
        }

        settings->save();

        for (const QString &failure : failures)
        {
            qWarning() << "shortcut-selftest:" << failure;
        }

        if (failures.isEmpty())
        {
            qInfo() << "shortcut-selftest: OK";
        }

        return failures.size();
    }
} // namespace ShortcutSelfTest
