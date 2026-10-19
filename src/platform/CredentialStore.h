#pragma once

#include <QString>

// 代理的用户名/密码。Windows 上落到凭据管理器（CRED_TYPE_GENERIC），不再是 INI 里
// 一段"用写死密钥做的 AES"——那种做法任何拿到二进制的人都能解密，等于没加密。
struct ProxyCredentials {
    QString userName;
    QString password;

    bool isEmpty() const { return userName.isEmpty() && password.isEmpty(); }
};

class CredentialStore {
public:
    static ProxyCredentials proxyCredentials();
    // 两个都空时删除已存的凭据。
    static void setProxyCredentials(const QString &userName, const QString &password);
};
