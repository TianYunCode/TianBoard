#include "core/DocumentManager.h"
#include "utils/UserNotices.h"

#include "utils/StringUtils.h"

#include "content/ExportFullPDF.h"
#include "content/ExportAdaptor.h"
#include "content/ExportDocument.h"
#include "content/ExportWeb.h"
#include "content/ExportIwb.h"
#include "content/ExportDocumentSetAdaptor.h"
#include "content/ImportDocument.h"
#include "content/ImportPDF.h"
#include "content/ImportImage.h"
#include "content/ImportIwb.h"
#include "content/ImportDocumentSetAdaptor.h"

#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsSvgItem.h"
#include "whiteboard/GraphicsPixmapItem.h"

#include "model/DocumentProxy.h"
#include "whiteboard/BoardController.h"

#include "core/Application.h"
#include "settings/Settings.h"
#include "core/PersistenceManager.h"

#include "content/ExportWeb.h"

DocumentManager *DocumentManager::sDocumentManager = 0;

DocumentManager *DocumentManager::documentManager()
{
    if (!sDocumentManager)
    {
        sDocumentManager = new DocumentManager(qApp);
    }
    return sDocumentManager;
}

DocumentManager::DocumentManager(QObject *parent) : QObject(parent)
{
    ExportFullPDF  *exportFullPdf               = new ExportFullPDF(this);
    ExportDocument *exportDocument              = new ExportDocument(this);

    ExportDocumentSetAdaptor *exportDocumentSet = new ExportDocumentSetAdaptor(this);
    mExportAdaptors.append(exportDocument);
    mExportAdaptors.append(exportDocumentSet);
    mExportAdaptors.append(exportFullPdf);

    // 导出器只回报警告，弹窗这件事留在认识界面的这一层。
    for (ExportAdaptor *adaptor : mExportAdaptors)
    {
        connect(adaptor, &ExportAdaptor::exportWarnings, this,
            [](const QString &title, const QList<QString> &messages)
            {
                UserNotices::presentWarnings(title, messages);
            });
    }

    ImportDocument *documentImport = new ImportDocument(this);
    mImportAdaptors.append(documentImport);
    ImportDocumentSetAdaptor *documentSetImport = new ImportDocumentSetAdaptor(this);
    mImportAdaptors.append(documentSetImport);
    ImportPDF *pdfImport = new ImportPDF(this);
    mImportAdaptors.append(pdfImport);
    ImportImage *imageImport = new ImportImage(this);
    mImportAdaptors.append(imageImport);
    ImportIwb *iwbImport = new ImportIwb(this);
    mImportAdaptors.append(iwbImport);
}

DocumentManager::~DocumentManager()
{
}

QStringList DocumentManager::importFileExtensions(bool notUbx)
{
    QStringList result;

    foreach (ImportAdaptor *importAdaptor, mImportAdaptors)
    {
        // 某些场景下要排除文档集（tbx）格式，此时跳过以它为首要扩展名的适配器。
        if (!(notUbx && importAdaptor->supportedExtentions().at(0) == "tbx"))
        {
            result << importAdaptor->supportedExtentions();
        }
    }
    return result;
}

QString DocumentManager::importFileFilter(bool notUbx)
{
    QString result;

    result += tr("All supported files (*.%1)").arg(importFileExtensions(notUbx).join(" *."));
    foreach (ImportAdaptor *importAdaptor, mImportAdaptors)
    {
        if (importAdaptor->importFileFilter().length() > 0)
        {
            // 与扩展名列表一致，这里也要按需排除文档集格式。
            if (!(notUbx && importAdaptor->supportedExtentions().at(0) == "tbx"))
            {
                if (result.length())
                {
                    result += ";;";
                }

                result += importAdaptor->importFileFilter();
            }
        }
    }
    qDebug() << "import file filter" << result;
    return result;
}

