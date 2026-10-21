/**
 * @file tst_cookierules.cpp
 * @brief Cookie 隐私纯规则的单元测试：策略名映射与保留域匹配。
 *
 * 这两条规则决定“哪些 Cookie 会被收下、退出时哪些站保留登录态”，写错不会崩也不会报错，
 * 只会让用户以为设好了隐私开关。因此把它们从 QtWebEngine 里抽成纯函数单独测。
 */

#include <QtTest>

#include "utils/CookieRules.h"

using CookieRules::Policy;

class TestCookieRules : public QObject
{
    Q_OBJECT

private slots:
    /// @brief 配置名与枚举互逆；认不出的写法回落到默认策略而不是关掉过滤。
    void policyNameRoundTrip();
    /// @brief 整串相等与后缀匹配两条规则，加上顶级域、大小写与空白的处理。
    void keepDomainMatching();
    /// @brief 设置框里那一串域的拆分：逗号分号都行，空白与空条目丢掉。
    void domainListParsing();
};

void TestCookieRules::policyNameRoundTrip()
{
    QCOMPARE(CookieRules::policyFromName(QStringLiteral("DenyAll")), Policy::DenyAll);
    QCOMPARE(CookieRules::policyFromName(QStringLiteral("DenyThirdParty")), Policy::DenyThirdParty);
    QCOMPARE(CookieRules::policyFromName(QStringLiteral("AcceptAll")), Policy::AcceptAll);

    // 认不出来必须回落，不能因为拼错就变成“全收”。
    QCOMPARE(CookieRules::policyFromName(QStringLiteral("denyall")), Policy::DenyThirdParty);
    QCOMPARE(CookieRules::policyFromName(QString()), Policy::DenyThirdParty);
    QCOMPARE(CookieRules::policyFromName(QStringLiteral("whatever"), Policy::AcceptAll), Policy::AcceptAll);

    const Policy all[] = {Policy::DenyAll, Policy::DenyThirdParty, Policy::AcceptAll};

    for (Policy policy : all)
    {
        QCOMPARE(CookieRules::policyFromName(CookieRules::policyName(policy)), policy);
    }
}

void TestCookieRules::keepDomainMatching()
{
    const QStringList empty;
    QVERIFY(!CookieRules::isKeptDomain(QStringLiteral("example.com"), empty));
    QVERIFY(!CookieRules::isKeptDomain(QString(), {QStringLiteral("example.com")}));

    // 不带前置点：整串相等，子域与半截都不算命中。
    const QStringList exact = {QStringLiteral("example.com")};
    QVERIFY(CookieRules::isKeptDomain(QStringLiteral("example.com"), exact));
    QVERIFY(!CookieRules::isKeptDomain(QStringLiteral("www.example.com"), exact));
    QVERIFY(!CookieRules::isKeptDomain(QStringLiteral("notexample.com"), exact));
    QVERIFY(!CookieRules::isKeptDomain(QStringLiteral("ample.com"), exact));

    // 带前置点：按后缀匹配，且顶级域本身也要命中（浏览器写进去的域名里没有那个点）。
    const QStringList suffix = {QStringLiteral(".example.com")};
    QVERIFY(CookieRules::isKeptDomain(QStringLiteral("example.com"), suffix));
    QVERIFY(CookieRules::isKeptDomain(QStringLiteral("www.example.com"), suffix));
    QVERIFY(CookieRules::isKeptDomain(QStringLiteral("a.b.example.com"), suffix));
    QVERIFY(!CookieRules::isKeptDomain(QStringLiteral("goodexample.com"), suffix));
    QVERIFY(!CookieRules::isKeptDomain(QStringLiteral("example.org"), suffix));

    // 大小写与首尾空白不该影响判断：cookie 域与白名单两边都可能带。
    QVERIFY(CookieRules::isKeptDomain(QStringLiteral("WWW.Example.COM"), {QStringLiteral(".example.com")}));
    QVERIFY(CookieRules::isKeptDomain(QStringLiteral("example.com"), {QStringLiteral("  Example.COM  ")}));

    // 多条白名单：任一条命中即保留，中间夹空条目不能把判断带偏。
    const QStringList multi = {QString(), QStringLiteral("other.org"), QStringLiteral(".example.com")};
    QVERIFY(CookieRules::isKeptDomain(QStringLiteral("cdn.example.com"), multi));
    QVERIFY(CookieRules::isKeptDomain(QStringLiteral("other.org"), multi));
    QVERIFY(!CookieRules::isKeptDomain(QStringLiteral("third.org"), multi));
}

void TestCookieRules::domainListParsing()
{
    QStringList parsed   = CookieRules::parseDomainList(QStringLiteral("a.example.com, .b.example.com ; c.example.org ,, "));
    QStringList expected = {QStringLiteral("a.example.com"), QStringLiteral(".b.example.com"), QStringLiteral("c.example.org")};
    QCOMPARE(parsed, expected);

    QVERIFY(CookieRules::parseDomainList(QString()).isEmpty());
    QVERIFY(CookieRules::parseDomainList(QStringLiteral("  ,  ;  ")).isEmpty());

    // 拆出来的结果要能直接喂给匹配：这是设置页与清理逻辑之间唯一的约定。
    QVERIFY(CookieRules::isKeptDomain(QStringLiteral("x.b.example.com"), parsed));
}

QTEST_GUILESS_MAIN(TestCookieRules)
#include "tst_cookierules.moc"
