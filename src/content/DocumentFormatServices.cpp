#include "content/DocumentFormatServices.h"

#include "content/MetadataDcSubsetAdaptor.h"
#include "content/ThumbnailAdaptor.h"

QMap<QString, QVariant> DocumentFormatServices::loadMetadata(const QString &documentPath)
{
    return MetadataDcSubsetAdaptor::load(documentPath);
}

void DocumentFormatServices::persistMetadata(DocumentProxy *proxy)
{
    MetadataDcSubsetAdaptor::persist(proxy);
}

const QPixmap *DocumentFormatServices::pageThumbnail(DocumentProxy *proxy, int pageIndex)
{
    return ThumbnailAdaptor::get(proxy, pageIndex);
}

void DocumentFormatServices::loadPageThumbnails(DocumentProxy *proxy, QList<const QPixmap *> &out)
{
    ThumbnailAdaptor::load(proxy, out);
}
