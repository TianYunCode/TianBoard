#include "content/QPdfRenderer.h"

#include <QtGui>


namespace
{
    // QPdfDocument::render() paints the whole page, so the cache holds one full
    // page per renderer. The edge cap bounds the cost of a deeply zoomed page
    // (2048x2048 ARGB32 is 16 MB) at the price of a softer image beyond it.
    const int kMaxRenderEdge = 2048;
}

QPdfRenderer::QPdfRenderer(const QString &filename, bool importingFile)
    : mCachedPageIndex(-1)
{
    Q_UNUSED(importingFile);

    mDocument.load(filename);
}

QPdfRenderer::~QPdfRenderer()
{
    // NOOP
}

bool QPdfRenderer::isValid() const
{
    return mDocument.status() == QPdfDocument::Status::Ready
        && mDocument.error() == QPdfDocument::Error::None
        && mDocument.pageCount() > 0;
}

int QPdfRenderer::pageCount() const
{
    return isValid() ? mDocument.pageCount() : 0;
}

QSizeF QPdfRenderer::pageSizeF(int pageNumber) const
{
    if (!isValid() || pageNumber < 1 || pageNumber > mDocument.pageCount())
        return QSizeF();

    // pagePointSize() is the size the rendered image comes back with, i.e. it
    // already takes the page /Rotate into account, so no transposition here.
    const QSizeF pointSize = mDocument.pagePointSize(pageNumber - 1);

    return QSizeF(pointSize.width() * dpiForRendering / 72.0,
                  pointSize.height() * dpiForRendering / 72.0);
}

int QPdfRenderer::pageRotation(int pageNumber) const
{
    Q_UNUSED(pageNumber);

    // Qt6::Pdf applies the page's own rotation inside render() and does not
    // expose the raw /Rotate value. Nothing in the application reads this
    // accessor; it only exists to complete the PDFRenderer interface.
    return 0;
}

QString QPdfRenderer::title() const
{
    if (!isValid())
        return QString();

    return mDocument.metaData(QPdfDocument::MetaDataField::Title).toString();
}

QImage QPdfRenderer::renderedPage(int pageIndex, const QSize &pixelSize)
{
    if (pageIndex == mCachedPageIndex && pixelSize == mCachedPixelSize && !mCachedImage.isNull())
        return mCachedImage;

    const QImage image = mDocument.render(pageIndex, pixelSize);
    if (image.isNull())
        return image;

    mCachedPageIndex = pageIndex;
    mCachedPixelSize = pixelSize;
    mCachedImage = image;

    return mCachedImage;
}

void QPdfRenderer::render(QPainter *p, int pageNumber, const QRectF &bounds)
{
    if (!isValid() || !p)
        return;

    const QSizeF pageSize = pageSizeF(pageNumber);
    if (pageSize.isEmpty())
        return;

    const QTransform world = p->worldTransform();

    // The item may also be rotated, so the scale is the length of each mapped
    // axis rather than the diagonal terms alone.
    const qreal xscale = QVector2D(world.m11(), world.m12()).length();
    const qreal yscale = QVector2D(world.m21(), world.m22()).length();

    if (qFuzzyIsNull(xscale) || qFuzzyIsNull(yscale))
        return;

    const int width = qBound(1, qCeil(pageSize.width() * xscale), kMaxRenderEdge);
    const int height = qBound(1, qCeil(pageSize.height() * yscale), kMaxRenderEdge);

    const QImage image = renderedPage(pageNumber - 1, QSize(width, height));
    if (image.isNull())
        return;

    const QRectF target = bounds.isNull() ? QRectF(QPointF(0, 0), pageSize) : bounds;

    QRectF source(target.x() * image.width() / pageSize.width(),
                  target.y() * image.height() / pageSize.height(),
                  target.width() * image.width() / pageSize.width(),
                  target.height() * image.height() / pageSize.height());
    source = source.intersected(QRectF(0, 0, image.width(), image.height()));

    p->drawImage(target, image, source.toRect());
}
