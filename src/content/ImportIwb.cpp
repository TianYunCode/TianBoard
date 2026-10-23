#include <QDir>
#include "model/IDocumentStore.h"
#include "utils/UserNotices.h"
#include <QList>

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

ImportIwb::ImportIwb(QObject *parent) : DocumentBasedImportAdaptor(parent)
{
}

ImportIwb::~ImportIwb()
{
}

QStringList ImportIwb::supportedExtentions()
{
    return QStringList("iwb");
}

QString ImportIwb::importFileFilter()
{
    QString     filter  = tr("Interactive Whiteboard File (");
    QStringList formats = supportedExtentions();
    bool        isFirst = true;

    foreach (QString format, formats)
    {
        if (isFirst)
        {
            isFirst = false;
        }
        else
        {
            filter.append(" ");
        }

        filter.append("*." + format);
    }

    filter.append(")");

    return filter;
}

bool ImportIwb::addFileToDocument(DocumentProxy *pDocument, const QFile &pFile)
{
    QFileInfo fi(pFile);
    UserNotices::presentStatusMessage(tr("Importing file %1...").arg(fi.baseName()), true);

    // IWB 通常是 zip 包；解压失败时按裸 XML 处理（有些工具直接导出 content.xml）。
    QString path               = QDir::tempPath();

    QString documentRootFolder = expandFileToDir(pFile, path);
    QString contentFile;
    if (documentRootFolder.isEmpty()) // 解压失败：输入可能本身就是 XML
    {
        contentFile = pFile.fileName();
    }
    else // IWB 规范规定内容入口为包根目录下的 content.xml
    {
        contentFile = documentRootFolder.append("/content.xml");
    }

    if (!contentFile.length())
    {
        UserNotices::presentStatusMessage(tr("Import of file %1 failed.").arg(fi.baseName()));
        return false;
    }
    else
    {
        // 先建一个临时文档目录承接转换结果，成功后整体并入目标文档并删除临时目录。
        DocumentProxy *destDocument = new DocumentProxy(documentStore()->generateUniqueDocumentPath());
        QDir           dir;
        dir.mkdir(destDocument->persistencePath());

        // 转换成功后把页面与资源并入现有文档，临时目录不再需要。
        if (IwbSubsetAdaptor::convertIwbFileToDocumentPackage(contentFile, destDocument))
        {
            documentStore()->addDirectoryContentToDocument(destDocument->persistencePath(), pDocument);
            FileSystemUtils::deleteDir(destDocument->persistencePath());
            delete destDocument;
            UserNotices::presentStatusMessage(tr("Import successful."));
            return true;
        }
        else
        {
            FileSystemUtils::deleteDir(destDocument->persistencePath());
            delete destDocument;
            UserNotices::presentStatusMessage(tr("Import failed."));
            return false;
        }
    }
}

