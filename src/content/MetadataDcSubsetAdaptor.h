#pragma once

#include <QtWidgets>

class DocumentProxy;

class MetadataDcSubsetAdaptor {
public:
    MetadataDcSubsetAdaptor();
    virtual ~MetadataDcSubsetAdaptor();

    static void persist(DocumentProxy *proxy);
    static QMap<QString, QVariant> load(QString pPath);

    static const QString nsRdf;
    static const QString nsDc;
    static const QString metadataFilename;

    static const QString rdfIdentifierDomain;
};
