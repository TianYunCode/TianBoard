/**
 * @file DocumentController.h
 * @brief 声明文档域的模型与控制器：文档树节点/模型、树视图与代理、重命名委托以及文档控制器。
 *
 * 本文件是 model 域的核心：DocumentTreeModel 维护“我的文档/回收站”的目录树，
 * DocumentController（继承 DocumentContainer）把树模型、缩略图视图与 BoardController
 * 串起来，DocumentReplaceDialog 处理导入时的重名冲突。
 */

#pragma once

#include <QRegExp>
#include <QRegularExpression>

#include <QtWidgets>
#include "model/DocumentContainer.h"
#include "core/ApplicationController.h"
#include "core/Application.h"
#include "model/SortFilterProxyModel.h"

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
 * @brief 文档树节点：目录树中的一个条目，目录节点可带子节点，文档节点持有 DocumentProxy。
 *
 * 节点由 DocumentTreeModel 拥有并挂在其根节点之下，生命周期与模型一致。文档节点的
 * mProxy 是 QPointer；节点被克隆用于拖放复制时按引用或按内容复制文档。节点自身不负责
 * 持久化，目录名同时作为虚拟路径的一段。
 */
class DocumentTreeNode
{
public:
    friend class DocumentTreeModel;

    /** @brief 节点类型。 */
    enum Type
    {
        Catalog = 0, ///< 目录（文件夹）
        Document     ///< 文档
    };

    /**
     * @brief 构造节点。
     * @param pType 节点类型。
     * @param pName 节点在树中的名称（同时参与虚拟路径）。
     * @param pDisplayName 显示名，空则退回 pName。
     * @param pProxy 文档节点关联的代理，目录节点为空。
     */
    DocumentTreeNode(Type pType, const QString &pName, const QString &pDisplayName = QString(), DocumentProxy *pProxy = 0);

    /** @brief 构造空目录节点，供反序列化时占位。 */
    DocumentTreeNode() : mType(Catalog), mParent(0), mProxy(0)
    {
        ;
    }

    /**
     * @brief 析构：递归删除子节点，并删除自己持有的文档代理。
     */
    ~DocumentTreeNode();

    /** @brief 返回子节点列表。 */
    QList<DocumentTreeNode *> children() const
    {
        return mChildren;
    }

    /** @brief 返回父节点，根节点返回空。 */
    DocumentTreeNode *parentNode()
    {
        return mParent;
    }

    /** @brief 返回节点类型。 */
    Type nodeType() const
    {
        return mType;
    }

    /** @brief 返回节点在树中的名称。 */
    QString nodeName() const
    {
        return mName;
    }

    /** @brief 返回节点显示名。 */
    QString displayName() const
    {
        return mDisplayName;
    }

    /** @brief 同时修改节点名与显示名。 */
    void setNodeName(const QString &str)
    {
        mName        = str;
        mDisplayName = str;
    }

    /**
     * @brief 在子节点末尾追加一个子节点并认领父子关系。
     */
    void addChild(DocumentTreeNode *pChild);

    /**
     * @brief 在指定位置插入子节点并认领父子关系。
     */
    void insertChild(int pIndex, DocumentTreeNode *pChild);

    /**
     * @brief 把 child 从本节点移到 newParent 的 index 位置。
     * @note child 不是本节点的直接子节点时不做任何事。
     */
    void moveChild(DocumentTreeNode *child, int index, DocumentTreeNode *newParent);

    /**
     * @brief 删除指定位置的子树（含后代节点）。
     */
    void removeChild(int index);

    /** @brief 返回节点关联的文档代理，目录节点为空。 */
    DocumentProxy *proxyData() const
    {
        return mProxy;
    }

    /** @brief 是否是没有父节点的根节点。 */
    bool isRoot()
    {
        return !mParent;
    }

    /** @brief 是否是第二层节点（父节点为根），即“我的文档/回收站”这类固定入口。 */
    bool isTopLevel()
    {
        if (mParent)
        {
            return !mParent->mParent;
        }
        else
        {
            return false;
        }
    }

