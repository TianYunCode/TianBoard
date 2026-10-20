/**
 * @file ProxyDialog.h
 * @brief 代理/站点认证对话框的声明：输入用户名、密码并选择是否记住。
 */

#pragma once

#include <QDialog>

#include <memory>

/**
 * @brief 询问网络认证凭据的模态对话框。
 *
 * 界面上是“Connect to Proxy”标题、用户名与密码输入框（密码回显为圆点）、
 * “保存用户名和密码”复选框以及 OK/Cancel 按钮。由 NetworkAccessManager 在
 * 收到 authenticationRequired 时临时创建，exec() 后即销毁；用户在 OK 时
 * 读取输入值决定是否写入凭据存储。
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

private:
    struct Private;
    std::unique_ptr<Private> d;
};
