#include "whiteboard/QPdfRenderer.h"

#include <QtGui>


namespace {
// QPdfDocument::render() 一次渲染整页，因此每个渲染器缓存一整页图像。
// 边长上限用于限制深度放大时的内存开销（2048x2048 的 ARGB32 占 16 MB），
// 代价是超过上限后图像会变得柔和。
const int kMaxRenderEdge = 2048;
} // namespace

QPdfRenderer::QPdfRenderer(const QString &filename, bool importingFile) : mCachedPageIndex(-1) {
    Q_UNUSED(importingFile);

    mDocument.load(filename);
}

QPdfRenderer::~QPdfRenderer() {
    // 有意为空：成员均自动析构
}

bool QPdfRenderer::isValid() const {
    return mDocument.status() == QPdfDocument::Status::Ready &&
           mDocument.error() == QPdfDocument::Error::None && mDocument.pageCount() > 0;
}

int QPdfRenderer::pageCount() const {
    return isValid() ? mDocument.pageCount() : 0;
}

QSizeF QPdfRenderer::pageSizeF(int pageNumber) const {
    if (!isValid() || pageNumber < 1 || pageNumber > mDocument.pageCount())
        return QSizeF();

    // pagePointSize() 返回的就是渲染图像的尺寸，已把页面的 /Rotate 考虑在内，
    // 因此这里无需转置。
    const QSizeF pointSize = mDocument.pagePointSize(pageNumber - 1);

    return QSizeF(pointSize.width() * dpiForRendering / 72.0, pointSize.height() * dpiForRendering / 72.0);
}

int QPdfRenderer::pageRotation(int pageNumber) const {
    Q_UNUSED(pageNumber);

    // Qt6 的 Pdf 模块在 render() 内部处理页面自身旋转，不对外暴露原始 /Rotate 值。
    // 应用内没有代码读取此接口，它只是为补齐 PDFRenderer 接口而存在。
    return 0;
}

QString QPdfRenderer::title() const {
    if (!isValid())
        return QString();

    return mDocument.metaData(QPdfDocument::MetaDataField::Title).toString();
}

QImage QPdfRenderer::renderedPage(int pageIndex, const QSize &pixelSize) {
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

void QPdfRenderer::render(QPainter *p, int pageNumber, const QRectF &bounds) {
    if (!isValid() || !p)
        return;

    const QSizeF pageSize = pageSizeF(pageNumber);
    if (pageSize.isEmpty())
        return;

    const QTransform world = p->worldTransform();

    // 项可能带有旋转，因此缩放系数取映射后各轴向量的长度，而不是只看对角元素
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
