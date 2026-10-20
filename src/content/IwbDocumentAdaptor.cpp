#include "content/IwbDocumentAdaptor.h"
#include <algorithm>

#include <QtCore>
#include <QtXml>
#include <QTransform>
#include <QGraphicsItem>
#include <QSvgRenderer>
#include <QPainter>

#include "content/IwbConstants.h"
#include "utils/Globals.h"

THIRD_PARTY_WARNINGS_DISABLE
#include "quazip.h"
#include "quazipfile.h"
#include "quazipfileinfo.h"
THIRD_PARTY_WARNINGS_ENABLE

IwbDocumentAdaptor::IwbDocumentAdaptor()
{
}

bool IwbDocumentAdaptor::convertPageToIwb(const QString &from, const QString &to)
{
    qDebug() << "starting converion from" << from << "to" << to;

    QString source = QString();
    if (QFileInfo(from).isDir() && QFile::exists(from))
    {
        qDebug() << "File specified is dir, continuing convertion";
        source = from;
    }
    else
    {
        source = uncompressZip(from);
        if (!source.isNull())
        {
            qDebug() << "File specified is zip file. Uncompressed to tmp dir, continuing convertion";
        }
    }
    if (source.isNull())
    {
        qDebug() << "File specified is not a dir or a zip file, stopping covretion";
        return false;
    }

    QString tmpDestination = createNewTmpDir();
    if (tmpDestination.isNull())
    {
        qDebug() << "can't create temp destination folder. Stopping parsing...";
        return false;
    }

    ToIwbConverter tmpConvertrer(source, tmpDestination);
    if (!tmpConvertrer)
    {
        qDebug() << "The convertrer class is invalid, stopping conversion. Error message" << tmpConvertrer.lastErrStr();
        return false;
    }

    bool bParceRes = tmpConvertrer.parse();

    mConversionMessages << tmpConvertrer.getMessages();

    if (!bParceRes)
    {
        return false;
    }

    if (!compressZip(tmpDestination, to))
    {
        qDebug() << "error in compression";
    }

    // 源是 .tbz 时解压出来的是临时目录，转换完立即删除；源本身是目录时保留。
    if (!QFileInfo(from).isDir())
    {
        if (!freeDir(source))
        {
            qDebug() << "can't delete tmp directory" << QDir(source).absolutePath() << "try to delete them manually";
        }
    }

    if (!freeDir(tmpDestination))
    {
        qDebug() << "can't delete tmp directory" << QDir(tmpDestination).absolutePath() << "try to delete them manually";
    }

    return true;
}

QString IwbDocumentAdaptor::uncompressZip(const QString &zipFile)
{
    QuaZip zip(zipFile);

    if (!zip.open(QuaZip::mdUnzip))
    {
        qWarning() << "Import failed. Cause zip.open(): " << zip.getZipError();
        return QString();
    }

    zip.setFileNameCodec("UTF-8");
    QuaZipFileInfo info;
    QuaZipFile     file(&zip);

    QString documentRootFolder = createNewTmpDir();

    if (documentRootFolder.isNull())
    {
        qDebug() << "can't create tmp directory for zip file" << zipFile;
        return QString();
    }

    QDir  rootDir(documentRootFolder);
    QFile out;
    char  c;
    bool  allOk = true;
    for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile())
    {
        if (!zip.getCurrentFileInfo(&info))
        {
            qWarning() << "Import failed. Cause: getCurrentFileInfo(): " << zip.getZipError();
            allOk = false;
            break;
        }
        if (!file.open(QIODevice::ReadOnly))
        {
            allOk = false;
            break;
        }
        if (file.getZipError() != UNZ_OK)
        {
            qWarning() << "Import failed. Cause: file.getFileName(): " << zip.getZipError();
            allOk = false;
            break;
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
            qWarning() << "Import failed. Cause: " << zip.getZipError();
            allOk = false;
            break;
        }
        if (!file.atEnd())
        {
            qWarning() << "Import failed. Cause: read all but not EOF";
            allOk = false;
            break;
        }
        file.close();

        if (file.getZipError() != UNZ_OK)
        {
            qWarning() << "Import failed. Cause: file.close(): " << file.getZipError();
            allOk = false;
            break;
        }
    }

    if (!allOk)
    {
        out.close();
        file.close();
        zip.close();
        return QString();
    }

    if (zip.getZipError() != UNZ_OK)
    {
        qWarning() << "Import failed. Cause: zip.close(): " << zip.getZipError();
        return QString();
    }

    return documentRootFolder;
}

bool IwbDocumentAdaptor::compressZip(const QString &source, const QString &destination)
{
    QDir toDir = QFileInfo(destination).dir();
    if (!toDir.exists())
    {
        if (!QDir().mkpath(toDir.absolutePath()))
        {
            qDebug() << "can't create destination folder to uncompress file";
            return false;
        }
    }

    QuaZip zip(destination);
    zip.setFileNameCodec("UTF-8");
    if (!zip.open(QuaZip::mdCreate))
    {
        qDebug("Export failed. Cause: zip.open(): %d", zip.getZipError());
        return false;
    }

    QuaZipFile outZip(&zip);

    QFileInfo sourceInfo(source);
    if (sourceInfo.isDir())
    {
        if (!compressDir(QFileInfo(source).absoluteFilePath(), "", &outZip))
        {
            return false;
        }
    }
    else if (sourceInfo.isFile())
    {
        if (!compressFile(QFileInfo(source).absoluteFilePath(), "", &outZip))
        {
            return false;
        }
    }

    return true;
}

bool IwbDocumentAdaptor::compressDir(const QString &dirName, const QString &parentDir, QuaZipFile *outZip)
{
    QFileInfoList            dirFiles = QDir(dirName).entryInfoList(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    QListIterator<QFileInfo> iter(dirFiles);
    while (iter.hasNext())
    {
        QFileInfo curFile = iter.next();

        if (curFile.isDir())
        {
            if (!compressDir(curFile.absoluteFilePath(), parentDir + curFile.fileName() + "/", outZip))
            {
                qDebug() << "error at compressing dir" << curFile.absoluteFilePath();
                return false;
            }
        }
        else if (curFile.isFile())
        {
            if (!compressFile(curFile.absoluteFilePath(), parentDir, outZip))
            {
                return false;
            }
        }
    }

    return true;
}

bool IwbDocumentAdaptor::compressFile(const QString &fileName, const QString &parentDir, QuaZipFile *outZip)
{
    QFile sourceFile(fileName);

    if (!sourceFile.open(QIODevice::ReadOnly))
    {
        qDebug() << "Compression of file" << sourceFile.fileName() << " failed. Cause: inFile.open(): " << sourceFile.errorString();
        return false;
    }

    if (!outZip->open(QIODevice::WriteOnly, QuaZipNewInfo(parentDir + QFileInfo(fileName).fileName(), sourceFile.fileName())))
    {
        qDebug() << "Compression of file" << sourceFile.fileName() << " failed. Cause: outFile.open(): " << outZip->getZipError();
        sourceFile.close();
        return false;
    }

    outZip->write(sourceFile.readAll());
    if (outZip->getZipError() != UNZ_OK)
    {
        qDebug() << "Compression of file" << sourceFile.fileName() << " failed. Cause: outFile.write(): " << outZip->getZipError();

        sourceFile.close();
        outZip->close();
        return false;
    }

    if (outZip->getZipError() != UNZ_OK)
    {
        qWarning() << "Compression of file" << sourceFile.fileName() << " failed. Cause: outFile.close(): " << outZip->getZipError();

        sourceFile.close();
        outZip->close();
        return false;
    }

    outZip->close();
    sourceFile.close();

    return true;
}

QString IwbDocumentAdaptor::createNewTmpDir()
{
    int  tmpNumber = 0;
    QDir systemTmp = QDir::temp();

    while (true)
    {
        QString dirName = QString("iwb_adaptor_filedata_store%1.%2").arg(QDateTime::currentDateTime().toString("dd_MM_yyyy_HH-mm")).arg(tmpNumber++);
        if (!systemTmp.exists(dirName))
        {
            if (systemTmp.mkdir(dirName))
            {
                QString result = systemTmp.absolutePath() + "/" + dirName;
                tmpDirs.append(result);
                return result;
            }
            else
            {
                qDebug() << "Can't create temporary dir maybe due to permissions";
                return QString();
            }
        }
        else if (tmpNumber == 10)
        {
            qWarning() << "Import failed. Failed to create temporary file ";
            return QString();
        }
        tmpNumber++;
    }

    return QString();
}
bool IwbDocumentAdaptor::deleteDir(const QString &pDirPath) const
{
    if (pDirPath == "" || pDirPath == "." || pDirPath == "..")
    {
        return false;
    }

    QDir dir(pDirPath);

    if (dir.exists())
    {
        foreach (QFileInfo dirContent, dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System, QDir::Name))
        {
            if (dirContent.isDir())
            {
                deleteDir(dirContent.absoluteFilePath());
            }
            else
            {
                if (!dirContent.dir().remove(dirContent.fileName()))
                {
                    return false;
                }
            }
        }
    }

    return dir.rmdir(pDirPath);
}

