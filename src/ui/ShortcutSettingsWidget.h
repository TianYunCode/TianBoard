/**
 * @file ShortcutSettingsWidget.h
 * @brief 偏好设置“快捷键”页的声明：一张快捷键清单、一个改绑输入框和两枚恢复默认按钮。
 */

#pragma once

#include <QWidget>

#include <memory>

class ShortcutManager;

/**
 * @brief 让用户查看并重新指定应用命令的快捷键。
 *
 * 详细说明：清单直接架在注入的 ShortcutManager 上，本页不持有任何绑定数据——
 * 改绑、清空与恢复默认一律调用管理器，由它做冲突检查、写用户配置并刷新表格；
 * 本页负责把结果与失败原因写成一句提示。表里只选不编辑，因为双击单元格直接录入
 * 既容易吃到半截组合键，也没地方解释“为什么这个键被拒了”。
 * 生命周期：由偏好设置对话框创建为一个标签页；注入的管理器活得比本页久。
 * 线程约束：GUI 线程专用。
 */
class ShortcutSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 用注入的注册表建页。
     * @param manager 快捷键注册表，必须是活的实例（应用共用那一份）。
     * @param parent 父窗口，通常是偏好设置对话框。
     */
    explicit ShortcutSettingsWidget(ShortcutManager *manager, QWidget *parent = nullptr);
    /// @brief 析构。
    ~ShortcutSettingsWidget() override;

    /**
     * @brief 把全部快捷键恢复到出厂默认，返回真正被改动的条数。
     * @note 供对话框的“恢复默认设置”按钮在当前页是本页时调用。
     */
    int restoreAllShortcuts();

private slots:
    /// @brief 把输入框里的组合键绑到当前选中的命令上。
    void applyEditedSequence();
    /// @brief 清掉当前选中命令的快捷键（不恢复默认，是真的不绑键）。
    void clearSelectedSequence();
    /// @brief 把当前选中的命令恢复到默认快捷键。
    void resetSelected();
    /// @brief 全部恢复默认并在提示行报告条数。
    void resetAll();

private:
    struct Private;
    std::unique_ptr<Private> d;
};