QString ImportIwb::expandFileToDir(const QFile &pZipFile, const QString &pDir)
{
    QuaZip zip(pZipFile.fileName());

    if (!zip.open(QuaZip::mdUnzip))
    {
        qWarning() << "导入失败，压缩包打不开，zip 错误码:" << zip.getZipError();
        return "";
    }

    zip.setFileNameCodec("UTF-8");
    QuaZipFileInfo info;
    QuaZipFile     file(&zip);

    // 解压目录名用“时间戳 + 序号”，避免同一分钟内多次导入时互相覆盖。
    QString documentRootFolder;
    int     tmpNumber = 0;
    QDir    rootDir;
    while (true)
    {
        QString tempPath = QString("%1/importedIwb%2.%3").arg(pDir).arg(QDateTime::currentDateTime().toString("dd_MM_yyyy_HH-mm")).arg(tmpNumber);
        if (!rootDir.exists(tempPath))
        {
            documentRootFolder = tempPath;
            break;
        }
        tmpNumber++;
        if (tmpNumber == 100000)
        {
            qWarning() << "导入失败，为 iwb 文件建不出临时目录";
            return "";
        }
    }
    if (!rootDir.mkdir(documentRootFolder))
    {
        qWarning() << "导入失败，为 iwb 包建不出临时目录";
    }

    QFile out;
    char  c;
    for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile())
    {
        if (!zip.getCurrentFileInfo(&info))
        {
            qWarning() << "导入失败，读不到当前条目信息，zip 错误码:" << zip.getZipError();
            return "";
        }
        // 解压失败已在上面返回；此处打开失败按 zip 条目损坏处理。
        file.open(QIODevice::ReadOnly);
        if (file.getZipError() != UNZ_OK)
        {
            qWarning() << "导入失败，取条目名出错，zip 错误码:" << zip.getZipError();
            return "";
        }

        QString newFileName = documentRootFolder + "/" + file.getActualFileName();

        QFileInfo newFileInfo(newFileName);
        rootDir.mkpath(newFileInfo.absolutePath());

        out.setFileName(newFileName);
        out.open(QIODevice::WriteOnly);

        while (file.getChar(&c))
        {
            out.putChar(c);
        }

        out.close();

        if (file.getZipError() != UNZ_OK)
        {
            qWarning() << "导入失败，zip 错误码:" << zip.getZipError();
            return "";
        }
        if (!file.atEnd())
        {
            qWarning() << "导入失败，内容读完了却没到文件末尾";
            return "";
        }

        file.close();

        if (file.getZipError() != UNZ_OK)
        {
            qWarning() << "导入失败，关闭压缩包里的文件出错，zip 错误码:" << file.getZipError();
            return "";
        }
    }

    zip.close();

    if (zip.getZipError() != UNZ_OK)
    {
        qWarning() << "导入失败，关闭压缩包出错，zip 错误码:" << zip.getZipError();
        return "";
    }

    return documentRootFolder;
}

DocumentProxy *ImportIwb::importFile(const QFile &pFile, const QString &pGroup)
{
    Q_UNUSED(pGroup); // 分组信息由文件内的元数据决定，调用方传入的 pGroup 仅在下方覆盖显示名

    QFileInfo fi(pFile);
    UserNotices::presentStatusMessage(tr("Importing file %1...").arg(fi.baseName()), true);

    // 与 addFileToDocument 相同的解包策略：zip 优先，失败则当作裸 XML。
    QString path               = QDir::tempPath();

    QString documentRootFolder = expandFileToDir(pFile, path);
    QString contentFile;
    if (documentRootFolder.isEmpty())
    {
        // 解压失败：输入可能本身就是未压缩的 content.xml
        contentFile = pFile.fileName();
    }
    else
    {
        // IWB 规范规定内容入口为包根目录下的 content.xml
        contentFile = QString("%1/content.xml").arg(documentRootFolder);
    }

    if (!contentFile.length())
    {
        UserNotices::presentStatusMessage(tr("Import of file %1 failed.").arg(fi.baseName()));
        return 0;
    }
    else
    {
        // 转换器在解析第一页前就会写 metadata.rdf，因此版本/更新时间必须提前填好。
        DocumentProxy *destDocument = new DocumentProxy(documentStore()->generateUniqueDocumentPath());
        QDir           dir;
        dir.mkdir(destDocument->persistencePath());
        if (pGroup.length() > 0)
        {
            destDocument->setMetaData(Settings::documentGroupName, pGroup);
        }
        if (!fi.baseName().isEmpty())
        {
            destDocument->setMetaData(Settings::documentName, fi.baseName());
        }

        destDocument->setMetaData(Settings::documentVersion, Settings::currentFileVersion);
        destDocument->setMetaData(Settings::documentUpdatedAt, StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));

        DocumentProxy *newDocument = NULL;
        // 转换成功后再登记为正式文档；失败则整个临时目录丢弃。
        if (IwbSubsetAdaptor::convertIwbFileToDocumentPackage(contentFile, destDocument))
        {
            newDocument = documentStore()->createDocumentFromDir(destDocument->persistencePath(), "", "", false, false, true);

            UserNotices::presentStatusMessage(tr("Import successful."));
        }
        else
        {
            FileSystemUtils::deleteDir(destDocument->persistencePath());
            UserNotices::presentStatusMessage(tr("Import failed."));
        }
        delete destDocument;

        if (documentRootFolder.length() != 0)
        {
            FileSystemUtils::deleteDir(documentRootFolder);
        }
        return newDocument;
    }
}
