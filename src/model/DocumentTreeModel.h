/**
 * @file DocumentTreeModel.h
 * @brief 文档目录树的节点、模型与内部拖放载荷声明。
 */

#pragma once

#include <QAbstractItemModel>
#include <QDateTime>
#include <QMimeData>
#include <QModelIndex>
#include <QPointer>
#include <QStringList>
#include <QVariant>

#include "model/DocumentProxy.h"

class DocumentTreeModel;

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
    /** @brief 当前节点，视图切换文档时更新；没有当前文档时为空。
     *  必须显式初始化：构造函数过去不碰它，Debug 下它一直是调试堆的 0xCD… 填充值，
     *  于是 removeRows 里那句 `mCurrentNode && …` 恒真，紧接着拿它去走父链就是崩溃。 */
    DocumentTreeNode *mCurrentNode = nullptr;

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
