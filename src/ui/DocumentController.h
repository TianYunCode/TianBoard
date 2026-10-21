/**
 * @file DocumentController.h
 * @brief 文档模式的界面与控制器声明：树视图、重命名委托、重名对话框与文档控制器。
 *
 * 详细说明：文档树本身（节点、模型、拖放载荷）是域侧的东西，住在
 * model/DocumentTreeModel.h；本文件剩下的都是界面：DocumentTreeView 与它的委托、
 * 导入重名时的 DocumentReplaceDialog，以及把树模型、缩略图视图与 BoardController
 * 串起来的 DocumentController（继承 DocumentContainer）。
 */

#pragma once

#include <QRegExp>
#include <QRegularExpression>

#include <QtWidgets>
#include "model/DocumentContainer.h"
#include "core/ApplicationController.h"
#include "core/Application.h"
#include "model/SortFilterProxyModel.h"
#include "model/DocumentTreeModel.h"

#include "ui/MessageWindow.h"

class GraphicsScene;
class QDialog;
class DocumentProxy;
class BoardController;
class DocumentsWidget;
class ThumbnailsScene;
class DocumentGroupTreeItem;
class DocumentProxyTreeItem;
class MainWindow;
class DocumentToolsPalette;

/**
 * @brief 导入文件重名时的替换对话框：让用户决定沿用旧名替换还是输入新名新建文档。
 *
 * 只在导入流程中由 DocumentController 弹出。输入内容由正则校验器约束，列表 mFileNameList
 * 保存同级已存在的名称；输入命中已存在名称时确认按钮变为“替换”。对话框本身不落盘，
 * 结果由调用方读取 lineEditText()。
 */
class DocumentReplaceDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造对话框。
     * @param pIncommingName 待导入文档的原始名称。
     * @param pFileList 同级已存在的文档名列表。
     * @param parent 父窗口。
     * @param pFlags 窗口标志。
     */
    DocumentReplaceDialog(
        const QString &pIncommingName, const QStringList &pFileList, QWidget *parent = 0, Qt::WindowFlags pFlags = Qt::WindowFlags());

    /**
     * @brief 设置名称输入框使用的正则校验规则。
     * @warning mValidator 在该实现中未创建，调用前应确认其非空。
     */
    void setRegexp(const QRegularExpression &pRegExp);

    /**
     * @brief 判断输入框当前内容是否通过校验。
     * @param pStr 待校验字符串（当前实现未使用，直接查询输入框状态）。
     */
    bool validString(const QString &pStr);

    /**
     * @brief 重置对话框的文件名与同级名称列表（例如连续导入多个文件时复用对话框）。
     */
    void setFileNameAndList(const QString &fileName, const QStringList &pLst);

    /**
     * @brief 生成带文档名的提示文案。
     */
    QString labelTextWithName(const QString &documentName) const;

    /** @brief 返回用户最终输入的名称。 */
    QString lineEditText() const
    {
        return mLineEdit->text();
    }

signals:
    /**
     * @brief 用户要求把当前名称作为新文件夹创建时发出。
     * @param str 输入的名称。
     */
    void createNewFolder(QString str);

    /** @brief 对话框被取消时发出，供调用方清理状态。 */
    void closeDialog();

private slots:
    /**
     * @brief 接受对话框（名称合法时由确认按钮触发）。
     */
    void accept();

    /**
     * @brief 取消对话框：清空输入并发出 closeDialog。
     */
    void reject();

    /**
     * @brief 输入变化时刷新校验底色与确认按钮文案。
     */
    void reactOnTextChanged(const QString &pStr);

private:
    /** @brief 名称输入框。 */
    QLineEdit *mLineEdit;
    /** @brief 名称校验器；当前实现未初始化。 */
    QRegularExpressionValidator *mValidator;
    /** @brief 同级已存在的名称列表。 */
    QStringList mFileNameList;
    /** @brief 待导入文档的原始名称。 */
    QString mIncommingName;
    /** @brief 确认按钮。 */
    QPushButton *acceptButton;
    /** @brief 确认按钮文案：接受。 */
    const QString acceptText;
    /** @brief 确认按钮文案：替换。 */
    const QString replaceText;
    /** @brief 取消按钮文案。 */
    const QString cancelText;
    /** @brief 对话框上方的提示标签。 */
    QLabel *mLabelText;
};

/**
 * @brief 文档目录树视图：在 QTreeView 之上处理拖放高亮、选中展开和源/代理索引映射。
 *
 * 视图的 model 通常是 SortFilterProxyModel；本类所有对外接口都接受源模型索引，内部
 * 负责映射到视图索引，避免把源索引直接交给代理模型。
 */
