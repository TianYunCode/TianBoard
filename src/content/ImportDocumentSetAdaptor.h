#pragma once

#include <QtGui>
#include "content/ImportAdaptor.h"

class DocumentProxy;

class ImportDocumentSetAdaptor : public ImportAdaptor
{
    Q_OBJECT

    public:
        ImportDocumentSetAdaptor(QObject *parent = 0);
        virtual ~ImportDocumentSetAdaptor();

        virtual QStringList supportedExtentions();
        virtual QString importFileFilter();

        QFileInfoList importData(const QString &zipFile, const QString &destination);

//        virtual DocumentProxy* importFile(const QFile& pFile, const QString& pGroup);
//        virtual bool addFileToDocument(DocumentProxy* pDocument, const QFile& pFile);

    private:
        bool extractFileToDir(const QFile& pZipFile, const QString& pDir);

};

