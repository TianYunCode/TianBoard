#include "core/MimeData.h"

#include <QtWidgets>

#include "core/Application.h"
#include "whiteboard/Item.h"


MimeDataItem::MimeDataItem(DocumentProxy* proxy, int sceneIndex)
    : mProxy(proxy)
    , mSceneIndex(sceneIndex)
{
    // NOOP
}

MimeDataItem::~MimeDataItem()
{
    // NOOP
}

MimeData::MimeData(const QList<MimeDataItem> &items)
    : QMimeData()
    , mItems(items)
{
    setData(Application::mimeTypeBoardPage, QByteArray());
}

MimeData::~MimeData()
{
    // NOOP
}

MimeDataGraphicsItem::MimeDataGraphicsItem(QList<Item*> pItems)
{
        mItems = pItems;
}

MimeDataGraphicsItem::~MimeDataGraphicsItem()
{
// Explanation: selected items are owned by the scene and handled by this class
}