class DocumentTreeView : public QTreeView
{
    Q_OBJECT

public:
    /** @brief 构造视图并启用根节点装饰。 */
    DocumentTreeView(QWidget *parent = 0);

    /** @brief 把视图（代理）索引映射回源模型索引。 */
    QModelIndex mapIndexToSource(const QModelIndex &index);

    /** @brief 批量映射回源模型索引。 */
    QModelIndexList mapIndexesToSource(const QModelIndexList &indexes);

public slots:
    /**
     * @brief 选中并展开指定源索引。
     * @param pIndex 源模型索引。
     * @param pExpand 选中/取消选中。
     * @param pEdit 是否立即进入编辑。
     */
    void setSelectedAndExpanded(const QModelIndex &pIndex, bool pExpand = true, bool pEdit = false);

    /** @brief 横向滚动条范围变化时自动滚到最右端。 */
    void hSliderRangeChanged(int min, int max);

protected:
    /** @brief 接受外部拖入。 */
    void dragEnterEvent(QDragEnterEvent *event);

    /** @brief 拖动离开时清除高亮。 */
    void dragLeaveEvent(QDragLeaveEvent *event);

    /** @brief 拖动移动时校验落点并高亮目标文档。 */
    void dragMoveEvent(QDragMoveEvent *event);

    /** @brief 处理落下：页面复制到文档，或树内条目移动/移入回收站。 */
    void dropEvent(QDropEvent *event);

    /** @brief 绘制事件，当前直接交给基类。 */
    void paintEvent(QPaintEvent *event);

    /** @brief 把当前 model 转为 DocumentTreeModel，失败返回空。 */
    DocumentTreeModel *fullModel()
    {
        return qobject_cast<DocumentTreeModel *>(model());
    }

    /** @brief 行删除前通知基类。 */
    void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);

private:
    /** @brief 判断拖动项能否落到目标项。 */
    bool isAcceptable(const QModelIndex &dragIndex, const QModelIndex &atIndex);
    /** @brief 返回可接受的拖放动作。 */
    Qt::DropAction acceptableAction(const QModelIndex &dragIndex, const QModelIndex &atIndex);
    /** @brief 刷新目标行上下若干行的绘制区域，抹掉旧高亮。 */
    void updateIndexEnvirons(const QModelIndex &index);
};

/**
 * @brief 文档树重命名委托：第 0 列用 QLineEdit 编辑，并实时提示同级重名。
 *
 * 编辑期间用 mExistingFileNames 缓存同级已有名称；重名时输入框变红但不阻止提交，
 * 真正写回由 setModelData 再次校验。
 */
class DocumentTreeItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    /** @brief 构造委托。 */
    DocumentTreeItemDelegate(QObject *parent = 0);

private slots:
    /** @brief 编辑完成时提交数据并请求重新排序。 */
    void commitAndCloseEditor();
    /** @brief 输入变化时按是否同名切换输入框底色。 */
    void processChangedText(const QString &str) const;
    /** @brief 判断名称是否未被同级占用。 */
    bool validateString(const QString &str) const;

protected:
    /** @brief 只为第 0 列创建行编辑器。 */
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    /** @brief 用当前名称填充编辑器并全选。 */
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    /** @brief 校验通过后把编辑器内容写回模型。 */
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    /** @brief 默认绘制。 */
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
    /** @brief 编辑期间缓存的同级已有名称，mutable 以支持 const 槽。 */
    mutable QStringList mExistingFileNames;
};

/**
 * @brief 文档控制器：文档模式的总控，连接文档树、缩略图视图、排序代理与 BoardController。
 *
 * 继承 DocumentContainer 获得“当前文档 + 缩略图缓存”，并在此基础上负责新建/导入/删除/
 * 重命名/拖放/导出等文档操作，维护选择类型（文件夹、文档、页），并根据选择状态刷新
 * 主窗口动作。生命周期由主窗口持有，mDocumentWidget 作为文档模式的中央控件。
 */
class DocumentController : public DocumentContainer
{
    Q_OBJECT

public:
    /** @brief 删除行为的类型，决定删除动作的文案与后果。 */
    enum DeletionType
    {
        MoveToTrash = 0, ///< 移入回收站
        CompleteDelete,  ///< 永久删除
        EmptyFolder,     ///< 清空普通目录（内容移入回收站）
        EmptyTrash,      ///< 清空回收站（永久删除）
        DeletePage,      ///< 删除页面
        NoDeletion       ///< 当前选择不允许删除
    };

