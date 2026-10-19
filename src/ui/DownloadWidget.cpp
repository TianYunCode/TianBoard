#include <QDebug>
#include <QHeaderView>
#include <QStyleOptionProgressBar>
#include <QApplication>

#include "ui/DownloadWidget.h"

#include "utils/Globals.h"

#include "core/Application.h"

struct DownloadWidget::Private
{
    QVBoxLayout* mpLayout{nullptr};
    QHBoxLayout* mpBttnLayout{nullptr};
    QTreeWidget* mpTree{nullptr};
    QPushButton* mpCancelBttn{nullptr};
    QTreeWidgetItem* mpItem{nullptr};
    DownloadProgressDelegate mProgressBarDelegate;
};

/**
 * \brief Constructor
 * @param parent as the parent widget
 * @param name as the widget object name
 */
DownloadWidget::DownloadWidget(QWidget *parent, const char *name):QWidget(parent)
  , d(new Private)
{
    setObjectName(name);
    setWindowTitle(tr("Downloading files"));
    SET_STYLE_SHEET();
    resize(400, 300);

    d->mpLayout = new QVBoxLayout(this);
    setLayout(d->mpLayout);

    d->mpTree = new QTreeWidget(this);
    d->mpTree->setRootIsDecorated(false);
    d->mpTree->setColumnCount(2);
    d->mpTree->header()->setStretchLastSection(false);
    d->mpTree->header()->setSectionResizeMode(eItemColumn_Desc, QHeaderView::Stretch);
    d->mpTree->header()->setSectionResizeMode(eItemColumn_Close, QHeaderView::Custom);
    d->mpTree->resizeColumnToContents(eItemColumn_Close);
    d->mpTree->header()->close();
    d->mpLayout->addWidget(d->mpTree, 1);

    d->mpBttnLayout = new QHBoxLayout();
    d->mpBttnLayout->addStretch(1);
    d->mpCancelBttn = new QPushButton(tr("Cancel"), this);
    d->mpCancelBttn->setObjectName("DockPaletteWidgetButton");
    d->mpBttnLayout->addWidget(d->mpCancelBttn, 0);
    d->mpLayout->addLayout(d->mpBttnLayout);

    connect(DownloadManager::downloadManager(), SIGNAL(fileAddedToDownload()), this, SLOT(onFileAddedToDownload()));
    connect(DownloadManager::downloadManager(), SIGNAL(downloadUpdated(int,qint64,qint64)), this, SLOT(onDownloadUpdated(int,qint64,qint64)));
    connect(DownloadManager::downloadManager(), SIGNAL(downloadFinished(int)), this, SLOT(onDownloadFinished(int)));
    connect(d->mpCancelBttn, SIGNAL(clicked()), this, SLOT(onCancelClicked()));
    connect(d->mpTree, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(onItemClicked(QTreeWidgetItem*,int)));
}

/**
 * \brief Destructor
 */
DownloadWidget::~DownloadWidget()
{
    if(NULL != d->mpCancelBttn)
    {
        delete d->mpCancelBttn;
        d->mpCancelBttn = NULL;
    }
    if(NULL != d->mpTree)
    {
        delete d->mpTree;
        d->mpTree = NULL;
    }
    if(NULL != d->mpBttnLayout)
    {
        delete d->mpBttnLayout;
        d->mpBttnLayout = NULL;
    }
    if(NULL != d->mpLayout)
    {
        delete d->mpLayout;
        d->mpLayout = NULL;
    }
}

/**
 * \brief Refresh the tree of downloaded files
 */
void DownloadWidget::onFileAddedToDownload()
{
    if(NULL != d->mpTree)
    {
        d->mpTree->clear();
        addCurrentDownloads();
        addPendingDownloads();
    }
}

/**
 * \brief Add the current downloads
 */
