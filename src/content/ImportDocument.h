#pragma once

#include <QtGui>
#include "content/ImportAdaptor.h"

class DocumentProxy;

class ImportDocument : public DocumentBasedImportAdaptor {
    Q_OBJECT;

public:
    ImportDocument(QObject *parent = 0);
    virtual ~ImportDocument();

    virtual QStringList supportedExtentions();
    virtual QString importFileFilter();

    virtual DocumentProxy *importFile(const QFile &pFile, const QString &pGroup);
    virtual bool addFileToDocument(DocumentProxy *pDocument, const QFile &pFile);

private:
    bool extractFileToDir(const QFile &pZipFile, const QString &pDir, QString &documentRoot);
};
