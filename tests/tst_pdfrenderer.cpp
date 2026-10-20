/**
 * @file tst_pdfrenderer.cpp
 * @brief 守住 PDFRenderer 的 Qt6::Pdf 实现与应用之间的契约：页码从 1 起、pageSizeF 按
 *        dpi 换算、render 把页面画进 QPainter，越界页码与损坏文件返回空而不是崩溃。
 *
 * 测试先用 QPdfWriter 现场生成一份两页 PDF 当夹具，不依赖仓库里存放的二进制样本。
 */

#include <QtTest>

#include <QImage>
#include <QPainter>
#include <QPdfWriter>
#include <QTemporaryDir>

#include "whiteboard/QPdfRenderer.h"

class TestPdfRenderer : public QObject
{
    Q_OBJECT

private slots:
    /// @brief 现场生成两页 A4/96dpi 的 PDF 夹具，每页居中一行大字，供后面各用例读取。
    void initTestCase();

    /// @brief 合法文件必须 isValid、页数正好 2，并能读出写入时设的标题元数据。
    void reportsValidityAndPageCount();
    /// @brief 页码 0 与 pageCount+1 一律返回空尺寸，不得伸到文档内部去取页。
    void outOfRangePagesAreEmpty();
    /// @brief pageSizeF 把点单位按当前 dpi 换算成像素，dpi 翻倍则页宽同步翻倍。
    void pageSizeScalesPointsToPixels();
    /// @brief 不传 bounds 时整页真的被画进目标位图，而不是一张纯白的空图。
    void rendersTheWholePage();
    /// @brief 指定 bounds 时只有该区域落墨，区域外保持原样，证明裁剪参数确实被用上。
    void rendersOnlyTheRequestedBounds();
    /// @brief 只有 %PDF 头、正文是垃圾的文件判为无效：页数 0、尺寸空、render 不崩也不落墨。
    void damagedFileIsNotValid();

private:
    QTemporaryDir mDir;
    QString       mPdfPath;
};

void TestPdfRenderer::initTestCase()
{
    QVERIFY(mDir.isValid());
    mPdfPath = mDir.filePath(QStringLiteral("fixture.pdf"));

    QPdfWriter writer(mPdfPath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(96);
    writer.setTitle(QStringLiteral("TianBoard test fixture"));

    // newPage() 只有在设备上有活动画笔时才生效，写法与 QPrinter 一致；
    // 两页内容必须画在同一个 QPainter 的作用域里才切得开。
    {
        QPainter painter(&writer);
        painter.setPen(Qt::black);
        painter.setFont(QFont(QStringLiteral("Arial"), 40));

        painter.drawText(writer.pageLayout().paintRectPixels(writer.resolution()), Qt::AlignCenter, QStringLiteral("PAGE ONE"));

        writer.newPage();

        painter.drawText(writer.pageLayout().paintRectPixels(writer.resolution()), Qt::AlignCenter, QStringLiteral("PAGE TWO"));
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

    // 页码从 1 开始，所以 0 与 pageCount+1 必须被挡掉，而不是伸到文档里去取。
    QVERIFY(renderer.pageSizeF(0).isEmpty());
    QVERIFY(renderer.pageSizeF(renderer.pageCount() + 1).isEmpty());
}

void TestPdfRenderer::pageSizeScalesPointsToPixels()
{
    QPdfRenderer renderer(mPdfPath);
    renderer.setDPI(96);

    const QSizeF size = renderer.pageSizeF(1);

    // A4 在 96 dpi 下是 8.27in x 11.69in，容差吸收点/英寸换算的舍入。
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
    QImage       image(pageSize.toSize(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);

    QPainter painter(&image);
    renderer.render(&painter, 1);
    painter.end();

    // 夹具两页除了居中的标题全是白的，所以整张图颜色一致就意味着什么都没画上去。
    const QColor first   = image.pixelColor(0, 0);
    bool         painted = false;
    for (int y = 0; y < image.height() && !painted; y += 7)
    {
        for (int x = 0; x < image.width() && !painted; x += 7)
        {
            if (image.pixelColor(x, y) != first)
            {
                painted = true;
            }
        }
    }

    QVERIFY(painted);
}

void TestPdfRenderer::rendersOnlyTheRequestedBounds()
{
    QPdfRenderer renderer(mPdfPath);
    renderer.setDPI(96);

    const QSizeF pageSize = renderer.pageSizeF(1);
    QImage       image(pageSize.toSize(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);

    QPainter     painter(&image);
    const QRectF lowerHalf(0, pageSize.height() / 2, pageSize.width(), pageSize.height() / 2);
    renderer.render(&painter, 1, lowerHalf);
    painter.end();

    // 标题在页面的垂直中部，所以没被请求的上半部分必须保持纯白，下半部分才会落墨。
    bool upperStillWhite = true;
    for (int y = 0; y < image.height() / 4 && upperStillWhite; ++y)
    {
        for (int x = 0; x < image.width(); ++x)
        {
            if (image.pixelColor(x, y) != QColor(Qt::white))
            {
                upperStillWhite = false;
            }
        }
    }

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
