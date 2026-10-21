#include "content/ExportFullPDF.h"
#include "model/DocumentTreeModel.h"
#include "core/VirtualDesktop.h"

#include <QtCore>
#include <QtSvg>
#include <QtSvgWidgets>
#include <QPrinter>

#include "core/Application.h"

#include "model/DocumentProxy.h"

#include "content/ExportPDF.h"

ExportFullPDF::ExportFullPDF(QObject *parent) : ExportAdaptor(parent)
{
    // 屏幕坐标以像素计，PDF 以点（1/72 英寸）计，按物理 DPI 求换算系数。
    VirtualDesktop *desktop   = VirtualDesktop::instance();
    int             dpiCommon = desktop->physicalDpi();
    mScaleFactor              = 72.0f / dpiCommon; // 1pt = 1/72 inch

    mSimpleExporter           = new ExportPDF();
}

ExportFullPDF::~ExportFullPDF()
{
}

void ExportFullPDF::saveOverlayPdf(DocumentProxy *pDocumentProxy, const QString &filename)
{
    if (!pDocumentProxy || filename.length() == 0 || pDocumentProxy->pageCount() == 0)
    {
        return;
    }

    mSimpleExporter->persistsDocument(pDocumentProxy, filename);
}

void ExportFullPDF::persist(DocumentProxy *pDocumentProxy, const QModelIndex &selectedTreeIndex)
{
    Q_UNUSED(selectedTreeIndex);

    persistLocally(pDocumentProxy, tr("Export as PDF File"));
}

bool ExportFullPDF::persistsDocument(DocumentProxy *pDocumentProxy, const QString &filename)
{
    QFile file(filename);
    if (file.exists())
    {
        file.remove();
    }

    QString overlayName = filename;
    overlayName.replace(".pdf", "_overlay.pdf");

    QFile previousOverlay(overlayName);
    if (previousOverlay.exists())
    {
        previousOverlay.remove();
    }

    mHasPDFBackgrounds = false;

    saveOverlayPdf(pDocumentProxy, overlayName);

    if (!mHasPDFBackgrounds)
    {
        QFile f(overlayName);
        f.rename(filename);
    }
    else
    {
        // PDF 背景与批注的矢量合并需要外部合并库；当前构建没有该依赖，
        // mHasPDFBackgrounds 始终为 false，本分支只在将来接入合并器后才会走到。
        if (!Application::app()->isVerbose())
        {
            QFile::remove(overlayName);
        }
    }

    return true;
}

bool ExportFullPDF::associatedActionactionAvailableFor(const QModelIndex &selectedIndex)
{
    const DocumentTreeModel *docModel = qobject_cast<const DocumentTreeModel *>(selectedIndex.model());
    if (!selectedIndex.isValid() || docModel->isCatalog(selectedIndex))
    {
        return false;
    }

    return true;
}

QString ExportFullPDF::exportExtention()
{
    return QString(".pdf");
}

QString ExportFullPDF::exportName()
{
    return tr("Export to PDF");
}
