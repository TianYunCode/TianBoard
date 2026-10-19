#pragma once

#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QItemDelegate>

#include "core/DownloadManager.h"

#include <memory>

typedef enum { eItemColumn_Desc, eItemColumn_Close } eItemColumn;

class DownloadProgressDelegate : public QItemDelegate {
    Q_OBJECT
public:
    DownloadProgressDelegate(QObject *parent = 0);

protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class DownloadWidget : public QWidget {
    Q_OBJECT
public:
    DownloadWidget(QWidget *parent = 0, const char *name = "DownloadWidget");
    ~DownloadWidget();

private slots:
    void onFileAddedToDownload();
    void onDownloadUpdated(int id, qint64 crnt, qint64 total);
    void onDownloadFinished(int id);
    void onCancelClicked();
    void onItemClicked(QTreeWidgetItem *pItem, int col);

private:
    void addCurrentDownloads();
    void addPendingDownloads();

    struct Private;
    std::unique_ptr<Private> d;
};