void DownloadWidget::addCurrentDownloads()
{
    QVector<sDownloadFileDesc> actualDL = DownloadManager::downloadManager()->currentDownloads();
    qDebug() << "Actual downloads size: " << actualDL.size();
    for(int i=0; i<actualDL.size();i++)
    {
        d->mpItem = new QTreeWidgetItem(d->mpTree);
        d->mpItem->setText(eItemColumn_Desc, actualDL.at(i).name);
        d->mpItem->setData(eItemColumn_Desc, Qt::UserRole, QVariant(actualDL.at(i).id));
        d->mpItem->setIcon(eItemColumn_Close, QIcon(":images/close.svg"));
        d->mpTree->addTopLevelItem(d->mpItem);
        d->mpItem = new QTreeWidgetItem(d->mpTree);
        d->mpItem->setData(eItemColumn_Desc, Qt::UserRole, actualDL.at(i).currentSize);
        d->mpItem->setData(eItemColumn_Desc, Qt::UserRole + 1, actualDL.at(i).totalSize);
        d->mpItem->setData(eItemColumn_Desc, Qt::UserRole + 2, actualDL.at(i).id);
        d->mpTree->addTopLevelItem(d->mpItem);
        d->mpTree->setItemDelegateForRow(((i+1)*2)-1, &d->mProgressBarDelegate);
    }
}

/**
 * \brief Add the pending downloads
 */
void DownloadWidget::addPendingDownloads()
{
    QVector<sDownloadFileDesc> pendingDL = DownloadManager::downloadManager()->pendingDownloads();
    for(int i=0; i<pendingDL.size(); i++)
    {
        d->mpItem = new QTreeWidgetItem(d->mpTree);
        d->mpItem->setText(eItemColumn_Desc, pendingDL.at(i).name);
        d->mpItem->setData(eItemColumn_Desc, Qt::UserRole, QVariant(pendingDL.at(i).id));
        d->mpTree->addTopLevelItem(d->mpItem);
    }
}

/**
 * \brief Update the progress bar
 * @param id as the downloaded file id
 * @param crnt as the current transfered size
 * @param total as the total size of the file
 */
void DownloadWidget::onDownloadUpdated(int id, qint64 crnt, qint64 total)
{
    if(NULL != d->mpTree)
    {
        QAbstractItemModel* model = d->mpTree->model();
        if(NULL != model)
        {
            for(int i=0; i< model->rowCount(); i++)
            {
                QModelIndex currentIndex = model->index(i, eItemColumn_Desc);
                if(id == currentIndex.data(Qt::UserRole + 2))
                {
                    // We found the right item, now we update the progress bar
                    model->setData(currentIndex, crnt, Qt::UserRole);
                    model->setData(currentIndex, total, Qt::UserRole + 1);
                    break;
                }
            }
        }
    }
}

/**
 * \brief Handles the download finish notification
 * @param id as the downloaded file id
 */
void DownloadWidget::onDownloadFinished(int id)
{
    Q_UNUSED(id);
    // Refresh the file's list
    onFileAddedToDownload();
}

/**
 * \brief Handles the Cancel button action
 */
void DownloadWidget::onCancelClicked()
{
    DownloadManager::downloadManager()->cancelDownloads();
}

/**
 * \brief Handles the item click notification
 * @param pItem as the item clicked
 * @param col as the column containing the item clicked
 */
void DownloadWidget::onItemClicked(QTreeWidgetItem *pItem, int col)
{
    if( eItemColumn_Close == col
            && "" != pItem->text(eItemColumn_Desc)){

        // Stop the download of the clicked item and remove it from the list
        DownloadManager::downloadManager()->cancelDownload(pItem->data(eItemColumn_Desc, Qt::UserRole).toInt());
    }
}

// ---------------------------------------------------------------------------------------------
DownloadProgressDelegate::DownloadProgressDelegate(QObject *parent):QItemDelegate(parent)
{

}

void DownloadProgressDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionProgressBar opt;
    opt.rect = option.rect;
    opt.minimum = 0;
    opt.maximum = index.data(Qt::UserRole + 1).toInt();
    opt.progress = index.data(Qt::UserRole).toInt();

    if(0 == index.column()){
        QApplication::style()->drawControl(QStyle::CE_ProgressBar, &opt, painter, 0);
    }
}
