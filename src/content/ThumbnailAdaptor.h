#pragma once

#include <QtCore>

class Document;
class DocumentProxy;
class GraphicsScene;

class ThumbnailAdaptor
{
    Q_DECLARE_TR_FUNCTIONS(ThumbnailAdaptor)

public:
    static QUrl thumbnailUrl(DocumentProxy* proxy, int pageIndex);

    static void persistScene(DocumentProxy* proxy, GraphicsScene* pScene, int pageIndex, bool overrideModified = false);

    static const QPixmap* get(DocumentProxy* proxy, int index);
    static void load(DocumentProxy* proxy, QList<const QPixmap*>& list);

private:
    static void generateMissingThumbnails(DocumentProxy* proxy);

    ThumbnailAdaptor() {}
};