    /**
     * @brief 浅克隆节点；文档节点的代理用拷贝构造新建。
     * @return 新节点，调用方负责挂到树上或释放。
     */
    DocumentTreeNode *clone();

    /** @brief 返回从第一层目录到本节点父节点的虚拟目录路径（不含本节点名）。 */
    QString dirPathInHierarchy();

    /**
     * @brief 判断 node 是否是本节点的后代。
     * @param node 待检查的节点（注意：实现从 node 的父节点开始向上找）。
     */
    bool findNode(DocumentTreeNode *node);

    /** @brief 返回同级下一个节点，没有则返回空。 */
    DocumentTreeNode *nextSibling();

    /** @brief 返回同级上一个节点，没有则返回空。 */
    DocumentTreeNode *previousSibling();

private:
    /** @brief 节点类型。 */
    Type mType;
    /** @brief 树内名称。 */
    QString mName;
    /** @brief 显示名称。 */
    QString mDisplayName;
    /** @brief 父节点，根节点为空。 */
    DocumentTreeNode *mParent;
    /** @brief 子节点列表。 */
    QList<DocumentTreeNode *> mChildren;
    /** @brief 文档节点关联的代理，代理可能先于节点被销毁。 */
    QPointer<DocumentProxy> mProxy;
};
Q_DECLARE_METATYPE(DocumentTreeNode *)

/**
 * @brief 文档目录树模型：以 DocumentTreeNode 为内部指针，向视图暴露目录/文档三层结构。
 *
 * 模型拥有整棵树（mRootNode）与其中的 DocumentProxy；顶层固定为“我的文档”和“回收站”。
 * DocumentController 通常把它包在 SortFilterProxyModel 后面，所以模型自身只管源索引，
 * 视图索引的映射由代理完成。拖放通过 DocumentTreeMimeData 传递内部索引。
 */
class DocumentTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    /** @brief 顶层固定节点标识。 */
    enum eAncestors
    {
        aMyDocuments, ///< 我的文档
        aModel,       ///< 模型库（历史遗留，当前未创建该节点）
        aTrash        ///< 回收站
    };

    /** @brief 节点复制方式。 */
    enum eCopyMode
    {
        aReference,  ///< 按引用复制：新节点与原节点共享同一份文档
        aContentCopy ///< 按内容复制：复制一份独立的文档数据
    };

    /** @brief 插入位置选择方式。 */
    enum eAddItemMode
    {
        aEnd = 0,       ///< 追加到子节点末尾
        aDetectPosition ///< 按名称/日期规则插入到合适位置
    };

    /** @brief 自定义数据角色。 */
    enum eDocumentData
    {
        DataNode = Qt::UserRole + 1, ///< 返回节点指针
        CreationDate,                ///< 返回节点（或目录下）的创建时间
        UpdateDate                   ///< 返回节点（或目录下）的更新时间
    };

    /**
     * @brief 构造模型并创建根节点、“我的文档”和“回收站”两个顶层节点。
     */
    DocumentTreeModel(QObject *parent = 0);

    /**
     * @brief 析构时删除整棵树，树中的文档代理随之释放。
     */
    ~DocumentTreeModel();

    /** @brief 按行、列和父索引返回子节点索引。 */
    QModelIndex index(int row, int column, const QModelIndex &parent) const;

    /** @brief 返回子索引的父索引。 */
    QModelIndex parent(const QModelIndex &child) const;

    /** @brief 返回指定父节点下的子节点数量。 */
    int rowCount(const QModelIndex &parent) const;

    /** @brief 固定返回 3 列：名称、创建日期、更新日期。 */
    int columnCount(const QModelIndex &parent) const;

    /** @brief 提供显示文本、图标、字体、颜色与自定义日期角色。 */
    QVariant data(const QModelIndex &index, int role) const;

    /** @brief 仅支持 EditRole 的重命名。 */
    bool setData(const QModelIndex &index, const QVariant &value, int role);

    /** @brief 返回拖放与编辑标志：回收站外可拖动，目录可接收拖放。 */
    Qt::ItemFlags flags(const QModelIndex &index) const;

    /** @brief 支持移动与复制两种拖放动作。 */
    Qt::DropActions supportedDropActions() const
    {
        return Qt::MoveAction | Qt::CopyAction;
    }

    /** @brief 声明内部拖放使用的 MIME 类型。 */
    QStringList mimeTypes() const;

    /**
     * @brief 把选中的源索引打包为 DocumentTreeMimeData。
     */
    QMimeData *mimeData(const QModelIndexList &indexes) const;

    /**
     * @brief 处理拖放落下：页面拖入文档时复制页面，树内条目按动作移动或复制。
     */
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

    /**
     * @brief 删除指定范围的子树。
     * @note 会递归删除子节点，并从“新文档”列表中移除对应代理。
     */
    bool removeRows(int row, int count, const QModelIndex &parent);

    /** @brief 判断目录（递归）下是否至少有一份文档。 */
    bool containsDocuments(const QModelIndex &index);

    /** @brief 由节点指针查找模型索引，找不到返回无效索引。 */
    QModelIndex indexForNode(DocumentTreeNode *pNode) const;

    /** @brief 由节点指针生成持久模型索引。 */
    QPersistentModelIndex persistentIndexForNode(DocumentTreeNode *pNode);

    /**
     * @brief 把 source 复制到 newParent 下。
     * @param pMode 按引用或按内容复制。
     * @return 新节点的持久索引。
     */
    QPersistentModelIndex copyIndexToNewParent(const QModelIndex &source, const QModelIndex &newParent, eCopyMode pMode = aReference);

    /**
     * @brief 批量复制多个索引，只处理第 0 列。
     */
    void copyIndexToNewParent(const QModelIndexList &list, const QModelIndex &newParent, eCopyMode pMode);

    /**
     * @brief 把 what 移动到 destination（目录）下。
     */
    void moveIndex(const QModelIndex &what, const QModelIndex &destination);

    /**
     * @brief 返回当前节点。
     * @note 排序后视图索引会变化，这里保留节点指针而非索引，供代理模型回查。
     */
    DocumentTreeNode *currentNode() const
    {
        return mCurrentNode;
    }

    /** @brief 设置当前节点。 */
    void setCurrentNode(DocumentTreeNode *pNode)
    {
        mCurrentNode = pNode;
    }

    /** @brief 返回当前节点对应的模型索引。 */
    QModelIndex currentIndex()
    {
        return indexForNode(mCurrentNode);
    }

    /** @brief 查找代理对应的模型索引。 */
    QModelIndex indexForProxy(DocumentProxy *pSearch) const;

    /** @brief 把某份文档设为当前文档。 */
    void setCurrentDocument(DocumentProxy *pDocument);

    /** @brief 设置根节点，模型接管其所有权。 */
    void setRootNode(DocumentTreeNode *pRoot);

    /** @brief 返回根节点。 */
    DocumentTreeNode *rootNode() const
    {
        return mRootNode;
    }

    /** @brief 返回索引对应的文档代理，目录节点返回空。 */
    DocumentProxy *proxyForIndex(const QModelIndex &pIndex) const;

    /** @brief 返回索引对应的虚拟目录（不含自身名称）。 */
    QString virtualDirForIndex(const QModelIndex &pIndex) const;

    /** @brief 返回索引对应的虚拟路径（目录加自身名称）。 */
    QString virtualPathForIndex(const QModelIndex &pIndex) const;

    /** @brief 返回目录下直接子节点的名称列表。 */
    QStringList nodeNameList(const QModelIndex &pIndex) const;

    /** @brief 判断在选中位置是否允许新建条目（回收站内不允许）。 */
    bool newNodeAllowed(const QModelIndex &pSelectedIndex) const;

    /**
     * @brief 按虚拟路径进入目录，缺失的层级会自动创建。
     * @param dir 以“/”分隔的虚拟路径。
     */
    QModelIndex goTo(const QString &dir);

    /** @brief 判断索引是否位于回收站之下。 */
    bool inTrash(const QModelIndex &index) const;

    /** @brief 判断索引是否是目录节点。 */
    bool isCatalog(const QModelIndex &index) const
    {
        return nodeFromIndex(index)->nodeType() == DocumentTreeNode::Catalog;
    }

    /** @brief 判断索引是否是文档节点。 */
    bool isDocument(const QModelIndex &index) const
    {
        return nodeFromIndex(index)->nodeType() == DocumentTreeNode::Document;
    }

    /** @brief 判断索引是否是第二层固定节点。 */
    bool isToplevel(const QModelIndex &index) const
    {
        return nodeFromIndex(index) ? nodeFromIndex(index)->isTopLevel() : false;
    }

    /** @brief 判断索引是否是固定节点（不允许删除/移动的顶层入口）。 */
    bool isConstant(const QModelIndex &index) const
    {
        return isToplevel(index);
    }

    /** @brief 判断索引是否可重命名（以可编辑标志为准）。 */
    bool isOkToRename(const QModelIndex &index) const
    {
        return flags(index) & Qt::ItemIsEditable;
    }

    /** @brief 返回索引对应的文档代理。 */
    DocumentProxy *proxyData(const QModelIndex &index) const
    {
        return nodeFromIndex(index)->proxyData();
    }

    /**
     * @brief 按代理元数据中的分组名把文档挂到树上。
     * @param pParent 为空时使用代理记录的分组路径。
     */
    void addDocument(DocumentProxy *pProxyData, const QModelIndex &pParent = QModelIndex());

    /**
     * @brief 添加本次会话新建的文档，并记入“新文档”列表。
     */
    void addNewDocument(DocumentProxy *pProxyData, const QModelIndex &pParent = QModelIndex());

    /** @brief 在父目录下新建目录节点。 */
    QModelIndex addCatalog(const QString &pName, const QModelIndex &pParent);

    /** @brief 返回本次会话新建、尚未落盘的文档列表。 */
    QList<DocumentProxy *> newDocuments()
    {
        return mNewDocuments;
    }

    /** @brief 把已存在于树中的文档标记为新建（用于新建后立即保存）。 */
    void markDocumentAsNew(DocumentProxy *pDoc)
    {
        if (indexForProxy(pDoc).isValid())
        {
            mNewDocuments << pDoc;
        }
    }

    /**
     * @brief 重命名节点。
     *
     * 目录重命名会递归更新子孙节点的虚拟路径（内部用“+!##s”分隔层级）；
     * 文档重命名只改名称/分组元数据。
     */
    void setNewName(const QModelIndex &index, const QString &newName);

    /** @brief 若同级已有同名节点，则追加数字后缀生成唯一名称。 */
    QString adjustNameForParentIndex(const QString &pName, const QModelIndex &pIndex);

    /** @brief 返回“我的文档”顶层索引。 */
    QPersistentModelIndex myDocumentsIndex() const
    {
        return mMyDocuments;
    }

    /** @brief 返回“回收站”顶层索引。 */
    QPersistentModelIndex trashIndex() const
    {
        return mTrash;
    }

    /** @brief 把模型索引转换为节点指针；无效索引返回根节点。 */
    DocumentTreeNode *nodeFromIndex(const QModelIndex &pIndex) const;

    /** @brief 按节点名称比较节点，供排序使用。 */
    static bool nodeLessThan(const DocumentTreeNode *firstIndex, const DocumentTreeNode *secondIndex);

    /** @brief 设置拖放时高亮的节点。 */
    void setHighLighted(const QModelIndex &newHighLighted)
    {
        mHighLighted = newHighLighted;
    }

    /** @brief 返回拖放时高亮的节点。 */
    QModelIndex highLighted()
    {
        return mHighLighted;
    }

    /** @brief 当前日期列的排序方向（升序取最早日期，降序取最晚日期）。 */
    bool ascendingOrder() const
    {
        return mAscendingOrder;
    }

    /** @brief 递归查找节点（或目录下）指定类型的日期。 */
    QDateTime findNodeDate(DocumentTreeNode *node, QString type) const;

    /** @brief 判断索引是否位于“我的文档”之下。 */
    bool inMyDocuments(const QModelIndex &index) const;

    /**
     * @brief 批量把源索引移动到目标目录，跳过已是目标子节点或目标祖先的项。
     */
    void moveIndexes(const QModelIndexList &source, const QModelIndex &destination);

    /** @brief 判断 pPossibleDescendant 是否是 pPossibleAncestor 的后代。 */
    bool isDescendantOf(const QModelIndex &pPossibleDescendant, const QModelIndex &pPossibleAncestor) const;

