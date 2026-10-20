#include "content/MetadataDcSubsetAdaptor.h"

#include <QtWidgets>
#include <QtXml>
#include "core/VirtualDesktop.h"

#include "settings/Settings.h"
#include "core/Application.h"
#include "whiteboard/BoardController.h"

#include "model/DocumentProxy.h"

const QString MetadataDcSubsetAdaptor::nsRdf            = "http://www.w3.org/1999/02/22-rdf-syntax-ns#";
const QString MetadataDcSubsetAdaptor::nsDc             = "http://purl.org/dc/elements/1.1/";
const QString MetadataDcSubsetAdaptor::metadataFilename = "metadata.rdf";

MetadataDcSubsetAdaptor::MetadataDcSubsetAdaptor()
{
}

MetadataDcSubsetAdaptor::~MetadataDcSubsetAdaptor()
{
}

void MetadataDcSubsetAdaptor::persist(DocumentProxy *proxy)
{
    if (!QDir(proxy->persistencePath()).exists())
    {
        // 文档尚未落地（新建后还没保存过），此时没有目录可写，直接跳过。
        return;
    }
    QString fileName = proxy->persistencePath() + "/" + metadataFilename;
    qDebug() << "Persisting document; path is" << fileName;
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qCritical() << "cannot open " << fileName << " for writing ...";
        qCritical() << "error : " << file.errorString();
        return;
    }

    QXmlStreamWriter xmlWriter(&file);
    xmlWriter.setAutoFormatting(true);

    xmlWriter.writeStartDocument();
    xmlWriter.writeDefaultNamespace(nsRdf);
    xmlWriter.writeNamespace(nsDc, "dc");
    xmlWriter.writeNamespace(Settings::documentNamespaceUri, "tb");

    xmlWriter.writeStartElement("RDF");

    xmlWriter.writeStartElement("Description");
    xmlWriter.writeAttribute("about", proxy->metaData(Settings::documentIdentifer).toString());

    xmlWriter.writeTextElement(nsDc, "title", proxy->metaData(Settings::documentName).toString());
    xmlWriter.writeTextElement(nsDc, "type", proxy->metaData(Settings::documentGroupName).toString());
    xmlWriter.writeTextElement(nsDc, "date", proxy->metaData(Settings::documentDate).toString());
    xmlWriter.writeTextElement(nsDc, "format", "image/svg+xml");

    xmlWriter.writeTextElement(nsDc, "identifier", proxy->metaData(Settings::documentIdentifer).toString());
    xmlWriter.writeTextElement(Settings::documentNamespaceUri, "version", Settings::currentFileVersion);
    QString width  = QString::number(proxy->defaultDocumentSize().width());
    QString height = QString::number(proxy->defaultDocumentSize().height());
    xmlWriter.writeTextElement(Settings::documentNamespaceUri, "size", QString("%1x%2").arg(width).arg(height));

    xmlWriter.writeTextElement(Settings::documentNamespaceUri, "updated-at", StringUtils::toUtcIsoDateTime(QDateTime::currentDateTimeUtc()));

    xmlWriter.writeTextElement(Settings::documentNamespaceUri, "page-count", QString::number(proxy->pageCount()));

    xmlWriter.writeEndElement();
    xmlWriter.writeEndElement();

    xmlWriter.writeEndDocument();

    file.flush();
    file.close();
}

QMap<QString, QVariant> MetadataDcSubsetAdaptor::load(QString pPath)
{
    QMap<QString, QVariant> metadata;

    QString fileName = pPath + "/" + metadataFilename;

    QFile file(fileName);

    bool    sizeFound      = false;
    bool    updatedAtFound = false;
    QString date;
    // untagged doc version 4.1。必须声明在循环外：写进 metadata 的动作要等整份 rdf
    // 读完再做，否则 <size>/<updated-at>/<page-count> 每个起始元素都会把它覆盖回 4.1。
    QString docVersion = "4.1";

    if (file.exists())
    {
        if (!file.open(QIODevice::ReadOnly))
        {
            qWarning() << "Cannot open file " << fileName << " for reading ...";
            return metadata;
        }

        QXmlStreamReader xml(&file);

        while (!xml.atEnd())
        {
            xml.readNext();

            if (xml.isStartElement())
            {
                if (xml.name() == "title")
                {
                    metadata.insert(Settings::documentName, xml.readElementText());
                }
                else if (xml.name() == "type")
                {
                    metadata.insert(Settings::documentGroupName, xml.readElementText());
                }
                else if (xml.name() == "date")
                {
                    date = xml.readElementText();
                }
                else if (xml.name() == "identifier")
                {
                    metadata.insert(Settings::documentIdentifer, xml.readElementText());
                }
                else if (xml.name() == "version" && xml.namespaceUri() == Settings::documentNamespaceUri)
                {
                    docVersion = xml.readElementText();
                }
                else if (xml.name() == "size" && xml.namespaceUri() == Settings::documentNamespaceUri)
                {
                    QString     size      = xml.readElementText();
                    QStringList sizeParts = size.split("x");
                    bool        ok        = false;
                    int         width, height;
                    if (sizeParts.count() >= 2)
                    {
                        bool widthOK, heightOK;
                        width  = sizeParts.at(0).toInt(&widthOK);
                        height = sizeParts.at(1).toInt(&heightOK);
                        ok     = widthOK && heightOK;

                        QSize docSize(width, height);

                        if (width == 1024 && height == 768) // 旧版默认页面尺寸，迁移到当前配置尺寸
                        {
                            docSize = Settings::settings()->pageSize->get().toSize();
                        }

                        metadata.insert(Settings::documentSize, QVariant(docSize));
                    }
                    if (!ok)
                    {
                        qDebug() << "Invalid document size:" << size;
                    }

                    sizeFound = true;
                }
                else if (xml.name() == "updated-at" && xml.namespaceUri() == Settings::documentNamespaceUri)
                {
                    metadata.insert(Settings::documentUpdatedAt, xml.readElementText());
                    updatedAtFound = true;
                }
                else if (xml.name() == "page-count" && xml.namespaceUri() == Settings::documentNamespaceUri)
                {
                    metadata.insert(Settings::documentPageCount, xml.readElementText());
                }
            }

            if (xml.hasError())
            {
                qWarning() << "error parsing metadata.rdf file " << xml.errorString();
            }
        }

        metadata.insert(Settings::documentVersion, docVersion);
        file.close();
    }

    if (!sizeFound)
    {
        VirtualDesktop *dw                 = VirtualDesktop::instance();
        int             controlScreenIndex = dw->primaryScreen();

        QSize docSize                      = dw->screenGeometry(controlScreenIndex).size();
        docSize.setHeight(docSize.height() - 70); // 扣掉工具栏高度，避免页面被工具条遮住

        qDebug() << "Document size not found, using default view size" << docSize;

        metadata.insert(Settings::documentSize, QVariant(docSize));
    }

    // 旧文件只写了日期没有时间，补成 UTC 零点，保证后续按 ISO 时间解析不失败。
    QString dateString = metadata.value(Settings::documentDate).toString();
    if (dateString.length() < 10)
    {
        metadata.remove(Settings::documentDate);
        metadata.insert(Settings::documentDate, dateString + "T00:00:00Z");
    }

    if (!updatedAtFound)
    {
        metadata.insert(Settings::documentUpdatedAt, dateString);
    }

    metadata.insert(Settings::documentDate, QVariant(date));

    return metadata;
}
