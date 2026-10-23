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
    qDebug() << "开始转换:" << from << "->" << to;

    QString source = QString();
    if (QFileInfo(from).isDir() && QFile::exists(from))
    {
        qDebug() << "给的是目录，继续转换";
        source = from;
    }
    else
    {
        source = uncompressZip(from);
        if (!source.isNull())
        {
            qDebug() << "给的是 zip 包，已解压到临时目录，继续转换";
        }
    }
    if (source.isNull())
    {
        qDebug() << "给的路径既不是目录也不是 zip 包，停止转换";
        return false;
    }

    QString tmpDestination = createNewTmpDir();
    if (tmpDestination.isNull())
    {
        qDebug() << "建不出临时目标目录，停止解析";
        return false;
    }

    ToIwbConverter tmpConvertrer(source, tmpDestination);
    if (!tmpConvertrer)
    {
        qDebug() << "转换器对象不可用，停止转换，错误信息:" << tmpConvertrer.lastErrStr();
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
        qDebug() << "压缩打包出错";
    }

    // 源是 .tbz 时解压出来的是临时目录，转换完立即删除；源本身是目录时保留。
    if (!QFileInfo(from).isDir())
    {
        if (!freeDir(source))
        {
            qDebug() << "删不掉临时目录" << QDir(source).absolutePath() << "（请手动删除）";
        }
    }

    if (!freeDir(tmpDestination))
    {
        qDebug() << "删不掉临时目录" << QDir(tmpDestination).absolutePath() << "（请手动删除）";
    }

    return true;
}

QString IwbDocumentAdaptor::uncompressZip(const QString &zipFile)
{
    QuaZip zip(zipFile);

    if (!zip.open(QuaZip::mdUnzip))
    {
        qWarning() << "导入失败，压缩包打不开，zip 错误码:" << zip.getZipError();
        return QString();
    }

    zip.setFileNameCodec("UTF-8");
    QuaZipFileInfo info;
    QuaZipFile     file(&zip);

    QString documentRootFolder = createNewTmpDir();

    if (documentRootFolder.isNull())
    {
        qDebug() << "为 zip 包建不出临时目录:" << zipFile;
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
            qWarning() << "导入失败，读不到当前条目信息，zip 错误码:" << zip.getZipError();
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
            qWarning() << "导入失败，取条目名出错，zip 错误码:" << zip.getZipError();
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
            qWarning() << "导入失败，zip 错误码:" << zip.getZipError();
            allOk = false;
            break;
        }
        if (!file.atEnd())
        {
            qWarning() << "导入失败，内容读完了却没到文件末尾";
            allOk = false;
            break;
        }
        file.close();

        if (file.getZipError() != UNZ_OK)
        {
            qWarning() << "导入失败，关闭压缩包里的文件出错，zip 错误码:" << file.getZipError();
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
        qWarning() << "导入失败，关闭压缩包出错，zip 错误码:" << zip.getZipError();
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
            qDebug() << "建不出解压文件的目标目录";
            return false;
        }
    }

    QuaZip zip(destination);
    zip.setFileNameCodec("UTF-8");
    if (!zip.open(QuaZip::mdCreate))
    {
        qDebug("导出失败，压缩包打不开，zip 错误码: %d", zip.getZipError());
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
                qDebug() << "压缩目录出错:" << curFile.absoluteFilePath();
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
        qDebug() << "压缩文件" << sourceFile.fileName() << "失败，源文件打不开，原因:" << sourceFile.errorString();
        return false;
    }

    if (!outZip->open(QIODevice::WriteOnly, QuaZipNewInfo(parentDir + QFileInfo(fileName).fileName(), sourceFile.fileName())))
    {
        qDebug() << "压缩文件" << sourceFile.fileName() << "失败，压缩包里的输出打不开，zip 错误码:" << outZip->getZipError();
        sourceFile.close();
        return false;
    }

    outZip->write(sourceFile.readAll());
    if (outZip->getZipError() != UNZ_OK)
    {
        qDebug() << "压缩文件" << sourceFile.fileName() << "失败，写入压缩包内容出错，zip 错误码:" << outZip->getZipError();

        sourceFile.close();
        outZip->close();
        return false;
    }

    if (outZip->getZipError() != UNZ_OK)
    {
        qWarning() << "压缩文件" << sourceFile.fileName() << "失败，关闭压缩包输出出错，zip 错误码:" << outZip->getZipError();

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
                qDebug() << "建不出临时目录，可能是权限不够";
                return QString();
            }
        }
        else if (tmpNumber == 10)
        {
            qWarning() << "导入失败，建不出临时文件";
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
        qDebug() << "文档元数据无效，没法解析";
        return false;
    }

    qDebug() << "开始解析 tbz";

    QFile outFile(contentIWBFileName());
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qDebug() << "输出文件打不开，写不进去";
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

    qDebug() << "解析完成";

    return true;
}
bool IwbDocumentAdaptor::ToIwbConverter::parseMetadata()
{
    QFile metaDataFile(sourcePath + "/" + fMetadata);

    if (!metaDataFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        errorStr = "can't open" + QFileInfo(sourcePath + "/" + fMetadata).absoluteFilePath();
        qDebug() << errorStr;
        return false;
    }
    else if (QDomDocument::ParseResult parseResult =
                 mDataModel->setContent(metaDataFile.readAll(), QDomDocument::ParseOption::UseNamespaceProcessing);
        !parseResult)
    {
        qWarning() << "解析 XML 出错，行:" << parseResult.errorLine << "列:" << parseResult.errorColumn << ":" << parseResult.errorMessage;
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
                        qDebug() << "读不出 svg 部分的尺寸";
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
        qDebug() << "找不到任何内容文件";
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
    qDebug() << "开始解析页面：" + pageFileName;
    mSvgElements.clear();

    QFile pageFile(sourcePath + "/" + pageFileName);
    if (!pageFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "打不开文件" << pageFileName << "来读取";
        return QDomElement();
    }
    else if (QDomDocument::ParseResult parseResult = mDataModel->setContent(pageFile.readAll(), QDomDocument::ParseOption::UseNamespaceProcessing);
        !parseResult)
    {
        qWarning() << "解析 XML 出错，行:" << parseResult.errorLine << "列:" << parseResult.errorColumn << ":" << parseResult.errorMessage;
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
                qDebug() << "这一页是空的。";
                pageFile.close();
                return QDomElement();
            }
        }
        else if (tagname == tPageGroups)
        {
            group = parseGroupsPageSection(nextTopElement);
            if (group.isNull())
            {
                qDebug() << "这一页里没有任何分组。";
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
        qDebug() << "iwb 扩展内容列表是空的";
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
        qDebug() << "分组根节点是空的";
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

    qDebug() << "解析 tbz 的分组部分";
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
        qDebug() << "|创建背景网格出错";
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
        qDebug() << "IWB 不支持这种格式";
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
                            for (int k = 0; k < iwbAttributes.count(); k++)
                            {
                                QString     attr    = iwbAttributes.at(k).trimmed();
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
    qDebug() << "|创建元素背景";

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
        qDebug() << "|创建元素背景出错";
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
    qDebug() << "|解析 g 分组部分";
    QDomElement nextElement = element.firstChildElement();
    if (nextElement.isNull())
    {
        qDebug() << "g 元素是空的";
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
    qDebug() << "|解析 image 元素";

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
        qDebug() << "|解析 image 出错";
        errorStr = "ImageParsingError";
        return false;
    }
}

bool IwbDocumentAdaptor::ToIwbConverter::parsePageVideo(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList)
{
    qDebug() << "|解析 video 元素";

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
        qDebug() << "|解析 video 出错";
        errorStr = "VideoParsingError";
        return false;
    }
}

bool IwbDocumentAdaptor::ToIwbConverter::parsePageAudio(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList)
{
    qDebug() << "|解析 audio 元素";

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
        qDebug() << "|解析 audio 出错";
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

    qDebug() << "|解析 foreign object 元素";

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
        qDebug() << "|解析 foreign object 出错";
        errorStr = "ForeignObjectParsingError";
        return false;
    }
}

bool IwbDocumentAdaptor::ToIwbConverter::parsePageText(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList)
{
    qDebug() << "|解析 text 元素";

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
        qDebug() << "|解析 text 出错";
        errorStr = "TextParsingError";
        return false;
    }
}

