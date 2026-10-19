#include <QtTest>

#include "utils/Version.h"

class TestVersion : public QObject {
    Q_OBJECT

private slots:
    void twoPartVersion();
    void threePartVersion();
    void prereleaseVersion();
    void ordering();
    void documentNewerThanWriterOnly();
    void unknownFormatYieldsZero();
    void currentDocumentFormatParses();
};

void TestVersion::twoPartVersion() {
    // "1.0" is a short form of "1.0.0": both must mean the same release.
    QCOMPARE(Version(QStringLiteral("1.0")).toUInt(), Version(QStringLiteral("1.0.0")).toUInt());
}

void TestVersion::threePartVersion() {
    QCOMPARE(Version(QStringLiteral("4.8.0")).toUInt(), 408001700U);
    QCOMPARE(Version(QStringLiteral("4.8.1")).toUInt(), 408011700U);
    QCOMPARE(Version(QStringLiteral("4.10.0")).toUInt(), 410001700U);
}

void TestVersion::prereleaseVersion() {
    // 1.0.0.a.3 -> Alpha(10), 1.0.0.b.3 -> Beta(11), 1.0.0.rc.3 -> RC(15)
    const Version alpha(QStringLiteral("1.0.0.a.3"));
    const Version beta(QStringLiteral("1.0.0.b.3"));
    const Version candidate(QStringLiteral("1.0.0.rc.3"));
    const Version final(QStringLiteral("1.0.0"));

    QVERIFY(alpha < beta);
    QVERIFY(beta < candidate);
    QVERIFY(candidate < final);
}

void TestVersion::ordering() {
    QVERIFY(Version(QStringLiteral("4.7.0")) < Version(QStringLiteral("4.8.0")));
    QVERIFY(Version(QStringLiteral("4.8.0")) > Version(QStringLiteral("4.7.9")));
    QVERIFY(Version(QStringLiteral("5.0")) > Version(QStringLiteral("4.99.99")));
    QVERIFY(Version(QStringLiteral("4.8.0")) == Version(QStringLiteral("4.8.0")));
}

void TestVersion::documentNewerThanWriterOnly() {
    // DocumentController::isOKToOpenDocument() 只该挡“比当前读写器还新”的文档：
    // 相等和更旧都必须放行，否则本程序自己存的文档每次打开都要弹一次丢失警告。
    const Version current(QStringLiteral("4.8.0"));

    QVERIFY(Version(QStringLiteral("4.8.0")) <= current);
    QVERIFY(Version(QStringLiteral("4.1")) <= current);
    QVERIFY(!(Version(QStringLiteral("5.0")) <= current));

    // 认不出来的串折算成 0，也就一律放行，而不是对着能读的文档误报。
    QVERIFY(Version(QStringLiteral("2.3.4.5")) <= current);
    QVERIFY(Version(QString()) <= current);
}

void TestVersion::unknownFormatYieldsZero() {
    // Four groups and empty strings are not a format the writer ever emits; the
    // reader relies on 0 meaning "no usable version" rather than crashing.
    QCOMPARE(Version(QStringLiteral("1.2.3.4")).toUInt(), 0U);
    QCOMPARE(Version(QString()).toUInt(), 0U);
}

void TestVersion::currentDocumentFormatParses() {
    // Settings::currentFileVersion is stamped into every saved document, so the
    // version actually shipped must survive this parser. Kept as a literal so the
    // test does not have to link the settings stack; update both together.
    const Version current(QStringLiteral("4.8.0"));
    QVERIFY(current.toUInt() > 0);
    QVERIFY(current > Version(QStringLiteral("4.1")));
}

QTEST_MAIN(TestVersion)
#include "tst_version.moc"
