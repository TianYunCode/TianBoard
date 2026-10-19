#pragma once

#include <QtGui>

class GraphicsItem;
class GraphicsScene;
class DocumentProxy;

class ImportAdaptor : public QObject
{
    Q_OBJECT;

    protected:
        ImportAdaptor(bool _documentBased, QObject *parent = 0);
        virtual ~ImportAdaptor();

    public:
        virtual QStringList supportedExtentions() = 0;
        virtual QString importFileFilter() = 0;

        bool isDocumentBased(){return documentBased;}

    private:
        bool documentBased;
        
};

class PageBasedImportAdaptor : public ImportAdaptor
{
	protected:
        PageBasedImportAdaptor(QObject *parent = 0);

	public:
        virtual QList<GraphicsItem*> import(const QUuid& uuid, const QString& filePath) = 0;
        virtual void placeImportedItemToScene(GraphicsScene* scene, GraphicsItem* item) = 0;
        virtual const QString& folderToCopy() = 0;
};

class DocumentBasedImportAdaptor : public ImportAdaptor
{
	protected:
        DocumentBasedImportAdaptor(QObject *parent = 0);
	public:
    virtual DocumentProxy* importFile(const QFile& pFile, const QString& pGroup) = 0;
    virtual bool addFileToDocument(DocumentProxy* pDocument, const QFile& pFile) = 0;
};


