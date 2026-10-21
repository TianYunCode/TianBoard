#include "model/IDocumentFormatServices.h"

#include <QDebug>
#include <QPixmap>

namespace
{
    IDocumentFormatServices *gServices = nullptr;

    /// @brief 没装上服务时只报一次，免得文档库刷新时刷屏。
    void warnMissingServices()
    {
        static bool reported = false;

        if (!reported)
        {
            reported = true;
            qWarning() << "文档格式服务没有装上：元数据与缩略图不可用（装配点应调用 installDocumentFormatServices）";
        }
    }
} // namespace

void installDocumentFormatServices(IDocumentFormatServices *services)
{
    gServices = services;
}

QMap<QString, QVariant> loadDocumentMetadata(const QString &documentPath)
{
    if (!gServices)
    {
        warnMissingServices();
        return QMap<QString, QVariant>();
    }

    return gServices->loadMetadata(documentPath);
}

void persistDocumentMetadata(DocumentProxy *proxy)
{
    if (!gServices)
    {
        warnMissingServices();
        return;
    }

    gServices->persistMetadata(proxy);
}

const QPixmap *documentPageThumbnail(DocumentProxy *proxy, int pageIndex)
{
    if (!gServices)
    {
        warnMissingServices();
        return new QPixmap();
    }

    return gServices->pageThumbnail(proxy, pageIndex);
}

void loadDocumentPageThumbnails(DocumentProxy *proxy, QList<const QPixmap *> &out)
{
    if (!gServices)
    {
        warnMissingServices();
        qDeleteAll(out);
        out.clear();
        return;
    }

    gServices->loadPageThumbnails(proxy, out);
}
