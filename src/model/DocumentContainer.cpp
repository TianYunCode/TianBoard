#include "model/DocumentContainer.h"
#include "model/IDocumentStore.h"
#include "model/IDocumentFormatServices.h"

DocumentContainer::DocumentContainer(QObject *parent) : QObject(parent), mCurrentDocument(NULL)
{
}

DocumentContainer::~DocumentContainer()
{
    foreach (const QPixmap *pm, mDocumentThumbs)
    {
        delete pm;
        pm = NULL;
    }
}

void DocumentContainer::setDocument(DocumentProxy *document, bool forceReload)
{
    if (mCurrentDocument != document || forceReload)
    {
        mCurrentDocument = document;

        reloadThumbnails();
        emit documentSet(mCurrentDocument);
    }
}

void DocumentContainer::duplicatePages(QList<int> &pageIndexes)
{
    // 每复制一页都会在源页之后插入一页，后续源页的索引因此整体后移；
    // 这里用 offset 把原始页号换算成插入后的真实索引。
    int offset = 0;
    foreach (int sceneIndex, pageIndexes)
    {
        documentStore()->duplicateDocumentScene(mCurrentDocument, sceneIndex + offset);
        offset++;
    }
}

bool DocumentContainer::movePageToIndex(int source, int target)
{
    // 持久化层完成场景移动后，本地缩略图缓存用“删源、插目标”同步即可，
    // 不必整份重新加载，避免大文档移动一页就重建全部缩略图。
    documentStore()->moveSceneToIndex(mCurrentDocument, source, target);
    deleteThumbPage(source);
    insertThumbPage(target);
    emit documentThumbnailsUpdated(this);
    return true;
}

void DocumentContainer::deletePages(QList<int> &pageIndexes)
{
    // 先让持久化层批量删除，再按删除顺序逐页清理缓存：
    // 每删一页，后面页的索引就前移一位，用 offset 抵消即可。
    documentStore()->deleteDocumentScenes(mCurrentDocument, pageIndexes);
    int offset = 0;
    foreach (int index, pageIndexes)
    {
        deleteThumbPage(index - offset);
        offset++;
    }
    emit documentThumbnailsUpdated(this);
}

void DocumentContainer::addPage(int index)
{
    documentStore()->createDocumentSceneAt(mCurrentDocument, index);
    insertThumbPage(index);

    emit documentThumbnailsUpdated(this);
}

void DocumentContainer::addPixmapAt(const QPixmap *pix, int index)
{
    mDocumentThumbs.insert(index, pix);
    emit documentThumbnailsUpdated(this);
}

void DocumentContainer::clearThumbPage()
{
    qDeleteAll(mDocumentThumbs);
    mDocumentThumbs.clear();
}

void DocumentContainer::initThumbPage()
{
    clearThumbPage();

    for (int i = 0; i < selectedDocument()->pageCount(); i++)
    {
        insertThumbPage(i);
    }
}

void DocumentContainer::updatePage(int index)
{
    updateThumbPage(index);
    emit documentThumbnailsUpdated(this);
}

void DocumentContainer::deleteThumbPage(int index)
{
    delete mDocumentThumbs.takeAt(index);
}

void DocumentContainer::updateThumbPage(int index)
{
    // documentPageThumbnail 每次返回新分配的 QPixmap，替换前必须先释放旧图，
    // 否则频繁编辑页面会持续泄漏缩略图内存。
    if (mDocumentThumbs.size() > index)
    {
        delete mDocumentThumbs.at(index);
        mDocumentThumbs[index] = documentPageThumbnail(mCurrentDocument, index);
        emit documentPageUpdated(index);
    }
    else
    {
        qDebug() << "错误 [updateThumbPage]：索引比 mDocumentThumbs 的长度还大。";
    }
}

void DocumentContainer::insertThumbPage(int index)
{
    mDocumentThumbs.insert(index, documentPageThumbnail(mCurrentDocument, index));
}

void DocumentContainer::reloadThumbnails()
{
    if (mCurrentDocument)
    {
        loadDocumentPageThumbnails(mCurrentDocument, mDocumentThumbs);
    }
    emit documentThumbnailsUpdated(this);
}

// 场景索引从 0 开始，而界面页号从 1 开始（第 0 页显示为“标题页”），二者只差 1。
int DocumentContainer::pageFromSceneIndex(int sceneIndex)
{
    return sceneIndex + 1;
}

int DocumentContainer::sceneIndexFromPage(int page)
{
    return page - 1;
}

void DocumentContainer::addEmptyThumbPage()
{
    const QPixmap *pThumb = new QPixmap();
    mDocumentThumbs.append(pThumb);
}
