#include "content/ThumbnailAdaptor.h"

#include <QtCore>

#include "utils/FileSystemUtils.h"

#include "core/PersistenceManager.h"
#include "core/Application.h"
#include "settings/Settings.h"

#include "whiteboard/BoardController.h"
#include "whiteboard/BoardPaletteManager.h"

#include "model/DocumentProxy.h"

#include "whiteboard/GraphicsScene.h"

#include "content/SvgSubsetAdaptor.h"


void ThumbnailAdaptor::generateMissingThumbnails(DocumentProxy* proxy)
{
    int existingPageCount = proxy->pageCount();

    for (int iPageNo = 0; iPageNo < existingPageCount; ++iPageNo)
    {
        QString thumbFileName = proxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", iPageNo);

        QFile thumbFile(thumbFileName);

        if (!thumbFile.exists())
        {
            bool displayMessage = (existingPageCount > 5);

            int thumbCount = 0;

            GraphicsScene* scene = SvgSubsetAdaptor::loadScene(proxy, iPageNo);

            if (scene)
            {
                thumbCount++;

                if (displayMessage && thumbCount == 1)
                    Application::showMessage(tr("Generating preview thumbnails ..."));

                persistScene(proxy, scene, iPageNo);
            }

            if (displayMessage && thumbCount > 0)
                Application::showMessage(tr("%1 thumbnails generated ...").arg(thumbCount));

        }
    }
}

const QPixmap* ThumbnailAdaptor::get(DocumentProxy* proxy, int pageIndex)
{
    QString fileName = proxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", pageIndex);

    QFile file(fileName);
    if (!file.exists())
    {
        generateMissingThumbnails(proxy);
    }

    QPixmap* pix = new QPixmap();
    if (file.exists())
    {
        //Warning. Works only with modified Qt
#ifdef Q_OS_LINUX
        pix->load(fileName, 0, Qt::AutoColor);
#else
        pix->load(fileName, 0, Qt::AutoColor);
#endif
    }
    return pix;
}

void ThumbnailAdaptor::load(DocumentProxy* proxy, QList<const QPixmap*>& list)
{
    generateMissingThumbnails(proxy);

    foreach(const QPixmap* pm, list){
        delete pm;
        pm = NULL;
    }
    list.clear();
    for(int i=0; i<proxy->pageCount(); i++)
        list.append(get(proxy, i));
}

void ThumbnailAdaptor::persistScene(DocumentProxy* proxy, GraphicsScene* pScene, int pageIndex, bool overrideModified)
{
    QString fileName = proxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", pageIndex);

    QFile thumbFile(fileName);

    if (pScene->isModified() || overrideModified || !thumbFile.exists())
    {
        qreal nominalWidth = pScene->nominalSize().width();
        qreal nominalHeight = pScene->nominalSize().height();
        qreal ratio = nominalWidth / nominalHeight;
        QRectF sceneRect = pScene->normalizedSceneRect(ratio);

        qreal width = Settings::maxThumbnailWidth;
        qreal height = width / ratio;

        QImage thumb(width, height, QImage::Format_ARGB32);

        QRectF imageRect(0, 0, width, height);

        QPainter painter(&thumb);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        if (pScene->isDarkBackground())
        {
            painter.fillRect(imageRect, Qt::black);
        }
        else
        {
            painter.fillRect(imageRect, Qt::white);
        }

        pScene->setRenderingContext(GraphicsScene::NonScreen);
        pScene->setRenderingQuality(Item::RenderingQualityHigh);

        pScene->render(&painter, imageRect, sceneRect, Qt::KeepAspectRatio);

        pScene->setRenderingContext(GraphicsScene::Screen);
        pScene->setRenderingQuality(Item::RenderingQualityNormal);

        thumb.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation).save(fileName, "JPG");
    }
}


QUrl ThumbnailAdaptor::thumbnailUrl(DocumentProxy* proxy, int pageIndex)
{
    QString fileName = proxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", pageIndex);

    return QUrl::fromLocalFile(fileName);
}
