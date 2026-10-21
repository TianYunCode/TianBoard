#include "content/ExportIwb.h"
#include "utils/UserNotices.h"
#include "model/DocumentTreeModel.h"
#include "content/IwbDocumentAdaptor.h"
#include "model/DocumentProxy.h"

#include <QModelIndex>
#include <QObject>

ExportIwb::ExportIwb(QObject *parent) : ExportAdaptor(parent)
{
}

ExportIwb::~ExportIwb()
{
}
QString ExportIwb::exportName()
{
    return tr("Export to IWB");
}

QString ExportIwb::exportExtention()
{
    return QString(".iwb");
}

void ExportIwb::persist(DocumentProxy *pDocument, const QModelIndex &selectedTreeIndex)
{
    Q_UNUSED(selectedTreeIndex);

    QString src = pDocument->persistencePath();

    if (!pDocument)
    {
        return;
    }

    QString filename = askForFileName(pDocument, tr("Export as IWB File"));

    if (filename.length() > 0)
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        if (mIsVerbose)
        {
            UserNotices::presentStatusMessage(tr("Exporting document..."));
        }

        IwbDocumentAdaptor toIWBExporter;
        if (toIWBExporter.convertPageToIwb(src, filename))
        {
            if (mIsVerbose)
            {
                UserNotices::presentStatusMessage(tr("Export successful."));
            }
        }
        else if (mIsVerbose)
        {
            UserNotices::presentStatusMessage(tr("Export failed."));
        }

        reportErrorsList(toIWBExporter.getConversionMessages());

        QApplication::restoreOverrideCursor();
    }
}

bool ExportIwb::associatedActionactionAvailableFor(const QModelIndex &selectedIndex)
{
    const DocumentTreeModel *docModel = qobject_cast<const DocumentTreeModel *>(selectedIndex.model());
    if (!selectedIndex.isValid() || docModel->isCatalog(selectedIndex))
    {
        return false;
    }

    return true;
}