signals:
    /**
     * @brief 当前文档节点被移动后发出。
     * @param newIndex 移动后的索引。
     * @param previous 移动前的索引。
     */
    void currentIndexMoved(const QModelIndex &newIndex, const QModelIndex &previous);

private:
    /** @brief 根节点，模型拥有其所有权。 */
    DocumentTreeNode *mRootNode;
    /** @brief 当前节点，视图切换文档时更新。 */
    DocumentTreeNode *mCurrentNode;

    /** @brief 在子树中递归查找持有指定代理的节点。 */
    DocumentTreeNode *findProxy(DocumentProxy *pSearch, DocumentTreeNode *pParent) const;
    /** @brief 递归按节点指针构造索引。 */
    QModelIndex pIndexForNode(const QModelIndex &parent, DocumentTreeNode *pNode) const;
    /** @brief 把游离节点插入父节点并发出行插入通知。 */
    QModelIndex addNode(DocumentTreeNode *pFreeNode, const QModelIndex &pParent, eAddItemMode pMode = aDetectPosition);
    /** @brief 计算游离节点在父节点中应处的有序位置。 */
    int positionForParent(DocumentTreeNode *pFreeNode, DocumentTreeNode *pParentNode);
    /** @brief 移动后若目标层存在重名，则给节点补唯一后缀。 */
    void fixNodeName(const QModelIndex &source, const QModelIndex &dest);
    /** @brief 递归把节点及其子孙的名称/分组元数据重新持久化。 */
    void updateIndexNameBindings(DocumentTreeNode *nd);
    /** @brief 根节点索引的持久副本。 */
    QPersistentModelIndex mRoot;
    /** @brief “我的文档”索引的持久副本。 */
    QPersistentModelIndex mMyDocuments;
    /** @brief “回收站”索引的持久副本。 */
    QPersistentModelIndex mTrash;
    /** @brief 本次会话新建的文档代理。 */
    QList<DocumentProxy *> mNewDocuments;
    /** @brief 拖放悬停时高亮的节点索引。 */
    QModelIndex mHighLighted;

    /** @brief 日期列的排序方向。 */
    bool mAscendingOrder;

    /** @brief 递归求目录下最新的更新时间。 */
    QDateTime findCatalogUpdatedDate(DocumentTreeNode *node) const;
    /** @brief 递归求目录下最早的创建时间。 */
    QDateTime findCatalogCreationDate(DocumentTreeNode *node) const;
};

/**
 * @brief 文档树内部拖放使用的 MIME 数据：携带被拖动的源模型索引。
 *
 * 只在 DocumentTreeModel 与其视图之间传递，不跨进程序列化。
 */
class DocumentTreeMimeData : public QMimeData
{
    Q_OBJECT

public:
    /** @brief 返回被拖动的索引列表。 */
    QList<QModelIndex> indexes() const
    {
        return mIndexes;
    }

    /** @brief 设置被拖动的索引列表。 */
    void setIndexes(const QList<QModelIndex> &pIndexes)
    {
        mIndexes = pIndexes;
    }

private:
    /** @brief 被拖动的源模型索引。 */
    QList<QModelIndex> mIndexes;
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
