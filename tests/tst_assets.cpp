// 资源完整性测试：qrc 里登记的每个文件都必须真的在磁盘上，res/images 下的每个
// 文件都必须能被解码，品牌图形必须真的画得出内容。之前出现过 mask.png /
// toolbar/internet.png 这类"代码或 qrc 引用了不存在的图"的问题，这里一次性守住。
#include <QtTest>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QRegularExpression>
#include <QSvgRenderer>
#include <QTextStream>

namespace
{
QStringList gCapturedWarnings;

void captureWarning(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    if (type == QtWarningMsg)
        gCapturedWarnings << msg;
}
}

class TestAssets : public QObject
{
    Q_OBJECT

private slots:
    void qrcEntriesExistOnDisk();
    void everyImageDecodes();
    void brandAssetsAreOwnAndRegistered();
    void appIconRendersAsDesigned();
    void blackoutMarkIsWhiteOnTransparent();
    void svgAssetsPaintWithoutTruncation();

private:
    static QString resPath(const QString &relative);
    static QStringList qrcFileEntries();
    static QImage renderSvg(const QString &relative, const QSize &size);
};

QString TestAssets::resPath(const QString &relative)
{
    return QStringLiteral(TIANBOARD_RES_DIR) + QLatin1Char('/') + relative;
}

QStringList TestAssets::qrcFileEntries()
{
    QFile qrc(resPath(QStringLiteral("TianBoard.qrc")));
    if (!qrc.open(QIODevice::ReadOnly | QIODevice::Text))
        return QStringList();

    QTextStream in(&qrc);
    QStringList entries;
    static const QRegularExpression fileTag(QStringLiteral("<file[^>]*>([^<]+)</file>"));

    while (!in.atEnd()) {
        const QRegularExpressionMatch match = fileTag.match(in.readLine());
        if (match.hasMatch())
            entries << match.captured(1).trimmed();
    }

    return entries;
}

QImage TestAssets::renderSvg(const QString &relative, const QSize &size)
{
    QSvgRenderer renderer(resPath(relative));
    if (!renderer.isValid())
        return QImage();

    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    renderer.render(&painter);
    painter.end();

    return image;
}

void TestAssets::qrcEntriesExistOnDisk()
{
    const QStringList entries = qrcFileEntries();
    QVERIFY2(!entries.isEmpty(), "could not read res/TianBoard.qrc");

    QStringList missing;
    for (const QString &entry : entries) {
        if (!QFile::exists(resPath(entry)))
            missing << entry;
    }

    QVERIFY2(missing.isEmpty(),
             qPrintable(QStringLiteral("qrc references %1 missing file(s): %2")
                            .arg(missing.size()).arg(missing.join(QStringLiteral(", ")))));
}

void TestAssets::everyImageDecodes()
{
    QDir images(resPath(QStringLiteral("images")));
    QVERIFY(images.exists());

    const QStringList files = images.entryList(QDir::Files, QDir::Name);
    QVERIFY(files.size() > 20);

    QStringList broken;
    for (const QString &name : files) {
        const QString path = images.filePath(name);

        if (name.endsWith(QStringLiteral(".svg"), Qt::CaseInsensitive)) {
            if (!QSvgRenderer(path).isValid())
                broken << name;
        } else {
            if (QImage(path).isNull())
                broken << name;
        }
    }

    QVERIFY2(broken.isEmpty(),
             qPrintable(QStringLiteral("undecodable image(s): %1").arg(broken.join(QStringLiteral(", ")))));
}

void TestAssets::brandAssetsAreOwnAndRegistered()
{
    QVERIFY(QFile::exists(resPath(QStringLiteral("images/TianBoardIcon.svg"))));
    QVERIFY(QFile::exists(resPath(QStringLiteral("images/TianBoardMark.svg"))));
    QVERIFY(!QFile::exists(resPath(QStringLiteral("images/TianBoard.png"))));

    const QStringList entries = qrcFileEntries();
    QVERIFY(entries.contains(QStringLiteral("images/TianBoardIcon.svg")));
    QVERIFY(entries.contains(QStringLiteral("images/TianBoardMark.svg")));
}

