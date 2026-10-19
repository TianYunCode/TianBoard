#pragma once

#include <QtWidgets>
#include "model/DocumentProxy.h"

class DocumentContainer : public QObject {
    Q_OBJECT

public:
    DocumentContainer(QObject *parent = 0);
    virtual ~DocumentContainer();

    void setDocument(DocumentProxy *document, bool forceReload = false);
    void pureSetDocument(DocumentProxy *document) { mCurrentDocument = document; }

    DocumentProxy *selectedDocument() { return mCurrentDocument; }
    int pageCount() { return mCurrentDocument->pageCount(); }
    const QPixmap *pageAt(int index) {
        if (index < mDocumentThumbs.size())
            return mDocumentThumbs[index];
        else {
            return NULL;
        }
    }

    static int pageFromSceneIndex(int sceneIndex);
    static int sceneIndexFromPage(int sceneIndex);

    void duplicatePages(QList<int> &pageIndexes);
    bool movePageToIndex(int source, int target);
    void deletePages(QList<int> &pageIndexes);
    void clearThumbPage();
    void initThumbPage();
    void addPage(int index);
    void addPixmapAt(const QPixmap *pix, int index);
    void updatePage(int index);
    void addEmptyThumbPage();
    void reloadThumbnails();

    void insertThumbPage(int index);

private:
    DocumentProxy *mCurrentDocument;
    QList<const QPixmap *> mDocumentThumbs;

protected:
    void deleteThumbPage(int index);
    void updateThumbPage(int index);

signals:
    void documentSet(DocumentProxy *document);
    void documentPageUpdated(int index);

    void initThumbnailsRequired(DocumentContainer *source);
    void addThumbnailRequired(DocumentContainer *source, int index);
    void removeThumbnailRequired(int index);
    void moveThumbnailRequired(int from, int to);
    void updateThumbnailsRequired();

    void documentThumbnailsUpdated(DocumentContainer *source);
};
