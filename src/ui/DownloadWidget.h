/**
 * @file DownloadWidget.h
 * @brief 提供下载任务列表与进度显示控件。
 */

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

/** 下载列表的列：文件名与右侧的关闭图标。 */
typedef enum { eItemColumn_Desc, eItemColumn_Close } eItemColumn;


/**
 * @brief 在树控件第二行绘制下载进度的委托。
 *
 * DownloadWidget 为每个正在下载的文件插入两行：第一行是文件名与关闭图标，第二行
 * 只存进度数据，由本委托画成进度条。委托不拥有数据，生命周期由 DownloadWidget
 * 的私有结构持有。
 */
class DownloadProgressDelegate : public QItemDelegate {
    Q_OBJECT
public:
    /**
     * @brief 构造进度条委托。
     * @param parent 父对象
     */
    DownloadProgressDelegate(QObject *parent = 0);

protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

/**
 * @brief 下载任务列表控件：显示进行中的下载、进度条与取消按钮。
 *
 * 既被 DockDownloadWidget 嵌入右侧停靠面板，也被 MainWindow 用于独立下载窗口。
 * 控件直接连接 DownloadManager 的信号刷新列表：每个下载占两行（名称行和进度行），
 * 点击名称行的关闭图标可取消单个下载，底部 Cancel 取消全部下载。
 */
class DownloadWidget : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief 构造列表、取消按钮并连接下载管理器信号。
     * @param parent 父控件
     * @param name 对象名
     */
    DownloadWidget(QWidget *parent = 0, const char *name = "DownloadWidget");
    ~DownloadWidget();

private slots:
    /** @brief 有新下载加入时重建整个列表。 */
    void onFileAddedToDownload();
    /**
     * @brief 按文件 id 更新对应进度行的数值。
     * @param id 下载任务 id
     * @param crnt 已传输字节数
     * @param total 总字节数
     */
    void onDownloadUpdated(int id, qint64 crnt, qint64 total);
    /**
     * @brief 下载结束时刷新列表。
     * @param id 结束的任务 id（当前未使用）
     */
    void onDownloadFinished(int id);
    /** @brief Cancel 按钮：取消所有下载。 */
    void onCancelClicked();
    /**
     * @brief 点击列表项：点中关闭列时取消该下载。
     * @param pItem 被点击的项
     * @param col 列下标
     */
    void onItemClicked(QTreeWidgetItem *pItem, int col);

private:
    /** @brief 把当前正在下载的任务（名称行加进度行）加入列表。 */
    void addCurrentDownloads();
    /** @brief 把排队等待的任务加入列表。 */
    void addPendingDownloads();

    struct Private;
    std::unique_ptr<Private> d;
};
