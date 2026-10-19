#include "platform/CredentialStore.h"

#include <QDebug>

#include <qt_windows.h>
#include <wincred.h>

// CredReadW/CredWriteW/CredDeleteW 在 advapi32（本工程按 WaveRecorder/WindowsMediaFile
// 的老规矩就地 comment 链接，不在 CMake 里铺一串系统库）。
#pragma comment(lib, "advapi32.lib")

namespace {
const wchar_t kHttpProxyTarget[] = L"TianBoard/HttpProxy";

// Windows 上 wchar_t 与 char16_t 都是 16 位，QString 内部就是 UTF-16，所以直接借
// 数据指针，省一次拷贝。
const wchar_t *utf16(const QString &value) {
    return reinterpret_cast<const wchar_t *>(value.constData());
}

// CredWrite 用 nullptr 表示"这个字段没有"；空串的 constData() 也返回一个指向 NUL 的
// 有效指针，所以空必须显式判，不能靠指针是否为空。
wchar_t *optionalUtf16(const QString &value) {
    return value.isEmpty() ? nullptr : const_cast<wchar_t *>(utf16(value));
}
} // namespace

ProxyCredentials CredentialStore::read(const QString &targetName) {
    ProxyCredentials credentials;

    CREDENTIALW *record = nullptr;
    if (!CredReadW(utf16(targetName), CRED_TYPE_GENERIC, 0, &record))
        return credentials;

    credentials.userName = QString::fromWCharArray(record->UserName);

    // 按 Microsoft 的约定，generic 凭据的 blob 是 UTF-16 的密码（不含终止符）。
    if (record->CredentialBlob && record->CredentialBlobSize >= sizeof(wchar_t)) {
        const int passwordLength = static_cast<int>(record->CredentialBlobSize / sizeof(wchar_t));
        credentials.password = QString::fromWCharArray(
            reinterpret_cast<const wchar_t *>(record->CredentialBlob), passwordLength);
    }

    CredFree(record);
    return credentials;
}

void CredentialStore::write(const QString &targetName, const QString &userName, const QString &password) {
    if (userName.isEmpty() && password.isEmpty()) {
        // 没存过时 CredDeleteW 会失败，而那正是我们要的状态。
        CredDeleteW(utf16(targetName), CRED_TYPE_GENERIC, 0);
        return;
    }

    const int passwordBytes = static_cast<int>(password.size() * sizeof(wchar_t));
    if (passwordBytes > CRED_MAX_CREDENTIAL_BLOB_SIZE) {
        qWarning() << "Secret is too long for the credential store; nothing was saved for" << targetName;
        return;
    }

    CREDENTIALW record = {};
    record.Type = CRED_TYPE_GENERIC;
    record.TargetName = const_cast<wchar_t *>(utf16(targetName));
    record.Persist = CRED_PERSIST_LOCAL_MACHINE;
    record.UserName = optionalUtf16(userName);
    record.CredentialBlob = reinterpret_cast<LPBYTE>(optionalUtf16(password));
    record.CredentialBlobSize = static_cast<DWORD>(passwordBytes);

    if (!CredWriteW(&record, 0))
        qWarning() << "Failed to write" << targetName << "to the credential store";
}

ProxyCredentials CredentialStore::proxyCredentials() {
    return read(QString::fromWCharArray(kHttpProxyTarget));
}

void CredentialStore::setProxyCredentials(const QString &userName, const QString &password) {
    write(QString::fromWCharArray(kHttpProxyTarget), userName, password);
}
