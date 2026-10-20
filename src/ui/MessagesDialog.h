/**
 * @file MessagesDialog.h
 * @brief 多消息提示窗口的声明：逐条只读显示导入等流程收集到的错误信息。
 */

#pragma once

#include <QtWidgets>
#include <QWidget>

#include <memory>

/**
 * @brief 以工具窗口形式罗列多条消息的简易对话框。
 *
 * 名为 Dialog 但继承自 QWidget：窗口置顶且不进入任务栏，每个消息一个只读
 * QTextEdit，底部一个“关闭”按钮。由 DocumentManager 在导入完成后发现错误时
 * 创建并显示，关闭时自行 delete，因此没有长期持有者。
 */
class MessagesDialog : public QWidget
{
    Q_OBJECT

public:
    /// 构造空窗口；消息内容要等 setMessages 才填入。
    MessagesDialog(QString windowTitle, QWidget *parent = NULL);
    ~MessagesDialog() override;

    /**
     * @brief 用给定消息重建窗口内容。
     * @param messages 要显示的消息列表；为空时不创建任何子控件。
     * @note 只在构造后调用一次；重复调用会再叠一层布局。
     */
    void setMessages(const QList<QString> messages);

private slots:
    /// 关闭按钮的槽：直接销毁自身。
    void dispose();

private:
    struct Private;
    std::unique_ptr<Private> d;
};
