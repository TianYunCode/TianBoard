#include "content/PdfPageItem.h"

#include <qglobal.h>

#include <QStyleOptionGraphicsItem>


PdfPageItem::PdfPageItem(PDFRenderer *renderer, int pageNumber, QGraphicsItem *parentItem)
    : QObject(0), QGraphicsItem(parentItem)
    , mRenderer(renderer)
    , mPageNumber(pageNumber)
{
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    mRenderer->attach();
}

PdfPageItem::~PdfPageItem()
{
    mRenderer->detach();
}

QRectF PdfPageItem::boundingRect() const
{
    if (!mRenderer->isValid())
    {
        return QRectF();
    }

    return QRectF(QPointF(0, 0), mRenderer->pageSizeF(mPageNumber));
}

void PdfPageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget)

    if (!mRenderer || !mRenderer->isValid())
        return;


    if (option)
        mRenderer->render(painter, mPageNumber, option->exposedRect);
    else
        qWarning("PdfPageItem::paint: option is null, ignoring painting");

}
