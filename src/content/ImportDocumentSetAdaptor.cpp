#include "content/ImportDocumentSetAdaptor.h"
#include "model/IDocumentStore.h"

#include "model/DocumentProxy.h"

#include "utils/FileSystemUtils.h"

#include "settings/Settings.h"

#include "utils/Globals.h"

THIRD_PARTY_WARNINGS_DISABLE
#include "quazip.h"
#include "quazipfile.h"
#include "quazipfileinfo.h"
THIRD_PARTY_WARNINGS_ENABLE

ImportDocumentSetAdaptor::ImportDocumentSetAdaptor(QObject *parent) : ImportAdaptor(parent)
{
}

ImportDocumentSetAdaptor::~ImportDocumentSetAdaptor()
{
}

QStringList ImportDocumentSetAdaptor::supportedExtentions()
{
    return QStringList("tbx");
}

QString ImportDocumentSetAdaptor::importFileFilter()
{
    return tr("TianBoard Document Set (*.tbx)");
}

QFileInfoList ImportDocumentSetAdaptor::importData(const QString &zipFile, const QString &destination)
{
    QString       tmpDir;
    int           i = 0;
    QFileInfoList result;
    do
    {
        tmpDir = QDir::tempPath() + "/TianBoard_tmpImport_" + QString::number(i++);
    } while (QFileInfo(tmpDir).exists());

    QDir(QDir::tempPath()).mkdir(tmpDir);

    QFile fZipFile(zipFile);

    if (!extractFileToDir(fZipFile, tmpDir))
    {
        FileSystemUtils::deleteDir(tmpDir);
        return QFileInfoList();
    }

    QDir tDir(tmpDir);

    foreach (QFileInfo readDir, tDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden, QDir::Name))
    {
        QString newFileName = readDir.fileName();
        if (QFileInfo(destination + "/" + readDir.fileName()).exists())
        {
            newFileName = QFileInfo(documentStore()->generateUniqueDocumentPath(tmpDir)).fileName();
        }
        QString newFilePath = destination + "/" + newFileName;
        if (FileSystemUtils::copy(readDir.absoluteFilePath(), newFilePath))
        {
            result.append(QFileInfo(newFilePath));
        }
    }

    FileSystemUtils::deleteDir(tmpDir);

    return result;
}

bool ImportDocumentSetAdaptor::extractFileToDir(const QFile &pZipFile, const QString &pDir)
{
    QDir   rootDir(pDir);
    QuaZip zip(pZipFile.fileName());

    if (!zip.open(QuaZip::mdUnzip))
    {
        qWarning() << "导入失败，压缩包打不开，zip 错误码:" << zip.getZipError();
        return false;
    }

    zip.setFileNameCodec("UTF-8");
    QuaZipFileInfo info;
    QuaZipFile     file(&zip);

    QFile out;
    char  c;

    QString documentRoot = QFileInfo(pDir).absoluteFilePath();
    for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile())
    {
        if (!zip.getCurrentFileInfo(&info))
        {
            qWarning() << "导入失败，读不到当前条目信息，zip 错误码:" << zip.getZipError();
            return false;
        }

        if (!file.open(QIODevice::ReadOnly))
        {
            qWarning() << "导入失败，压缩包里的文件打不开，zip 错误码:" << zip.getZipError();
            return false;
        }

        if (file.getZipError() != UNZ_OK)
        {
            qWarning() << "导入失败，取条目名出错，zip 错误码:" << zip.getZipError();
            return false;
        }

        QString   actFileName = file.getActualFileName();
        QString   newFileName = documentRoot + "/" + actFileName;
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
            qWarning() << "导入失败，写不出文件";
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
            qWarning() << "导入失败，zip 错误码:" << zip.getZipError();
            return false;
        }

        if (!file.atEnd())
        {
            qWarning() << "导入失败，内容读完了却没到文件末尾";
            return false;
        }

        file.close();

        if (file.getZipError() != UNZ_OK)
        {
            qWarning() << "导入失败，关闭压缩包里的文件出错，zip 错误码:" << file.getZipError();
            return false;
        }
    }

    zip.close();

    if (zip.getZipError() != UNZ_OK)
    {
        qWarning() << "导入失败，关闭压缩包出错，zip 错误码:" << zip.getZipError();
        return false;
    }

    return true;
}
