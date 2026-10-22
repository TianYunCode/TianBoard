#include "whiteboard/GraphicsSvgItem.h"
#include "model/PackageLayout.hpp"

#include <QtWidgets>

#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsItemDelegate.h"
#include "whiteboard/GraphicsPixmapItem.h"

#include "whiteboard/BoardController.h"

#include "utils/FileSystemUtils.h"

GraphicsSvgItem::GraphicsSvgItem(const QString &pFilePath, QGraphicsItem *parent) : QGraphicsSvgItem(pFilePath, parent)
{
    init();

    QFile f(pFilePath);

    if (f.open(QIODevice::ReadOnly))
    {
        mFileData = f.readAll();
        f.close();
    }
    else
    {
        // 基类拿不到字节就画成一个不到一像素的东西，既不崩也不报错；不留一行日志的话，
        // 用户只会看到"页上那个对象没了"，而盘上的页文件里其实还留着它的引用。
        qWarning() << "SVG 图元的资源读不到，这一页上的这个对象会是空的:" << pFilePath;
    }
}

GraphicsSvgItem::GraphicsSvgItem(const QByteArray &pFileData, QGraphicsItem *parent) : QGraphicsSvgItem(parent)
{
    init();

    QSvgRenderer *renderer = new QSvgRenderer(pFileData, this);

    setSharedRenderer(renderer);
    mFileData = pFileData;
}

void GraphicsSvgItem::init()
{
    // 新旧两个层级键都要写：文档读写与部分委托逻辑仍读旧键 ItemLayerType。
    setData(GraphicsItemData::ItemLayerType, ItemLayerType::Object);

    setDelegate(new GraphicsItemDelegate(this, 0, GF_COMMON | GF_RESPECT_RATIO | GF_REVOLVABLE));
    GraphicsFlags dfl = Delegate()->frameFlags();
    Delegate()->setFrameFlags(dfl | GF_FLIPPABLE_ALL_AXIS | GF_REVOLVABLE);

    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
    setMaximumCacheSize(boundingRect().size().toSize() * MAX_ZOOM_LEVEL);

    setData(GraphicsItemData::itemLayerType,
        QVariant(itemLayerType::ObjectItem)); // Z 值分配依赖这份层级数据，不设置的话图元会被排到错误的层级上

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
    QMimeData *pMime = new QMimeData();
    // 直接渲染成 QPixmap；toPixmapItem() 会 new 一个带 delegate 的 GraphicsPixmapItem，
    // 只为取它的 pixmap 就扔掉了，每按一次泄漏一个。
    QPixmap pixmap   = renderToPixmap();
    pMime->setImageData(pixmap.toImage());
    Delegate()->setMimeData(pMime);
    qreal k = (qreal)pixmap.width() / 100.0;

    QSize newSize((int)(pixmap.width() / k), (int)(pixmap.height() / k));

    Delegate()->setDragPixmap(pixmap.scaled(newSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    if (!Delegate()->mousePressEvent(event))
    {
        QGraphicsSvgItem::mousePressEvent(event);
    }
}

void GraphicsSvgItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (Delegate()->mouseMoveEvent(event))
    {
        // 委托已经接管了这次拖动（移动/缩放/旋转），这里故意什么都不做。
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
    // 绝不画 Qt 自带的选中橡皮筋，选中效果由委托的选择框绘制。
    QStyleOptionGraphicsItem styleOption  = QStyleOptionGraphicsItem(*option);
    styleOption.state                    &= ~QStyle::State_Selected;

    QGraphicsSvgItem::paint(painter, &styleOption, widget);
    Delegate()->postpaint(painter, option, widget);
}

Item *GraphicsSvgItem::deepCopy() const
{
    GraphicsSvgItem *copy = new GraphicsSvgItem(this->fileData());

    // 构造函数已经给副本发了自己的 UUID，这里不再改回源图元那一份：文件名按 UUID 取，
    // 两份图元共用一个文件时，删掉任一份会把另一份引用的文件也删掉（见 clearSource）。
    // 副本自己的文件由写出器在保存这一页时落盘（见 SvgSubsetAdaptor 的 svgItemToLinkedSvg）。
    copyItemParameters(copy);

    return copy;
}

void GraphicsSvgItem::copyItemParameters(Item *copy) const
{
    GraphicsSvgItem *cp = dynamic_cast<GraphicsSvgItem *>(copy);
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

GraphicsScene *GraphicsSvgItem::scene()
{
    return qobject_cast<GraphicsScene *>(QGraphicsItem::scene());
}

QPixmap GraphicsSvgItem::renderToPixmap() const
{
    QImage   image(renderer()->viewBox().size(), QImage::Format_ARGB32);
    QPainter painter(&image);
    renderer()->render(&painter);

    return QPixmap::fromImage(image);
}

GraphicsPixmapItem *GraphicsSvgItem::toPixmapItem() const
{
    GraphicsPixmapItem *pixmapItem = new GraphicsPixmapItem();
    pixmapItem->setPixmap(renderToPixmap());

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
    setData(GraphicsItemData::ItemUuid,
        QVariant(pUuid)); // 把 UUID 同时写进图元 data，场景内按 UUID 查找图元时就不必逐个做类型转换，速度更快
}

void GraphicsSvgItem::clearSource()
{
    QString fileName = PackageLayout::imageDirectory + "/" + uuid().toString() + ".svg";
    QString diskPath = BoardController::instance()->selectedDocument()->persistencePath() + "/" + fileName;
    FileSystemUtils::deleteFile(diskPath);
}
