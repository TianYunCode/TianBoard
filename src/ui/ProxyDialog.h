/**
 * @file ProxyDialog.h
 * @brief 代理/站点认证对话框的声明：输入用户名、密码并选择是否记住。
 */

#pragma once

#include <QDialog>
#include <QUrl>

#include <memory>

/**
 * @brief 询问网络认证凭据的模态对话框。
 *
 * 界面上是“Connect to Proxy”标题、用户名与密码输入框（密码回显为圆点）、
 * “保存用户名和密码”复选框以及 OK/Cancel 按钮。站点要认证时由
 * NetworkAccessManager 的征询钩子经 prompt() 临时创建，exec() 后即销毁；
 * 是否把凭据写进凭据管理器由调用方按 remember 决定（服务层存、界面层收）。
 */
class ProxyDialog : public QDialog
{
    Q_OBJECT

public:
    /// 构造对话框并搭建网格布局，默认填入提示文字与标准问号图标。
    ProxyDialog(QWidget *parent = nullptr);
    ~ProxyDialog() override;

    /// 替换顶部的说明文字（例如要认证的 URL）。
    void setIntroText(const QString &text);
    /// 预填用户名，通常来自已保存的代理凭据。
    void setUserName(const QString &userName);
    /// 预填密码。
    void setPassword(const QString &password);

    /// 返回输入框中的用户名。
    QString userName() const;
    /// 返回输入框中的密码。
    QString password() const;
    /// 返回用户是否勾选了“保存用户名和密码”。
    bool shouldSaveCredentials() const;

    /**
     * @brief 弹一次凭据框，供 NetworkAccessManager 的征询钩子调用（见 app/main.cpp）。
     * @param parent 对话框父窗口（当前活动窗口），为空时居中显示。
     * @param url 要认证的地址，写进说明文字。
     * @param user password 进来是已存凭据的预填值，出去是用户确认的值。
     * @param remember 用户是否勾选了“记住”。
     * @return 用户点了确定返回 true；取消或没有输入返回 false。
     */
    static bool prompt(QWidget *parent, const QUrl &url, QString &user, QString &password, bool &remember);

private:
    struct Private;
    std::unique_ptr<Private> d;
};