QFileInfoList DocumentManager::importUbx(const QString &Incomingfile, const QString &destination)
{
    // 必须显式初始化：适配器列表为空时循环体一次都不执行，下面的 !docSetAdaptor 判断
    // 读的就是未初始化值，会拿一个随机指针去调 importData。
    ImportDocumentSetAdaptor *docSetAdaptor = nullptr;
    foreach (ImportAdaptor *curAdaptor, mImportAdaptors)
    {
        docSetAdaptor = qobject_cast<ImportDocumentSetAdaptor *>(curAdaptor);
        if (docSetAdaptor)
        {
            break;
        }
    }
    if (!docSetAdaptor)
    {
        return QFileInfoList();
    }

    return docSetAdaptor->importData(Incomingfile, destination);
}

DocumentProxy *DocumentManager::importFile(const QFile &pFile, const QString &pGroup)
{
    QFileInfo fileInfo(pFile);

    foreach (ImportAdaptor *adaptor, mImportAdaptors)
    {
        if (adaptor->supportedExtentions().lastIndexOf(fileInfo.suffix().toLower()) != -1)
        {
            DocumentProxy *document;
            Application::setDisabled(true);

            if (adaptor->isDocumentBased())
            {
                DocumentBasedImportAdaptor *importAdaptor = (DocumentBasedImportAdaptor *)adaptor;

                document                                  = importAdaptor->importFile(pFile, pGroup);
            }
            else
            {
                PageBasedImportAdaptor *importAdaptor = (PageBasedImportAdaptor *)adaptor;

                // 页面型导入：先建文档，再把每一页依次落到场景中。
                QString documentName                  = QFileInfo(pFile.fileName()).completeBaseName();
                document                              = PersistenceManager::persistenceManager()->createDocument(pGroup, documentName,
                    false // 导入 PDF 时不再补空白页
                    ,
                    QString(), 0, true);
                if (document)
                {
                    QUuid   uuid     = QUuid::createUuid();
                    QString filepath = pFile.fileName();
                    if (importAdaptor->folderToCopy() != "")
                    {
                        bool b = PersistenceManager::persistenceManager()->addFileToDocument(
                            document, pFile.fileName(), importAdaptor->folderToCopy(), uuid, filepath);
                        if (!b)
                        {
                            Application::setDisabled(false);
                            return NULL;
                        }
                    }

                    QList<GraphicsItem *> pages = importAdaptor->import(uuid, filepath);
                    int                   nPage = 0;
                    foreach (GraphicsItem *page, pages)
                    {
                        UserNotices::presentStatusMessage(tr("Inserting page %1 of %2").arg(++nPage).arg(pages.size()), true);

                        int            pageIndex = document->pageCount();
                        GraphicsScene *scene     = PersistenceManager::persistenceManager()->createDocumentSceneAt(document, pageIndex);
                        importAdaptor->placeImportedItemToScene(scene, page);
                        PersistenceManager::persistenceManager()->persistDocumentScene(document, scene, pageIndex);
                    }

                    PersistenceManager::persistenceManager()->persistDocumentMetadata(document);
                    UserNotices::presentStatusMessage(tr("Import successful."));
                }
            }

            Application::setDisabled(false);
            return document;
        }
    }
    return NULL;
}