    /** @brief 最近一次选中的元素类型。 */
    enum LastSelectedElementType
    {
        None = 0, ///< 无选择
        Folder,   ///< 目录
        Document, ///< 文档
        Page      ///< 页面缩略图
    };

    /** @brief 排序方向。 */
    enum SortOrder
    {
        ASC = 0, ///< 升序
        DESC     ///< 降序
    };

    /** @brief 排序依据。 */
    enum SortKind
    {
        CreationDate, ///< 按创建日期
        UpdateDate,   ///< 按更新日期
        Alphabetical  ///< 按名称字母序
    };

    /**
     * @brief 构造控制器：建立文档视图、工具栏与信号连接。
     * @param mainWindow 主窗口，控制器从中取得中央控件与动作。
     */
    DocumentController(MainWindow *mainWindow);

    /** @brief 析构；文档视图随 mDocumentWidget 一起由 Qt 销毁。 */
    virtual ~DocumentController();

    /** @brief 关闭前调用，停止响应视图信号。 */
    void closing();

    /** @brief 返回文档模式的中央控件。 */
    QWidget *controlView();

    /**
     * @brief 查找文档对应的树条目。
     * @return 当前实现总是选中该文档并返回 nullptr（树条目类型已废弃）。
     */
    DocumentProxyTreeItem *findDocument(DocumentProxy *proxy);

    /**
     * @brief 通过文件对话框把受支持文件追加到指定文档。
     * @return 是否成功导入。
     */
    bool addFileToDocument(DocumentProxy *document);

    /**
     * @brief 删除缩略图视图中选中的页面。
     */
    void deletePages(QList<QGraphicsItem *> itemsToDelete);

    /** @brief 返回缩略图视图中最后选中项的场景索引，没有选择时返回 -1。 */
    int getSelectedItemIndex();

    /** @brief 判断页面能否上移（页号 >= 1）。 */
    bool pageCanBeMovedUp(int page);

    /** @brief 判断页面能否下移（不是最后一页）。 */
    bool pageCanBeMovedDown(int page);

    /** @brief 判断页面能否复制（第一页/标题页不能）。 */
    bool pageCanBeDuplicated(int page);

    /** @brief 判断页面能否删除（第一页/标题页不能）。 */
    bool pageCanBeDeleted(int page);

    /** @brief 回收站分组名（本地化）。 */
    QString documentTrashGroupName()
    {
        return mDocumentTrashGroupName;
    }

    /** @brief 默认文档分组名（本地化）。 */
    QString defaultDocumentGroupName()
    {
        return mDefaultDocumentGroupName;
    }

    /**
     * @brief 切换当前文档。
     * @param forceReload 为 true 时强制重载缩略图。
     */
    void setDocument(DocumentProxy *document, bool forceReload = false);

    /** @brief 返回树视图中第一个选中的源索引。 */
    QModelIndex firstSelectedTreeIndex();

    /** @brief 返回树视图中第一个选中项对应的文档代理。 */
    DocumentProxy *firstSelectedTreeProxy();

    /**
     * @brief 根据选择类型与索引推导应执行的删除方式。
     */
    inline DeletionType deletionTypeForSelection(
        LastSelectedElementType pTypeSelection, const QModelIndex &selectedIndex, DocumentTreeModel *docModel) const;

    /** @brief 判断是否只选中了第一页（唯一一页）。 */
    bool firstAndOnlySceneSelected() const;

    /** @brief 返回文档模式的中央控件。 */
    QWidget *mainWidget() const
    {
        return mDocumentWidget;
    }

    /**
     * @brief 把单项移入回收站。
     */
    void moveToTrash(QModelIndex &index, DocumentTreeModel *docModel);

    /**
     * @brief 递归删除目录中超过指定天数的文档。
     */
    void deleteDocumentsInFolderOlderThan(const QModelIndex &index, const int days);

    /**
     * @brief 递归删除不含任何文档的目录。
     */
    void deleteEmptyFolders(const QModelIndex &index);

    /** @brief 视图索引到源索引。 */
    QModelIndex mapIndexToSource(const QModelIndex &index);

    /** @brief 批量视图索引到源索引。 */
    QModelIndexList mapIndexesToSource(const QModelIndexList &indexes);

    /**
     * @brief 按种类与方向排序文档树。
     * @param kind SortKind。
     * @param order SortOrder。
     */
    void sortDocuments(int kind, int order);

    /**
     * @brief 批量把索引移入回收站，并处理当前文档被移走后的回落选择。
     */
    void moveIndexesToTrash(const QModelIndexList &list, DocumentTreeModel *docModel);

    /** @brief 向前查找第一个未选中的兄弟项（递归到父级）。 */
    QModelIndex findPreviousSiblingNotSelected(const QModelIndex &index, QItemSelectionModel *selectionModel);

