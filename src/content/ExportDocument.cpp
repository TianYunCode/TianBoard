#include "content/ExportDocument.h"

#include "platform/PlatformUtils.h"

#include "core/DocumentManager.h"
#include "core/Application.h"

#include "model/DocumentProxy.h"
#include "model/DocumentController.h"

#include "utils/Globals.h"

THIRD_PARTY_WARNINGS_DISABLE
#include "quazip.h"
#include "quazipfile.h"
THIRD_PARTY_WARNINGS_ENABLE


ExportDocument::ExportDocument(QObject *parent)
    : ExportAdaptor(parent)
{
        ExportDocument::tr("Page"); // dummy slot for translation
}

ExportDocument::~ExportDocument()
{
    // NOOP
}

void ExportDocument::persist(DocumentProxy* pDocumentProxy)
{
    persistLocally(pDocumentProxy, tr("Export as TianBoard Document"));
}


bool ExportDocument::persistsDocument(DocumentProxy* pDocumentProxy, const QString &filename)
{
    QuaZip zip(filename);
    zip.setFileNameCodec("UTF-8");
    if(!zip.open(QuaZip::mdCreate))
    {
        qWarning("Export failed. Cause: zip.open(): %d", zip.getZipError());
        return false;
    }

    QDir documentDir = QDir(pDocumentProxy->persistencePath());

    QuaZipFile outFile(&zip);
    FileSystemUtils::compressDirInZip(documentDir, "", &outFile, true, this);

    zip.close();

    if(zip.getZipError() != 0)
    {
        qWarning("Export failed. Cause: zip.close(): %d", zip.getZipError());
        return false;
    }


    PlatformUtils::setFileType(filename, 0x5542647A /* UBdz */);

    return true;
}


void ExportDocument::processing(const QString& pObjectName, int pCurrent, int pTotal)
{
    QString localized = ExportDocument::tr(pObjectName.toUtf8());

    if (mIsVerbose)
        Application::showMessage(tr("Exporting %1 %2 of %3").arg(localized).arg(pCurrent).arg(pTotal));
}



QString ExportDocument::exportExtention()
{
    return QString(".tbz");
}

QString ExportDocument::exportName()
{
    return tr("Export to TianBoard Format");
}

bool ExportDocument::associatedActionactionAvailableFor(const QModelIndex &selectedIndex)
{
    const DocumentTreeModel *docModel = qobject_cast<const DocumentTreeModel*>(selectedIndex.model());
    if (!selectedIndex.isValid() || docModel->isCatalog(selectedIndex)) {
        return false;
    }

    return true;
}
