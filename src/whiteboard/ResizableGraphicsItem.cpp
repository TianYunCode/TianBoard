#include "whiteboard/ResizableGraphicsItem.h"


ResizableGraphicsItem::ResizableGraphicsItem()
{
    // NOOP

}

ResizableGraphicsItem::~ResizableGraphicsItem()
{
    // NOOP
}

void ResizableGraphicsItem::resize(const QSizeF& pSize)
{
    resize(pSize.width(), pSize.height());
}
