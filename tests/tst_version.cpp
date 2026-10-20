/**
 * @file tst_version.cpp
 * @brief 守住 Version 把版本串折算成全序整数的契约：短形式等价、预发布排在正式版之前、
 *        认不出的串折算成 0（放行而不是误报），以及当前文档格式串真的能被解析。
 *
 * 打开文档时的“版本过新”提示完全依赖这套折算：相等与更旧必须放行，否则本程序自己存的
 * 文档每次打开都要弹一次警告；而解析失败若返回非 0，读得懂的旧文档反而会被拦下来。
 */

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
    // "1.0" 是 "1.0.0" 的简写：两者必须落在同一个发行版上。
    QCOMPARE(Version(QStringLiteral("1.0")).toUInt(), Version(QStringLiteral("1.0.0")).toUInt());
}

void TestVersion::threePartVersion() {
    QCOMPARE(Version(QStringLiteral("4.8.0")).toUInt(), 408001700U);
    QCOMPARE(Version(QStringLiteral("4.8.1")).toUInt(), 408011700U);
    QCOMPARE(Version(QStringLiteral("4.10.0")).toUInt(), 410001700U);
}

void TestVersion::prereleaseVersion() {
    // 1.0.0.a.3 -> Alpha(10)，1.0.0.b.3 -> Beta(11)，1.0.0.rc.3 -> RC(15)
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
    // 四段和空串都不是写出方会产出的格式；读取方依赖“0 表示没有可用版本”，
    // 而不是在这里崩掉或猜一个版本。
    QCOMPARE(Version(QStringLiteral("1.2.3.4")).toUInt(), 0U);
    QCOMPARE(Version(QString()).toUInt(), 0U);
}

void TestVersion::currentDocumentFormatParses() {
    // Settings::currentFileVersion 会被写进每一份保存的文档，所以真正随包发布的版本串
    // 必须过得了这个解析器。这里写成字面量是为了不让测试去链设置那一栈，
    // 两者要改就一起改。
    const Version current(QStringLiteral("4.8.0"));
    QVERIFY(current.toUInt() > 0);
    QVERIFY(current > Version(QStringLiteral("4.1")));
}

QTEST_MAIN(TestVersion)
#include "tst_version.moc"
