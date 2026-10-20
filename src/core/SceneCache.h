/**
 * @file SceneCache.h
 * @brief 声明页面场景缓存：以文档 + 页下标为键保存已加载的 GraphicsScene。
 */

#pragma once

#include <QtCore>

#include "whiteboard/GraphicsScene.h"

class DocumentProxy;

/**
 * @brief 页面缓存的键：文档代理指针与页面下标。
 *
 * 详细说明：只做值语义比较；文档代理由文档树拥有，本键不延长其生命周期。
 */
class SceneCacheID
{
public:
    SceneCacheID() : documentProxy(0), pageIndex(-1)
    {
    }

    SceneCacheID(DocumentProxy *pDocumentProxy, int pPageIndex)
    {
        documentProxy = pDocumentProxy;
        pageIndex     = pPageIndex;
    }

    DocumentProxy *documentProxy;
    int            pageIndex;
};

inline bool operator==(const SceneCacheID &id1, const SceneCacheID &id2)
{
    return id1.documentProxy == id2.documentProxy && id1.pageIndex == id2.pageIndex;
}

inline size_t qHash(const SceneCacheID &id, size_t seed = 0)
{
    return qHash(id.pageIndex, seed);
}

/**
 * @brief 页面场景缓存：保存已加载的页面，按缓存上限淘汰最久未使用的非活动页。
 *
 * 详细说明：由 PersistenceManager 按值持有一个实例；缓存不拥有文档代理，但拥有
 * 其中的 GraphicsScene，淘汰或移除时用 deleteLater 释放。活动页（isActive()）
 * 不会被淘汰，其视图状态会暂存并在重新加载时恢复。
 * 线程约束：GUI 线程专用。
 */
class SceneCache : public QHash<SceneCacheID, GraphicsScene *>
{
public:
    SceneCache();
    virtual ~SceneCache();

    /**
     * @brief 创建新场景并放入缓存。
     * @param proxy 所属文档。
     * @param pageIndex 页面下标。
     * @param useUndoRedoStack 是否让场景接入撤销栈。
     * @return 新场景，所有权归缓存。
     */
    GraphicsScene *createScene(DocumentProxy *proxy, int pageIndex, bool useUndoRedoStack);

    /**
     * @brief 放入或替换指定页的场景。
     * @param proxy 所属文档。
     * @param pageIndex 页面下标。
     * @param scene 场景；为空时忽略。
     * @note 若该页此前保存过视图状态，会在放入时恢复。
     */
    void insert(DocumentProxy *proxy, int pageIndex, GraphicsScene *scene);

    /// 判断指定页是否已缓存。
    bool contains(DocumentProxy *proxy, int pageIndex) const;

    /// 取出指定页的场景并刷新其使用顺序；未缓存时为空。
    GraphicsScene *value(DocumentProxy *proxy, int pageIndex);

    /// 按完整键取出场景，不刷新使用顺序。
    GraphicsScene *value(const SceneCacheID &key) const
    {
        return QHash<SceneCacheID, GraphicsScene *>::value(key);
    }

    /// 移除并释放指定页的场景；活动页不会被移除。
    void removeScene(DocumentProxy *proxy, int pageIndex);

    /// 移除并释放某文档的全部缓存页面。
    void removeAllScenes(DocumentProxy *proxy);

    /// 页面移动后同步调整缓存键。
    void moveScene(DocumentProxy *proxy, int sourceIndex, int targetIndex);

    /// 文档代理被替换后，把缓存键与场景的所属文档迁移到新代理。
    void reassignDocProxy(DocumentProxy *newDocument, DocumentProxy *oldDocument);

    /// 在指定位置插入页面后，把该下标及之后的缓存键整体后移一位。
    void shiftUpScenes(DocumentProxy *proxy, int startIncIndex, int endIncIndex);

private:
    /// 按设置中的缓存上限淘汰最久未使用的非活动页。
    void enforceCacheLimit();

    /// 仅调整缓存键，不涉及磁盘。
    void internalMoveScene(DocumentProxy *proxy, int sourceIndex, int targetIndex);

    /// 调试用：输出缓存内容。
    void dumpCacheContent();

    int mCachedSceneCount;

    QQueue<SceneCacheID> mCachedKeyFIFO;

    QHash<SceneCacheID, GraphicsScene::SceneViewState> mViewStates;
};