void TestAssets::appIconRendersAsDesigned()
{
    const QImage icon = renderSvg(QStringLiteral("images/TianBoardIcon.svg"), QSize(512, 512));
    QVERIFY2(!icon.isNull(), "TianBoardIcon.svg did not render");
    QCOMPARE(icon.size(), QSize(512, 512));

    // Outside the rounded tile there must be nothing.
    QCOMPARE(icon.pixelColor(8, 8).alpha(), 0);

    // Inside the tile, away from the board and the strokes: the blue gradient.
    const QColor tile = icon.pixelColor(120, 400);
    QVERIFY2(tile.alpha() > 200, "the tile is not opaque where it should be solid");
    QVERIFY2(tile.blue() > tile.red(), qPrintable(QStringLiteral("tile colour %1 is not blue-ish").arg(tile.name())));

    // The horizontal arm of the T is white.
    const QColor stroke = icon.pixelColor(250, 189);
    QVERIFY2(stroke.red() > 230 && stroke.green() > 230 && stroke.blue() > 230,
             qPrintable(QStringLiteral("expected a white marker stroke, got %1").arg(stroke.name())));
}

void TestAssets::blackoutMarkIsWhiteOnTransparent()
{
    const QImage mark = renderSvg(QStringLiteral("images/TianBoardMark.svg"), QSize(512, 512));
    QVERIFY2(!mark.isNull(), "TianBoardMark.svg did not render");

    QCOMPARE(mark.pixelColor(4, 4).alpha(), 0);
    QCOMPARE(mark.pixelColor(507, 507).alpha(), 0);

    const QColor stroke = mark.pixelColor(250, 193);
    QVERIFY2(stroke.red() > 230 && stroke.green() > 230 && stroke.blue() > 230 && stroke.alpha() > 230,
             qPrintable(QStringLiteral("expected a white stroke on the mark, got %1").arg(stroke.name())));
}

// QtSvg 对单条 path 的坐标量有硬上限，超限只打印一句 “Invalid path data; path
// truncated.” 然后画出一半的图形。世界地图以前就是这种情况，所以这里把随包发布的
// 每个 SVG 都真渲染一遍：既不能报解析告警，也必须真的画出像素。
void TestAssets::svgAssetsPaintWithoutTruncation()
{
    const QStringList roots = { resPath(QStringLiteral("library")), resPath(QStringLiteral("images")) };
    QStringList problems;
    int checked = 0;

    for (const QString &rootPath : roots) {
        QDir root(rootPath);
        QVERIFY2(root.exists(), qPrintable(QStringLiteral("missing asset root %1").arg(rootPath)));

        QDirIterator it(root.path(), { QStringLiteral("*.svg") }, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString file = it.next();
            ++checked;

            gCapturedWarnings.clear();
            QtMessageHandler previous = qInstallMessageHandler(captureWarning);

            QString problem;
            {
                // 告警出现在 QSvgRenderer 构造期的解析阶段，必须在构造前后接管 handler。
                QSvgRenderer renderer(file);
                if (!renderer.isValid()) {
                    problem = QStringLiteral("%1: not a valid SVG").arg(file);
                } else {
                    QSize size = renderer.defaultSize();
                    if (size.isEmpty())
                        size = QSize(128, 128);
                    if (size.width() > 256 || size.height() > 256)
                        size.scale(256, 256, Qt::KeepAspectRatio);

                    QImage image(size, QImage::Format_ARGB32_Premultiplied);
                    image.fill(Qt::transparent);
                    QPainter painter(&image);
                    renderer.render(&painter);
                    painter.end();

                    bool painted = false;
                    for (int y = 0; y < image.height() && !painted; ++y) {
                        const uchar *line = image.constScanLine(y);
                        for (int x = 0; x < image.width(); ++x) {
                            if (line[4 * x + 3] != 0) {
                                painted = true;
                                break;
                            }
                        }
                    }
                    if (!painted)
                        problem = QStringLiteral("%1: renders nothing").arg(file);
                }
            }

            qInstallMessageHandler(previous);

            if (!gCapturedWarnings.isEmpty())
                problem = QStringLiteral("%1: %2").arg(file, gCapturedWarnings.join(QStringLiteral(" | ")));

            if (!problem.isEmpty())
                problems << problem;
        }
    }

    QVERIFY2(checked > 50, "suspiciously few SVG assets were checked");
    QVERIFY2(problems.isEmpty(),
             qPrintable(QStringLiteral("SVG problem(s):\n%1").arg(problems.join(QStringLiteral("\n")))));
}

QTEST_MAIN(TestAssets)
#include "tst_assets.moc"
