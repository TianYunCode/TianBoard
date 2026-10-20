/**
 * @file tst_language.cpp
 * @brief 守住中文语言包真的能编译、能被装上、并且按预期改变取词结果；摘掉后回到英文原文。
 *
 * 这一步是语言切换功能的地基：上下文名（MainWindow 等）一旦和代码里的 tr() 归属对不上，
 * 切换后界面不会有任何变化，而这个错误只有加载 .qm 才暴露得出来。
 */

#include <QtTest>

#include <QCoreApplication>
#include <QFileInfo>
#include <QTranslator>

class TestLanguage : public QObject
{
    Q_OBJECT

private slots:
    /// @brief 装上 .qm 后 MainWindow 与 PreferencesDialog 两个上下文都取到中文，带参文案仍留 %1。
    void chineseCatalogueTranslatesMainWindowTexts();
    /// @brief 摘掉翻译器后同一个 key 回落成英文原文，与界面上选择 "en" 时的行为一致。
    void removingTranslatorFallsBackToSourceText();
};

static QString cataloguePath()
{
    return qEnvironmentVariable("TIANBOARD_ZH_QM");
}

void TestLanguage::chineseCatalogueTranslatesMainWindowTexts()
{
    const QString path = cataloguePath();
    QVERIFY2(!path.isEmpty(), "set TIANBOARD_ZH_QM to the built TianBoard_zh_CN.qm (ctest does it through "
                              "the test's ENVIRONMENT property)");
    QVERIFY2(QFileInfo::exists(path), qPrintable(QStringLiteral("missing catalogue: %1").arg(path)));

    QTranslator translator;
    QVERIFY2(translator.load(path), qPrintable(QStringLiteral("lrelease output failed to load: %1").arg(path)));
    QVERIFY(QCoreApplication::instance()->installTranslator(&translator));

    QCOMPARE(QCoreApplication::translate("MainWindow", "Preferences"), QString::fromUtf8("设置"));
    QCOMPARE(QCoreApplication::translate("MainWindow", "Documents"), QString::fromUtf8("文档"));
    QCOMPARE(QCoreApplication::translate("MainWindow", "Undo"), QString::fromUtf8("撤销"));

    // 版本号与关于页文案在手写表单的 PreferencesDialog 上下文里：上下文名一错，
    // 切语言后这两处不会变；译文里少了 %1，arg() 就没地方填，界面只剩占位符。
    QCOMPARE(QCoreApplication::translate("PreferencesDialog", "version: %1"), QString::fromUtf8("版本：%1"));
    QVERIFY(QCoreApplication::translate("PreferencesDialog", "TianBoard is copyright © %1. All rights reserved.").contains(QLatin1String("%1")));

    QCoreApplication::removeTranslator(&translator);
}

void TestLanguage::removingTranslatorFallsBackToSourceText()
{
    // 摘掉翻译器之后必须回到英文原文，这正是选择 "en" 时的行为。
    QCOMPARE(QCoreApplication::translate("MainWindow", "Preferences"), QString::fromLatin1("Preferences"));
}

QTEST_GUILESS_MAIN(TestLanguage)
#include "tst_language.moc"