QList<QString> IwbDocumentAdaptor::getConversionMessages()
{
    return mConversionMessages;
}

bool IwbDocumentAdaptor::freeDir(const QString &dir)
{
    bool result = true;
    if (!deleteDir(dir))
    {
        result = false;
    }

    tmpDirs.removeAll(QDir(dir).absolutePath());

    return result;
}
void IwbDocumentAdaptor::freeTmpDirs()
{
    foreach (QString dir, tmpDirs)
    {
        freeDir(dir);
    }
}

IwbDocumentAdaptor::~IwbDocumentAdaptor()
{
    freeTmpDirs();
}

IwbDocumentAdaptor::ToIwbConverter::ToIwbConverter(const QString &source, const QString &destination)
{
    sourcePath       = source;
    destinationPath  = destination;

    errorStr         = noErrorMsg;
    mDataModel       = new QDomDocument;
    mDocumentToWrite = new QDomDocument;
    mDocumentToWrite->setContent(QString("<doc></doc>"));

    mIWBContentWriter = new QXmlStreamWriter;
    mIWBContentWriter->setAutoFormatting(true);

    iwbSVGItemsAttributes.insert(tIWBImage, iwbSVGImageAttributes);
    iwbSVGItemsAttributes.insert(tIWBVideo, iwbSVGVideoAttributes);
    iwbSVGItemsAttributes.insert(tIWBText, iwbSVGTextAttributes);
    iwbSVGItemsAttributes.insert(tIWBTextArea, iwbSVGTextAreaAttributes);
    iwbSVGItemsAttributes.insert(tIWBPolyLine, iwbSVGPolyLineAttributes);
    iwbSVGItemsAttributes.insert(tIWBPolygon, iwbSVGPolygonAttributes);
    iwbSVGItemsAttributes.insert(tIWBRect, iwbSVGRectAttributes);
    iwbSVGItemsAttributes.insert(tIWBLine, iwbSVGLineAttributes);
    iwbSVGItemsAttributes.insert(tIWBTspan, iwbSVGTspanAttributes);
}

bool IwbDocumentAdaptor::ToIwbConverter::parse()
{
    if (!isValid())
    {
        qDebug() << "document metadata is not valid. Can't parse";
        return false;
    }

    qDebug() << "begin parsing tbz";

    QFile outFile(contentIWBFileName());
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "can't open output file for writing";
        errorStr = "createXMLOutputPatternError";
        return false;
    }

    mIWBContentWriter->setDevice(&outFile);

    mIWBContentWriter->writeStartDocument();
    mIWBContentWriter->writeStartElement(tIWBRoot);

    fillNamespaces();

    mIWBContentWriter->writeAttribute(aIWBVersion, avIWBVersionNo);

    if (!parseMetadata())
    {
        if (errorStr == noErrorMsg)
        {
            errorStr = "MetadataParsingError";
        }

        outFile.close();
        return false;
    }

    if (!parseContent())
    {
        if (errorStr == noErrorMsg)
        {
            errorStr = "ContentParsingError";
        }
        outFile.close();
        return false;
    }

    mIWBContentWriter->writeEndElement();
    mIWBContentWriter->writeEndDocument();

    outFile.close();

    qDebug() << "finished with success";

    return true;
}
bool IwbDocumentAdaptor::ToIwbConverter::parseMetadata()
{
    int   errorLine, errorColumn;
    QFile metaDataFile(sourcePath + "/" + fMetadata);

    if (!metaDataFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        errorStr = "can't open" + QFileInfo(sourcePath + "/" + fMetadata).absoluteFilePath();
        qDebug() << errorStr;
        return false;
    }
    else if (!mDataModel->setContent(metaDataFile.readAll(), true, &errorStr, &errorLine, &errorColumn))
    {
        qWarning() << "Error:Parseerroratline" << errorLine << ","
                   << "column" << errorColumn << ":" << errorStr;
        return false;
    }

    QDomElement nextInElement = mDataModel->documentElement();

    nextInElement             = nextInElement.firstChildElement(tDescription);
    if (!nextInElement.isNull())
    {
        mIWBContentWriter->writeStartElement(iwbNS, tIWBMeta);
        mIWBContentWriter->writeAttribute(aIWBName, aCreator);
        mIWBContentWriter->writeAttribute(aIWBContent, avCreator);
        mIWBContentWriter->writeEndElement();

        mIWBContentWriter->writeStartElement(iwbNS, tIWBMeta);
        mIWBContentWriter->writeAttribute(aIWBName, aOwner);
        mIWBContentWriter->writeAttribute(aIWBContent, avOwner);
        mIWBContentWriter->writeEndElement();

        mIWBContentWriter->writeStartElement(iwbNS, tIWBMeta);
        mIWBContentWriter->writeAttribute(aIWBName, aDescription);
        mIWBContentWriter->writeAttribute(aIWBContent, avDescription);
        mIWBContentWriter->writeEndElement();

        mIWBContentWriter->writeStartElement(iwbNS, tIWBMeta);
        mIWBContentWriter->writeAttribute(aIWBName, aAbout);
        mIWBContentWriter->writeAttribute(aIWBContent, nextInElement.attribute(aAbout));
        mIWBContentWriter->writeEndElement();

        nextInElement = nextInElement.firstChildElement();
        while (!nextInElement.isNull())
        {
            QString textContent = nextInElement.text();
            if (!textContent.trimmed().isEmpty())
            {
                if (nextInElement.tagName() == tPageSize)
                {
                    QSize tmpSize = getSVGDimentions(nextInElement.text());
                    if (!tmpSize.isNull())
                    {
                        mSVGSize = tmpSize;
                    }
                    else
                    {
                        qDebug() << "can't interpret svg section size";
                        errorStr = "InterpretSvgSizeError";
                        return false;
                    }
                }
                else
                {
                    mIWBContentWriter->writeStartElement(iwbNS, tIWBMeta);
                    mIWBContentWriter->writeAttribute(aIWBName, nextInElement.tagName());
                    mIWBContentWriter->writeAttribute(aIWBContent, textContent);
                    mIWBContentWriter->writeEndElement();
                }
            }
            nextInElement = nextInElement.nextSiblingElement();
        }
    }

    metaDataFile.close();
    return true;
}
bool IwbDocumentAdaptor::ToIwbConverter::parseContent()
{
    QDir        sourceDir(sourcePath);
    QStringList fileFilters;
    fileFilters << QString(pageAlias + "???." + pageFileExtension);
    QStringList pageList           = sourceDir.entryList(fileFilters, QDir::Files, QDir::Name | QDir::IgnoreCase);

    QDomElement svgDocumentSection = mDataModel->createElementNS(svgIWBNS, ":" + tSvg);

    if (!pageList.count())
    {
        qDebug() << "can't find any content file";
        errorStr = "ErrorContentFile";
        return false;
    }
    else
    {
        QDomElement pageset = parsePageset(pageList);
        if (pageset.isNull())
        {
            return false;
        }
        else
        {
            svgDocumentSection.appendChild(pageset);
        }
    }

    if (QRect() == mViewbox)
    {
        mViewbox.setRect(0, 0, mSVGSize.width(), mSVGSize.height());
    }

    svgDocumentSection.setAttribute(aIWBViewBox, rectToIWBAttr(mViewbox));
    svgDocumentSection.setAttribute(aWidth, QString("%1").arg(mViewbox.width()));
    svgDocumentSection.setAttribute(aHeight, QString("%1").arg(mViewbox.height()));

    writeQDomElementToXML(svgDocumentSection);

    if (!writeExtendedIwbSection())
    {
        if (errorStr == noErrorMsg)
        {
            errorStr = "writeExtendedIwbSectionError";
        }
        return false;
    }

    return true;
}

