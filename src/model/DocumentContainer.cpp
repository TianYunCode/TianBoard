#include "model/DocumentContainer.h"
#include "content/ThumbnailAdaptor.h"
#include "core/PersistenceManager.h"


DocumentContainer::DocumentContainer(QObject * parent)
    :QObject(parent)
    ,mCurrentDocument(NULL)
{}

DocumentContainer::~DocumentContainer()
{
    foreach(const QPixmap* pm, mDocumentThumbs){
        delete pm;
        pm = NULL;
    }
}

void DocumentContainer::setDocument(DocumentProxy* document, bool forceReload)
{
    if (mCurrentDocument != document || forceReload)
    {
        mCurrentDocument = document;

        reloadThumbnails();
        emit documentSet(mCurrentDocument);
    }
}

void DocumentContainer::duplicatePages(QList<int>& pageIndexes)
{
    int offset = 0;
    foreach(int sceneIndex, pageIndexes)
    {
        PersistenceManager::persistenceManager()->duplicateDocumentScene(mCurrentDocument, sceneIndex + offset);
        offset++;
    }
}

bool DocumentContainer::movePageToIndex(int source, int target)
{
    //on document view
    PersistenceManager::persistenceManager()->moveSceneToIndex(mCurrentDocument, source, target);
    deleteThumbPage(source);
    insertThumbPage(target);
    emit documentThumbnailsUpdated(this);
    //on board thumbnails view
    emit moveThumbnailRequired(source, target);
    return true;
}

void DocumentContainer::deletePages(QList<int>& pageIndexes)
{
    PersistenceManager::persistenceManager()->deleteDocumentScenes(mCurrentDocument, pageIndexes);
    int offset = 0;
    foreach(int index, pageIndexes)
    {
        deleteThumbPage(index - offset);
        emit removeThumbnailRequired(index - offset);
        offset++;

    }
    emit documentThumbnailsUpdated(this);
}

void DocumentContainer::addPage(int index)
{
    PersistenceManager::persistenceManager()->createDocumentSceneAt(mCurrentDocument, index);
    insertThumbPage(index);

    emit documentThumbnailsUpdated(this);
    emit addThumbnailRequired(this, index);
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

    for (int i=0; i < selectedDocument()->pageCount(); i++)
        insertThumbPage(i);
}

void DocumentContainer::updatePage(int index)
{
    updateThumbPage(index);
    emit documentThumbnailsUpdated(this);
}

void DocumentContainer::deleteThumbPage(int index)
{
    mDocumentThumbs.removeAt(index);
}

void DocumentContainer::updateThumbPage(int index)
{
    if (mDocumentThumbs.size() > index)
    {
        mDocumentThumbs[index] = ThumbnailAdaptor::get(mCurrentDocument, index);
        emit documentPageUpdated(index);
    }
    else
    {
        qDebug() << "error [updateThumbPage] : index > mDocumentThumbs' size.";
    }
}

void DocumentContainer::insertThumbPage(int index)
{
    mDocumentThumbs.insert(index, ThumbnailAdaptor::get(mCurrentDocument, index));
}

void DocumentContainer::reloadThumbnails()
{
    if (mCurrentDocument)
    {
        ThumbnailAdaptor::load(mCurrentDocument, mDocumentThumbs);
    }
    emit documentThumbnailsUpdated(this);
}

int DocumentContainer::pageFromSceneIndex(int sceneIndex)
{
    return sceneIndex+1;
}

int DocumentContainer::sceneIndexFromPage(int page)
{
    return page-1;
}

void DocumentContainer::addEmptyThumbPage()
{
    const QPixmap* pThumb = new QPixmap();
    mDocumentThumbs.append(pThumb);
}
