#include "model/MimeData.h"

#include "model/MimeTypes.hpp"

MimeDataItem::MimeDataItem(DocumentProxy *proxy, int sceneIndex) : mProxy(proxy), mSceneIndex(sceneIndex)
{
    // 刻意留空
}

MimeDataItem::~MimeDataItem()
{
    // 刻意留空
}

MimeData::MimeData(const QList<MimeDataItem> &items) : QMimeData(), mItems(items)
{
    setData(MimeTypes::boardPage, QByteArray());
}

MimeData::~MimeData()
{
    // 刻意留空
}