QDomElement IwbDocumentAdaptor::ToIwbConverter::parsePage(const QString &pageFileName)
{
    qDebug() << "begin parsing page" + pageFileName;
    mSvgElements.clear();

    int errorLine, errorColumn;

    QFile pageFile(sourcePath + "/" + pageFileName);
    if (!pageFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "can't open file" << pageFileName << "for reading";
        return QDomElement();
    }
    else if (!mDataModel->setContent(pageFile.readAll(), true, &errorStr, &errorLine, &errorColumn))
    {
        qWarning() << "Error:Parseerroratline" << errorLine << ","
                   << "column" << errorColumn << ":" << errorStr;
        pageFile.close();
        return QDomElement();
    }

    QDomElement page;
    QDomElement group;

    QDomElement nextTopElement = mDataModel->firstChildElement();
    while (!nextTopElement.isNull())
    {
        QString tagname = nextTopElement.tagName();
        if (tagname == tSvg)
        {
            page = parseSvgPageSection(nextTopElement);
            if (page.isNull())
            {
                qDebug() << "The page is empty.";
                pageFile.close();
                return QDomElement();
            }
        }
        else if (tagname == tPageGroups)
        {
            group = parseGroupsPageSection(nextTopElement);
            if (group.isNull())
            {
                qDebug() << "Page doesn't contains any groups.";
                pageFile.close();
                return QDomElement();
            }
        }

        nextTopElement = nextTopElement.nextSiblingElement();
    }

    pageFile.close();

    return page.hasChildNodes() ? page : QDomElement();
}

QDomElement IwbDocumentAdaptor::ToIwbConverter::parsePageset(const QStringList &pageFileNames)
{
    QMultiMap<int, QDomElement> pageList;
    int                         iPageNo = 1;

    QStringListIterator curPage(pageFileNames);

    while (curPage.hasNext())
    {
        QString     curPageFile = curPage.next();
        QDomElement iterElement = parsePage(curPageFile);
        if (!iterElement.isNull())
        {
            iterElement.setAttribute(tId, iPageNo);
            addSVGElementToResultModel(iterElement, pageList, iPageNo);
            iPageNo++;
        }
        else
        {
            return QDomElement();
        }
    }

    if (!pageList.count())
    {
        return QDomElement();
    }

    QDomElement svgPagesetElement = mDocumentToWrite->createElementNS(svgIWBNS, ":" + tIWBPageSet);

    QMultiMapIterator<int, QDomElement> nextSVGElement(pageList);
    nextSVGElement.toFront();
    while (nextSVGElement.hasNext())
    {
        svgPagesetElement.appendChild(nextSVGElement.next().value());
    }

    return svgPagesetElement.hasChildNodes() ? svgPagesetElement : QDomElement();
}
QDomElement IwbDocumentAdaptor::ToIwbConverter::parseSvgPageSection(const QDomElement &element)
{
    if (element.hasAttribute(aPageViewBox))
    {
        setViewBox(getViewboxRect(element.attribute(aPageViewBox)));
    }

    QMultiMap<int, QDomElement> svgElements;

    QDomElement svgElementPart = mDocumentToWrite->createElementNS(svgIWBNS, ":" + tIWBPage);

    if (element.hasAttribute(aDarkBackground))
    {
        createBackground(element, svgElements);
    }

    QDomElement nextElement = element.firstChildElement();
    while (!nextElement.isNull())
    {
        QString tagName = nextElement.tagName();
        if (tagName == tPageG)
        {
            parseSVGGGroup(nextElement, svgElements);
        }
        else if (tagName == tPageImage)
        {
            parsePageImage(nextElement, svgElements);
        }
        else if (tagName == tPageVideo)
        {
            parsePageVideo(nextElement, svgElements);
        }
        else if (tagName == tPageAudio)
        {
            parsePageAudio(nextElement, svgElements);
        }
        else if (tagName == tPageForeignObject)
        {
            parseForeignObject(nextElement, svgElements);
        }
        else if (tagName == tPageLine)
        {
            parsePageLine(nextElement, svgElements);
        }
        else if (tagName == tPagePolygon)
        {
            parsePagePolygon(nextElement, svgElements);
        }
        else if (tagName == tPagePolyline)
        {
            parsePagePolyline(nextElement, svgElements);
        }
        else if (tagName == tPageGroups)
        {
            parseGroupsPageSection(nextElement);
        }

        nextElement = nextElement.nextSiblingElement();
    }

    if (0 == svgElements.count())
    {
        return QDomElement();
    }

    QMultiMapIterator<int, QDomElement> nextSVGElement(svgElements);
    nextSVGElement.toFront();
    while (nextSVGElement.hasNext())
    {
        svgElementPart.appendChild(nextSVGElement.next().value());
    }

    return svgElementPart.hasChildNodes() ? svgElementPart : QDomElement();
}

void IwbDocumentAdaptor::ToIwbConverter::writeQDomElementToXML(const QDomNode &node)
{
    if (!node.isNull())
    {
        if (node.isText())
        {
            mIWBContentWriter->writeCharacters(node.nodeValue());
        }
        else
        {
            mIWBContentWriter->writeStartElement(node.namespaceURI(), node.toElement().tagName());

            for (int i = 0; i < node.toElement().attributes().count(); i++)
            {
                QDomAttr attr = node.toElement().attributes().item(i).toAttr();
                mIWBContentWriter->writeAttribute(attr.name(), attr.value());
            }
            QDomNode child = node.firstChild();
            while (!child.isNull())
            {
                writeQDomElementToXML(child);
                child = child.nextSibling();
            }

            mIWBContentWriter->writeEndElement();
        }
    }
}

bool IwbDocumentAdaptor::ToIwbConverter::writeExtendedIwbSection()
{
    if (!mExtendedElements.count())
    {
        qDebug() << "extended iwb content list is empty";
        errorStr = "EmptyExtendedIwbSectionContentError";
        return false;
    }
    QListIterator<QDomElement> nextExtendedIwbElement(mExtendedElements);
    while (nextExtendedIwbElement.hasNext())
    {
        writeQDomElementToXML(nextExtendedIwbElement.next());
    }

    return true;
}

QDomElement IwbDocumentAdaptor::ToIwbConverter::parseGroupsPageSection(const QDomElement &groupRoot)
{
    if (!groupRoot.hasChildNodes())
    {
        qDebug() << "Group root is empty";
        return QDomElement();
    }

    QDomElement groupElement = groupRoot.firstChildElement();

    while (!groupElement.isNull())
    {
        QDomElement extendedElement   = mDataModel->createElementNS(iwbNS, groupElement.tagName());
        QDomElement groupChildElement = groupElement.firstChildElement();
        while (!groupChildElement.isNull())
        {
            QDomElement extSubElement = mDataModel->createElementNS(iwbNS, groupChildElement.tagName());
            extSubElement.setAttribute(aRef, groupChildElement.attribute(aID, QUuid().toString()));
            extendedElement.appendChild(extSubElement);

            groupChildElement = groupChildElement.nextSiblingElement();
        }

        mExtendedElements.append(extendedElement);

        groupElement = groupElement.nextSiblingElement();
    }

    qDebug() << "parsing tbz group section";
    return groupRoot;
}

QString IwbDocumentAdaptor::ToIwbConverter::getDstContentFolderName(const QString &elementType)
{
    QString sRet;
    QString sDstContentFolderName;

    // Widget 在 IWB 里没有等价图元，统一转成 png 快照放进 images/ 目录。
    if ((tIWBImage == elementType) || (tPageForeignObject == elementType))
    {
        sDstContentFolderName = cfImages;
    }
    else if (tIWBVideo == elementType)
    {
        sDstContentFolderName = cfVideos;
    }
    else if (tIWBAudio == elementType)
    {
        sDstContentFolderName = cfAudios;
    }

    sRet = sDstContentFolderName;

    return sRet;
}

QString IwbDocumentAdaptor::ToIwbConverter::getSrcContentFolderName(QString href)
{
    QString sRet;

    QStringList ls = href.split("/");
    for (int i = 0; i < ls.count() - 1; i++)
    {
        QString sPart = ls.at(i);
        if (iwbPackageContentFolders.contains(sPart))
        {
            sRet = sPart;
        }
    }

    return sRet;
}

QString IwbDocumentAdaptor::ToIwbConverter::getFileNameFromPath(const QString sPath)
{
    QString     sRet;
    QStringList sl = sPath.split("/", Qt::SkipEmptyParts);

    if (0 < sl.count())
    {
        QString name      = sl.at(sl.count() - 1);
        QString extention = getExtentionFromFileName(name);

        if (feWgt == extention)
        {
            name.remove("{");
            name.remove("}");
        }

        name.remove(name.length() - extention.length(), extention.length());
        name += convertExtention(extention);

        sRet  = name;
    }
    return sRet;
}

QString IwbDocumentAdaptor::ToIwbConverter::getExtentionFromFileName(const QString &filename)
{
    QStringList sl = filename.split("/", Qt::SkipEmptyParts);

    if (0 < sl.count())
    {
        QString     name = sl.at(sl.count() - 1);
        QStringList tl   = name.split(".");
        return tl.at(tl.count() - 1);
    }
    return QString();
}

