#pragma once

#include <QtGui>
#include "content/ImportAdaptor.h"

class DocumentProxy;

class ImportImage : public PageBasedImportAdaptor {
    Q_OBJECT;

public:
    ImportImage(QObject *parent = 0);
    virtual ~ImportImage();

    virtual QStringList supportedExtentions();
    virtual QString importFileFilter();

    virtual QList<GraphicsItem *> import(const QUuid &uuid, const QString &filePath);
    virtual void placeImportedItemToScene(GraphicsScene *scene, GraphicsItem *item);
    virtual const QString &folderToCopy();
};
