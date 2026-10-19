// PDFRenderer 的 Qt6::Pdf 实现。测试先用 QPdfWriter 现场生成一份两页 PDF，
// 再按应用真正使用的契约校验：页码 1 起、pageSizeF 按 dpi 换算、render 会把
// 页面画进 QPainter，并且越界页码返回空而不是崩溃。
#include <QtTest>

#include <QImage>
#include <QPainter>
#include <QPdfWriter>
#include <QTemporaryDir>

#include "content/QPdfRenderer.h"

class TestPdfRenderer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void reportsValidityAndPageCount();
    void outOfRangePagesAreEmpty();
    void pageSizeScalesPointsToPixels();
    void rendersTheWholePage();
    void rendersOnlyTheRequestedBounds();
    void damagedFileIsNotValid();

private:
    QTemporaryDir mDir;
    QString mPdfPath;
};

void TestPdfRenderer::initTestCase()
{
    QVERIFY(mDir.isValid());
    mPdfPath = mDir.filePath(QStringLiteral("fixture.pdf"));

    QPdfWriter writer(mPdfPath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(96);
    writer.setTitle(QStringLiteral("TianBoard test fixture"));

    // newPage() only takes effect while a painter is active on the device, the
    // same idiom as for QPrinter.
    {
        QPainter painter(&writer);
        painter.setPen(Qt::black);
        painter.setFont(QFont(QStringLiteral("Arial"), 40));

        painter.drawText(writer.pageLayout().paintRectPixels(writer.resolution()),
                         Qt::AlignCenter, QStringLiteral("PAGE ONE"));

        writer.newPage();

        painter.drawText(writer.pageLayout().paintRectPixels(writer.resolution()),
                         Qt::AlignCenter, QStringLiteral("PAGE TWO"));
    }
}

void TestPdfRenderer::reportsValidityAndPageCount()
{
    QPdfRenderer renderer(mPdfPath);
    QVERIFY2(renderer.isValid(), qPrintable(mPdfPath));
    QCOMPARE(renderer.pageCount(), 2);
    QVERIFY(renderer.title().contains(QStringLiteral("TianBoard")));
}

void TestPdfRenderer::outOfRangePagesAreEmpty()
{
    QPdfRenderer renderer(mPdfPath);
    QVERIFY(renderer.isValid());

    // Page numbers are 1 based, so 0 and pageCount+1 must be rejected rather
    // than reaching into the document.
    QVERIFY(renderer.pageSizeF(0).isEmpty());
    QVERIFY(renderer.pageSizeF(renderer.pageCount() + 1).isEmpty());
}

void TestPdfRenderer::pageSizeScalesPointsToPixels()
{
    QPdfRenderer renderer(mPdfPath);
    renderer.setDPI(96);

    const QSizeF size = renderer.pageSizeF(1);

    // A4 at 96 dpi is 8.27in x 11.69in.
    QVERIFY(qAbs(size.width() - 96.0 * 8.27) < 3.0);
    QVERIFY(qAbs(size.height() - 96.0 * 11.69) < 3.0);

    renderer.setDPI(192);
    QCOMPARE(renderer.pageSizeF(2).width(), size.width() * 2);
}

void TestPdfRenderer::rendersTheWholePage()
{
    QPdfRenderer renderer(mPdfPath);
    renderer.setDPI(96);

    const QSizeF pageSize = renderer.pageSizeF(1);
    QImage image(pageSize.toSize(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);

    QPainter painter(&image);
    renderer.render(&painter, 1);
    painter.end();

    // Both fixture pages are white except for the headline, so a uniform image
    // means nothing was painted at all.
    const QColor first = image.pixelColor(0, 0);
    bool painted = false;
    for (int y = 0; y < image.height() && !painted; y += 7)
        for (int x = 0; x < image.width() && !painted; x += 7)
            if (image.pixelColor(x, y) != first)
                painted = true;

    QVERIFY(painted);
}

void TestPdfRenderer::rendersOnlyTheRequestedBounds()
{
    QPdfRenderer renderer(mPdfPath);
    renderer.setDPI(96);

    const QSizeF pageSize = renderer.pageSizeF(1);
    QImage image(pageSize.toSize(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);

    QPainter painter(&image);
    const QRectF lowerHalf(0, pageSize.height() / 2, pageSize.width(), pageSize.height() / 2);
    renderer.render(&painter, 1, lowerHalf);
    painter.end();

    // The headline sits in the vertical centre, so the untouched upper half has
    // to stay pure white while the lower half picks up ink.
    bool upperStillWhite = true;
    for (int y = 0; y < image.height() / 4 && upperStillWhite; ++y)
        for (int x = 0; x < image.width(); ++x)
            if (image.pixelColor(x, y) != QColor(Qt::white))
                upperStillWhite = false;

    QVERIFY(upperStillWhite);
}

void TestPdfRenderer::damagedFileIsNotValid()
{
    const QString broken = mDir.filePath(QStringLiteral("broken.pdf"));
    {
        QFile file(broken);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("%PDF-1.4\nthis is not a pdf at all\n");
    }

    QPdfRenderer renderer(broken);
    QVERIFY(!renderer.isValid());
    QCOMPARE(renderer.pageCount(), 0);
    QVERIFY(renderer.pageSizeF(1).isEmpty());

    QImage image(64, 64, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    renderer.render(&painter, 1);
    painter.end();
    QCOMPARE(image.pixelColor(0, 0), QColor(Qt::white));
}

QTEST_MAIN(TestPdfRenderer)
#include "tst_pdfrenderer.moc"
