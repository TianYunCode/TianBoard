#pragma once

#include <QtWidgets>
#include <QtCore/QObject>

#include "whiteboard/PDFRenderer.h"

class PdfPageItem : public QObject, public QGraphicsItem {
    Q_OBJECT
    Q_INTERFACES(QGraphicsItem);

public:
    PdfPageItem(PDFRenderer *renderer, int pageNumber, QGraphicsItem *parentItem = 0);
    virtual ~PdfPageItem();

    virtual QRectF boundingRect() const;

    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

    int pageNumber() const { return mPageNumber; }
    QUuid fileUuid() const { return mRenderer->fileUuid(); }
    QByteArray fileData() const { return mRenderer->fileData(); }

protected:
    PDFRenderer *mRenderer;
    int mPageNumber;
};
