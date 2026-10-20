#include "core/DocumentManager.h"

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
#include "ui/MessagesDialog.h"
#include "ui/MainWindow.h"

#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsSvgItem.h"
#include "whiteboard/GraphicsPixmapItem.h"

#include "model/DocumentProxy.h"
#include "model/DocumentController.h"
#include "whiteboard/BoardController.h"

#include "core/Application.h"
#include "settings/Settings.h"
#include "core/PersistenceManager.h"

#include "content/ExportWeb.h"


DocumentManager *DocumentManager::sDocumentManager = 0;

DocumentManager *DocumentManager::documentManager() {
    if (!sDocumentManager) {
        sDocumentManager = new DocumentManager(qApp);
    }
    return sDocumentManager;
}


DocumentManager::DocumentManager(QObject *parent) : QObject(parent) {
    QString dummyImages = tr("images");
    QString dummyVideos = tr("videos");
    QString dummyObjects = tr("objects");
    QString dummyWidgets = tr("widgets");

    //ExportIwb* iwbExporter = new ExportIwb(this);
    ExportFullPDF *exportFullPdf = new ExportFullPDF(this);
    ExportDocument *exportDocument = new ExportDocument(this);

    ExportDocumentSetAdaptor *exportDocumentSet = new ExportDocumentSetAdaptor(this);
    mExportAdaptors.append(exportDocument);
    mExportAdaptors.append(exportDocumentSet);
    //mExportAdaptors.append(webPublished);
    mExportAdaptors.append(exportFullPdf);
    //mExportAdaptors.append(iwbExporter);

    //     ExportWeb* exportWeb = new ExportWeb(this);
    //     mExportAdaptors.append(exportWeb);

    // 导出器只回报警告，弹窗这件事留在认识界面的这一层。
    for (ExportAdaptor *adaptor : mExportAdaptors) {
        connect(adaptor, &ExportAdaptor::exportWarnings, this,
                [](const QString &title, const QList<QString> &messages) {
                    MessagesDialog *dialog = new MessagesDialog(title, Application::mainWindow);
                    // 不设这个属性的话，每次导出警告都会在主窗口下面攒一个关不掉的对话框对象。
                    dialog->setAttribute(Qt::WA_DeleteOnClose);
                    dialog->setMessages(messages);
                    dialog->show();
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


DocumentManager::~DocumentManager() {
    // NOOP
}


QStringList DocumentManager::importFileExtensions(bool notUbx) {
    QStringList result;

    foreach (ImportAdaptor *importAdaptor, mImportAdaptors) {
        //issue 1629 - NNE - 20131213 : add test to remove tbx extention if necessary
        if (!(notUbx && importAdaptor->supportedExtentions().at(0) == "tbx")) {
            result << importAdaptor->supportedExtentions();
        }
    }
    return result;
}


QString DocumentManager::importFileFilter(bool notUbx) {
    QString result;

    result += tr("All supported files (*.%1)").arg(importFileExtensions(notUbx).join(" *."));
    foreach (ImportAdaptor *importAdaptor, mImportAdaptors) {
        if (importAdaptor->importFileFilter().length() > 0) {
            //issue 1629 - NNE - 20131213 : Add a test on tbx before put in the list
            if (!(notUbx && importAdaptor->supportedExtentions().at(0) == "tbx")) {
                if (result.length()) {
                    result += ";;";
                }

                result += importAdaptor->importFileFilter();
            }
        }
    }
    qDebug() << "import file filter" << result;
    return result;
}

QFileInfoList DocumentManager::importUbx(const QString &Incomingfile, const QString &destination) {
    ImportDocumentSetAdaptor *docSetAdaptor;
    foreach (ImportAdaptor *curAdaptor, mImportAdaptors) {
        docSetAdaptor = qobject_cast<ImportDocumentSetAdaptor *>(curAdaptor);
        if (docSetAdaptor) {
            break;
        }
    }
    if (!docSetAdaptor) {
        return QFileInfoList();
    }

    return docSetAdaptor->importData(Incomingfile, destination);
}

DocumentProxy *DocumentManager::importFile(const QFile &pFile, const QString &pGroup) {
    QFileInfo fileInfo(pFile);

    foreach (ImportAdaptor *adaptor, mImportAdaptors) {
        if (adaptor->supportedExtentions().lastIndexOf(fileInfo.suffix().toLower()) != -1) {
            DocumentProxy *document;
            Application::setDisabled(true);

            if (adaptor->isDocumentBased()) {
                DocumentBasedImportAdaptor *importAdaptor = (DocumentBasedImportAdaptor *)adaptor;

                document = importAdaptor->importFile(pFile, pGroup);

            } else {
                PageBasedImportAdaptor *importAdaptor = (PageBasedImportAdaptor *)adaptor;

                // Document import procedure.....
                QString documentName = QFileInfo(pFile.fileName()).completeBaseName();
                document = PersistenceManager::persistenceManager()->createDocument(
                    pGroup, documentName,
                    false // Issue 1630 - CFA - 201410503 - suppression de la page vide ajoutee à l'import des pdfs
                    ,
                    QString(), 0, true);
                if (document) {
                    QUuid uuid = QUuid::createUuid();
                    QString filepath = pFile.fileName();
                    if (importAdaptor->folderToCopy() != "") {
                        bool b = PersistenceManager::persistenceManager()->addFileToDocument(
                            document, pFile.fileName(), importAdaptor->folderToCopy(), uuid, filepath);
                        if (!b) {
                            Application::setDisabled(false);
                            return NULL;
                        }
                    }

                    QList<GraphicsItem *> pages = importAdaptor->import(uuid, filepath);
                    int nPage = 0;
                    foreach (GraphicsItem *page, pages) {
                        Application::showMessage(tr("Inserting page %1 of %2").arg(++nPage).arg(pages.size()),
                                                 true);

                        int pageIndex = document->pageCount();
                        GraphicsScene *scene =
                            PersistenceManager::persistenceManager()->createDocumentSceneAt(document,
                                                                                            pageIndex);
                        importAdaptor->placeImportedItemToScene(scene, page);
                        PersistenceManager::persistenceManager()->persistDocumentScene(document, scene,
                                                                                       pageIndex);
                    }

                    PersistenceManager::persistenceManager()->persistDocumentMetadata(document);
                    Application::showMessage(tr("Import successful."));
                }
            }

            Application::setDisabled(false);
            return document;
        }
    }
    return NULL;
}


int DocumentManager::addFilesToDocument(DocumentProxy *document, QStringList fileNames) {
    int nImportedDocuments = 0;
    foreach (const QString &fileName, fileNames) {
        Application::showMessage(tr("Importing file %1").arg(fileName));

        QFile file(fileName);
        QFileInfo fileInfo(file);

        foreach (ImportAdaptor *adaptor, mImportAdaptors) {
            if (adaptor->supportedExtentions().lastIndexOf(fileInfo.suffix().toLower()) != -1) {
                Application::setDisabled(true);

                if (adaptor->isDocumentBased()) {
                    //issue 1629 - NNE - 20131212 : Resolve a segfault, but for .tbx, actually
                    //the file will be not imported...
                    DocumentBasedImportAdaptor *importAdaptor =
                        dynamic_cast<DocumentBasedImportAdaptor *>(adaptor);

                    if (importAdaptor && importAdaptor->addFileToDocument(document, file))
                        nImportedDocuments++;
                } else {
                    PageBasedImportAdaptor *importAdaptor = (PageBasedImportAdaptor *)adaptor;

                    QUuid uuid = QUuid::createUuid();
                    QString filepath = file.fileName();
                    if (importAdaptor->folderToCopy() != "") {
                        bool b = PersistenceManager::persistenceManager()->addFileToDocument(
                            document, file.fileName(), importAdaptor->folderToCopy(), uuid, filepath);
                        if (!b) {
                            continue;
                        }
                    }

                    QList<GraphicsItem *> pages = importAdaptor->import(uuid, filepath);
                    int nPage = 0;
                    foreach (GraphicsItem *page, pages) {
                        Application::showMessage(tr("Inserting page %1 of %2").arg(++nPage).arg(pages.size()),
                                                 true);
                        int pageIndex = document->pageCount();
                        GraphicsScene *scene =
                            PersistenceManager::persistenceManager()->createDocumentSceneAt(document,
                                                                                            pageIndex);
                        importAdaptor->placeImportedItemToScene(scene, page);
                        PersistenceManager::persistenceManager()->persistDocumentScene(document, scene,
                                                                                       pageIndex);
                        Application::boardController->insertThumbPage(pageIndex);
                    }

                    PersistenceManager::persistenceManager()->persistDocumentMetadata(document);
                    Application::showMessage(tr("Import of file %1 successful.").arg(file.fileName()));
                    nImportedDocuments++;
                }

                Application::setDisabled(false);
            }
        }
    }
    return nImportedDocuments;
}


int DocumentManager::addImageDirToDocument(const QDir &pDir, DocumentProxy *pDocument) {
    QStringList filenames = pDir.entryList(QDir::Files | QDir::NoDotAndDotDot);

    filenames = StringUtils::sortByLastDigit(filenames);

    QStringList fileNames;

    foreach (QString f, filenames) {
        fileNames << pDir.absolutePath() + "/" + f;
    }

    return addFilesToDocument(pDocument, fileNames);
}


DocumentProxy *DocumentManager::importDir(const QDir &pDir, const QString &pGroup) {
    DocumentProxy *doc = PersistenceManager::persistenceManager()->createDocument(pGroup, pDir.dirName());

    int result = addImageDirToDocument(pDir, doc);

    if (result > 0) {
        doc->setMetaData(Settings::documentGroupName, pGroup);
        doc->setMetaData(Settings::documentName, pDir.dirName());

        PersistenceManager::persistenceManager()->persistDocumentMetadata(doc);

        Application::showMessage(tr("File %1 saved").arg(pDir.dirName()));

    } else {
        PersistenceManager::persistenceManager()->deleteDocument(doc);
    }

    return doc;
}


QList<ExportAdaptor *> DocumentManager::supportedExportAdaptors() {
    return mExportAdaptors;
}

void DocumentManager::emitDocumentUpdated(DocumentProxy *pDocument) {
    emit documentUpdated(pDocument);
}
