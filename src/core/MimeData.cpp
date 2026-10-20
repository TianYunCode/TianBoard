#include "core/MimeData.h"

#include <QtWidgets>

#include "core/Application.h"
#include "whiteboard/Item.h"

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
    setData(Application::mimeTypeBoardPage, QByteArray());
}

MimeData::~MimeData()
{
    // 刻意留空
}

MimeDataGraphicsItem::MimeDataGraphicsItem(QList<Item *> pItems)
{
    mItems = pItems;
}

MimeDataGraphicsItem::~MimeDataGraphicsItem()
{
    // 剪贴板里放的是这两条路径的 deepCopy()，场景不拥有它们：MimeData 被替换/清空时
    // 必须自己回收，否则每复制/剪切一次就泄漏一批图形项。
    qDeleteAll(mItems);
    mItems.clear();
}