bool IwbDocumentAdaptor::ToIwbConverter::parsePagePolygon(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList)
{
    qDebug() << "||解析 polygon 元素";

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
        qDebug() << "||解析 polygon 出错";
        errorStr = "PolygonParsingError";
        return false;
    }
}

bool IwbDocumentAdaptor::ToIwbConverter::parsePagePolyline(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList)
{
    qDebug() << "||解析 polyline 元素";
    QDomElement resElement;

    QDomDocument doc;

    QDomElement svgElementPart = doc.createElementNS(svgIWBNS, svgIWBNSPrefix + ":" + pageElementTypeName(element));
    QDomElement iwbElementPart = doc.createElementNS(iwbNS, iwbNsPrefix + ":" + tElement);

    if (copyPageCommonAttributes(element, iwbElementPart, svgElementPart))
    {
        // IWB 用 stroke 表示线条颜色，多数图元的颜色在 fill 上，所以要搬一次；但本页的 polyline
        // 颜色本来就写在 stroke 上、fill 恒为 none，无条件照搬会把颜色抹成空串。
        QString strokeValue = svgElementPart.attribute(aStroke);
        if (strokeValue.isEmpty() || strokeValue == QLatin1String("none"))
        {
            svgElementPart.setAttribute(aStroke, svgElementPart.attribute(aFill));
        }
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
        qDebug() << "||解析 polygon 出错";
        errorStr = "PolylineParsingError";
        return false;
    }
}

bool IwbDocumentAdaptor::ToIwbConverter::parsePageLine(const QDomElement &element, QMultiMap<int, QDomElement> &dstSvgList)
{
    qDebug() << "||解析 line 元素";
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
        qDebug() << "||解析 polygon 出错";
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
        qDebug() << "传进来的数据无效";
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
