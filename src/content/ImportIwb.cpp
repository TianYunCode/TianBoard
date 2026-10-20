#include <QDir>
#include <QList>

#include "core/Application.h"
#include "core/PersistenceManager.h"
#include "core/DocumentManager.h"
#include "core/PersistenceManager.h"
#include "model/DocumentProxy.h"
#include "whiteboard/GraphicsPDFItem.h"
#include "utils/FileSystemUtils.h"
#include "whiteboard/PDFRenderer.h"

#include "content/IwbSubsetAdaptor.h"
#include "content/ImportIwb.h"

#include "utils/Globals.h"

THIRD_PARTY_WARNINGS_DISABLE
#include "quazip.h"
#include "quazipfile.h"
#include "quazipfileinfo.h"
THIRD_PARTY_WARNINGS_ENABLE


ImportIwb::ImportIwb(QObject *parent) : DocumentBasedImportAdaptor(parent) {
    // NOOP
}


ImportIwb::~ImportIwb() {
    // NOOP
}


QStringList ImportIwb::supportedExtentions() {
    return QStringList("iwb");
}


QString ImportIwb::importFileFilter() {
    QString filter = tr("Interactive Whiteboard File (");
    QStringList formats = supportedExtentions();
    bool isFirst = true;

    foreach (QString format, formats) {
        if (isFirst)
            isFirst = false;
        else
            filter.append(" ");

        filter.append("*." + format);
    }

    filter.append(")");

    return filter;
}

bool ImportIwb::addFileToDocument(DocumentProxy *pDocument, const QFile &pFile) {
    QFileInfo fi(pFile);
    Application::showMessage(tr("Importing file %1...").arg(fi.baseName()), true);

    // first unzip the file to the correct place
    //TODO create temporary path for iwb file content
    QString path = QDir::tempPath();

    QString documentRootFolder = expandFileToDir(pFile, path);
    QString contentFile;
    if (documentRootFolder.isEmpty()) //if file has failed to unzip it is probably just xml file
        contentFile = pFile.fileName();
    else //get path to content xml (according to iwb specification)
        contentFile = documentRootFolder.append("/content.xml");

    if (!contentFile.length()) {
        Application::showMessage(tr("Import of file %1 failed.").arg(fi.baseName()));
        return false;
    } else {
        //TODO convert expanded IWB file content to the destination document
        //create destination document proxy
        //fill metadata and save
        DocumentProxy *destDocument =
            new DocumentProxy(PersistenceManager::persistenceManager()->generateUniqueDocumentPath());
        QDir dir;
        dir.mkdir(destDocument->persistencePath());

        //try to import iwb to document
        if (IwbSubsetAdaptor::convertIwbFileToDocumentPackage(contentFile, destDocument)) {
            PersistenceManager::persistenceManager()->addDirectoryContentToDocument(
                destDocument->persistencePath(), pDocument);
            FileSystemUtils::deleteDir(destDocument->persistencePath());
            delete destDocument;
            Application::showMessage(tr("Import successful."));
            return true;
        } else {
            FileSystemUtils::deleteDir(destDocument->persistencePath());
            delete destDocument;
            Application::showMessage(tr("Import failed."));
            return false;
        }
    }
}

