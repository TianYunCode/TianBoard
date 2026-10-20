#pragma once

#include <QtCore>

#include "whiteboard/GraphicsScene.h"

class DocumentProxy;
class GraphicsScene;
class GraphicsScene;

class SceneCacheID {
public:
    SceneCacheID() : documentProxy(0), pageIndex(-1) {}

    SceneCacheID(DocumentProxy *pDocumentProxy, int pPageIndex) {
        documentProxy = pDocumentProxy;
        pageIndex = pPageIndex;
    }

    DocumentProxy *documentProxy;
    int pageIndex;
};

inline bool operator==(const SceneCacheID &id1, const SceneCacheID &id2) {
    return id1.documentProxy == id2.documentProxy && id1.pageIndex == id2.pageIndex;
}

inline size_t qHash(const SceneCacheID &id, size_t seed = 0) {
    return qHash(id.pageIndex, seed);
}

class SceneCache : public QHash<SceneCacheID, GraphicsScene *> {
public:
    SceneCache();
    virtual ~SceneCache();

    GraphicsScene *createScene(DocumentProxy *proxy, int pageIndex, bool useUndoRedoStack);

    void insert(DocumentProxy *proxy, int pageIndex, GraphicsScene *scene);

    bool contains(DocumentProxy *proxy, int pageIndex) const;

    GraphicsScene *value(DocumentProxy *proxy, int pageIndex);

    GraphicsScene *value(const SceneCacheID &key) const {
        return QHash<SceneCacheID, GraphicsScene *>::value(key);
    }

    void removeScene(DocumentProxy *proxy, int pageIndex);

    void removeAllScenes(DocumentProxy *proxy);

    void moveScene(DocumentProxy *proxy, int sourceIndex, int targetIndex);

    void reassignDocProxy(DocumentProxy *newDocument, DocumentProxy *oldDocument);

    void shiftUpScenes(DocumentProxy *proxy, int startIncIndex, int endIncIndex);

private:
    void enforceCacheLimit();

    void internalMoveScene(DocumentProxy *proxy, int sourceIndex, int targetIndex);

    void dumpCacheContent();

    int mCachedSceneCount;

    QQueue<SceneCacheID> mCachedKeyFIFO;

    QHash<SceneCacheID, GraphicsScene::SceneViewState> mViewStates;
};
