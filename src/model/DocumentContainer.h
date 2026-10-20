/**
 * @file DocumentContainer.h
 * @brief 声明文档容器 DocumentContainer：持有当前文档代理并维护其页面缩略图缓存。
 */

#pragma once

#include <QtWidgets>
#include "model/DocumentProxy.h"

/**
 * @brief 文档容器：为“当前文档”维护缩略图列表，并把页面增删、移动、复制转发给 PersistenceManager。
 *
 * 位于 model 域的文档表示层。DocumentController 继承本类，把这里的页面操作接到文档视图、
 * 树模型与缩略图控件上；DocumentProxy 提供文档元数据和页数。容器不拥有 DocumentProxy，
 * 只保存裸指针；缩略图 QPixmap 由本类负责 new/delete。生命周期上随 DocumentController
 * 创建，父对象是主窗口中央控件。
 */
class DocumentContainer : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造容器，初始没有当前文档。
     * @param parent Qt 父对象。
     */
    DocumentContainer(QObject *parent = 0);

    /**
     * @brief 析构时释放全部缓存的缩略图。
     */
    virtual ~DocumentContainer();

    /**
     * @brief 把文档设为当前文档，并按需重建缩略图。
     * @param document 目标文档代理，可为 nullptr。
     * @param forceReload 为 true 时即使文档未变也重新加载缩略图。
     * @note 切换成功后会发出 documentSet。
     */
    void setDocument(DocumentProxy *document, bool forceReload = false);

    /**
     * @brief 只替换当前文档指针，不加载缩略图也不发信号。
     *
     * 供删除流程等需要先“摘掉”旧文档再自行刷新视图的场景使用。
     */
    void pureSetDocument(DocumentProxy *document)
    {
        mCurrentDocument = document;
    }

    /** @brief 返回当前文档代理，可能为空。 */
    DocumentProxy *selectedDocument()
    {
        return mCurrentDocument;
    }

    /** @brief 返回当前文档的页数（假定当前文档非空）。 */
    int pageCount()
    {
        return mCurrentDocument->pageCount();
    }

    /**
     * @brief 返回指定页的缩略图。
     * @param index 从 0 开始的页索引。
     * @return 缓存的缩略图指针；越界时返回 nullptr。
     */
    const QPixmap *pageAt(int index)
    {
        if (index < mDocumentThumbs.size())
        {
            return mDocumentThumbs[index];
        }
        else
        {
            return NULL;
        }
    }

    /**
     * @brief 把场景索引（从 0 开始）换算成对用户显示的页号（从 1 开始）。
     */
    static int pageFromSceneIndex(int sceneIndex);

    /**
     * @brief 把对用户显示的页号（从 1 开始）换算成场景索引（从 0 开始）。
     */
    static int sceneIndexFromPage(int sceneIndex);

    /**
     * @brief 复制选中的页面。
     * @param pageIndexes 源页索引列表，按值传递以便按插入偏移逐项修正。
     * @note 每复制一页后插入位置后移，因此后续源索引要加上 offset。
     */
    void duplicatePages(QList<int> &pageIndexes);

    /**
     * @brief 把一页从 source 移动到 target。
     * @return 目前恒为 true。
     * @note 持久化层移动场景后，本地缩略图缓存用“先删后插”同步。
     */
    bool movePageToIndex(int source, int target);

    /**
     * @brief 删除若干页并同步缩略图缓存。
     * @param pageIndexes 页索引列表，假定按升序排列。
     */
    void deletePages(QList<int> &pageIndexes);

    /** @brief 清空并释放所有缓存的缩略图。 */
    void clearThumbPage();

    /** @brief 按当前文档页数重新生成缩略图缓存。 */
    void initThumbPage();

    /**
     * @brief 在指定位置新建一页（持久化 + 缩略图）。
     * @param index 新页索引。
     */
    void addPage(int index);

    /**
     * @brief 直接插入一张外部传入的缩略图。
     * @param pix 缩略图，容器接管其所有权。
     * @param index 插入位置。
     */
    void addPixmapAt(const QPixmap *pix, int index);

    /**
     * @brief 重新生成指定页的缩略图并通知视图。
     */
    void updatePage(int index);

    /** @brief 在末尾追加一张空缩略图占位。 */
    void addEmptyThumbPage();

    /** @brief 从持久化层重新加载整份缩略图列表并通知视图。 */
    void reloadThumbnails();

    /**
     * @brief 生成并插入指定页的缩略图。
     * @param index 页索引。
     */
    void insertThumbPage(int index);

private:
    /** @brief 当前文档代理，不拥有所有权。 */
    DocumentProxy *mCurrentDocument;
    /** @brief 按页序缓存的缩略图，元素由本类释放。 */
    QList<const QPixmap *> mDocumentThumbs;

protected:
    /**
     * @brief 删除指定页的缩略图并释放其内存。
     */
    void deleteThumbPage(int index);

    /**
     * @brief 重新生成指定页的缩略图。
     * @note 索引越界时只打印错误，不抛异常。
     */
    void updateThumbPage(int index);

signals:
    /**
     * @brief 当前文档发生变化（切换或强制重载）后发出。
     * @param document 新的当前文档。
     */
    void documentSet(DocumentProxy *document);

    /**
     * @brief 单页缩略图被重新生成后发出。
     * @param index 页索引。
     */
    void documentPageUpdated(int index);

    /**
     * @brief 缩略图集合发生结构性变化（增删、移动、重载）后发出。
     * @param source 发出信号的容器，便于接收方区分来源。
     */
    void documentThumbnailsUpdated(DocumentContainer *source);
};
