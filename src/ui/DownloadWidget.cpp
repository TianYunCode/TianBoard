#include <QDebug>
#include <QHeaderView>
#include <QStyleOptionProgressBar>
#include <QApplication>

#include "ui/DownloadWidget.h"

#include "utils/Globals.h"

struct DownloadWidget::Private
{
    QVBoxLayout             *mpLayout{nullptr};
    QHBoxLayout             *mpBttnLayout{nullptr};
    QTreeWidget             *mpTree{nullptr};
    QPushButton             *mpCancelBttn{nullptr};
    QTreeWidgetItem         *mpItem{nullptr};
    DownloadProgressDelegate mProgressBarDelegate;
};

DownloadWidget::DownloadWidget(QWidget *parent, const char *name) : QWidget(parent), d(new Private)
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
    // 列表不带表头，第一列占满宽度，第二列只放关闭图标。
    d->mpTree->header()->close();
    d->mpLayout->addWidget(d->mpTree, 1);

    d->mpBttnLayout = new QHBoxLayout();
    d->mpBttnLayout->addStretch(1);
    d->mpCancelBttn = new QPushButton(tr("Cancel"), this);
    d->mpCancelBttn->setObjectName("DockPaletteWidgetButton");
    d->mpBttnLayout->addWidget(d->mpCancelBttn, 0);
    d->mpLayout->addLayout(d->mpBttnLayout);

    connect(DownloadManager::downloadManager(), &DownloadManager::fileAddedToDownload, this, &DownloadWidget::onFileAddedToDownload);
    connect(DownloadManager::downloadManager(), &DownloadManager::fileAddedToDownload, this, &DownloadWidget::syncModalVisibility);
    connect(DownloadManager::downloadManager(), &DownloadManager::downloadModalFinished, this, &DownloadWidget::syncModalVisibility);
    connect(DownloadManager::downloadManager(), &DownloadManager::downloadUpdated, this, &DownloadWidget::onDownloadUpdated);
    connect(DownloadManager::downloadManager(), qOverload<int>(&DownloadManager::downloadFinished), this, &DownloadWidget::onDownloadFinished);
    connect(d->mpCancelBttn, &QPushButton::clicked, this, &DownloadWidget::onCancelClicked);
    connect(d->mpTree, &QTreeWidget::itemClicked, this, &DownloadWidget::onItemClicked);
}

DownloadWidget::~DownloadWidget()
{
    if (NULL != d->mpCancelBttn)
    {
        delete d->mpCancelBttn;
        d->mpCancelBttn = NULL;
    }
    if (NULL != d->mpTree)
    {
        delete d->mpTree;
        d->mpTree = NULL;
    }
    if (NULL != d->mpBttnLayout)
    {
        delete d->mpBttnLayout;
        d->mpBttnLayout = NULL;
    }
    if (NULL != d->mpLayout)
    {
        delete d->mpLayout;
        d->mpLayout = NULL;
    }
}

void DownloadWidget::syncModalVisibility()
{
    if (DownloadManager::downloadManager()->hasModalDownload())
    {
        show();
    }
    else
    {
        hide();
    }
}

void DownloadWidget::onFileAddedToDownload()
{
    if (NULL != d->mpTree)
    {
        // 直接整表重建，下载任务数量很少，逻辑比增量更新简单可靠。
        d->mpTree->clear();
        addCurrentDownloads();
        addPendingDownloads();
    }
}

void DownloadWidget::addCurrentDownloads()
{
    QVector<sDownloadFileDesc> actualDL = DownloadManager::downloadManager()->currentDownloads();
    qDebug() << "实际的下载条数:" << actualDL.size();
    for (int i = 0; i < actualDL.size(); i++)
    {
        // 每个下载占两行：名称行保存任务 id，进度行保存当前/总字节数。
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
        d->mpTree->setItemDelegateForRow(((i + 1) * 2) - 1, &d->mProgressBarDelegate);
    }
}

void DownloadWidget::addPendingDownloads()
{
    QVector<sDownloadFileDesc> pendingDL = DownloadManager::downloadManager()->pendingDownloads();
    for (int i = 0; i < pendingDL.size(); i++)
    {
        // 排队任务只显示名称，没有进度行。
        d->mpItem = new QTreeWidgetItem(d->mpTree);
        d->mpItem->setText(eItemColumn_Desc, pendingDL.at(i).name);
        d->mpItem->setData(eItemColumn_Desc, Qt::UserRole, QVariant(pendingDL.at(i).id));
        d->mpTree->addTopLevelItem(d->mpItem);
    }
}

void DownloadWidget::onDownloadUpdated(int id, qint64 crnt, qint64 total)
{
    if (NULL != d->mpTree)
    {
        QAbstractItemModel *model = d->mpTree->model();
        if (NULL != model)
        {
            // 按进度行保存的任务 id 找到对应行，再更新两个进度字段。
            for (int i = 0; i < model->rowCount(); i++)
            {
                QModelIndex currentIndex = model->index(i, eItemColumn_Desc);
                if (id == currentIndex.data(Qt::UserRole + 2))
                {
                    model->setData(currentIndex, crnt, Qt::UserRole);
                    model->setData(currentIndex, total, Qt::UserRole + 1);
                    break;
                }
            }
        }
    }
}

void DownloadWidget::onDownloadFinished(int id)
{
    Q_UNUSED(id);
    // 任务结束后统一重建列表，隐藏已完成项并显示下一个排队任务。
    onFileAddedToDownload();
}

void DownloadWidget::onCancelClicked()
{
    DownloadManager::downloadManager()->cancelDownloads();
}

void DownloadWidget::onItemClicked(QTreeWidgetItem *pItem, int col)
{
    // 只有点中关闭列（且该行确实对应一个任务）才取消下载。
    if (eItemColumn_Close == col && "" != pItem->text(eItemColumn_Desc))
    {
        DownloadManager::downloadManager()->cancelDownload(pItem->data(eItemColumn_Desc, Qt::UserRole).toInt());
    }
}

// ---------------------------------------------------------------------------------------------
DownloadProgressDelegate::DownloadProgressDelegate(QObject *parent) : QItemDelegate(parent)
{
}

void DownloadProgressDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionProgressBar opt;
    opt.rect     = option.rect;
    opt.minimum  = 0;
    opt.maximum  = index.data(Qt::UserRole + 1).toInt();
    opt.progress = index.data(Qt::UserRole).toInt();

    if (0 == index.column())
    {
        QApplication::style()->drawControl(QStyle::CE_ProgressBar, &opt, painter, 0);
    }
}