int DocumentManager::addFilesToDocument(DocumentProxy *document, QStringList fileNames)
{
    int nImportedDocuments = 0;
    foreach (const QString &fileName, fileNames)
    {
        UserNotices::presentStatusMessage(tr("Importing file %1").arg(fileName));

        QFile     file(fileName);
        QFileInfo fileInfo(file);

        foreach (ImportAdaptor *adaptor, mImportAdaptors)
        {
            if (adaptor->supportedExtentions().lastIndexOf(fileInfo.suffix().toLower()) != -1)
            {
                Application::setDisabled(true);

                if (adaptor->isDocumentBased())
                {
                    // 用 dynamic_cast 再判空：文档集格式在这里可能并不支持追加，
                    // 直接按类型转换后调用会导致崩溃。
                    DocumentBasedImportAdaptor *importAdaptor = dynamic_cast<DocumentBasedImportAdaptor *>(adaptor);

                    if (importAdaptor && importAdaptor->addFileToDocument(document, file))
                    {
                        nImportedDocuments++;
                    }
                }
                else
                {
                    PageBasedImportAdaptor *importAdaptor = (PageBasedImportAdaptor *)adaptor;

                    QUuid   uuid                          = QUuid::createUuid();
                    QString filepath                      = file.fileName();
                    if (importAdaptor->folderToCopy() != "")
                    {
                        bool b = PersistenceManager::persistenceManager()->addFileToDocument(
                            document, file.fileName(), importAdaptor->folderToCopy(), uuid, filepath);
                        if (!b)
                        {
                            // 这里 continue 会绕过循环末尾的 setDisabled(false)，
                            // 界面从此一直处于禁用状态。
                            Application::setDisabled(false);
                            continue;
                        }
                    }

                    QList<GraphicsItem *> pages = importAdaptor->import(uuid, filepath);
                    int                   nPage = 0;
                    foreach (GraphicsItem *page, pages)
                    {
                        UserNotices::presentStatusMessage(tr("Inserting page %1 of %2").arg(++nPage).arg(pages.size()), true);
                        int            pageIndex = document->pageCount();
                        GraphicsScene *scene     = PersistenceManager::persistenceManager()->createDocumentSceneAt(document, pageIndex);
                        importAdaptor->placeImportedItemToScene(scene, page);
                        PersistenceManager::persistenceManager()->persistDocumentScene(document, scene, pageIndex);
                        Application::boardController->insertThumbPage(pageIndex);
                    }

                    PersistenceManager::persistenceManager()->persistDocumentMetadata(document);
                    UserNotices::presentStatusMessage(tr("Import of file %1 successful.").arg(file.fileName()));
                    nImportedDocuments++;
                }

                Application::setDisabled(false);
            }
        }
    }
    return nImportedDocuments;
}

int DocumentManager::addImageDirToDocument(const QDir &pDir, DocumentProxy *pDocument)
{
    QStringList filenames = pDir.entryList(QDir::Files | QDir::NoDotAndDotDot);

    filenames             = StringUtils::sortByLastDigit(filenames);

    QStringList fileNames;

    foreach (QString f, filenames)
    {
        fileNames << pDir.absolutePath() + "/" + f;
    }

    return addFilesToDocument(pDocument, fileNames);
}

DocumentProxy *DocumentManager::importDir(const QDir &pDir, const QString &pGroup)
{
    DocumentProxy *doc = PersistenceManager::persistenceManager()->createDocument(pGroup, pDir.dirName());

    int result         = addImageDirToDocument(pDir, doc);

    if (result > 0)
    {
        doc->setMetaData(Settings::documentGroupName, pGroup);
        doc->setMetaData(Settings::documentName, pDir.dirName());

        PersistenceManager::persistenceManager()->persistDocumentMetadata(doc);

        UserNotices::presentStatusMessage(tr("File %1 saved").arg(pDir.dirName()));
    }
    else
    {
        PersistenceManager::persistenceManager()->deleteDocument(doc);
    }

    return doc;
}

int DocumentManager::exportAdaptorCount() const
{
    return mExportAdaptors.size();
}

QString DocumentManager::exportAdaptorName(int index) const
{
    return mExportAdaptors.at(index)->exportName();
}

void DocumentManager::associateExportAction(int index, QAction *action)
{
    mExportAdaptors.at(index)->setAssociatedAction(action);
}

void DocumentManager::exportDocumentWithAdaptor(int index, DocumentProxy *proxy, const QModelIndex &selectedTreeIndex)
{
    mExportAdaptors.at(index)->persist(proxy, selectedTreeIndex);
}

void DocumentManager::refreshExportActions(const QModelIndex &selectedIndex)
{
    for (ExportAdaptor *adaptor : mExportAdaptors)
    {
        if (QAction *action = adaptor->associatedAction())
        {
            action->setEnabled(adaptor->associatedActionactionAvailableFor(selectedIndex));
        }
    }
}
