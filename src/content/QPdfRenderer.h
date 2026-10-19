#pragma once

#include "content/PDFRenderer.h"

#include <QImage>
#include <QtPdf/QPdfDocument>

class QPainter;

// PDFRenderer backed by Qt6::Pdf, replacing the Xpdf based renderer whose
// implementation had been commented out.
class QPdfRenderer : public PDFRenderer {
    Q_OBJECT

public:
    QPdfRenderer(const QString &filename, bool importingFile = false);
    ~QPdfRenderer() override;

    bool isValid() const override;

    int pageCount() const override;

    QSizeF pageSizeF(int pageNumber) const override;

    int pageRotation(int pageNumber) const override;

    QString title() const override;

public slots:
    void render(QPainter *p, int pageNumber, const QRectF &bounds = QRectF()) override;

private:
    QImage renderedPage(int pageIndex, const QSize &pixelSize);

    QPdfDocument mDocument;

    int mCachedPageIndex;
    QSize mCachedPixelSize;
    QImage mCachedImage;
};
