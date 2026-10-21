/**
 * @file DocumentReplaceDialog.h
 * @brief 导入时文档重名的裁决对话框：改名新建、顶掉同名旧文档，或放弃这次导入。
 */

#pragma once

#include <QtWidgets>

#include "core/PersistenceManager.h"

/**
 * @brief 导入文件重名时的替换对话框。
 *
 * 它是 PersistenceManager 的重名裁决钩子的实现，由装配点装上（app/main.cpp）；
 * 除了 ask() 之外没有人构造它。输入命中同级已有名称时确认按钮变成“替换”，输入新名字
 * 则是新建一份，取消即放弃这次导入。
 */
class DocumentReplaceDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造对话框。
     * @param pIncommingName 待导入文档的原始名称。
     * @param pFileList 同级已存在的文档名列表。
     * @param parent 父窗口。
     * @param pFlags 窗口标志。
     */
    DocumentReplaceDialog(
        const QString &pIncommingName, const QStringList &pFileList, QWidget *parent = nullptr, Qt::WindowFlags pFlags = Qt::WindowFlags());

    /** @brief 返回用户最终输入的名称。 */
    QString lineEditText() const
    {
        return mLineEdit->text();
    }

    /**
     * @brief 重名裁决钩子的实现：弹出对话框，把用户的选择折成一份裁决。
     * @param documentName 待登记文档的名字。
     * @param siblingNames 同分组里已有的文档名。
     * @note finalName 落在 siblingNames 里即“替换那一份”；取消或输入空名字都返回 proceed=false。
     */
    static PersistenceManager::DuplicateNameDecision ask(const QString &documentName, const QStringList &siblingNames);

private slots:
    /** @brief 接受对话框（名称合法时由确认按钮触发）。 */
    void accept();

    /** @brief 取消对话框：清空输入框。 */
    void reject();

    /** @brief 输入变化时刷新校验底色与确认按钮文案。 */
    void reactOnTextChanged(const QString &pStr);

private:
    /** @brief 判断输入框当前内容是否可用。 */
    bool validString(const QString &pStr);

    /** @brief 生成带文档名的提示文案。 */
    QString labelTextWithName(const QString &documentName) const;

    /** @brief 名称输入框。 */
    QLineEdit *mLineEdit;
    /** @brief 同级已存在的名称列表。 */
    QStringList mFileNameList;
    /** @brief 确认按钮。 */
    QPushButton *acceptButton;
    /** @brief 确认按钮文案：接受。 */
    const QString acceptText;
    /** @brief 确认按钮文案：替换。 */
    const QString replaceText;
    /** @brief 对话框上方的提示标签。 */
    QLabel *mLabelText;
};
