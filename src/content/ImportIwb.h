#pragma once

#include <QtGui>
#include "content/ImportAdaptor.h"

class DocumentProxy;

class ImportIwb : public DocumentBasedImportAdaptor {
    Q_OBJECT;

public:
    ImportIwb(QObject *parent = 0);
    virtual ~ImportIwb();

    virtual QStringList supportedExtentions();
    virtual QString importFileFilter();

    virtual bool addFileToDocument(DocumentProxy *pDocument, const QFile &pFile);
    virtual DocumentProxy *importFile(const QFile &pFile, const QString &pGroup);

private:
    QString expandFileToDir(const QFile &pZipFile, const QString &pDir);
};
