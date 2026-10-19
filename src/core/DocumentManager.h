#pragma once

#include <QtCore>

class ExportAdaptor;
class ImportAdaptor;
class DocumentProxy;

class DocumentManager : public QObject
{
    Q_OBJECT

    public:
        static DocumentManager* documentManager();
        virtual ~DocumentManager();


        QString importFileFilter(bool notUbx = false);

        QStringList importFileExtensions(bool notUbx = false);

        QFileInfoList importUbx(const QString &Incomingfile, const QString &destination);
        DocumentProxy* importFile(const QFile& pFile, const QString& pGroup);

        int addFilesToDocument(DocumentProxy* pDocument, QStringList fileNames);

        DocumentProxy* importDir(const QDir& pDir, const QString& pGroup);
        int addImageDirToDocument(const QDir& pDir, DocumentProxy* pDocument);

        QList<ExportAdaptor*> supportedExportAdaptors();
        void emitDocumentUpdated(DocumentProxy* pDocument);

    signals:
        void documentUpdated(DocumentProxy *pDocument);

    private:
        DocumentManager(QObject *parent = 0);
        QList<ExportAdaptor*> mExportAdaptors;
        QList<ImportAdaptor*> mImportAdaptors;

        static DocumentManager* sDocumentManager;
};

