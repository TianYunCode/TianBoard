#include "core/SceneCache.h"

#include "whiteboard/GraphicsScene.h"

#include "core/PersistenceManager.h"
#include "core/Application.h"
#include "settings/Settings.h"
#include "settings/Setting.h"

#include "model/DocumentProxy.h"

SceneCache::SceneCache() : mCachedSceneCount(0)
{
    // 无额外动作，计数已在初始化列表里置零。
}

SceneCache::~SceneCache()
{
    // 无动作。注意：这里并不释放仍留在缓存里的 GraphicsScene。
}

GraphicsScene *SceneCache::createScene(DocumentProxy *proxy, int pageIndex, bool useUndoRedoStack)
{
    GraphicsScene *newScene = new GraphicsScene(proxy, useUndoRedoStack);
    insert(proxy, pageIndex, newScene);

    return newScene;
}

void SceneCache::insert(DocumentProxy *proxy, int pageIndex, GraphicsScene *scene)
{
    // 空指针也必须挡在门外：一旦插进哈希表，contains() 就会为真，
    // PersistenceManager::loadDocumentScene 便直接返回空，而不再回落到从磁盘加载。
    if (!proxy || !scene)
    {
        return;
    }

    QList<SceneCacheID> existingKeys = QHash<SceneCacheID, GraphicsScene *>::keys(scene);

    foreach (SceneCacheID key, existingKeys)
    {
        mCachedSceneCount -= QHash<SceneCacheID, GraphicsScene *>::remove(key);
    }

    SceneCacheID key(proxy, pageIndex);

    if (QHash<SceneCacheID, GraphicsScene *>::contains(key))
    {
        QHash<SceneCacheID, GraphicsScene *>::insert(key, scene);
        mCachedKeyFIFO.enqueue(key);
    }
    else
    {
        QHash<SceneCacheID, GraphicsScene *>::insert(key, scene);
        mCachedKeyFIFO.enqueue(key);

        mCachedSceneCount++;
    }

    if (mViewStates.contains(key))
    {
        scene->setViewState(mViewStates.value(key));
    }

    enforceCacheLimit();
}

void SceneCache::enforceCacheLimit()
{
    const int limit = qMax(1, Settings::settings()->pageCacheSize->get().toInt());
    // 最久未使用的先淘汰；活动页不能删（正在显示），放回队尾。
    // 队列里可能有 move/reinsert 留下的过期 key，跳过即可。
    int attempts    = mCachedKeyFIFO.size();
    while (mCachedSceneCount > limit && attempts-- > 0 && !mCachedKeyFIFO.isEmpty())
    {
        const SceneCacheID key = mCachedKeyFIFO.dequeue();
        if (!QHash<SceneCacheID, GraphicsScene *>::contains(key))
        {
            continue;
        }
        GraphicsScene *scene = QHash<SceneCacheID, GraphicsScene *>::value(key);
        if (!scene || scene->isActive())
        {
            mCachedKeyFIFO.enqueue(key);
            continue;
        }
        mViewStates.insert(key, scene->viewState());
        QHash<SceneCacheID, GraphicsScene *>::remove(key);
        mCachedSceneCount--;
        scene->deleteLater();
    }
}

bool SceneCache::contains(DocumentProxy *proxy, int pageIndex) const
{
    SceneCacheID key(proxy, pageIndex);
    return QHash<SceneCacheID, GraphicsScene *>::contains(key);
}

GraphicsScene *SceneCache::value(DocumentProxy *proxy, int pageIndex)
{
    SceneCacheID key(proxy, pageIndex);

    if (QHash<SceneCacheID, GraphicsScene *>::contains(key))
    {
        GraphicsScene *scene = QHash<SceneCacheID, GraphicsScene *>::value(key);

        mCachedKeyFIFO.removeAll(key);
        mCachedKeyFIFO.enqueue(key);

        return scene;
    }
    else
    {
        return 0;
    }
}

void SceneCache::removeScene(DocumentProxy *proxy, int pageIndex)
{
    GraphicsScene *scene = value(proxy, pageIndex);
    if (scene && !scene->isActive())
    {
        SceneCacheID key(proxy, pageIndex);
        int          count = QHash<SceneCacheID, GraphicsScene *>::remove(key);
        mCachedKeyFIFO.removeAll(key);

        mViewStates.insert(key, scene->viewState());

        scene->deleteLater();

        mCachedSceneCount -= count;
    }
}

