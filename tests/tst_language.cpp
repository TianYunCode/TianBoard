// 校验中文语言包真的能编译、能被装上、并且按预期改变取词结果。
// 这一步是语言切换功能的地基：上下文名（MainWindow 等）一旦和代码里的 tr()
// 归属对不上，切换后界面不会有任何变化，而这个错误只有加载 .qm 才暴露得出来。
#include <QtTest>

#include <QCoreApplication>
#include <QFileInfo>
#include <QTranslator>

class TestLanguage : public QObject
{
    Q_OBJECT

private slots:
    void chineseCatalogueTranslatesMainWindowTexts();
    void removingTranslatorFallsBackToSourceText();
};

static QString cataloguePath()
{
    return qEnvironmentVariable("TIANBOARD_ZH_QM");
}

void TestLanguage::chineseCatalogueTranslatesMainWindowTexts()
{
    const QString path = cataloguePath();
    QVERIFY2(!path.isEmpty(),
             "set TIANBOARD_ZH_QM to the built TianBoard_zh_CN.qm (ctest does it through the test's ENVIRONMENT property)");
    QVERIFY2(QFileInfo::exists(path), qPrintable(QStringLiteral("missing catalogue: %1").arg(path)));

    QTranslator translator;
    QVERIFY2(translator.load(path), qPrintable(QStringLiteral("lrelease output failed to load: %1").arg(path)));
    QVERIFY(QCoreApplication::instance()->installTranslator(&translator));

    QCOMPARE(QCoreApplication::translate("MainWindow", "Preferences"), QString::fromUtf8("设置"));
    QCOMPARE(QCoreApplication::translate("MainWindow", "Documents"), QString::fromUtf8("文档"));
    QCOMPARE(QCoreApplication::translate("MainWindow", "Undo"), QString::fromUtf8("撤销"));

    QCoreApplication::removeTranslator(&translator);
}

void TestLanguage::removingTranslatorFallsBackToSourceText()
{
    // 摘掉翻译器之后必须回到英文原文，这正是选择 "en" 时的行为。
    QCOMPARE(QCoreApplication::translate("MainWindow", "Preferences"), QString::fromLatin1("Preferences"));
}

QTEST_GUILESS_MAIN(TestLanguage)
#include "tst_language.moc"
