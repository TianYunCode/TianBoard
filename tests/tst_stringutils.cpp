#include <QtTest>

#include "utils/StringUtils.h"

class TestStringUtils : public QObject
{
    Q_OBJECT

private slots:
    void canonicalUuidDropsBraces();
    void utcIsoDateTimeRoundTrip();
    void netxDigitizedName();
    void sortByLastDigit();
};

void TestStringUtils::canonicalUuidDropsBraces()
{
    // Qt 6 renders QUuid lowercase; the RFC 4122 canonical form is lowercase
    // anyway, so the writer emits lowercase and readers must not compare the
    // text case-sensitively.
    QUuid uuid("{1B4D2F6E-7A11-4C3B-9E0D-5F8A2C3D4E5F}");
    QCOMPARE(StringUtils::toCanonicalUuid(uuid).toUpper(),
             QStringLiteral("1B4D2F6E-7A11-4C3B-9E0D-5F8A2C3D4E5F"));
    QVERIFY(!StringUtils::toCanonicalUuid(uuid).startsWith('{'));

    // The writer is fed a default-constructed uuid when a scene has none; the
    // document must still carry a syntactically valid nil uuid.
    QCOMPARE(StringUtils::toCanonicalUuid(QUuid()).toUpper(),
             QStringLiteral("00000000-0000-0000-0000-000000000000"));
}

void TestStringUtils::utcIsoDateTimeRoundTrip()
{
    const QDateTime dt = QDateTime(QDate(2026, 3, 5), QTime(7, 8, 9), Qt::UTC);

    QCOMPARE(StringUtils::toUtcIsoDateTime(dt), QStringLiteral("2026-03-05T07:08:09Z"));

    // fromUtcIsoDate interprets the text as UTC and shifts it to local time, so
    // only the instant may be compared, never the wall clock text.
    const QDateTime back = StringUtils::fromUtcIsoDate("2026-03-05T07:08:09Z");
    QCOMPARE(back.toMSecsSinceEpoch(), dt.toMSecsSinceEpoch());
}

void TestStringUtils::netxDigitizedName()
{
    QCOMPARE(StringUtils::netxDigitizedName(QStringLiteral("Page 1")),
             QStringLiteral("Page 2"));
    QCOMPARE(StringUtils::netxDigitizedName(QStringLiteral("Untitled 9")),
             QStringLiteral("Untitled 10"));

    // No trailing number at all: the first index is appended.
    QCOMPARE(StringUtils::netxDigitizedName(QStringLiteral("Notes")),
             QStringLiteral("Notes 1"));
}

void TestStringUtils::sortByLastDigit()
{
    const QStringList names {
        QStringLiteral("page.10.png"),
        QStringLiteral("page.2.png"),
        QStringLiteral("page.1.png"),
    };

    const QStringList expectedSorted {
        QStringLiteral("page.1.png"),
        QStringLiteral("page.2.png"),
        QStringLiteral("page.10.png"),
    };
    QCOMPARE(StringUtils::sortByLastDigit(names), expectedSorted);

    // Names without the "<non-digit><digits><dot>" shape all sort together,
    // alphabetically, ahead of the numbered ones.
    const QStringList mixed {
        QStringLiteral("page.3.png"),
        QStringLiteral("zebra"),
        QStringLiteral("alpha"),
    };

    const QStringList expectedMixed {
        QStringLiteral("alpha"),
        QStringLiteral("zebra"),
        QStringLiteral("page.3.png"),
    };
    QCOMPARE(StringUtils::sortByLastDigit(mixed), expectedMixed);
}

QTEST_MAIN(TestStringUtils)
#include "tst_stringutils.moc"
