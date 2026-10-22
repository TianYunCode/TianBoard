/**
 * @file tst_packagelayout.cpp
 * @brief 钉住 .tbz 文档包的磁盘布局常量：目录名与 folders.xml 的词表。
 *
 * 存在的理由不是"测一个赋值"，而是补上往返自检看不见的那条缝：`tst_format` 存与读用的是
 * 同一份常量，所以把 `images` 改成 `image` 对它完全自洽、一次都不会红，但用户盘上已有的
 * 文档当场就读不出图片了。只有把字面量钉在这里，改常量才会变成一个会红的用例。
 */

#include <QSet>
#include <QString>
#include <QStringList>
#include <QtTest>

#include "model/PackageLayout.hpp"

class TestPackageLayout : public QObject
{
    Q_OBJECT

private slots:
    /// 包内子目录名：这些就是磁盘上的目录，取值一动，旧文档的对象就找不到归属。
    void documentSubDirectories();
    /// 仓库顶层分组名与 folders.xml 的文件名/标签/属性名。
    void catalogVocabulary();
    /// 子目录集合不许漏：新增图形项类型时最容易只加常量、忘了让它进 mAllDirectories。
    void subDirectoryValuesAreDistinct();
};

void TestPackageLayout::documentSubDirectories()
{
    QCOMPARE(PackageLayout::imageDirectory, QStringLiteral("images"));
    QCOMPARE(PackageLayout::objectDirectory, QStringLiteral("objects"));
    QCOMPARE(PackageLayout::widgetDirectory, QStringLiteral("widgets"));
    QCOMPARE(PackageLayout::videoDirectory, QStringLiteral("videos"));
    QCOMPARE(PackageLayout::audioDirectory, QStringLiteral("audios"));
    QCOMPARE(PackageLayout::fileDirectory, QStringLiteral("files"));
}

void TestPackageLayout::catalogVocabulary()
{
    QCOMPARE(PackageLayout::myDocumentsName, QStringLiteral("MyDocuments"));
    QCOMPARE(PackageLayout::modelsName, QStringLiteral("Models"));
    QCOMPARE(PackageLayout::foldersFile, QStringLiteral("folders.xml"));
    QCOMPARE(PackageLayout::folderTag, QStringLiteral("folder"));
    QCOMPARE(PackageLayout::nameAttribute, QStringLiteral("name"));
}

void TestPackageLayout::subDirectoryValuesAreDistinct()
{
    const QStringList directories = {
        PackageLayout::imageDirectory,  PackageLayout::objectDirectory, PackageLayout::widgetDirectory,
        PackageLayout::videoDirectory,  PackageLayout::audioDirectory,  PackageLayout::fileDirectory,
    };

    // 两个类型共用一个子目录会让对象互相覆盖，且 metadata 里的相对路径无法还原归属。
    QCOMPARE(QSet<QString>(directories.begin(), directories.end()).size(), directories.size());

    // 与 PersistenceManager::mAllDirectories 的构造顺序一致：这条只是防止有人改名时把
    // 大小写写歪（磁盘名区分不开，但代码里读起来会找不着）。
    for (const QString &name : directories)
    {
        QVERIFY(!name.isEmpty());
        QCOMPARE(name, name.toLower());
        QVERIFY(!name.contains('/') && !name.contains('\\'));
    }
}

QTEST_MAIN(TestPackageLayout)
#include "tst_packagelayout.moc"
