#include "content/ExportPDF.h"
#include "core/VirtualDesktop.h"

#include <QtCore>
#include <QtSvg>
#include <QtSvgWidgets>
#include <QPrinter>
#include <QPdfWriter>

#include "core/Application.h"
#include "settings/Settings.h"
#include "settings/Setting.h"
#include "core/PersistenceManager.h"

#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsSvgItem.h"
#include "whiteboard/GraphicsPDFItem.h"

#include "model/DocumentProxy.h"
#include "model/DocumentController.h"

#include "content/PdfPageItem.h"


ExportPDF::ExportPDF(QObject *parent) : ExportAdaptor(parent) {
    // NOOP
}

ExportPDF::~ExportPDF() {
    // NOOP
}

void ExportPDF::persist(DocumentProxy *pDocumentProxy) {
    persistLocally(pDocumentProxy, tr("Export as PDF File"));
}

bool ExportPDF::associatedActionactionAvailableFor(const QModelIndex &selectedIndex) {
    const DocumentTreeModel *docModel = qobject_cast<const DocumentTreeModel *>(selectedIndex.model());
    if (!selectedIndex.isValid() || docModel->isCatalog(selectedIndex)) {
        return false;
    }

    return true;
}


bool ExportPDF::persistsDocument(DocumentProxy *pDocumentProxy, const QString &filename) {
    QPdfWriter pdfWriter(filename);

    qDebug() << "exporting document to PDF" << filename;

    pdfWriter.setResolution(Settings::settings()->pdfResolution->get().toInt());
    pdfWriter.setPageMargins(QMarginsF());
    pdfWriter.setTitle(pDocumentProxy->name());
    pdfWriter.setCreator("TianBoard PDF export");

    //need to calculate screen resolution
    VirtualDesktop *desktop = VirtualDesktop::instance();
    int dpiCommon = desktop->physicalDpi();
    float scaleFactor = 72.0f / dpiCommon;

    QPainter pdfPainter;
    bool painterNeedsBegin = true;

    int existingPageCount = pDocumentProxy->pageCount();

    for (int pageIndex = 0; pageIndex < existingPageCount; pageIndex++) {
        GraphicsScene *scene =
            PersistenceManager::persistenceManager()->loadDocumentScene(pDocumentProxy, pageIndex);
        Application::showMessage(tr("Exporting page %1 of %2").arg(pageIndex + 1).arg(existingPageCount));

        // set background to white, no crossing for PDF output
        bool isDark = scene->isDarkBackground();
        PageBackground pageBackground = scene->pageBackground();
        scene->setBackground(false, PageBackground::plain);

        // pageSize is the output PDF page size; it is set to equal the scene's boundary size; if the contents
        // of the scene overflow from the boundaries, they will be scaled down.
        QSize pageSize = scene->sceneSize();

        // set high res rendering
        scene->setRenderingQuality(Item::RenderingQualityHigh);
        scene->setRenderingContext(GraphicsScene::NonScreen);

        // Setting output page size
        QPageSize outputPageSize = QPageSize(
            QSizeF(pageSize.width() * scaleFactor, pageSize.height() * scaleFactor), QPageSize::Point);
        pdfWriter.setPageSize(outputPageSize);

        // Call begin only once
        if (painterNeedsBegin)
            painterNeedsBegin = !pdfPainter.begin(&pdfWriter);

        else if (pageIndex < existingPageCount)
            pdfWriter.newPage();

        // Render the scene
        scene->render(&pdfPainter, QRectF(), scene->normalizedSceneRect());

        // Restore screen rendering quality
        scene->setRenderingContext(GraphicsScene::Screen);
        scene->setRenderingQuality(Item::RenderingQualityNormal);

        // Restore background state
        scene->setBackground(isDark, pageBackground);
    }

    if (!painterNeedsBegin)
        pdfPainter.end();

    return true;
}

QString ExportPDF::exportExtention() {
    return QString(".pdf");
}

QString ExportPDF::exportName() {
    return tr("Export to PDF");
}