void SceneCache::removeAllScenes(DocumentProxy *proxy)
{
    for (int i = 0; i < proxy->pageCount(); i++)
    {
        removeScene(proxy, i);
    }
}

void SceneCache::moveScene(DocumentProxy *proxy, int sourceIndex, int targetIndex)
{
    SceneCacheID keySource(proxy, sourceIndex);

    GraphicsScene *scene = 0;

    if (QHash<SceneCacheID, GraphicsScene *>::contains(keySource))
    {
        scene = QHash<SceneCacheID, GraphicsScene *>::value(keySource);
        mCachedKeyFIFO.removeAll(keySource);
    }

    if (sourceIndex < targetIndex)
    {
        for (int i = sourceIndex + 1; i <= targetIndex; i++)
        {
            internalMoveScene(proxy, i, i - 1);
        }
    }
    else
    {
        for (int i = sourceIndex - 1; i >= targetIndex; i--)
        {
            internalMoveScene(proxy, i, i + 1);
        }
    }

    SceneCacheID keyTarget(proxy, targetIndex);

    if (scene)
    {
        insert(proxy, targetIndex, scene);
        mCachedKeyFIFO.enqueue(keyTarget);
    }
    else if (QHash<SceneCacheID, GraphicsScene *>::contains(keyTarget))
    {
        scene = QHash<SceneCacheID, GraphicsScene *>::take(keyTarget);
        mCachedKeyFIFO.removeAll(keyTarget);
    }
}

void SceneCache::reassignDocProxy(DocumentProxy *newDocument, DocumentProxy *oldDocument)
{
    if (!newDocument || !oldDocument)
    {
        return;
    }
    if (newDocument->pageCount() != oldDocument->pageCount())
    {
        return;
    }
    if (!QFileInfo(oldDocument->persistencePath()).exists())
    {
        return;
    }
    for (int i = 0; i < oldDocument->pageCount(); i++)
    {
        SceneCacheID   sourceKey(oldDocument, i);
        GraphicsScene *currentScene = value(sourceKey);
        if (currentScene)
        {
            currentScene->setDocument(newDocument);
        }
        mCachedKeyFIFO.removeAll(sourceKey);
        int count          = QHash<SceneCacheID, GraphicsScene *>::remove(sourceKey);
        mCachedSceneCount -= count;

        insert(newDocument, i, currentScene);
    }
}

void SceneCache::shiftUpScenes(DocumentProxy *proxy, int startIncIndex, int endIncIndex)
{
    for (int i = endIncIndex; i >= startIncIndex; i--)
    {
        internalMoveScene(proxy, i, i + 1);
    }
}

void SceneCache::internalMoveScene(DocumentProxy *proxy, int sourceIndex, int targetIndex)
{
    SceneCacheID sourceKey(proxy, sourceIndex);

    if (QHash<SceneCacheID, GraphicsScene *>::contains(sourceKey))
    {
        GraphicsScene *scene = QHash<SceneCacheID, GraphicsScene *>::take(sourceKey);
        mCachedKeyFIFO.removeAll(sourceKey);

        SceneCacheID targetKey(proxy, targetIndex);
        QHash<SceneCacheID, GraphicsScene *>::insert(targetKey, scene);
        mCachedKeyFIFO.enqueue(targetKey);
    }
    else
    {
        SceneCacheID targetKey(proxy, targetIndex);
        if (QHash<SceneCacheID, GraphicsScene *>::contains(targetKey))
        {
            /*GraphicsScene* scene = */ QHash<SceneCacheID, GraphicsScene *>::take(targetKey);

            mCachedKeyFIFO.removeAll(targetKey);
        }
    }
}

void SceneCache::dumpCacheContent()
{
    foreach (SceneCacheID key, keys())
    {
        GraphicsScene *scene = 0;

        if (QHash<SceneCacheID, GraphicsScene *>::contains(key))
        {
            scene = QHash<SceneCacheID, GraphicsScene *>::value(key);
        }

        int index = key.pageIndex;

        qDebug() << "SceneCache::dumpCacheContent:" << index << " : " << scene;
    }
}
