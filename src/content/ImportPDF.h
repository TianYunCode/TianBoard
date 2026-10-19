#pragma once

#include <QtGui>
#include "content/ImportAdaptor.h"

class DocumentProxy;

class ImportPDF : public PageBasedImportAdaptor {
    Q_OBJECT;

public:
    ImportPDF(QObject *parent = 0);
    virtual ~ImportPDF();

    virtual QStringList supportedExtentions();
    virtual QString importFileFilter();

    virtual QList<GraphicsItem *> import(const QUuid &uuid, const QString &filePath);
    virtual void placeImportedItemToScene(GraphicsScene *scene, GraphicsItem *item);
    virtual const QString &folderToCopy();

private:
    int dpi;
};
