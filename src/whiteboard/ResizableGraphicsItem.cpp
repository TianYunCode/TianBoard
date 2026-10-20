#include "whiteboard/ResizableGraphicsItem.h"

ResizableGraphicsItem::ResizableGraphicsItem()
{
    // 刻意留空
}

ResizableGraphicsItem::~ResizableGraphicsItem()
{
    // 刻意留空
}

void ResizableGraphicsItem::resize(const QSizeF &pSize)
{
    resize(pSize.width(), pSize.height());
}