    /** @brief 向后查找第一个未选中的兄弟项（递归到父级）。 */
    QModelIndex findNextSiblingNotSelected(const QModelIndex &index, QItemSelectionModel *selectionModel);

    /** @brief 判断 child 的任一祖先是否被选中。 */
    bool parentIsSelected(const QModelIndex &child, QItemSelectionModel *selectionModel);

signals:
    /** @brief 导出完成后发出，主窗口据此恢复界面状态。 */
    void exportDone();

    /** @brief 文档集合发生变化、需要重新排序时发出。 */
    void reorderDocumentsRequested();

public slots:
    /** @brief 在当前选中目录下新建空白文档并进入重命名。 */
    void createNewDocument();

    /** @brief 按当前排序设置刷新创建/更新日期列的显示。 */
    void refreshDateColumns();

    /** @brief 按视图当前排序控件重新排序。 */
    void reorderDocuments();

    /** @brief 在“我的文档”根下新建一份空白文档。 */
    void createEmptyDocument();

    /** @brief 在当前选中目录下新建文件夹并进入重命名。 */
    void createNewDocumentGroup();

    /** @brief 删除当前选择（先弹确认框）。 */
    void deleteSelectedItem();

    /** @brief 处理单项删除，按删除类型分派。 */
    void deleteSingleItem(QModelIndex index, DocumentTreeModel *docModel);

    /** @brief 处理多选删除，逐项按各自位置计算删除类型。 */
    void deleteMultipleItems(QModelIndexList indexes, DocumentTreeModel *docModel);

    /** @brief 清空目录：内容移入回收站或永久删除。 */
    void emptyFolder(const QModelIndex &index, DeletionType pDeletionType = MoveToTrash);

    /** @brief 递归删除索引对应的节点及其文档数据。 */
    void deleteIndexAndAssociatedData(const QModelIndex &pIndex);

    /** @brief 进入当前选中项的重命名编辑。 */
    void renameSelectedItem();

    /** @brief 打开当前选中的文档或页面。 */
    void openSelectedItem();

    /** @brief 复制当前选择（页面或文档）。 */
    void duplicateSelectedItem();

    /** @brief 通过文件对话框导入文件。 */
    void importFile();

    /** @brief 拖放移动页面后同步文档更新时间与看板当前页。 */
    void moveSceneToIndex(DocumentProxy *proxy, int source, int target);

    /**
     * @brief 选中指定文档。
     * @param setAsCurrentDocument 是否同时切换到看板当前文档。
     * @param onImport 是否处于导入流程（影响看板加载方式）。
     * @param editMode 是否立即进入重命名。
     */
    void selectDocument(DocumentProxy *proxy, bool setAsCurrentDocument = true, const bool onImport = false, const bool editMode = false);

    /** @brief 进入文档模式时刷新选择与缩略图。 */
    void show();

    /** @brief 离开文档模式时的钩子，当前无操作。 */
    void hide();

    /** @brief 在缩略图视图下方显示消息（可带转轮）。 */
    void showMessage(const QString &message, bool showSpinningWheel = false);

    /** @brief 隐藏消息窗口。 */
    void hideMessage();

    /** @brief 显示/隐藏文档工具面板。 */
    void toggleDocumentToolsPalette();

    /** @brief 焦点变化时更新最近选择类型。 */
    void focusChanged(QWidget *old, QWidget *current);

    /** @brief 按当前选择刷新主窗口各动作的可用性与文案。 */
    void updateActions();

    /** @brief 当前文档节点被移动后的处理。 */
    void currentIndexMoved(const QModelIndex &newIndex, const QModelIndex &PreviousIndex);

    /** @brief 排序依据变化。 */
    void onSortKindChanged(int index);

    /** @brief 排序方向变化。 */
    void onSortOrderChanged(bool order);

    /** @brief 分栏拖动后保存分栏尺寸。 */
    void onSplitterMoved(int size, int index);

    /** @brief 折叠全部目录，但保持“我的文档”展开。 */
    void collapseAll();

    /** @brief 展开全部目录。 */
    void expandAll();

protected:
    /** @brief 建立文档中央控件、树视图、代理模型与信号连接。 */
    virtual void setupViews();

    /** @brief 在文档工具栏上插入间距并连接工具面板动作。 */
    virtual void setupToolbar();

    /** @brief 创建文档工具面板并初始化显隐。 */
    void setupPalettes();

    /** @brief 检查文档版本是否高于当前程序，必要时询问用户。 */
    bool isOKToOpenDocument(DocumentProxy *proxy);