QString ImportIwb::expandFileToDir(const QFile &pZipFile, const QString &pDir) {
    QuaZip zip(pZipFile.fileName());

    if (!zip.open(QuaZip::mdUnzip)) {
        qWarning() << "Import failed. Cause zip.open(): " << zip.getZipError();
        return "";
    }

    zip.setFileNameCodec("UTF-8");
    QuaZipFileInfo info;
    QuaZipFile file(&zip);

    //create unique iwb document root fodler
    //use current date/time and temp number for folder name
    QString documentRootFolder;
    int tmpNumber = 0;
    QDir rootDir;
    while (true) {
        QString tempPath = QString("%1/importedIwb%2.%3")
                               .arg(pDir)
                               .arg(QDateTime::currentDateTime().toString("dd_MM_yyyy_HH-mm"))
                               .arg(tmpNumber);
        if (!rootDir.exists(tempPath)) {
            documentRootFolder = tempPath;
            break;
        }
        tmpNumber++;
        if (tmpNumber == 100000) {
            qWarning() << "Import failed. Failed to create temporary directory for iwb file";
            return "";
        }
    }
    if (!rootDir.mkdir(documentRootFolder)) {
        qWarning() << "Import failed. Couse: failed to create temp folder for iwb package";
    }

    QFile out;
    char c;
    for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile()) {
        if (!zip.getCurrentFileInfo(&info)) {
            qWarning() << "Import failed. Cause: getCurrentFileInfo(): " << zip.getZipError();
            return "";
        }
        //        if(!file.open(QIODevice::ReadOnly)) {
        //            qWarning() << "Import failed. Cause: file.open(): " << zip.getZipError();
        //            return "";
        //        }
        file.open(QIODevice::ReadOnly);
        if (file.getZipError() != UNZ_OK) {
            qWarning() << "Import failed. Cause: file.getFileName(): " << zip.getZipError();
            return "";
        }

        QString newFileName = documentRootFolder + "/" + file.getActualFileName();

        QFileInfo newFileInfo(newFileName);
        rootDir.mkpath(newFileInfo.absolutePath());

        out.setFileName(newFileName);
        out.open(QIODevice::WriteOnly);

        while (file.getChar(&c))
            out.putChar(c);

        out.close();

        if (file.getZipError() != UNZ_OK) {
            qWarning() << "Import failed. Cause: " << zip.getZipError();
            return "";
        }
        if (!file.atEnd()) {
            qWarning() << "Import failed. Cause: read all but not EOF";
            return "";
        }

        file.close();

        if (file.getZipError() != UNZ_OK) {
            qWarning() << "Import failed. Cause: file.close(): " << file.getZipError();
            return "";
        }
    }

    zip.close();

    if (zip.getZipError() != UNZ_OK) {
        qWarning() << "Import failed. Cause: zip.close(): " << zip.getZipError();
        return "";
    }

    return documentRootFolder;
}


DocumentProxy *ImportIwb::importFile(const QFile &pFile, const QString &pGroup) {
    Q_UNUSED(pGroup); // group is defined in the imported file

    QFileInfo fi(pFile);
    Application::showMessage(tr("Importing file %1...").arg(fi.baseName()), true);

    // first unzip the file to the correct place
    //TODO create temporary path for iwb file content
    QString path = QDir::tempPath();

    QString documentRootFolder = expandFileToDir(pFile, path);
    QString contentFile;
    if (documentRootFolder.isEmpty())
        //if file has failed to umzip it is probably just xml file
        contentFile = pFile.fileName();
    else
        //get path to content xml
        contentFile = QString("%1/content.xml").arg(documentRootFolder);

    if (!contentFile.length()) {
        Application::showMessage(tr("Import of file %1 failed.").arg(fi.baseName()));
        return 0;
    } else {
        //create destination document proxy
        //fill metadata and save
        DocumentProxy *destDocument =
            new DocumentProxy(PersistenceManager::persistenceManager()->generateUniqueDocumentPath());
        QDir dir;
        dir.mkdir(destDocument->persistencePath());
        if (pGroup.length() > 0)
            destDocument->setMetaData(Settings::documentGroupName, pGroup);
        if (!fi.baseName().isEmpty())
            destDocument->setMetaData(Settings::documentName, fi.baseName());

        destDocument->setMetaData(Settings::documentVersion, Settings::currentFileVersion);
        destDocument->setMetaData(Settings::documentUpdatedAt,
                                  StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));

        DocumentProxy *newDocument = NULL;
        //try to import iwb to document
        if (IwbSubsetAdaptor::convertIwbFileToDocumentPackage(contentFile, destDocument)) {
            newDocument = PersistenceManager::persistenceManager()->createDocumentFromDir(
                destDocument->persistencePath(), "", "", false, false, true);

            Application::showMessage(tr("Import successful."));
        } else {
            FileSystemUtils::deleteDir(destDocument->persistencePath());
            Application::showMessage(tr("Import failed."));
        }
        delete destDocument;

        if (documentRootFolder.length() != 0)
            FileSystemUtils::deleteDir(documentRootFolder);
        return newDocument;
    }
}
