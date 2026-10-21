#include "content/ImportPDF.h"
#include "utils/UserNotices.h"
#include "platform/VirtualDesktop.h"

#include "core/PersistenceManager.h"

#include "whiteboard/GraphicsPDFItem.h"

#include "whiteboard/PDFRenderer.h"

ImportPDF::ImportPDF(QObject *parent) : PageBasedImportAdaptor(parent)
{
    VirtualDesktop *desktop = VirtualDesktop::instance();
    this->dpi               = desktop->physicalDpi();
}

ImportPDF::~ImportPDF()
{
}

QStringList ImportPDF::supportedExtentions()
{
    return QStringList("pdf");
}

QString ImportPDF::importFileFilter()
{
    return tr("Portable Document Format (*.pdf)");
}

QList<GraphicsItem *> ImportPDF::import(const QUuid &uuid, const QString &filePath)
{
    QList<GraphicsItem *> result;

    // 渲染器由 PDFRenderer 按 uuid 缓存，图元析构、引用计数归零后会自动释放。
    PDFRenderer *pdfRenderer = PDFRenderer::rendererForUuid(uuid, filePath, true); // 第三个参数表示允许创建

    if (!pdfRenderer->isValid())
    {
        UserNotices::presentStatusMessage(tr("PDF import failed."));
        return result;
    }
    pdfRenderer->setDPI(this->dpi);

    int pdfPageCount = pdfRenderer->pageCount();

    for (int pdfPageNumber = 1; pdfPageNumber <= pdfPageCount; pdfPageNumber++)
    {
        UserNotices::presentStatusMessage(tr("Importing page %1 of %2").arg(pdfPageNumber).arg(pdfPageCount), true);
        // 所有权交给调用方，placeImportedItemToScene 时再转交场景。
        result << new GraphicsPDFItem(pdfRenderer, pdfPageNumber); // 由场景负责 delete
    }
    return result;
}

void ImportPDF::placeImportedItemToScene(GraphicsScene *scene, GraphicsItem *item)
{
    GraphicsPDFItem *pdfItem = (GraphicsPDFItem *)item;

    pdfItem->setPos(-pdfItem->boundingRect().width() / 2, -pdfItem->boundingRect().height() / 2);

    scene->setAsBackgroundObject(pdfItem, false, false);

    scene->setNominalSize(pdfItem->boundingRect().width(), pdfItem->boundingRect().height());
}

const QString &ImportPDF::folderToCopy()
{
    return PersistenceManager::objectDirectory;
}
