#include "content/ImportImage.h"

#include "model/DocumentProxy.h"

#include "whiteboard/BoardController.h"

#include "core/Application.h"
#include "core/PersistenceManager.h"
#include "core/DocumentManager.h"

#include "whiteboard/GraphicsPixmapItem.h"

#include "whiteboard/PDFRenderer.h"

ImportImage::ImportImage(QObject *parent) : PageBasedImportAdaptor(parent)
{
}

ImportImage::~ImportImage()
{
}

QStringList ImportImage::supportedExtentions()
{
    QStringList formats;

    for (int i = 0; i < QImageReader::supportedImageFormats().count(); ++i)
    {
        formats << QString(QImageReader::supportedImageFormats().at(i)).toLower();
    }

    return formats;
}

QString ImportImage::importFileFilter()
{
    QString     filter  = tr("Image Format (");
    QStringList formats = supportedExtentions();
    bool        isFirst = true;

    foreach (QString format, formats)
    {
        if (isFirst)
        {
            isFirst = false;
        }
        else
        {
            filter.append(" ");
        }

        filter.append("*." + format);
    }

    filter.append(")");

    return filter;
}

QList<GraphicsItem *> ImportImage::import(const QUuid &uuid, const QString &filePath)
{
    Q_UNUSED(uuid);
    QList<GraphicsItem *> result;

    QPixmap pix(filePath);
    if (pix.isNull())
    {
        return result;
    }

    GraphicsPixmapItem *pixmapItem = new GraphicsPixmapItem();
    pixmapItem->setPixmap(pix);
    result << pixmapItem;

    return result;
}

void ImportImage::placeImportedItemToScene(GraphicsScene *scene, GraphicsItem *item)
{
    GraphicsPixmapItem *pixmapItem = (GraphicsPixmapItem *)item;

    GraphicsPixmapItem *sceneItem  = scene->addPixmap(pixmapItem->pixmap(), NULL, QPointF(0, 0), 1.0, false, true);
    scene->setAsBackgroundObject(sceneItem, true);

    // 导入用的临时图元只为了携带 pixmap，场景已复制出背景对象，这里必须释放。
    delete pixmapItem;
}

const QString &ImportImage::folderToCopy()
{
    static QString f("");
    return f;
}