QString IwbDocumentAdaptor::ToIwbConverter::convertExtention(const QString &ext)
{
    QString sRet;

    if (feSvg == ext)
    {
        sRet = fePng;
    }
    else if (feWgt == ext)
    {
        sRet = fePng;
    }
    else
    {
        sRet = ext;
    }

    return sRet;
}

QString IwbDocumentAdaptor::ToIwbConverter::pageElementTypeName(const QDomElement &element)
{
    QString sRet;
    if (tPageForeignObject == element.tagName())
    {
        QString sPath;
        if (element.hasAttribute(aPageType))
        {
            if (avPageText == element.attribute(aPageType))
            {
                sRet = tIWBTextArea;
            }
            else
            {
                sRet = element.attribute(aPageType);
            }
        }
        else
        {
            if (element.hasAttribute(aSrc))
            {
                sPath = element.attribute(aSrc);
            }
            else if (element.hasAttribute(aPageHref))
            {
                sPath = element.attribute(aPageHref);
            }

            QStringList tsl = sPath.split(".", Qt::SkipEmptyParts);
            if (0 < tsl.count())
            {
                QString elementType = tsl.at(tsl.count() - 1);
                if (iwbElementImage.contains(elementType))
                {
                    sRet = tIWBImage;
                }
                else if (iwbElementAudio.contains(elementType))
                {
                    sRet = tIWBAudio;
                }
                else if (iwbElementVideo.contains(elementType))
                {
                    sRet = tIWBVideo;
                }
            }
        }
    }
    else
    {
        sRet = element.tagName();
    }

    return sRet;
}

int IwbDocumentAdaptor::ToIwbConverter::getElementLayer(const QDomElement &element)
{
    int iRetLayer = 0;
    if (element.hasAttribute(aZLayer))
    {
        iRetLayer = (int)element.attribute(aZLayer).toDouble();
    }
    else
    {
        iRetLayer = DEFAULT_LAYER;
    }

    return iRetLayer;
}

bool IwbDocumentAdaptor::ToIwbConverter::itIsSupportedFormat(const QString &format) const
{
    bool bRet;

    QStringList tsl = format.split(".", Qt::SkipEmptyParts);
    if (0 < tsl.count())
    {
        bRet = iwbSupportedFileFormats.contains(tsl.at(tsl.count() - 1).toLower());
    }
    else
    {
        bRet = false;
    }

    return bRet;
}

bool IwbDocumentAdaptor::ToIwbConverter::itIsFormatToConvert(const QString &format) const
{
    foreach (QString f, iwbPackageFormatsToConvert.split(","))
    {
        if (format == f)
        {
            return true;
        }
    }
    return false;
}

bool IwbDocumentAdaptor::ToIwbConverter::itIsSVGElementAttribute(const QString ItemType, const QString &AttrName)
{
    QString allowedElementAttributes = iwbSVGItemsAttributes[ItemType];

    allowedElementAttributes.remove("/t");
    allowedElementAttributes.remove(" ");
    foreach (QString attr, allowedElementAttributes.split(","))
    {
        if (AttrName == attr.trimmed())
        {
            return true;
        }
    }
    return false;
}

bool IwbDocumentAdaptor::ToIwbConverter::itIsIWBAttribute(const QString &attribute) const
{
    foreach (QString attr, iwbElementAttributes.split(","))
    {
        if (attribute == attr.trimmed())
        {
            return true;
        }
    }
    return false;
}

bool IwbDocumentAdaptor::ToIwbConverter::isPageAttributeToConvert(const QString &attribute) const
{
    foreach (QString attr, iwbPackageAttributesToConvert.split(","))
    {
        if (attribute == attr.trimmed())
        {
            return true;
        }
    }
    return false;
}

bool IwbDocumentAdaptor::ToIwbConverter::ibwAddLine(int x1, int y1, int x2, int y2, QString color, int width, bool isBackground)
{
    bool bRet = true;

    QDomDocument doc;

    QDomElement svgBackgroundCrossPart = doc.createElementNS(svgIWBNS, svgIWBNSPrefix + ":line");
    QDomElement iwbBackgroundCrossPart = doc.createElementNS(iwbNS, iwbNsPrefix + ":" + tElement);

    QString sUUID                      = QUuid::createUuid().toString();

    svgBackgroundCrossPart.setTagName(tIWBLine);

    svgBackgroundCrossPart.setAttribute(aX + "1", x1);
    svgBackgroundCrossPart.setAttribute(aY + "1", y1);
    svgBackgroundCrossPart.setAttribute(aX + "2", x2);
    svgBackgroundCrossPart.setAttribute(aY + "2", y2);

    svgBackgroundCrossPart.setAttribute(aStroke, color);
    svgBackgroundCrossPart.setAttribute(aStrokeWidth, width);

    svgBackgroundCrossPart.setAttribute(aID, sUUID);

    if (isBackground)
    {
        iwbBackgroundCrossPart.setAttribute(aRef, sUUID);
        iwbBackgroundCrossPart.setAttribute(aLocked, avTrue);

        addIWBElementToResultModel(iwbBackgroundCrossPart);
    }

    addSVGElementToResultModel(svgBackgroundCrossPart, mSvgElements, DEFAULT_BACKGROUND_CROSS_LAYER);

    if (!bRet)
    {
        qDebug() << "|error at creating crosses on background";
        errorStr = "CreatingCrossedBackgroundParsingError.";
    }

    return bRet;
}

QTransform IwbDocumentAdaptor::ToIwbConverter::transformOfPageElement(const QDomElement &pageElement)
{
    QTransform trRet;

    QStringList transformParameters;

    // 只处理 matrix(a,b,c,d,e,f) 形式：IWB 导出端把 rotate/translate 都折算成矩阵，
    // 其它函数写法在这里会被忽略，保持单位变换。
    QString transformAttribute = pageElement.attribute(aTransform);
    transformAttribute.remove("matrix");
    transformAttribute.remove("(");
    transformAttribute.remove(")");

    transformParameters = transformAttribute.split(",", Qt::SkipEmptyParts);

    if (6 <= transformParameters.count())
    {
        QTransform *tr = NULL;
        tr    = new QTransform(transformParameters.at(0).toDouble(), transformParameters.at(1).toDouble(), transformParameters.at(2).toDouble(),
            transformParameters.at(3).toDouble(), transformParameters.at(4).toDouble(), transformParameters.at(5).toDouble());

        trRet = *tr;

        delete tr;
    }

    if (6 <= transformParameters.count())
    {
        QTransform *tr = NULL;
        tr    = new QTransform(transformParameters.at(0).toDouble(), transformParameters.at(1).toDouble(), transformParameters.at(2).toDouble(),
            transformParameters.at(3).toDouble(), transformParameters.at(4).toDouble(), transformParameters.at(5).toDouble());

        trRet = *tr;

        delete tr;
    }
    return trRet;
}

qreal IwbDocumentAdaptor::ToIwbConverter::getAngleFromTransform(const QTransform &tr)
{
    qreal angle = -(atan(tr.m21() / tr.m11()) * 180 / PI);
    if (tr.m21() > 0 && tr.m11() < 0)
    {
        angle += 180;
    }
    else if (tr.m21() < 0 && tr.m11() < 0)
    {
        angle += 180;
    }
    return angle;
}

void IwbDocumentAdaptor::ToIwbConverter::setGeometryFromPageElement(const QDomElement &pageElement, QDomElement &iwbElement)
{
    setCoordinatesFromPageElement(pageElement, iwbElement);
}

void IwbDocumentAdaptor::ToIwbConverter::setCoordinatesFromPageElement(const QDomElement &pageElement, QDomElement &iwbElement)
{
    QTransform tr;

    if (QString() != pageElement.attribute(aTransform))
    {
        tr = transformOfPageElement(pageElement);
    }

    qreal x      = pageElement.attribute(aX).toDouble();
    qreal y      = pageElement.attribute(aY).toDouble();
    qreal height = pageElement.attribute(aHeight).toDouble();
    qreal width  = pageElement.attribute(aWidth).toDouble();

    qreal alpha  = getAngleFromTransform(tr);

    QRectF            itemRect;
    QGraphicsRectItem item;

    item.setRect(0, 0, width, height);
    item.setTransform(tr);
    item.setRotation(-alpha);
    QTransform sceneMatrix = item.sceneTransform();

    iwbElement.setAttribute(aX, x);
    iwbElement.setAttribute(aY, y);
    iwbElement.setAttribute(aHeight, height * sceneMatrix.m22());
    iwbElement.setAttribute(aWidth, width * sceneMatrix.m11());
    iwbElement.setAttribute(aTransform, QString("rotate(%1) translate(%2,%3)").arg(alpha).arg(sceneMatrix.dx()).arg(sceneMatrix.dy()));
}

