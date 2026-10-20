/**
 * @file tst_stringutils.cpp
 * @brief 守住 StringUtils 里被文档读写与页面命名直接依赖的四个字符串契约：UUID 规范形、
 *        UTC 时间往返、末尾数字递增、按末段数字自然排序。
 */

#include <QtTest>

#include "utils/StringUtils.h"

class TestStringUtils : public QObject
{
    Q_OBJECT

private slots:
    /// @brief toCanonicalUuid 输出去掉花括号的 UUID 文本，默认构造的空 UUID 也要写成合法 nil 形式。
    void canonicalUuidDropsBraces();
    /// @brief 时间写成 UTC 的 ISO 串再解析回来仍是同一个时刻，不受本地时区影响。
    void utcIsoDateTimeRoundTrip();
    /// @brief 名称末尾的数字加一并能进位（9 到 10），完全没有数字时补上第一个序号。
    void netxDigitizedName();
    /// @brief 按末段数字自然排序（page.2 在 page.10 前），不带编号的名字按字母序整体排在前面。
    void sortByLastDigit();
};

void TestStringUtils::canonicalUuidDropsBraces()
{
    // Qt 6 把 QUuid 输出成小写；RFC 4122 的标准形式本来也是小写，所以写入端发出的
    // 是小写文本，读取端比较时就不能区分大小写。
    QUuid uuid("{1B4D2F6E-7A11-4C3B-9E0D-5F8A2C3D4E5F}");
    QCOMPARE(StringUtils::toCanonicalUuid(uuid).toUpper(), QStringLiteral("1B4D2F6E-7A11-4C3B-9E0D-5F8A2C3D4E5F"));
    QVERIFY(!StringUtils::toCanonicalUuid(uuid).startsWith('{'));

    // 场景没有 uuid 时，写入端拿到的是默认构造的 QUuid；
    // 文档里仍必须留下一个语法合法的 nil uuid。
    QCOMPARE(StringUtils::toCanonicalUuid(QUuid()).toUpper(), QStringLiteral("00000000-0000-0000-0000-000000000000"));
}

void TestStringUtils::utcIsoDateTimeRoundTrip()
{
    const QDateTime dt = QDateTime(QDate(2026, 3, 5), QTime(7, 8, 9), Qt::UTC);

    QCOMPARE(StringUtils::toUtcIsoDateTime(dt), QStringLiteral("2026-03-05T07:08:09Z"));

    // fromUtcIsoDate 把文本当 UTC 解释、再换算到本地时间，
    // 所以只能比较时刻，绝不能比较墙钟文本。
    const QDateTime back = StringUtils::fromUtcIsoDate("2026-03-05T07:08:09Z");
    QCOMPARE(back.toMSecsSinceEpoch(), dt.toMSecsSinceEpoch());
}

void TestStringUtils::netxDigitizedName()
{
    QCOMPARE(StringUtils::netxDigitizedName(QStringLiteral("Page 1")), QStringLiteral("Page 2"));
    QCOMPARE(StringUtils::netxDigitizedName(QStringLiteral("Untitled 9")), QStringLiteral("Untitled 10"));

    // 末尾完全没有数字：直接把序号接上去。
    QCOMPARE(StringUtils::netxDigitizedName(QStringLiteral("Notes")), QStringLiteral("Notes 1"));
}

void TestStringUtils::sortByLastDigit()
{
    const QStringList names{
        QStringLiteral("page.10.png"),
        QStringLiteral("page.2.png"),
        QStringLiteral("page.1.png"),
    };

    const QStringList expectedSorted{
        QStringLiteral("page.1.png"),
        QStringLiteral("page.2.png"),
        QStringLiteral("page.10.png"),
    };
    QCOMPARE(StringUtils::sortByLastDigit(names), expectedSorted);

    // 不符合“非数字+数字+点”形状的串互相按字母序排，且都排在带编号的之前。
    const QStringList mixed{
        QStringLiteral("page.3.png"),
        QStringLiteral("zebra"),
        QStringLiteral("alpha"),
    };

    const QStringList expectedMixed{
        QStringLiteral("alpha"),
        QStringLiteral("zebra"),
        QStringLiteral("page.3.png"),
    };
    QCOMPARE(StringUtils::sortByLastDigit(mixed), expectedMixed);
}

QTEST_MAIN(TestStringUtils)
#include "tst_stringutils.moc"
