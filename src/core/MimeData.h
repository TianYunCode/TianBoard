#pragma once

#include <QtWidgets>

class DocumentProxy;
class Item;

struct MimeDataItem {
public:
    MimeDataItem(DocumentProxy *proxy, int sceneIndex);
    virtual ~MimeDataItem();

    DocumentProxy *documentProxy() const { return mProxy; }
    int sceneIndex() const { return mSceneIndex; }

private:
    DocumentProxy *mProxy;
    int mSceneIndex;
};

class MimeDataGraphicsItem : public QMimeData {
    Q_OBJECT

public:
    MimeDataGraphicsItem(QList<Item *> pItems);
    virtual ~MimeDataGraphicsItem();

    QList<Item *> items() const { return mItems; }

private:
    QList<Item *> mItems;
};

class MimeData : public QMimeData {
    Q_OBJECT

public:
    MimeData(const QList<MimeDataItem> &items);
    virtual ~MimeData();

    QList<MimeDataItem> items() const { return mItems; }

private:
    QList<MimeDataItem> mItems;
};
