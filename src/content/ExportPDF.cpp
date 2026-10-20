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

#include "whiteboard/PdfPageItem.h"


ExportPDF::ExportPDF(QObject *parent) : ExportAdaptor(parent) {}

ExportPDF::~ExportPDF() {}

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

    // 场景坐标是像素，PDF 页面用点：按物理 DPI 换算成 1/72 英寸。
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

        // 导出页不保留深色/网格背景：PDF 阅读器默认白底，深色背景会掩盖笔迹。
        bool isDark = scene->isDarkBackground();
        PageBackground pageBackground = scene->pageBackground();
        scene->setBackground(false, PageBackground::plain);

        // 输出页尺寸等于场景边界；超出边界的内容由 render() 按 KeepAspectRatio 缩放。
        QSize pageSize = scene->sceneSize();

        // 非屏幕渲染 + 高质量，避免导出结果带上为屏幕做的降采样。
        scene->setRenderingQuality(Item::RenderingQualityHigh);
        scene->setRenderingContext(GraphicsScene::NonScreen);

        // QPdfWriter 的页面尺寸单位是点，因此把换算后的像素尺寸当 Point 传入。
        QPageSize outputPageSize = QPageSize(
            QSizeF(pageSize.width() * scaleFactor, pageSize.height() * scaleFactor), QPageSize::Point);
        pdfWriter.setPageSize(outputPageSize);

        // QPainter 只能 begin 一次；后续页通过 newPage() 切页，
        // painterNeedsBegin 同时充当“begin 是否失败”的标志。
        if (painterNeedsBegin)
            painterNeedsBegin = !pdfPainter.begin(&pdfWriter);

        else if (pageIndex < existingPageCount)
            pdfWriter.newPage();

        // 用 normalizedSceneRect 保证内容完整落在页面内，不按 1:1 裁切。
        scene->render(&pdfPainter, QRectF(), scene->normalizedSceneRect());

        // 恢复屏幕渲染参数，否则后续交互仍停留在非屏幕模式。
        scene->setRenderingContext(GraphicsScene::Screen);
        scene->setRenderingQuality(Item::RenderingQualityNormal);

        // 背景是临时改的，导出后恢复原状，避免屏幕上出现“白底”闪变。
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
