#include "content/ExportDocumentSetAdaptor.h"
#include "model/DocumentController.h"
#include "model/DocumentTreeModel.h"
#include "content/ExportDocument.h"

#include "platform/PlatformUtils.h"

#include "core/DocumentManager.h"
#include "core/Application.h"

#include "model/DocumentProxy.h"

#include "utils/Globals.h"
#include "core/PersistenceManager.h"
#include "core/ForeignObjectsHandler.h"

THIRD_PARTY_WARNINGS_DISABLE
#include "quazip.h"
#include "quazipfile.h"
THIRD_PARTY_WARNINGS_ENABLE

ExportDocumentSetAdaptor::ExportDocumentSetAdaptor(QObject *parent) : ExportAdaptor(parent)
{
}

ExportDocumentSetAdaptor::~ExportDocumentSetAdaptor()
{
}

void ExportDocumentSetAdaptor::persist(DocumentProxy *pDocumentProxy)
{
    QModelIndex         treeViewParentIndex;
    PersistenceManager *persistenceManager = PersistenceManager::persistenceManager();
    DocumentTreeModel  *treeModel          = persistenceManager->mDocumentTreeStructureModel;
    QString             filename;

    if (pDocumentProxy)
    {
        treeViewParentIndex = treeModel->indexForProxy(pDocumentProxy);
        if (!treeViewParentIndex.isValid())
        {
            qDebug() << "failed to export";
            Application::showMessage(tr("Failed to export..."));
            return;
        }
        filename = askForFileName(pDocumentProxy, tr("Export as TianBoard Document Set"));
    }
    else
    {
        treeViewParentIndex = Application::documentController->firstSelectedTreeIndex();
        if (!treeViewParentIndex.isValid())
        {
            qDebug() << "failed to export";
            Application::showMessage(tr("Failed to export..."));
            return;
        }

        DocumentTreeNode *node = treeModel->nodeFromIndex(treeViewParentIndex);
        DocumentProxy     proxy;
        proxy.setMetaData(Settings::documentName, node->displayName());
        filename = askForFileName(&proxy, tr("Export as TianBoard Document Set"));
    }

    if (filename.length() > 0)
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        if (mIsVerbose)
        {
            Application::showMessage(tr("Exporting document..."));
        }

        if (persistData(treeViewParentIndex, filename))
        {
            if (mIsVerbose)
            {
                Application::showMessage(tr("Export successful."));
            }
        }
        else
        {
            if (mIsVerbose)
            {
                Application::showMessage(tr("Export failed."));
            }
        }

        QApplication::restoreOverrideCursor();
    }
}

bool ExportDocumentSetAdaptor::persistData(const QModelIndex &pRootIndex, QString filename)
{
    PersistenceManager *persistenceManager = PersistenceManager::persistenceManager();
    DocumentTreeModel  *treeModel          = persistenceManager->mDocumentTreeStructureModel;

    QModelIndex index                      = pRootIndex;

    if (!index.isValid())
    {
        return false;
    }

    QuaZip zip(filename);
    zip.setFileNameCodec("UTF-8");

    if (!zip.open(QuaZip::mdCreate))
    {
        qWarning("Export failed. Cause: zip.open(): %d", zip.getZipError());
        return false;
    }

    if (!addDocumentToZip(pRootIndex, treeModel, zip))
    {
        zip.close();
        return false;
    }

    zip.close();

    return true;
}

QString ExportDocumentSetAdaptor::exportExtention()
{
    return QString(".tbx");
}

QString ExportDocumentSetAdaptor::exportName()
{
    return tr("Export to TianBoard Document Set Format");
}

bool ExportDocumentSetAdaptor::addDocumentToZip(const QModelIndex &pIndex, DocumentTreeModel *model, QuaZip &zip)
{
    QModelIndex parentIndex = pIndex;
    if (!parentIndex.isValid())
    {
        return false;
    }

    DocumentProxy *pDocumentProxy = model->proxyForIndex(parentIndex);
    if (pDocumentProxy)
    {
        QString documentPath(pDocumentProxy->persistencePath());

        // 每个文档目录以自身目录名作为包内前缀，子文档因此天然落在父目录之下。
        QDir       documentDir = QDir(pDocumentProxy->persistencePath());
        QuaZipFile zipFile(&zip);
        FileSystemUtils::compressDirInZip(documentDir, QFileInfo(documentPath).fileName() + "/", &zipFile, false);

        if (zip.getZipError() != 0)
        {
            qWarning("Export failed. Cause: zip.close(): %d", zip.getZipError());
        }
    }

    for (int i = 0; i < model->rowCount(parentIndex); ++i)
    {
        QModelIndex curIndex = model->index(i, 0, parentIndex);
        if (!addDocumentToZip(curIndex, model, zip))
        {
            return false;
        }
    }

    return true;
}

bool ExportDocumentSetAdaptor::associatedActionactionAvailableFor(const QModelIndex &selectedIndex)
{
    const DocumentTreeModel *docModel = qobject_cast<const DocumentTreeModel *>(selectedIndex.model());
    if (!selectedIndex.isValid() || docModel->isDocument(selectedIndex))
    {
        return false;
    }

    return true;
}
