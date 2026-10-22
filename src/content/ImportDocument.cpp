#include "content/ImportDocument.h"
#include "model/IDocumentStore.h"
#include "utils/UserNotices.h"
#include "model/DocumentProxy.h"

#include "utils/FileSystemUtils.h"

#include "settings/Settings.h"

#include "utils/Globals.h"

THIRD_PARTY_WARNINGS_DISABLE
#include "quazip.h"
#include "quazipfile.h"
#include "quazipfileinfo.h"
THIRD_PARTY_WARNINGS_ENABLE

ImportDocument::ImportDocument(QObject *parent) : DocumentBasedImportAdaptor(parent)
{
}

ImportDocument::~ImportDocument()
{
}

QStringList ImportDocument::supportedExtentions()
{
    return QStringList("tbz");
}

QString ImportDocument::importFileFilter()
{
    return tr("TianBoard (*.tbz)");
}

bool ImportDocument::extractFileToDir(const QFile &pZipFile, const QString &pDir, QString &documentRoot)
{
    QDir   rootDir(pDir);
    QuaZip zip(pZipFile.fileName());

    if (!zip.open(QuaZip::mdUnzip))
    {
        qWarning() << "Import failed. Cause zip.open(): " << zip.getZipError();
        return false;
    }

    zip.setFileNameCodec("UTF-8");
    QuaZipFileInfo info;
    QuaZipFile     file(&zip);

    QFile out;
    char  c;
    documentRoot = documentStore()->generateUniqueDocumentPath(pDir);
    for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile())
    {
        if (!zip.getCurrentFileInfo(&info))
        {
            qWarning() << "Import failed. Cause: getCurrentFileInfo(): " << zip.getZipError();
            return false;
        }

        if (!file.open(QIODevice::ReadOnly))
        {
            qWarning() << "Import failed. Cause: file.open(): " << zip.getZipError();
            return false;
        }

        if (file.getZipError() != UNZ_OK)
        {
            qWarning() << "Import failed. Cause: file.getFileName(): " << zip.getZipError();
            return false;
        }

        QString   newFileName = documentRoot + "/" + file.getActualFileName();
        QFileInfo newFileInfo(newFileName);
        if (!rootDir.mkpath(newFileInfo.absolutePath()))
        {
            return false;
        }

        out.setFileName(newFileName);
        if (!out.open(QIODevice::WriteOnly))
        {
            return false;
        }

        QByteArray outFileContent = file.readAll();
        if (out.write(outFileContent) == -1)
        {
            qWarning() << "Import failed. Cause: Unable to write file";
            out.close();
            return false;
        }

        while (file.getChar(&c))
        {
            out.putChar(c);
        }

        out.close();

        if (file.getZipError() != UNZ_OK)
        {
            qWarning() << "Import failed. Cause: " << zip.getZipError();
            return false;
        }

        if (!file.atEnd())
        {
            qWarning() << "Import failed. Cause: read all but not EOF";
            return false;
        }

        file.close();

        if (file.getZipError() != UNZ_OK)
        {
            qWarning() << "Import failed. Cause: file.close(): " << file.getZipError();
            return false;
        }
    }

    zip.close();

    if (zip.getZipError() != UNZ_OK)
    {
        qWarning() << "Import failed. Cause: zip.close(): " << zip.getZipError();
        return false;
    }

    return true;
}

DocumentProxy *ImportDocument::importFile(const QFile &pFile, const QString &pGroup)
{
    Q_UNUSED(pGroup);

    QFileInfo fi(pFile);
    UserNotices::presentStatusMessage(tr("Importing file %1...").arg(fi.baseName()), true);

    QString path = Settings::userDocumentDirectory();

    QString documentRootFolder;

    if (!extractFileToDir(pFile, path, documentRootFolder))
    {
        UserNotices::presentStatusMessage(tr("Import of file %1 failed.").arg(fi.baseName()));
        return NULL;
    }

    DocumentProxy *newDocument = documentStore()->createDocumentFromDir(documentRootFolder, pGroup, "", false, false, true);
    UserNotices::presentStatusMessage(tr("Import successful."));
    return newDocument;
}

bool ImportDocument::addFileToDocument(DocumentProxy *pDocument, const QFile &pFile)
{
    QFileInfo fi(pFile);
    UserNotices::presentStatusMessage(tr("Importing file %1...").arg(fi.baseName()), true);

    QString path = FileSystemUtils::createTempDir();

    QString documentRootFolder;
    if (!extractFileToDir(pFile, path, documentRootFolder))
    {
        UserNotices::presentStatusMessage(tr("Import of file %1 failed.").arg(fi.baseName()));
        return false;
    }

    if (!documentStore()->addDirectoryContentToDocument(documentRootFolder, pDocument))
    {
        UserNotices::presentStatusMessage(tr("Import of file %1 failed.").arg(fi.baseName()));
        return false;
    }

    FileSystemUtils::deleteDir(path);

    UserNotices::presentStatusMessage(tr("Import successful."));

    return true;
}
