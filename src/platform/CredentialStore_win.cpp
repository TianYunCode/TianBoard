#include "platform/CredentialStore.h"

#include <QDebug>

#include <qt_windows.h>
#include <wincred.h>

// CredReadW/CredWriteW/CredDeleteW 在 advapi32（本工程按 WaveRecorder/WindowsMediaFile
// 的老规矩就地 comment 链接，不在 CMake 里铺一串系统库）。
#pragma comment(lib, "advapi32.lib")

namespace {
const wchar_t kHttpProxyTarget[] = L"TianBoard/HttpProxy";
}

ProxyCredentials CredentialStore::proxyCredentials() {
    ProxyCredentials credentials;

    CREDENTIALW *record = nullptr;
    if (!CredReadW(kHttpProxyTarget, CRED_TYPE_GENERIC, 0, &record))
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

void CredentialStore::setProxyCredentials(const QString &userName, const QString &password) {
    if (userName.isEmpty() && password.isEmpty()) {
        // 没有凭据时 CredDeleteW 会失败，那正是我们要的状态。
        CredDeleteW(kHttpProxyTarget, CRED_TYPE_GENERIC, 0);
        return;
    }

    const int passwordBytes = static_cast<int>(password.size() * sizeof(wchar_t));
    if (passwordBytes > CRED_MAX_CREDENTIAL_BLOB_SIZE) {
        qWarning() << "Proxy password is too long for the credential store; not saved";
        return;
    }

    // QString 内部本来就是 UTF-16，直接取数据指针；空的 QString 也必须取 data()
    // （Qt6 里它返回一个指向 NUL 的有效指针），所以先算好长度再决定要不要给指针。
    const wchar_t *userNameData = reinterpret_cast<const wchar_t *>(userName.constData());
    const wchar_t *passwordData = reinterpret_cast<const wchar_t *>(password.constData());

    CREDENTIALW record = {};
    record.Type = CRED_TYPE_GENERIC;
    record.TargetName = const_cast<LPWSTR>(kHttpProxyTarget);
    record.Persist = CRED_PERSIST_LOCAL_MACHINE;
    record.UserName = userName.isEmpty() ? nullptr : const_cast<LPWSTR>(userNameData);
    record.CredentialBlob =
        password.isEmpty() ? nullptr : reinterpret_cast<LPBYTE>(const_cast<wchar_t *>(passwordData));
    record.CredentialBlobSize = static_cast<DWORD>(passwordBytes);

    if (!CredWriteW(&record, 0))
        qWarning() << "Failed to write the proxy credentials to the credential store";
}
