#pragma once

#include <QString>

// 一条凭据（用户名 + 秘密）。
struct ProxyCredentials {
    QString userName;
    QString password;

    bool isEmpty() const { return userName.isEmpty() && password.isEmpty(); }
};

// Windows 凭据管理器（CRED_TYPE_GENERIC）的薄封装：秘密由系统按用户托管，不进 INI，
// 也不会在二进制里留下任何密钥。按 target 名读写，所以可以被单独链接、单独测。
class CredentialStore {
public:
    static ProxyCredentials read(const QString &targetName);
    // 两个都空时等价于删除这条凭据。
    static void write(const QString &targetName, const QString &userName, const QString &password);

    // 本工程目前唯一的用例：HTTP 代理的凭据。
    static ProxyCredentials proxyCredentials();
    static void setProxyCredentials(const QString &userName, const QString &password);
};
