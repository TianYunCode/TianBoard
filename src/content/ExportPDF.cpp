#include "content/ExportPDF.h"
#include "model/IDocumentStore.h"
#include "utils/UserNotices.h"
#include "model/DocumentTreeModel.h"
#include "platform/VirtualDesktop.h"

#include <QtCore>
#include <QtSvg>
#include <QtSvgWidgets>
#include <QPrinter>
#include <QPdfWriter>

#include "settings/Settings.h"

#include "whiteboard/GraphicsScene.h"

#include "model/DocumentProxy.h"

ExportPDF::ExportPDF(QObject *parent) : ExportAdaptor(parent)
{
}

ExportPDF::~ExportPDF()
{
}

void ExportPDF::persist(DocumentProxy *pDocumentProxy, const QModelIndex &selectedTreeIndex)
{
    Q_UNUSED(selectedTreeIndex);

    persistLocally(pDocumentProxy, tr("Export as PDF File"));
}

bool ExportPDF::associatedActionactionAvailableFor(const QModelIndex &selectedIndex)
{
    const DocumentTreeModel *docModel = qobject_cast<const DocumentTreeModel *>(selectedIndex.model());
    if (!selectedIndex.isValid() || docModel->isCatalog(selectedIndex))
    {
        return false;
    }

    return true;
}

bool ExportPDF::persistsDocument(DocumentProxy *pDocumentProxy, const QString &filename)
{
    QPdfWriter pdfWriter(filename);

    qDebug() << "exporting document to PDF" << filename;

    pdfWriter.setResolution(Settings::settings()->pdfResolution->get().toInt());
    pdfWriter.setPageMargins(QMarginsF());
    pdfWriter.setTitle(pDocumentProxy->name());
    pdfWriter.setCreator("TianBoard PDF export");

    // 场景坐标是像素，PDF 页面用点：按物理 DPI 换算成 1/72 英寸。
    VirtualDesktop *desktop     = VirtualDesktop::instance();
    int             dpiCommon   = desktop->physicalDpi();
    float           scaleFactor = 72.0f / dpiCommon;

    QPainter pdfPainter;
    bool     painterNeedsBegin = true;

    int existingPageCount      = pDocumentProxy->pageCount();
    int renderedPages          = 0;

    for (int pageIndex = 0; pageIndex < existingPageCount; pageIndex++)
    {
        GraphicsScene *scene = documentStore()->loadDocumentScene(pDocumentProxy, pageIndex);
        UserNotices::presentStatusMessage(tr("Exporting page %1 of %2").arg(pageIndex + 1).arg(existingPageCount));

        if (!scene)
        {
            // 文档声称有这一页但盘上没有可读的页文件：跳过。为了让它"存在"而补一页就等于
            // 在导出时改用户的文档（loadDocumentScene 过去就是这么干的，页数越导越胖）。
            continue;
        }

        // 导出页不保留深色/网格背景：PDF 阅读器默认白底，深色背景会掩盖笔迹。
        bool           isDark         = scene->isDarkBackground();
        PageBackground pageBackground = scene->pageBackground();
        scene->setBackground(false, PageBackground::plain);

        // 输出页尺寸等于场景边界；超出边界的内容由 render() 按 KeepAspectRatio 缩放。
        QSize pageSize = scene->sceneSize();

        // 非屏幕渲染 + 高质量，避免导出结果带上为屏幕做的降采样。
        scene->setRenderingQuality(Item::RenderingQualityHigh);
        scene->setRenderingContext(GraphicsScene::NonScreen);

        // QPdfWriter 的页面尺寸单位是点，因此把换算后的像素尺寸当 Point 传入。
        QPageSize outputPageSize = QPageSize(QSizeF(pageSize.width() * scaleFactor, pageSize.height() * scaleFactor), QPageSize::Point);
        pdfWriter.setPageSize(outputPageSize);

        // QPainter 只能 begin 一次；后续页通过 newPage() 切页，
        // painterNeedsBegin 同时充当“begin 是否失败”的标志。
        if (painterNeedsBegin)
        {
            painterNeedsBegin = !pdfPainter.begin(&pdfWriter);
        }

        else if (pageIndex < existingPageCount)
        {
            pdfWriter.newPage();
        }

        // 用 normalizedSceneRect 保证内容完整落在页面内，不按 1:1 裁切。
        scene->render(&pdfPainter, QRectF(), scene->normalizedSceneRect());

        // 恢复屏幕渲染参数，否则后续交互仍停留在非屏幕模式。
        scene->setRenderingContext(GraphicsScene::Screen);
        scene->setRenderingQuality(Item::RenderingQualityNormal);

        // 背景是临时改的，导出后恢复原状，避免屏幕上出现“白底”闪变。
        scene->setBackground(isDark, pageBackground);

        renderedPages++;
    }

    if (!painterNeedsBegin)
    {
        pdfPainter.end();
    }

    // 一页都没画出来时不能只留一个空文件给用户：文档声称有页而盘上都没有可读的页文件，
    // 这件事只有导出器自己知道（读不出页也不许为了凑数去改文档，见 loadDocumentScene）。
    if (renderedPages == 0)
    {
        UserNotices::presentStatusMessage(tr("This document has no readable page; the PDF contains nothing"), false);
    }

    return true;
}

QString ExportPDF::exportExtention()
{
    return QString(".pdf");
}

QString ExportPDF::exportName()
{
    return tr("Export to PDF");
}
