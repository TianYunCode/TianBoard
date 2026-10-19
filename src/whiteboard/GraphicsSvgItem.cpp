#include "whiteboard/GraphicsSvgItem.h"

#include <QtWidgets>

#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsItemDelegate.h"
#include "whiteboard/GraphicsPixmapItem.h"

#include "core/Application.h"
#include "core/PersistenceManager.h"

#include "whiteboard/BoardController.h"

#include "utils/FileSystemUtils.h"


GraphicsSvgItem::GraphicsSvgItem(const QString& pFilePath, QGraphicsItem* parent)
    : QGraphicsSvgItem(pFilePath, parent)
{
    init();

    QFile f(pFilePath);

    if (f.open(QIODevice::ReadOnly))
    {
        mFileData = f.readAll();
        f.close();
    }
}

GraphicsSvgItem::GraphicsSvgItem(const QByteArray& pFileData, QGraphicsItem* parent)
    : QGraphicsSvgItem(parent)
{
    init();

    QSvgRenderer* renderer = new QSvgRenderer(pFileData, this);

    setSharedRenderer(renderer);
    mFileData = pFileData;
}


void GraphicsSvgItem::init()
{
    setData(GraphicsItemData::ItemLayerType, ItemLayerType::Object);

    setDelegate(new GraphicsItemDelegate(this, 0, GF_COMMON
                                           | GF_RESPECT_RATIO
                                           | GF_REVOLVABLE));
    GraphicsFlags dfl = Delegate()->frameFlags();
    Delegate()->setFrameFlags(dfl | GF_FLIPPABLE_ALL_AXIS | GF_REVOLVABLE);

    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    setMaximumCacheSize(boundingRect().size().toSize() * MAX_ZOOM_LEVEL);

    setData(GraphicsItemData::itemLayerType, QVariant(itemLayerType::ObjectItem)); //Necessary to set if we want z value to be assigned correctly

    setData(GraphicsItemData::ItemCanBeSetAsBackground, true);

    setUuid(QUuid::createUuid());
}

GraphicsSvgItem::~GraphicsSvgItem()
{
}


QByteArray GraphicsSvgItem::fileData() const
{
    return mFileData;
}


QVariant GraphicsSvgItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    QVariant newValue = Delegate()->itemChange(change, value);
    return QGraphicsSvgItem::itemChange(change, newValue);
}


void GraphicsSvgItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QMimeData* pMime = new QMimeData();
    QPixmap pixmap = toPixmapItem()->pixmap();
    pMime->setImageData(pixmap.toImage());
    Delegate()->setMimeData(pMime);
    qreal k = (qreal)pixmap.width() / 100.0;

    QSize newSize((int)(pixmap.width() / k), (int)(pixmap.height() / k));

    Delegate()->setDragPixmap(pixmap.scaled(newSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    if (!Delegate()->mousePressEvent(event))
        QGraphicsSvgItem::mousePressEvent(event);
}


void GraphicsSvgItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate()->mouseMoveEvent(event))
    {
        // NOOP;
    }
    else
    {
        QGraphicsSvgItem::mouseMoveEvent(event);
    }
}


void GraphicsSvgItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Delegate()->mouseReleaseEvent(event);
    QGraphicsSvgItem::mouseReleaseEvent(event);
}


void GraphicsSvgItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // Never draw the rubber band, we draw our custom selection with the DelegateFrame
    QStyleOptionGraphicsItem styleOption = QStyleOptionGraphicsItem(*option);
    styleOption.state &= ~QStyle::State_Selected;

    QGraphicsSvgItem::paint(painter, &styleOption, widget);
    Delegate()->postpaint(painter, option, widget);
}


Item* GraphicsSvgItem::deepCopy() const
{
    GraphicsSvgItem* copy = new GraphicsSvgItem(this->fileData());

    copy->setUuid(this->uuid()); // this is OK for now as long as Widgets are imutable

    copyItemParameters(copy);

    return copy;

}

void GraphicsSvgItem::copyItemParameters(Item *copy) const
{
    GraphicsSvgItem *cp = dynamic_cast<GraphicsSvgItem*>(copy);
    if (cp)
    {
        cp->setPos(this->pos());
        cp->setTransform(this->transform());
        cp->setFlag(QGraphicsItem::ItemIsMovable, true);
        cp->setFlag(QGraphicsItem::ItemIsSelectable, true);
        cp->setData(GraphicsItemData::ItemLayerType, this->data(GraphicsItemData::ItemLayerType));
        cp->setData(GraphicsItemData::ItemLocked, this->data(GraphicsItemData::ItemLocked));
        cp->setSourceUrl(this->sourceUrl());
        cp->setZValue(this->zValue());
    }
}

void GraphicsSvgItem::setRenderingQuality(RenderingQuality pRenderingQuality)
{
    Item::setRenderingQuality(pRenderingQuality);

    if (pRenderingQuality == RenderingQualityHigh)
    {
        setCacheMode(QGraphicsItem::NoCache);
    }
    else
    {
        setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    }
}


GraphicsScene* GraphicsSvgItem::scene()
{
    return qobject_cast<GraphicsScene*>(QGraphicsItem::scene());
}



GraphicsPixmapItem* GraphicsSvgItem::toPixmapItem() const
{
    QImage image(renderer()->viewBox().size(), QImage::Format_ARGB32);
    QPainter painter(&image);
    renderer()->render(&painter);

    GraphicsPixmapItem *pixmapItem =  new GraphicsPixmapItem();
    pixmapItem->setPixmap(QPixmap::fromImage(image));

    pixmapItem->setPos(this->pos());
    pixmapItem->setTransform(this->transform());
    pixmapItem->setFlag(QGraphicsItem::ItemIsMovable, true);
    pixmapItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
    pixmapItem->setData(GraphicsItemData::ItemLayerType, this->data(GraphicsItemData::ItemLayerType));

    return pixmapItem;
}

void GraphicsSvgItem::setUuid(const QUuid &pUuid)
{
    Item::setUuid(pUuid);
    setData(GraphicsItemData::ItemUuid, QVariant(pUuid)); //store item uuid inside the QGraphicsItem to fast operations with Items on the scene
}


void GraphicsSvgItem::clearSource()
{
    QString fileName = PersistenceManager::imageDirectory + "/" + uuid().toString() + ".svg";
    QString diskPath =  Application::boardController->selectedDocument()->persistencePath() + "/" + fileName;
    FileSystemUtils::deleteFile(diskPath);
}