bool IwbDocumentAdaptor::ToIwbConverter::setContentFromPageElement(const QDomElement &pageElement, QDomElement &svgElement)
{
    bool bRet = true;

    QString srcPath;
    if (tPageForeignObject != pageElement.tagName())
    {
        srcPath = pageElement.attribute(aPageHref);
    }
    else
    {
        srcPath = pageElement.attribute(aSrc);
    }

    QString sSrcContentFolder = getSrcContentFolderName(srcPath);
    QString sSrcFileName      = sourcePath + "/" + srcPath;
    QString fileExtention     = getExtentionFromFileName(sSrcFileName);
    QString sDstContentFolder = getDstContentFolderName(pageElement.tagName());
    QString sDstFileName(QString(QUuid::createUuid().toString() + "." + convertExtention(fileExtention)));

    if (itIsSupportedFormat(fileExtention))
    {
        sSrcFileName = sourcePath + "/" + sSrcContentFolder + "/" + getFileNameFromPath(srcPath);

        QFile srcFile;
        srcFile.setFileName(sSrcFileName);

        QDir dstDocFolder(destinationPath);

        if (!dstDocFolder.exists(sDstContentFolder))
        {
            bRet &= dstDocFolder.mkdir(sDstContentFolder);
        }

        if (bRet)
        {
            QString dstFilePath  = destinationPath + "/" + sDstContentFolder + "/" + sDstFileName;
            bRet                &= srcFile.copy(dstFilePath);
        }

        if (bRet)
        {
            svgElement.setAttribute(aSVGHref, sDstContentFolder + "/" + sDstFileName);
        }
    }
    else if (itIsFormatToConvert(fileExtention))
    {
        if (feSvg == fileExtention)
        {
            QDir dstDocFolder(destinationPath);

            if (!dstDocFolder.exists(sDstContentFolder))
            {
                bRet &= dstDocFolder.mkdir(sDstContentFolder);
            }

            if (bRet)
            {
                if (feSvg == fileExtention)
                {
                    QString dstFilePath  = destinationPath + "/" + sDstContentFolder + "/" + sDstFileName;
                    bRet                &= createPngFromSvg(sSrcFileName, dstFilePath, transformOfPageElement(pageElement));
                }
                else
                {
                    bRet = false;
                }
            }

            if (bRet)
            {
                svgElement.setAttribute(aSVGHref, sDstContentFolder + "/" + sDstFileName);
            }
        }
    }
    else
    {
        addLastExportError(QObject::tr("Element ID = ") + QString("%1 \r\n").arg(pageElement.attribute(aPageUuid)) + QString("Source file  = ") +
                           QString("%1 \r\n").arg(pageElement.attribute(aPageSource)) +
                           QObject::tr("Content is not supported in destination format."));
        bRet = false;
    }

    if (!bRet)
    {
        qDebug() << "format is not supported by IWB";
    }

    return bRet;
}

