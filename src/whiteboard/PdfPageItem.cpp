#include "whiteboard/PdfPageItem.h"

#include <qglobal.h>

#include <QStyleOptionGraphicsItem>


PdfPageItem::PdfPageItem(PDFRenderer *renderer, int pageNumber, QGraphicsItem *parentItem)
    : QObject(0), QGraphicsItem(parentItem), mRenderer(renderer), mPageNumber(pageNumber) {
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    mRenderer->attach();
}

PdfPageItem::~PdfPageItem() {
    mRenderer->detach();
}

QRectF PdfPageItem::boundingRect() const {
    if (!mRenderer->isValid()) {
        return QRectF();
    }

    return QRectF(QPointF(0, 0), mRenderer->pageSizeF(mPageNumber));
}

void PdfPageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(widget)

    if (!mRenderer || !mRenderer->isValid())
        return;

    // Qt 保证 option 非空；真遇到空 option 时退回整页绘制，而不是什么都不画。
    const QRectF target = option ? option->exposedRect : boundingRect();
    mRenderer->render(painter, mPageNumber, target);
}
