// CredentialStore 是全工程唯一碰 Windows 凭据管理器的地方，逻辑都在编码、长度、
// 空值这些容易写错的地方（blob 是 UTF-16 且不含终止符，空字段要用 nullptr 表示）。
// 它只依赖 Qt6::Core 与 advapi32，所以能像 utils 那几个用例一样单独链接、单独跑，
// 不用把整个应用栈拖进来。
#include <QtTest>

#include "platform/CredentialStore.h"

class TestCredentialStore : public QObject {
    Q_OBJECT

private slots:
    void roundTripKeepsUnicode();
    void unknownTargetReadsAsEmpty();
    void overwriteReplacesThePreviousSecret();
    void userNameWithoutSecret();
    void clearingBothRemovesTheCredential();
    void oversizedSecretIsNotStored();
    void cleanupTestCase();

private:
    QString target() const;
};

QString TestCredentialStore::target() const {
    // 每次运行用自己的 PID 当 target，避免和别的进程/别的用例互相踩；
    // 收尾时 cleanupTestCase() 会把它删掉。
    return QStringLiteral("TianBoard/Test/%1").arg(QCoreApplication::applicationPid());
}

void TestCredentialStore::roundTripKeepsUnicode() {
    CredentialStore::write(target(), QStringLiteral("tester"), QStringLiteral("sésame 密码 😀"));

    const ProxyCredentials stored = CredentialStore::read(target());
    QCOMPARE(stored.userName, QStringLiteral("tester"));
    QCOMPARE(stored.password, QStringLiteral("sésame 密码 😀"));
}

void TestCredentialStore::unknownTargetReadsAsEmpty() {
    const ProxyCredentials missing = CredentialStore::read(QStringLiteral("TianBoard/Test/no-such-target"));
    QVERIFY(missing.userName.isEmpty());
    QVERIFY(missing.password.isEmpty());
    QVERIFY(missing.isEmpty());
}

void TestCredentialStore::overwriteReplacesThePreviousSecret() {
    CredentialStore::write(target(), QStringLiteral("first"), QStringLiteral("one"));
    CredentialStore::write(target(), QStringLiteral("second"), QStringLiteral("two"));

    const ProxyCredentials stored = CredentialStore::read(target());
    QCOMPARE(stored.userName, QStringLiteral("second"));
    QCOMPARE(stored.password, QStringLiteral("two"));
}

void TestCredentialStore::userNameWithoutSecret() {
    CredentialStore::write(target(), QStringLiteral("only-user"), QString());

    const ProxyCredentials stored = CredentialStore::read(target());
    QCOMPARE(stored.userName, QStringLiteral("only-user"));
    QVERIFY(stored.password.isEmpty());
}

void TestCredentialStore::clearingBothRemovesTheCredential() {
    CredentialStore::write(target(), QStringLiteral("user"), QStringLiteral("secret"));
    CredentialStore::write(target(), QString(), QString());

    QVERIFY(CredentialStore::read(target()).isEmpty());
}

void TestCredentialStore::oversizedSecretIsNotStored() {
    // CRED_MAX_CREDENTIAL_BLOB_SIZE 是 5*512 = 2560 字节，即 1280 个 UTF-16 单元。
    // 超出的必须在写入前拒绝，而不是让 CredWrite 悄悄失败、界面上却显示"已保存"。
    const QString tooLong(1500, QLatin1Char('x'));

    CredentialStore::write(target(), QStringLiteral("user"), QStringLiteral("original"));
    CredentialStore::write(target(), QStringLiteral("user"), tooLong);

    QCOMPARE(CredentialStore::read(target()).password, QStringLiteral("original"));
}

void TestCredentialStore::cleanupTestCase() {
    CredentialStore::write(target(), QString(), QString());
}

QTEST_GUILESS_MAIN(TestCredentialStore)
#include "tst_credentialstore.moc"
