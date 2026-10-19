#include "content/ImportPDF.h"
#include "core/VirtualDesktop.h"

#include "model/DocumentProxy.h"

#include "core/Application.h"
#include "core/PersistenceManager.h"

#include "whiteboard/GraphicsPDFItem.h"

#include "content/PDFRenderer.h"


ImportPDF::ImportPDF(QObject *parent)
    : PageBasedImportAdaptor(parent)
{
    VirtualDesktop* desktop = VirtualDesktop::instance();
    this->dpi = (desktop->physicalDpiX() + desktop->physicalDpiY()) / 2;
}


ImportPDF::~ImportPDF()
{
    // NOOP
}


QStringList ImportPDF::supportedExtentions()
{
    return QStringList("pdf");
}


QString ImportPDF::importFileFilter()
{
    return tr("Portable Document Format (*.pdf)");
}


QList<GraphicsItem*> ImportPDF::import(const QUuid& uuid, const QString& filePath)
{
    QList<GraphicsItem*> result;

    PDFRenderer *pdfRenderer = PDFRenderer::rendererForUuid(uuid, filePath, true); // renderer is automatically deleted when not used anymore

    if (!pdfRenderer->isValid())
    {
        Application::showMessage(tr("PDF import failed."));
        return result;
    }
    pdfRenderer->setDPI(this->dpi);

    int pdfPageCount = pdfRenderer->pageCount();

    for(int pdfPageNumber = 1; pdfPageNumber <= pdfPageCount; pdfPageNumber++)
    {
        Application::showMessage(tr("Importing page %1 of %2").arg(pdfPageNumber).arg(pdfPageCount), true);
        result << new GraphicsPDFItem(pdfRenderer, pdfPageNumber); // deleted by the scene
    }
    return result;
}

void ImportPDF::placeImportedItemToScene(GraphicsScene* scene, GraphicsItem* item)
{
    GraphicsPDFItem *pdfItem = (GraphicsPDFItem*)item;

    pdfItem->setPos(-pdfItem->boundingRect().width() / 2, -pdfItem->boundingRect().height() / 2);

    scene->setAsBackgroundObject(pdfItem, false, false);

    scene->setNominalSize(pdfItem->boundingRect().width(), pdfItem->boundingRect().height());
}

const QString& ImportPDF::folderToCopy()
{
    return PersistenceManager::objectDirectory;
}