    /** @brief 返回树视图第一个选中项对应的文档代理。 */
    DocumentProxy *selectedDocumentProxy();

    /** @brief 返回所有选中项对应的文档代理。 */
    QList<DocumentProxy *> selectedProxies();

    /** @brief 返回树视图中所有选中行（第 0 列）的源索引。 */
    QModelIndexList selectedTreeIndexes();

    /** @brief 返回当前选中的文档树条目。 */
    DocumentProxyTreeItem *selectedDocumentProxyTreeItem();

    /** @brief 返回当前选中的目录树条目。 */
    DocumentGroupTreeItem *selectedDocumentGroupTreeItem();

    /** @brief 返回树中所有目录名。 */
    QStringList allGroupNames();

    /** @brief 把文档条目移入回收站。 */
    void moveDocumentToTrash(DocumentGroupTreeItem *groupTi, DocumentProxyTreeItem *proxyTi);

    /** @brief 把目录条目移入回收站。 */
    void moveFolderToTrash(DocumentGroupTreeItem *groupTi);

    /** @brief 最近一次选中的元素类型。 */
    LastSelectedElementType mSelectionType;

private:
    /** @brief 对话框使用的父控件。 */
    QWidget *mParentWidget;
    /** @brief 看板控制器，用于切换当前文档与页面。 */
    BoardController *mBoardController;
    /** @brief 文档视图控件（树 + 缩略图）。 */
    DocumentsWidget *mDocumentsView;
    /** @brief 主窗口。 */
    MainWindow *mMainWindow;
    /** @brief 文档模式的中央容器控件。 */
    QWidget *mDocumentWidget;
    /** @brief 缩略图区域上的消息窗口。 */
    QPointer<MessageWindow> mMessageWindow;
    /** @brief “添加图片文件夹”菜单动作。 */
    QAction *mAddFolderOfImagesAction;
    /** @brief “从文件添加页面”菜单动作。 */
    QAction *mAddFileToDocumentAction;
    /** @brief “添加图片”菜单动作。 */
    QAction *mAddImagesAction;
    /** @brief 是否正在关闭，关闭期间忽略视图信号。 */
    bool mIsClosing;
    /** @brief 文档工具面板。 */
    DocumentToolsPalette *mToolsPalette;
    /** @brief 工具面板是否已完成首次定位。 */
    bool mToolsPalettePositionned;

    /** @brief 回收站分组名。 */
    QString mDocumentTrashGroupName;
    /** @brief 默认文档分组名。 */
    QString mDefaultDocumentGroupName;

    /** @brief 当前索引是否刚被移动（用于选择变化时判定当前文档）。 */
    bool mCurrentIndexMoved;

    /** @brief 文档树排序代理。 */
    SortFilterProxyModel *mSortFilterProxyModel;

public slots:
    /** @brief 当前项变化时刷新缩略图与选择类型。 */
    void TreeViewSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
    /** @brief 选择集合变化时逐个处理，跳过父项也被选中的子项。 */
    void TreeViewSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

private slots:
    /** @brief 缩略图尺寸滑块变化时更新缩略图宽度并保存设置。 */
    void documentZoomSliderValueChanged(int value);
    /** @brief 选择类型变化后刷新动作状态。 */
    void itemSelectionChanged(LastSelectedElementType newSelection);
    /** @brief 导出当前文档。 */
    void exportDocument();
    /** @brief 批量导出，当前为空实现。 */
    void exportDocumentSet();

    /** @brief 缩略图视图尺寸变化时调整滑块上限。 */
    void thumbnailViewResized();
    /** @brief 页面选择变化时重新判定选择类型并刷新动作。 */
    void pageSelectionChanged();

    /** @brief 文档页面被创建或即将删除时刷新缩略图与选择。 */
    void documentSceneChanged(DocumentProxy *proxy, int pSceneIndex);

    /** @brief 双击页面缩略图时在看板中打开对应页。 */
    void thumbnailPageDoubleClicked(QGraphicsItem *item, int index);
    /** @brief 单击页面缩略图时刷新选择状态。 */
    void pageClicked(QGraphicsItem *item, int index);
    /** @brief 把选中的页面追加到当前看板文档。 */
    void addToDocument();

    /** @brief 把图片文件夹内容添加为页面。 */
    void addFolderOfImages();
    /** @brief 从文件添加页面（无参槽，作用于当前选中文档）。 */
    void addFileToDocument();
    /** @brief 把多个图片文件添加为页面。 */
    void addImages();
    /** @brief 缩略图集合变化时重建缩略图视图。 */
    void refreshDocumentThumbnailsView(DocumentContainer *source);
};
