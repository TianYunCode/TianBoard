/**
 * @file CredentialStore.h
 * @brief 声明 Windows 凭据管理器的薄封装，用于按目标名存取用户名与秘密。
 */

#pragma once

#include <QString>

/**
 * @brief 一条凭据（用户名 + 秘密）。
 *
 * 纯数据载体；isEmpty() 用来判断“没有配置”，而不是判断内容是否合法。
 */
struct ProxyCredentials
{
    QString userName; ///< 用户名。
    QString password; ///< 秘密（密码或令牌）。

    /** @brief 用户名与秘密都为空时返回 true。 */
    bool isEmpty() const
    {
        return userName.isEmpty() && password.isEmpty();
    }
};

/**
 * @brief Windows 凭据管理器（CRED_TYPE_GENERIC）的薄封装。
 *
 * 位于 Win32 平台层：秘密由系统按当前用户托管，不写入 INI，也不会在二进制里
 * 留下密钥。按 target 名读写，因此可被单独链接、单独测试。所有调用都在 GUI 线程，
 * 底层 API 失败时静默返回空凭据或空操作，不抛异常。
 */
class CredentialStore
{
public:
    /** @brief 读取指定 target 的凭据；不存在或读取失败时返回空对象。 */
    static ProxyCredentials read(const QString &targetName);
    /** @brief 写入指定 target 的凭据；用户名与密码都为空时等价于删除该条凭据。 */
    static void write(const QString &targetName, const QString &userName, const QString &password);

    /** @brief 读取本工程唯一的用例：HTTP 代理凭据。 */
    static ProxyCredentials proxyCredentials();
    /** @brief 保存 HTTP 代理凭据。 */
    static void setProxyCredentials(const QString &userName, const QString &password);
};