void IwbDocumentAdaptor::ToIwbConverter::setIwbTextFromHTMLTextNode(const QDomElement htmlTextNode, QDomElement &iwbElement)
{
    QDomDocument textDoc;

    QDomElement textParentElement = iwbElement;

    QString  textString;
    QDomNode htmlPNode = htmlTextNode.firstChild();
    bool     bTbreak   = false;

    while (!htmlPNode.isNull())
    {
        if (bTbreak)
        {
            bTbreak                = false;

            QDomElement tbreakNode = textDoc.createElementNS(svgIWBNS, svgIWBNSPrefix + ":" + tIWBTbreak);
            textParentElement.appendChild(tbreakNode.cloneNode(true));
        }

        QDomNode spanNode = htmlPNode.firstChild();

        while (!spanNode.isNull())
        {
            if (spanNode.isText())
            {
                QDomText nodeText = textDoc.createTextNode(spanNode.nodeValue());
                textParentElement.appendChild(nodeText.cloneNode(true));
            }
            else if (spanNode.isElement())
            {
                QDomElement pElementIwb;
                QDomElement spanElement = textDoc.createElementNS(svgIWBNS, svgIWBNSPrefix + ":" + tIWBTspan);
                copyPageCommonAttributes(htmlPNode.toElement(), pElementIwb, spanElement);

                if (spanNode.hasAttributes())
                {
                    int attrCount = spanNode.attributes().count();
                    if (0 < attrCount)
                    {
                        for (int i = 0; i < attrCount; i++)
                        {
                            QStringList iwbAttributes = spanNode.attributes().item(i).nodeValue().split(";", Qt::SkipEmptyParts);
                            {
                                for (int i = 0; i < iwbAttributes.count(); i++)
                                {
                                    QString     attr    = iwbAttributes.at(i).trimmed();
                                    QStringList AttrVal = attr.split(":", Qt::SkipEmptyParts);
                                    if (1 < AttrVal.count())
                                    {
                                        QString sAttr = packageAttrNameToIwbAttrName(AttrVal.at(0));
                                        if (itIsSVGElementAttribute(spanElement.tagName(), sAttr))
                                        {
                                            spanElement.setAttribute(sAttr, packageAttrValueToIwbAttrName(AttrVal.at(1)));
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                QDomText nodeText = textDoc.createTextNode(spanNode.firstChild().nodeValue());
                spanElement.appendChild(nodeText);
                textParentElement.appendChild(spanElement.cloneNode(true));
            }
            spanNode = spanNode.nextSibling();
        }

        bTbreak   = true;
        htmlPNode = htmlPNode.nextSibling();
    }
}

QString IwbDocumentAdaptor::ToIwbConverter::packageAttrNameToIwbAttrName(QString iwbAttrName)
{
    QString sRet = iwbAttrName;
    if (QString("color") == iwbAttrName)
    {
        sRet = QString("fill");
    }
    if (QString("align") == iwbAttrName)
    {
        sRet = QString("text-align");
    }

    return sRet;
}

QString IwbDocumentAdaptor::ToIwbConverter::packageAttrValueToIwbAttrName(QString iwbValue)
{
    QString sRet = iwbValue;
    if (QString("text") == iwbValue)
    {
        sRet = QString("normal");
    }

    return sRet;
}

bool IwbDocumentAdaptor::ToIwbConverter::setIwbAttribute(
    const QString &attributeName, const QString &attributeValue, const QDomElement &pageElement, QDomElement &iwbElement, QDomElement &svgElement)
{
    bool bRet             = true;
    bool bNeedsIWBSection = false;

    if (itIsIWBAttribute(attributeName))
    {
        if (!((aBackground == attributeName) && (avFalse == attributeValue)))
        {
            iwbElement.setAttribute(attributeName, attributeValue);
            bNeedsIWBSection = true;
        }
    }
    else if (isPageAttributeToConvert(attributeName))
    {
        if (aTransform == attributeName)
        {
            setGeometryFromPageElement(pageElement, svgElement);
        }
        else if (attributeName.contains(aPageUuid))
        {
            QString parentId = pageElement.attribute(aPageParent);
            QString id;
            if (!parentId.isEmpty())
            {
                id = "{" + parentId + "}" + "{" + pageElement.attribute(aPageUuid) + "}";
            }
            else
            {
                id = "{" + pageElement.attribute(aPageUuid) + "}";
            }

            svgElement.setAttribute(aID, id);
        }
        else if (attributeName.contains(aPageHref) || attributeName.contains(aSrc))
        {
            bRet             &= setContentFromPageElement(pageElement, svgElement);
            bNeedsIWBSection  = bRet || bNeedsIWBSection;
        }
    }
    else if (itIsSVGElementAttribute(svgElement.tagName(), attributeName))
    {
        svgElement.setAttribute(attributeName, attributeValue);
    }

    if (bNeedsIWBSection)
    {
        if (0 < iwbElement.attributes().count())
        {
            QStringList tl = pageElement.attribute(aSVGHref).split("/");
            QString     id = tl.at(tl.count() - 1);
            // 元素已有 id 时沿用，保证同一页面内 SVG 元素与 iwb:element 的 ref 对应；
            // 没有 href 的图元（例如矩形）才生成新 UUID。
            if (QString() == id)
            {
                id = QUuid::createUuid().toString();
            }

            svgElement.setAttribute(aID, id);
            iwbElement.setAttribute(aRef, id);
        }
    }

    return bRet;
}

bool IwbDocumentAdaptor::ToIwbConverter::copyPageCommonAttributes(const QDomElement &pageElement, QDomElement &iwbElement, QDomElement &svgElement)
{
    bool bRet = true;

    for (int i = 0; i < pageElement.attributes().count(); i++)
    {
        QDomNode attribute     = pageElement.attributes().item(i);
        QString  attributeName = packageAttrNameToIwbAttrName(attribute.nodeName().remove("tb:"));

        bRet &= setIwbAttribute(attributeName, packageAttrValueToIwbAttrName(attribute.nodeValue()), pageElement, iwbElement, svgElement);
        if (!bRet)
        {
            break;
        }
    }
    return bRet;
}

void IwbDocumentAdaptor::ToIwbConverter::setViewBox(QRect viewbox)
{
    mViewbox |= viewbox;
}

QDomNode IwbDocumentAdaptor::ToIwbConverter::findTextNode(const QDomNode &node)
{
    QDomNode iterNode = node;

    while (!iterNode.isNull())
    {
        if (iterNode.isText())
        {
            if (!iterNode.isNull())
            {
                return iterNode;
            }
        }
        else
        {
            if (!iterNode.firstChild().isNull())
            {
                QDomNode foundNode = findTextNode(iterNode.firstChild());
                if (!foundNode.isNull())
                {
                    if (foundNode.isText())
                    {
                        return foundNode;
                    }
                }
            }
        }
        if (!iterNode.nextSibling().isNull())
        {
            iterNode = iterNode.nextSibling();
        }
        else
        {
            break;
        }
    }
    return iterNode;
}

QDomNode IwbDocumentAdaptor::ToIwbConverter::findNodeByTagName(const QDomNode &node, QString tagName)
{
    QDomNode iterNode = node;

    while (!iterNode.isNull())
    {
        QString t = iterNode.toElement().tagName();
        if (tagName == t)
        {
            return iterNode;
        }
        else
        {
            if (!iterNode.firstChildElement().isNull())
            {
                QDomNode foundNode = findNodeByTagName(iterNode.firstChildElement(), tagName);
                if (!foundNode.isNull())
                {
                    if (foundNode.isElement())
                    {
                        if (tagName == foundNode.toElement().tagName())
                        {
                            return foundNode;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }

        if (!iterNode.nextSibling().isNull())
        {
            iterNode = iterNode.nextSibling();
        }
        else
        {
            break;
        }
    }
    return QDomNode();
}

bool IwbDocumentAdaptor::ToIwbConverter::createBackground(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList)
{
    qDebug() << "|creating element background";

    QDomDocument doc;

    QDomElement svgBackgroundElementPart = doc.createElementNS(svgIWBNS, svgIWBNSPrefix + ":" + tIWBRect);
    QDomElement iwbBackgroundElementPart = doc.createElementNS(iwbNS, iwbNsPrefix + ":" + tElement);

    QRect bckRect(mViewbox);

    if (0 <= mViewbox.topLeft().x())
    {
        bckRect.topLeft().setX(0);
    }

    if (0 <= mViewbox.topLeft().y())
    {
        bckRect.topLeft().setY(0);
    }

    if (QRect() != bckRect)
    {
        QString sElementID  = QUuid::createUuid().toString();

        bool darkBackground = (avTrue == element.attribute(aDarkBackground));
        svgBackgroundElementPart.setAttribute(aFill, darkBackground ? "black" : "white");
        svgBackgroundElementPart.setAttribute(aID, sElementID);
        svgBackgroundElementPart.setAttribute(aX, bckRect.x());
        svgBackgroundElementPart.setAttribute(aY, bckRect.y());
        svgBackgroundElementPart.setAttribute(aHeight, bckRect.height());
        svgBackgroundElementPart.setAttribute(aWidth, bckRect.width());

        iwbBackgroundElementPart.setAttribute(aRef, sElementID);
        iwbBackgroundElementPart.setAttribute(aBackground, avTrue);

        addSVGElementToResultModel(svgBackgroundElementPart, dstSvgList, DEFAULT_BACKGROUND_LAYER);
        addIWBElementToResultModel(iwbBackgroundElementPart);
        return true;
    }
    else
    {
        qDebug() << "|error at creating element background";
        errorStr = "CreatingElementBackgroundParsingError.";
        return false;
    }
}

QString IwbDocumentAdaptor::ToIwbConverter::createBackgroundImage(const QDomElement &element, QSize size)
{
    QString sRet;

    QString sDstFileName(fIWBBackground);

    bool bDirExists = true;
    QDir dstDocFolder(destinationPath);

    if (!dstDocFolder.exists(cfImages))
    {
        bDirExists &= dstDocFolder.mkdir(cfImages);
    }

    QString dstFilePath;
    if (bDirExists)
    {
        dstFilePath = destinationPath + "/" + cfImages + "/" + sDstFileName;
    }

    if (!QFile().exists(dstFilePath))
    {
        QRect rect(0, 0, size.width(), size.height());

        QImage *bckImage    = new QImage(size, QImage::Format_RGB888);

        QPainter *painter   = new QPainter(bckImage);

        bool darkBackground = (avTrue == element.attribute(aDarkBackground));

        QColor bCrossColor;

        bCrossColor  = darkBackground ? QColor(Qt::white) : QColor(Qt::blue);
        int penAlpha = (int)(255 / 2);
        bCrossColor.setAlpha(penAlpha);
        painter->setPen(bCrossColor);
        painter->setBrush(darkBackground ? QColor(Qt::black) : QColor(Qt::white));

        painter->drawRect(rect);

        if (avTrue == element.attribute(aCrossedBackground))
        {
            qreal firstY = ((int)(rect.y() / iCrossSize)) * iCrossSize;

            for (qreal yPos = firstY; yPos <= rect.y() + rect.height(); yPos += iCrossSize)
            {
                painter->drawLine(rect.x(), yPos, rect.x() + rect.width(), yPos);
            }

            qreal firstX = ((int)(rect.x() / iCrossSize)) * iCrossSize;

            for (qreal xPos = firstX; xPos <= rect.x() + rect.width(); xPos += iCrossSize)
            {
                painter->drawLine(xPos, rect.y(), xPos, rect.y() + rect.height());
            }
        }

        painter->end();
        painter->save();

        if (QString() != dstFilePath)
        {
            if (bckImage->save(dstFilePath))
            {
                sRet = cfImages + "/" + sDstFileName;
            }
        }

        delete bckImage;
        delete painter;
    }
    else
    {
        sRet = cfImages + "/" + sDstFileName;
    }

    return sRet;
}

bool IwbDocumentAdaptor::ToIwbConverter::createPngFromSvg(QString &svgPath, QString &dstPath, QTransform transformation, QSize size)
{
    if (QFile().exists(svgPath))
    {
        QImage i(svgPath);

        QSize iSize = (QSize() == size) ? QSize(i.size().width() * transformation.m11(), i.size().height() * transformation.m22()) : size;

        QImage image(iSize, QImage::Format_ARGB32_Premultiplied);
        image.fill(0);
        QPainter     imagePainter(&image);
        QSvgRenderer renderer(svgPath);
        renderer.render(&imagePainter);

        return image.save(dstPath);
    }
    else
    {
        return false;
    }
}

bool IwbDocumentAdaptor::ToIwbConverter::parseSVGGGroup(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList)
{
    qDebug() << "|parsing g section";
    QDomElement nextElement = element.firstChildElement();
    if (nextElement.isNull())
    {
        qDebug() << "Empty g element";
        errorStr = "EmptyGSection";
        return false;
    }

    QMultiMap<int, QDomElement> svgElements;

    QDomDocument doc;
    QDomElement  svgElementPart = doc.createElementNS(svgIWBNS, svgIWBNSPrefix + ":" + tIWBG);
    QDomElement  iwbElementPart = doc.createElementNS(iwbNS, iwbNsPrefix + ":" + tElement);

    // 元素的 z-value 不一定等于插入顺序，先按层号收集再统一排序写出。
    while (!nextElement.isNull())
    {
        QString tagName = nextElement.tagName();
        if (tagName == tPageLine)
        {
            parsePageLine(nextElement, svgElements);
        }
        else if (tagName == tPagePolygon)
        {
            parsePagePolygon(nextElement, svgElements);
        }
        else if (tagName == tPagePolyline)
        {
            parsePagePolyline(nextElement, svgElements);
        }

        nextElement = nextElement.nextSiblingElement();
    }

    QList<int>                          layers;
    QMultiMapIterator<int, QDomElement> nextSVGElement(svgElements);
    while (nextSVGElement.hasNext())
    {
        layers << nextSVGElement.next().key();
    }

    std::sort(layers.begin(), layers.end());
    // <g> 里可能只有本转换器不支持的元素（text/rect 等），此时 layers 为空；
    // 用默认层导出空组，而不是 at(0) 越界。
    int layer = layers.isEmpty() ? DEFAULT_LAYER : layers.at(0);

    nextSVGElement.toFront();
    while (nextSVGElement.hasNext())
    {
        svgElementPart.appendChild(nextSVGElement.next().value());
    }

    addSVGElementToResultModel(svgElementPart, dstSvgList, layer);

    return true;
}

bool IwbDocumentAdaptor::ToIwbConverter::parsePageImage(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList)
{
    qDebug() << "|parsing image";

    QDomDocument doc;

    QDomElement svgElementPart = doc.createElementNS(svgIWBNS, svgIWBNSPrefix + ":" + pageElementTypeName(element));
    QDomElement iwbElementPart = doc.createElementNS(iwbNS, iwbNsPrefix + ":" + tElement);

    if (copyPageCommonAttributes(element, iwbElementPart, svgElementPart))
    {
        addSVGElementToResultModel(svgElementPart, dstSvgList, getElementLayer(element));

        if (0 < iwbElementPart.attributes().count())
        {
            addIWBElementToResultModel(iwbElementPart);
        }
        return true;
    }
    else
    {
        qDebug() << "|error at image parsing";
        errorStr = "ImageParsingError";
        return false;
    }
}

bool IwbDocumentAdaptor::ToIwbConverter::parsePageVideo(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList)
{
    qDebug() << "|parsing video";

    QDomDocument doc;

    QDomElement svgElementPart = doc.createElementNS(svgIWBNS, svgIWBNSPrefix + ":" + pageElementTypeName(element));
    QDomElement iwbElementPart = doc.createElementNS(iwbNS, iwbNsPrefix + ":" + tElement);

    if (copyPageCommonAttributes(element, iwbElementPart, svgElementPart))
    {
        QDomElement svgSwitchSection = doc.createElementNS(svgIWBNS, svgIWBNSPrefix + ":" + tIWBSwitch);
        svgSwitchSection.appendChild(svgElementPart);

        QDomElement svgText = doc.createElementNS(svgIWBNS, svgIWBNSPrefix + ":" + tIWBTextArea);
        svgText.setAttribute(aX, svgElementPart.attribute(aX));
        svgText.setAttribute(aY, svgElementPart.attribute(aY));
        svgText.setAttribute(aWidth, svgElementPart.attribute(aWidth));
        svgText.setAttribute(aHeight, svgElementPart.attribute(aHeight));
        svgText.setAttribute(aTransform, svgElementPart.attribute(aTransform));

        QDomText text = doc.createTextNode("Cannot Open Content");
        svgText.appendChild(text);

        svgSwitchSection.appendChild(svgText);

        addSVGElementToResultModel(svgSwitchSection, dstSvgList, getElementLayer(element));

        if (0 < iwbElementPart.attributes().count())
        {
            addIWBElementToResultModel(iwbElementPart);
        }
        return true;
    }
    else
    {
        qDebug() << "|error at video parsing";
        errorStr = "VideoParsingError";
        return false;
    }
}

bool IwbDocumentAdaptor::ToIwbConverter::parsePageAudio(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList)
{
    qDebug() << "|parsing audio";

    QDomDocument doc;

    QDomElement svgElementPart = doc.createElementNS(svgIWBNS, svgIWBNSPrefix + ":" + pageElementTypeName(element));
    QDomElement iwbElementPart = doc.createElementNS(iwbNS, iwbNsPrefix + ":" + tElement);

    if (copyPageCommonAttributes(element, iwbElementPart, svgElementPart))
    {
        // IWB 的 <a> 元素只能引用媒体，图形本身要另写一个 image 作为可点击封面，
        // 因此这里先给音频生成 png 图标，再把原来的 audio 元素改名成 image。
        int     audioImageDimention = qMin(svgElementPart.attribute(aWidth).toInt(), svgElementPart.attribute(aHeight).toInt());
        QString srcAudioImageFile(sAudioElementImage);
        QString elementId                 = QString(QUuid::createUuid().toString());
        QString sDstAudioImageFileName    = elementId + "." + fePng;
        QString dstAudioImageFilePath     = destinationPath + "/" + cfImages + "/" + sDstAudioImageFileName;
        QString dstAudioImageRelativePath = cfImages + "/" + sDstAudioImageFileName;

        QFile srcFile(srcAudioImageFile);

        // 音频图标与普通图片共用 images/ 目录，按元素 uuid 命名避免冲突。
        QDir dstDocFolder(destinationPath);
        bool bRes = true;
        if (!dstDocFolder.exists(cfImages))
        {
            bRes &= dstDocFolder.mkdir(cfImages);
        }

        if (bRes && createPngFromSvg(
                        srcAudioImageFile, dstAudioImageFilePath, transformOfPageElement(element), QSize(audioImageDimention, audioImageDimention)))
        {
            // <a> 的 href 指向音频，image 的 href 指向图标，两者共同构成可播放元素。
            QDomElement svgASection = doc.createElementNS(svgIWBNS, svgIWBNSPrefix + ":" + tIWBA);
            svgASection.setAttribute(aSVGHref, svgElementPart.attribute(aSVGHref));

            svgElementPart.setTagName(tIWBImage);
            svgElementPart.setAttribute(aSVGHref, dstAudioImageRelativePath);
            svgElementPart.setAttribute(aHeight, audioImageDimention);
            svgElementPart.setAttribute(aWidth, audioImageDimention);

            svgASection.appendChild(svgElementPart);

            QDomElement svgText = doc.createElementNS(svgIWBNS, svgIWBNSPrefix + ":" + tIWBTextArea);
            svgText.setAttribute(aX, svgElementPart.attribute(aX));
            svgText.setAttribute(aY, svgElementPart.attribute(aY));
            svgText.setAttribute(aWidth, svgElementPart.attribute(aWidth));
            svgText.setAttribute(aHeight, svgElementPart.attribute(aHeight));
            svgText.setAttribute(aTransform, svgElementPart.attribute(aTransform));

            QDomText text = doc.createTextNode("Cannot Open Content");
            svgText.appendChild(text);

            addSVGElementToResultModel(svgASection /*svgSwitchSection*/, dstSvgList, getElementLayer(element));

            if (0 < iwbElementPart.attributes().count())
            {
                addIWBElementToResultModel(iwbElementPart);
            }
            return true;
        }
        return false;
    }
    else
    {
        qDebug() << "|error at audio parsing";
        errorStr = "AudioParsingError";
        return false;
    }
}

bool IwbDocumentAdaptor::ToIwbConverter::parseForeignObject(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList)
{
    if (element.attribute(aPageType) == avPageText)
    {
        return parsePageText(element, dstSvgList);
    }

    qDebug() << "|parsing foreign object";

    QDomDocument doc;

    QDomElement svgElementPart = doc.createElementNS(svgIWBNS, svgIWBNSPrefix + ":" + pageElementTypeName(element));
    QDomElement iwbElementPart = doc.createElementNS(iwbNS, iwbNsPrefix + ":" + tElement);

    if (copyPageCommonAttributes(element, iwbElementPart, svgElementPart))
    {
        addSVGElementToResultModel(svgElementPart, dstSvgList, getElementLayer(element));
        if (0 < iwbElementPart.attributes().count())
        {
            addIWBElementToResultModel(iwbElementPart);
        }
        return true;
    }
    else
    {
        qDebug() << "|error at parsing foreign object";
        errorStr = "ForeignObjectParsingError";
        return false;
    }
}

bool IwbDocumentAdaptor::ToIwbConverter::parsePageText(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList)
{
    qDebug() << "|parsing text";

    QDomDocument doc;

    QDomElement svgElementPart = doc.createElementNS(svgIWBNS, svgIWBNSPrefix + ":" + pageElementTypeName(element));
    QDomElement iwbElementPart = doc.createElementNS(iwbNS, iwbNsPrefix + ":" + tElement);

    if (element.hasChildNodes())
    {
        QDomDocument htmlDoc;
        htmlDoc.setContent(findTextNode(element).nodeValue());
        QDomNode bodyNode = findNodeByTagName(htmlDoc.firstChildElement(), "body");

        setIwbTextFromHTMLTextNode(bodyNode.toElement(), svgElementPart);

        if (copyPageCommonAttributes(element, iwbElementPart, svgElementPart))
        {
            QString commonParams;
            for (int i = 0; i < bodyNode.attributes().count(); i++)
            {
                commonParams += " " + bodyNode.attributes().item(i).nodeValue();
            }
            commonParams.remove(" ");
            commonParams.remove("'");

            QStringList commonAttributes = commonParams.split(";", Qt::SkipEmptyParts);
            for (int i = 0; i < commonAttributes.count(); i++)
            {
                QStringList AttrVal = commonAttributes.at(i).split(":", Qt::SkipEmptyParts);
                if (1 < AttrVal.count())
                {
                    QString sAttr = packageAttrNameToIwbAttrName(AttrVal.at(0));
                    QString sVal  = packageAttrValueToIwbAttrName(AttrVal.at(1));

                    setIwbAttribute(sAttr, sVal, element, iwbElementPart, svgElementPart);
                }
            }
            addSVGElementToResultModel(svgElementPart, dstSvgList, getElementLayer(element));
            if (0 < iwbElementPart.attributes().count())
            {
                addIWBElementToResultModel(iwbElementPart);
            }
            return true;
        }
        return false;
    }
    else
    {
        qDebug() << "|error at text parsing";
        errorStr = "TextParsingError";
        return false;
    }
}

bool IwbDocumentAdaptor::ToIwbConverter::parsePagePolygon(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList)
{
    qDebug() << "||parsing polygon";

    QDomDocument doc;

    QDomElement svgElementPart = doc.createElementNS(svgIWBNS, svgIWBNSPrefix + ":" + pageElementTypeName(element));
    QDomElement iwbElementPart = doc.createElementNS(iwbNS, iwbNsPrefix + ":" + tElement);

    if (copyPageCommonAttributes(element, iwbElementPart, svgElementPart))
    {
        svgElementPart.setAttribute(aStroke, svgElementPart.attribute(aFill));
        addSVGElementToResultModel(svgElementPart, dstSvgList, getElementLayer(element));

        if (0 < iwbElementPart.attributes().count())
        {
            QString id = svgElementPart.attribute(aPageUuid);
            if (id.isEmpty())
            {
                id = QUuid::createUuid().toString();
            }

            svgElementPart.setAttribute(aID, id);
            iwbElementPart.setAttribute(aRef, id);

            addIWBElementToResultModel(iwbElementPart);
        }
        return true;
    }
    else
    {
        qDebug() << "||error at parsing polygon";
        errorStr = "PolygonParsingError";
        return false;
    }
}

bool IwbDocumentAdaptor::ToIwbConverter::parsePagePolyline(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList)
{
    qDebug() << "||parsing polyline";
    QDomElement resElement;

    QDomDocument doc;

    QDomElement svgElementPart = doc.createElementNS(svgIWBNS, svgIWBNSPrefix + ":" + pageElementTypeName(element));
    QDomElement iwbElementPart = doc.createElementNS(iwbNS, iwbNsPrefix + ":" + tElement);

    if (copyPageCommonAttributes(element, iwbElementPart, svgElementPart))
    {
        svgElementPart.setAttribute(aStroke, svgElementPart.attribute(aFill));
        addSVGElementToResultModel(svgElementPart, dstSvgList, getElementLayer(element));

        if (0 < iwbElementPart.attributes().count())
        {
            QString id = QUuid::createUuid().toString();
            svgElementPart.setAttribute(aID, id);
            iwbElementPart.setAttribute(aRef, id);

            addIWBElementToResultModel(iwbElementPart);
        }
        return true;
    }
    else
    {
        qDebug() << "||error at parsing polygon";
        errorStr = "PolylineParsingError";
        return false;
    }
}

bool IwbDocumentAdaptor::ToIwbConverter::parsePageLine(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList)
{
    qDebug() << "||parsing line";
    QDomElement  resElement;
    QDomDocument doc;

    QDomElement svgElementPart = doc.createElementNS(svgIWBNS, svgIWBNSPrefix + ":" + pageElementTypeName(element));
    QDomElement iwbElementPart = doc.createElementNS(iwbNS, iwbNsPrefix + ":" + tElement);

    if (copyPageCommonAttributes(element, iwbElementPart, svgElementPart))
    {
        svgElementPart.setAttribute(aStroke, svgElementPart.attribute(aFill));
        addSVGElementToResultModel(svgElementPart, dstSvgList, getElementLayer(element));

        if (0 < iwbElementPart.attributes().count())
        {
            QString id = QUuid::createUuid().toString();
            svgElementPart.setAttribute(aID, id);
            iwbElementPart.setAttribute(aRef, id);

            addIWBElementToResultModel(iwbElementPart);
        }
    }
    else
    {
        qDebug() << "||error at parsing polygon";
        errorStr = "LineParsingError";
        return false;
    }
    return true;
}

void IwbDocumentAdaptor::ToIwbConverter::addSVGElementToResultModel(const QDomElement &element, QMultiMap<int, QDomElement> &dstList, int layer)
{
    int elementLayer        = (DEFAULT_LAYER == layer) ? DEFAULT_LAYER : layer;

    QDomElement rootElement = element.cloneNode(true).toElement();
    mDocumentToWrite->firstChildElement().appendChild(rootElement);
    dstList.insert(elementLayer, rootElement);
}

void IwbDocumentAdaptor::ToIwbConverter::addIWBElementToResultModel(const QDomElement &element)
{
    QDomElement rootElement = element.cloneNode(true).toElement();
    mDocumentToWrite->firstChildElement().appendChild(rootElement);
    mExtendedElements.append(rootElement);
}

IwbDocumentAdaptor::ToIwbConverter::~ToIwbConverter()
{
    if (mDataModel)
    {
        delete mDataModel;
    }
    if (mIWBContentWriter)
    {
        delete mIWBContentWriter;
    }
    if (mDocumentToWrite)
    {
        delete mDocumentToWrite;
    }
}
bool IwbDocumentAdaptor::ToIwbConverter::isValid() const
{
    bool result = QFileInfo(sourcePath).exists() && QFileInfo(sourcePath).isDir() && errorStr == noErrorMsg;

    if (!result)
    {
        qDebug() << "specified data is not valid";
        errorStr = "ValidateDataError";
    }

    return result;
}

void IwbDocumentAdaptor::ToIwbConverter::fillNamespaces()
{
    mIWBContentWriter->writeDefaultNamespace(svgPageNS);
    mIWBContentWriter->writeNamespace(iwbNS, iwbNsPrefix);
    mIWBContentWriter->writeNamespace(svgIWBNS, svgIWBNSPrefix);
    mIWBContentWriter->writeNamespace(xlinkNS, xlinkNSPrefix);
}

QString IwbDocumentAdaptor::ToIwbConverter::digitFileFormat(int digit) const
{
    return QString("%1").arg(digit, 3, 10, QLatin1Char('0'));
}
QString IwbDocumentAdaptor::ToIwbConverter::contentIWBFileName() const
{
    return destinationPath + "/" + fIWBContent;
}

// 解析 metadata.rdf 里的 "宽x高" 尺寸串；格式不合法时返回空 QSize 由调用方报错。
QSize IwbDocumentAdaptor::ToIwbConverter::getSVGDimentions(const QString &element)
{
    QStringList dimList;

    dimList = element.split(dimensionsDelimiter1, Qt::KeepEmptyParts);
    if (dimList.count() != 2)
    {
        return QSize();
    }

    bool ok;

    int width = dimList.takeFirst().toInt(&ok);
    if (!ok || !width)
    {
        return QSize();
    }

    int height = dimList.takeFirst().toInt(&ok);
    if (!ok || !height)
    {
        return QSize();
    }

    return QSize(width, height);
}

QRect IwbDocumentAdaptor::ToIwbConverter::getViewboxRect(const QString &element) const
{
    QStringList dimList;

    dimList = element.split(dimensionsDelimiter2, Qt::KeepEmptyParts);
    if (dimList.count() != 4)
    {
        return QRect();
    }

    bool ok = false;

    int x   = dimList.takeFirst().toInt(&ok);
    if (!ok || !x)
    {
        return QRect();
    }

    int y = dimList.takeFirst().toInt(&ok);
    if (!ok || !y)
    {
        return QRect();
    }

    int width = dimList.takeFirst().toInt(&ok);
    if (!ok || !width)
    {
        return QRect();
    }

    int height = dimList.takeFirst().toInt(&ok);
    if (!ok || !height)
    {
        return QRect();
    }

    return QRect(x, y, width, height);
}

QString IwbDocumentAdaptor::ToIwbConverter::rectToIWBAttr(const QRect &rect) const
{
    if (rect.isNull())
    {
        return QString();
    }

    return QString("%1 %2 %3 %4").arg(rect.topLeft().x()).arg(rect.topLeft().y()).arg(rect.width()).arg(rect.height());
}
