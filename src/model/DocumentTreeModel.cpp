#include "model/DocumentTreeModel.h"

#include <QApplication>
#include <QDebug>
#include <QMimeData>
#include <QUrl>

#include "core/Application.h"
#include "core/ApplicationController.h"
#include "core/MimeData.h"
#include "model/IDocumentFormatServices.h"
#include "core/PersistenceManager.h"
#include "model/DocumentProxy.h"
#include "settings/Setting.h"
#include "settings/Settings.h"
#include "utils/FileSystemUtils.h"
#include "utils/StringUtils.h"

static bool lessThan(DocumentTreeNode *lValue, DocumentTreeNode *rValue)
{
    // 目录永远排在文档之前；目录之间按名称升序。
    if (lValue->nodeType() == DocumentTreeNode::Catalog)
    {
        if (rValue->nodeType() == DocumentTreeNode::Catalog)
        {
            return lValue->nodeName() < rValue->nodeName();
        }
        else
        {
            return true;
        }
    }
    else
    {
        if (rValue->nodeType() == DocumentTreeNode::Catalog)
        {
            return false;
        }
        else
        {
            Q_ASSERT(lValue->proxyData());
            Q_ASSERT(rValue->proxyData());

            // 文档之间按创建时间倒序，最近创建的排在最上面。
            QDateTime lTime = lValue->proxyData()->documentDate();
            QDateTime rTime = rValue->proxyData()->documentDate();

            return lTime > rTime;
        }
    }
}

DocumentTreeNode::DocumentTreeNode(Type pType, const QString &pName, const QString &pDisplayName, DocumentProxy *pProxy)
    : mType(pType), mName(pName), mDisplayName(pDisplayName), mProxy(pProxy)
{
    if (pDisplayName.isEmpty())
    {
        mDisplayName = mName;
    }
    mParent = 0;
}

void DocumentTreeNode::addChild(DocumentTreeNode *pChild)
{
    if (pChild)
    {
        mChildren       += pChild;
        pChild->mParent  = this;
    }
}

void DocumentTreeNode::insertChild(int pIndex, DocumentTreeNode *pChild)
{
    if (pChild)
    {
        mChildren.insert(pIndex, pChild);
        pChild->mParent = this;
    }
}

void DocumentTreeNode::moveChild(DocumentTreeNode *child, int index, DocumentTreeNode *newParent)
{
    int childIndex = mChildren.indexOf(child);
    if (childIndex == -1)
    {
        return;
    }

    newParent->insertChild(index, child);
    mChildren.removeAt(childIndex);
}

void DocumentTreeNode::removeChild(int index)
{
    if (index < 0 || index > mChildren.count() - 1)
    {
        return;
    }

    DocumentTreeNode *curChild = mChildren[index];
    while (curChild->mChildren.count())
    {
        curChild->removeChild(0);
    }

    mChildren.removeAt(index);
    delete curChild;
}

DocumentTreeNode *DocumentTreeNode::clone()
{
    return new DocumentTreeNode(this->mType, this->mName, this->mDisplayName, this->mProxy ? new DocumentProxy(*this->mProxy) : 0);
}

QString DocumentTreeNode::dirPathInHierarchy()
{
    QString           result;
    DocumentTreeNode *curNode = this;
    // 只拼接第二层以下的目录名：顶层是“我的文档/回收站”这类固定入口，
    // 它们不参与文档的分组路径，否则虚拟路径会多出一段。
    while (curNode->parentNode() && !curNode->isTopLevel())
    {
        result.prepend(curNode->parentNode()->nodeName() + "/");
        curNode = curNode->parentNode();
    }

    if (result.endsWith("/"))
    {
        result.truncate(result.size() - 1);
    }

    return result;
}

DocumentTreeNode::~DocumentTreeNode()
{
    foreach (DocumentTreeNode *curChildren, mChildren)
    {
        delete (curChildren);
        curChildren = 0;
    }
    if (mProxy)
    {
        delete mProxy;
    }
}

bool DocumentTreeNode::findNode(DocumentTreeNode *node)
{
    DocumentTreeNode *parent = node->parentNode();

    bool hasFound            = false;

    while (parent)
    {
        if (parent == this)
        {
            hasFound = true;
            break;
        }

        parent = parent->parentNode();
    }

    return hasFound;
}

DocumentTreeNode *DocumentTreeNode::nextSibling()
{
    DocumentTreeNode *parent      = this->parentNode();
    DocumentTreeNode *nextSibling = NULL;

    int myIndex                   = parent->children().indexOf(this);
    int indexOfNextSibling        = myIndex + 1;

    if (indexOfNextSibling < parent->children().size())
    {
        nextSibling = parent->children().at(indexOfNextSibling);
    }

    return nextSibling;
}

DocumentTreeNode *DocumentTreeNode::previousSibling()
{
    DocumentTreeNode *parent          = this->parentNode();
    DocumentTreeNode *previousSibling = NULL;

    int myIndex                       = parent->children().indexOf(this);
    int indexOfPreviousSibling        = myIndex - 1;

    if (indexOfPreviousSibling >= 0)
    {
        previousSibling = parent->children().at(indexOfPreviousSibling);
    }

    return previousSibling;
}

DocumentTreeModel::DocumentTreeModel(QObject *parent) : QAbstractItemModel(parent), mRootNode(0)
{
    DocumentTreeNode *rootNode   = new DocumentTreeNode(DocumentTreeNode::Catalog, "root");

    QString trashName            = Settings::trashedDocumentGroupNamePrefix;

    DocumentTreeNode *myDocsNode = new DocumentTreeNode(DocumentTreeNode::Catalog, PersistenceManager::myDocumentsName, tr("My documents"));
    rootNode->addChild(myDocsNode);
    DocumentTreeNode *trashNode = new DocumentTreeNode(DocumentTreeNode::Catalog, trashName, tr("Trash"));
    rootNode->addChild(trashNode);

    setRootNode(rootNode);

    mRoot           = index(0, 0, QModelIndex());
    mMyDocuments    = index(0, 0, QModelIndex());
    mTrash          = index(1, 0, QModelIndex());
    mAscendingOrder = true;
}

QModelIndex DocumentTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!mRootNode || row < 0 || column < 0)
    {
        return QModelIndex();
    }

    // 无效父索引代表根节点；据此定位第 row 个子节点，并把节点指针作为 internalPointer。
    DocumentTreeNode *nodeParent = nodeFromIndex(parent);
    if (!nodeParent || row > nodeParent->children().count() - 1)
    {
        return QModelIndex();
    }

    DocumentTreeNode *requiredNode = nodeParent->children().at(row);
    if (!requiredNode)
    {
        return QModelIndex();
    }

    QModelIndex resIndex = createIndex(row, column, requiredNode);

    return resIndex;
}

QModelIndex DocumentTreeModel::parent(const QModelIndex &child) const
{
    DocumentTreeNode *nodeChild = nodeFromIndex(child);
    if (!nodeChild)
    {
        return QModelIndex();
    }

    DocumentTreeNode *nodeParent = nodeChild->parentNode();
    if (!nodeParent)
    {
        return QModelIndex();
    }

    // 根节点本身不暴露给视图：父节点是根时返回无效索引，使两个固定入口成为顶层。
    DocumentTreeNode *nodePreParent = nodeParent->parentNode();
    if (!nodePreParent)
    {
        return QModelIndex();
    }

    int row              = nodePreParent->children().indexOf(nodeParent);

    QModelIndex resIndex = createIndex(row, 0, nodeParent);

    return resIndex;
}

int DocumentTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
    {
        return 0;
    }

    DocumentTreeNode *nodeParent = nodeFromIndex(parent);
    if (!nodeParent)
    {
        return 0;
    }

    return nodeParent->children().count();
}

int DocumentTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return 3;
}

QVariant DocumentTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }

    DocumentTreeNode *dataNode = nodeFromIndex(index);

    if (!dataNode)
    {
        return QVariant();
    }

    if (role == Qt::DisplayRole)
    {
        if (index.column() == 0)
        {
            return dataNode->displayName();
        }
        else
        {
            DocumentProxy *proxy = proxyForIndex(index);

            QString displayText  = "";

            if (proxy)
            {
                QDateTime d;

                if (index.column() == 1)
                {
                    d = proxy->documentDate();
                }
                else if (index.column() == 2)
                {
                    d = proxy->lastUpdate();
                }

                displayText = d.toString("dd/MM/yyyy hh:mm");
            }

            return displayText;
        }
    }

    if (role == DocumentTreeModel::CreationDate)
    {
        return findNodeDate(dataNode, Settings::documentDate);
    }

    if (role == DocumentTreeModel::UpdateDate)
    {
        return findNodeDate(dataNode, Settings::documentUpdatedAt);
    }

    if (role == Qt::BackgroundRole)
    {
        if (isConstant(index))
        {
            return QBrush(0xD9DFEB);
        }

        if (mHighLighted.isValid() && index == mHighLighted)
        {
            return QBrush(0x6682B5);
        }
    }

    if (role == Qt::UserRole + 1)
    {
        return QVariant::fromValue(dataNode);
    }

    if (index.column() == 0)
    {
        switch (role)
        {
            case (Qt::DecorationRole):
                if (mCurrentNode && mCurrentNode == dataNode)
                {
                    return QIcon(":images/currentDocument.png");
                }
                else
                {
                    if (index == trashIndex())
                    {
                        return QIcon(":images/trash.png");
                    }
                    else if (isConstant(index))
                    {
                        return QIcon(":images/libpalette/ApplicationsCategory.svg");
                    }
                    switch (static_cast<int>(dataNode->nodeType()))
                    {
                        case DocumentTreeNode::Catalog:
                            return QIcon(":images/folder.png");
                        case DocumentTreeNode::Document:
                            return QIcon(":images/toolbar/board.png");
                    }
                }
                break;
            case (Qt::FontRole):
                if (isConstant(index))
                {
                    QFont font;
                    font.setBold(true);
                    return font;
                }
                break;
            case (Qt::ForegroundRole):
                if (isConstant(index))
                {
                    return QColor(Qt::darkGray);
                }
                break;
        }
    }

    return QVariant();
}

bool DocumentTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    switch (role)
    {
        case Qt::EditRole:
            if (!index.isValid() || value.toString().isEmpty())
            {
                return false;
            }
            setNewName(index, value.toString());
            return true;
    }
    return QAbstractItemModel::setData(index, value, role);
}

Qt::ItemFlags DocumentTreeModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags     resultFlags = QAbstractItemModel::flags(index);
    DocumentTreeNode *indexNode   = nodeFromIndex(index);

    if (index.isValid())
    {
        if (!indexNode->isRoot() && !isConstant(index))
        {
            if (!inTrash(index))
            {
                resultFlags |= Qt::ItemIsEditable;
            }
            resultFlags |= Qt::ItemIsDragEnabled;
        }
        if (indexNode->nodeType() == DocumentTreeNode::Catalog)
        {
            resultFlags |= Qt::ItemIsDropEnabled;
        }
    }

    return resultFlags;
}

QDateTime DocumentTreeModel::findNodeDate(DocumentTreeNode *node, QString type) const
{
    if (type == Settings::documentDate)
    {
        return findCatalogCreationDate(node);
    }
    else if (type == Settings::documentUpdatedAt)
    {
        return findCatalogUpdatedDate(node);
    }

    return QDateTime();
}

QDateTime DocumentTreeModel::findCatalogUpdatedDate(DocumentTreeNode *node) const
{
    DocumentProxy *proxy = node->proxyData();

    if (proxy)
    {
        return proxy->metaData(Settings::documentUpdatedAt).toDateTime();
    }
    else if (node->children().size() > 0)
    {
        QDateTime d = findCatalogUpdatedDate(node->children().at(0));

        for (int i = 1; i < node->children().size(); i++)
        {
            QDateTime dChild = findCatalogUpdatedDate(node->children().at(i));

            if (dChild != QDateTime())
            {
                if (mAscendingOrder)
                {
                    d = qMin(d, dChild);
                }
                else
                {
                    d = qMax(d, dChild);
                }
            }
        }

        return d;
    }

    return QDateTime();
}

QDateTime DocumentTreeModel::findCatalogCreationDate(DocumentTreeNode *node) const
{
    DocumentProxy *proxy = node->proxyData();

    if (proxy)
    {
        return proxy->metaData(Settings::documentDate).toDateTime();
    }
    else if (node->children().size() > 0)
    {
        QDateTime d = findCatalogCreationDate(node->children().at(0));

        for (int i = 1; i < node->children().size(); i++)
        {
            QDateTime dChild = findCatalogCreationDate(node->children().at(i));

            if (dChild != QDateTime())
            {
                if (mAscendingOrder)
                {
                    d = qMin(d, dChild);
                }
                else
                {
                    d = qMax(d, dChild);
                }
            }
        }

        return d;
    }

    return QDateTime();
}

QStringList DocumentTreeModel::mimeTypes() const
{
    QStringList types;
    types << "text/uri-list" << "image/png" << "image/tiff" << "image/gif" << "image/jpeg";
    return types;
}

QMimeData *DocumentTreeModel::mimeData(const QModelIndexList &indexes) const
{
    DocumentTreeMimeData *mimeData = new DocumentTreeMimeData();
    QList<QModelIndex>    indexList;
    QList<QUrl>           urlList;

    foreach (QModelIndex index, indexes)
    {
        if (index.isValid())
        {
            indexList.append(index);
            urlList.append(QUrl());
        }
    }

    mimeData->setUrls(urlList);
    mimeData->setIndexes(indexList);

    return mimeData;
}

bool DocumentTreeModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    if (action == Qt::IgnoreAction)
    {
        return false;
    }

    if (data->hasFormat(Application::mimeTypeBoardPage))
    {
        DocumentTreeNode *curNode        = nodeFromIndex(index(row - 1, column, parent));
        DocumentProxy    *targetDocProxy = curNode->proxyData();
        const MimeData   *mimeValue      = qobject_cast<const MimeData *>(data);
        if (!targetDocProxy || !mimeValue || !mimeValue->items().count())
        {
            qDebug() << "an error ocured while parsing " << Application::mimeTypeBoardPage;
            return false;
        }

        int total = mimeValue->items().size();

        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        foreach (MimeDataItem sourceItem, mimeValue->items())
        {
            DocumentProxy *fromProxy = sourceItem.documentProxy();
            int            fromIndex = sourceItem.sceneIndex();
            int            toIndex   = targetDocProxy->pageCount();

            PersistenceManager::persistenceManager()->copyDocumentScene(fromProxy, fromIndex, targetDocProxy, toIndex);
        }

        QApplication::restoreOverrideCursor();

        Application::applicationController->showMessage(tr("%1 pages copied", "", total).arg(total), false);

        return true;
    }

    const DocumentTreeMimeData *mimeData = qobject_cast<const DocumentTreeMimeData *>(data);
    if (!mimeData)
    {
        qDebug() << "Incorrect mimeData, only internal one supported";
        return false;
    }

    if (!parent.isValid())
    {
        return false;
    }

    DocumentTreeNode *newParentNode = nodeFromIndex(parent);

    if (!newParentNode)
    {
        qDebug() << "incorrect incoming parent node;";
        return false;
    }

    QList<QModelIndex> incomingIndexes = mimeData->indexes();

    foreach (QModelIndex curIndex, incomingIndexes)
    {
        if (curIndex.column() == 0)
        {
            QModelIndex clonedTopLevel = copyIndexToNewParent(curIndex, parent, action == Qt::MoveAction ? aReference : aContentCopy);
            if (nodeFromIndex(curIndex) == mCurrentNode && action == Qt::MoveAction)
            {
                emit currentIndexMoved(clonedTopLevel, curIndex);
            }
        }
    }

    Q_UNUSED(action)
    Q_UNUSED(row)
    Q_UNUSED(column)
    Q_UNUSED(parent)

    return true;
}

bool DocumentTreeModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (row < 0 || row + count > rowCount(parent))
    {
        return false;
    }

    beginRemoveRows(parent, row, row + count - 1);

    DocumentTreeNode *parentNode = nodeFromIndex(parent);
    for (int i = row; i < row + count; i++)
    {
        DocumentTreeNode *curChildNode = parentNode->children().at(i);
        // QModelIndex::sibling() 取的是同一个父节点下的另一行，不是"这个节点的第 i 个孩子"，
        // 清空子树必须走 index(i, 0, parent)。
        QModelIndex curChildIndex      = index(i, 0, parent);
        if (curChildNode)
        {
            // 先自底向上清空子树，保证每个被删节点都在模型的 begin/endRemoveRows 范围内；
            // 注意每删一个子节点，剩余兄弟的行号会前移。
            if (rowCount(curChildIndex))
            {
                while (rowCount(curChildIndex))
                {
                    removeRows(0, 1, curChildIndex);
                }
            }

            // mCurrentNode 指着正被删掉的这一棵子树时必须清干净：节点析构会 delete 掉它，
            // 留着的话之后任何一次 currentIndex() 都会拿悬垂指针去走父链。
            if (mCurrentNode && (curChildNode == mCurrentNode || curChildNode->findNode(mCurrentNode)))
            {
                mCurrentNode = nullptr;
            }
        }
        mNewDocuments.removeAll(curChildNode->proxyData());
        parentNode->removeChild(i);
    }

    endRemoveRows();
    return true;
}

bool DocumentTreeModel::containsDocuments(const QModelIndex &index)
{
    for (int i = 0; i < rowCount(index); i++)
    {
        QModelIndex child = this->index(i, 0, index);
        if (isCatalog(child))
        {
            if (containsDocuments(child))
            {
                return true;
            }
        }
        else if (isDocument(child))
        {
            return true;
        }
        else
        {
            qDebug() << "Who the hell are you ?";
        }
    }

    return false;
}

QModelIndex DocumentTreeModel::indexForNode(DocumentTreeNode *pNode) const
{
    if (pNode == 0)
    {
        return QModelIndex();
    }

    return pIndexForNode(QModelIndex(), pNode);
}

QPersistentModelIndex DocumentTreeModel::persistentIndexForNode(DocumentTreeNode *pNode)
{
    return QPersistentModelIndex(indexForNode(pNode));
}

DocumentTreeNode *DocumentTreeModel::findProxy(DocumentProxy *pSearch, DocumentTreeNode *pParent) const
{
    foreach (DocumentTreeNode *curNode, pParent->children())
    {
        if (DocumentTreeNode::Catalog != curNode->nodeType())
        {
            if (curNode->proxyData()->theSameDocument(pSearch))
            {
                return curNode;
            }
        }
        else if (curNode->children().count())
        {
            DocumentTreeNode *recursiveDescendResult = findProxy(pSearch, curNode);
            if (recursiveDescendResult)
            {
                return findProxy(pSearch, curNode);
            }
        }
    }

    return 0;
}

QModelIndex DocumentTreeModel::pIndexForNode(const QModelIndex &parent, DocumentTreeNode *pNode) const
{
    for (int i = 0; i < rowCount(parent); i++)
    {
        QModelIndex curIndex = index(i, 0, parent);
        if (curIndex.internalPointer() == pNode)
        {
            return curIndex;
        }
        else if (rowCount(curIndex) > 0)
        {
            QModelIndex recursiveDescendIndex = pIndexForNode(curIndex, pNode);
            if (recursiveDescendIndex.isValid())
            {
                return recursiveDescendIndex;
            }
        }
    }
    return QModelIndex();
}

void DocumentTreeModel::copyIndexToNewParent(const QModelIndexList &list, const QModelIndex &newParent, eCopyMode pMode)
{
    for (int i = 0; i < list.size(); i++)
    {
        if (list.at(i).column() == 0)
        {
            copyIndexToNewParent(list.at(i), newParent, pMode);
        }
    }
}

QPersistentModelIndex DocumentTreeModel::copyIndexToNewParent(const QModelIndex &source, const QModelIndex &newParent, eCopyMode pMode)
{
    DocumentTreeNode *nodeParent = nodeFromIndex(newParent);
    DocumentTreeNode *nodeSource = nodeFromIndex(source);

    if (!nodeParent || !nodeSource)
    {
        return QModelIndex();
    }

    DocumentTreeNode *clonedNodeSource = 0;
    switch (static_cast<int>(pMode))
    {
        case aReference:
            // 按引用复制：只克隆节点与代理的元数据，底层页面数据仍指向同一目录。
            clonedNodeSource = nodeSource->clone();
            if (mNewDocuments.contains(nodeSource->proxyData()))
            { // 新文档的引用关系也要一并迁移
                mNewDocuments << clonedNodeSource->proxyData();

                PersistenceManager::persistenceManager()->reassignDocProxy(clonedNodeSource->proxyData(), nodeSource->proxyData());
            }
            break;

        case aContentCopy:
            // 按内容复制：让持久化层真正复制一份文档目录，再给新代理盖新的更新时间。
            DocumentProxy *duplicatedProxy = 0;
            if (nodeSource->nodeType() == DocumentTreeNode::Document && nodeSource->proxyData())
            {
                duplicatedProxy = PersistenceManager::persistenceManager()->duplicateDocument(nodeSource->proxyData());
                duplicatedProxy->setMetaData(Settings::documentUpdatedAt, StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
                persistDocumentMetadata(duplicatedProxy);
            }
            clonedNodeSource = new DocumentTreeNode(nodeSource->nodeType(), nodeSource->nodeName(), nodeSource->displayName(), duplicatedProxy);
            break;
    }

    // 复制到同一层级或按内容复制时，同级可能出现重名，需要补数字后缀；
    // 移入回收站则保留原名，避免用户在回收站里看到一堆“副本”。
    QString newName = clonedNodeSource->nodeName();
    if ((source.parent() != newParent || pMode != aReference) && (newParent != trashIndex() || !inTrash(newParent)))
    {
        newName = adjustNameForParentIndex(newName, newParent);
        clonedNodeSource->setNodeName(newName);
    }

    if (clonedNodeSource->proxyData())
    {
        clonedNodeSource->proxyData()->setMetaData(Settings::documentGroupName, virtualPathForIndex(newParent));
        clonedNodeSource->proxyData()->setMetaData(Settings::documentName, newName);
        PersistenceManager::persistenceManager()->persistDocumentMetadata(clonedNodeSource->proxyData());
    }

    addNode(clonedNodeSource, newParent);

    QPersistentModelIndex newParentIndex = createIndex(rowCount(newParent), 0, clonedNodeSource);

    // 目录复制要递归处理整棵子树，让每个后代都按同一模式重新挂载。
    if (rowCount(source))
    {
        for (int i = 0; i < rowCount(source); i++)
        {
            QModelIndex curNewParentIndexChild = index(i, 0, source);
            copyIndexToNewParent(curNewParentIndexChild, newParentIndex, pMode);
        }
    }

    return newParentIndex;
}

void DocumentTreeModel::moveIndexes(const QModelIndexList &source, const QModelIndex &destination)
{
    QModelIndex destinationParent = destination;
    while (!isCatalog(destinationParent))
    {
        destinationParent = destinationParent.parent();
    }

    DocumentTreeNode *newParentNode = nodeFromIndex(destinationParent);

    for (int i = 0; i < source.size(); i++)
    {
        DocumentTreeNode *sourceNode = nodeFromIndex(source.at(i));
        QModelIndex       s          = source.at(i);

        if (newParentNode == sourceNode->parentNode() || sourceNode->findNode(newParentNode))
        {
            continue;
        }

        if (s.internalId() != destinationParent.internalId())
        {
            int sourceIndex = source.at(i).row();
            int destIndex   = positionForParent(sourceNode, newParentNode);

            // 每个 beginMoveRows 必须有自己的 endMoveRows：多选移动时只在最外面 end 一次，
            // 前面的 rowsAboutToBeMoved 就没有对应的 rowsMoved，代理模型与持久索引会错位。
            beginMoveRows(s.parent(), sourceIndex, sourceIndex, destinationParent, destIndex);
            fixNodeName(s, destinationParent);
            sourceNode->parentNode()->moveChild(sourceNode, destIndex, newParentNode);
            updateIndexNameBindings(sourceNode);
            endMoveRows();
        }
    }
}

void DocumentTreeModel::moveIndex(const QModelIndex &what, const QModelIndex &destination)
{
    QModelIndexList list;
    list.push_back(what);
    moveIndexes(list, destination);
}

void DocumentTreeModel::setCurrentDocument(DocumentProxy *pDocument)
{
    DocumentTreeNode *testCurNode = findProxy(pDocument, mRootNode);

    if (testCurNode)
    {
        setCurrentNode(testCurNode);
    }
}

QModelIndex DocumentTreeModel::indexForProxy(DocumentProxy *pSearch) const
{
    DocumentTreeNode *proxy = findProxy(pSearch, mRootNode);
    if (!proxy)
    {
        return QModelIndex();
    }

    return indexForNode(proxy);
}

void DocumentTreeModel::setRootNode(DocumentTreeNode *pRoot)
{
    // 所有权随指针转移；这里不调用 reset()，因为构造期视图尚未挂到模型上。
    mRootNode = pRoot;
}

DocumentProxy *DocumentTreeModel::proxyForIndex(const QModelIndex &pIndex) const
{
    DocumentTreeNode *node = nodeFromIndex(pIndex);
    if (!node)
    {
        return 0;
    }

    return node->proxyData();
}

QString DocumentTreeModel::virtualDirForIndex(const QModelIndex &pIndex) const
{
    QString           result;
    DocumentTreeNode *curNode = nodeFromIndex(pIndex);
    // 与 DocumentTreeNode::dirPathInHierarchy 一样，跳过顶层固定入口，只保留用户目录层级。
    while (curNode->parentNode() && !curNode->isTopLevel())
    {
        result.prepend(curNode->parentNode()->nodeName() + "/");
        curNode = curNode->parentNode();
    }

    if (result.endsWith("/"))
    {
        result.truncate(result.size() - 1);
    }

    return result;
}

QString DocumentTreeModel::virtualPathForIndex(const QModelIndex &pIndex) const
{
    DocumentTreeNode *curNode = nodeFromIndex(pIndex);
    Q_ASSERT(curNode);

    return virtualDirForIndex(pIndex) + "/" + curNode->nodeName();
}

QStringList DocumentTreeModel::nodeNameList(const QModelIndex &pIndex) const
{
    QStringList result;

    DocumentTreeNode *catalog = nodeFromIndex(pIndex);
    if (catalog->nodeType() != DocumentTreeNode::Catalog)
    {
        return QStringList();
    }

    foreach (DocumentTreeNode *curNode, catalog->children())
    {
        result << curNode->nodeName();
    }

    return result;
}

bool DocumentTreeModel::newNodeAllowed(const QModelIndex &pSelectedIndex) const
{
    if (!pSelectedIndex.isValid())
    {
        return false;
    }

    if (inTrash(pSelectedIndex) || pSelectedIndex == trashIndex())
    {
        return false;
    }

    return true;
}

QModelIndex DocumentTreeModel::goTo(const QString &dir)
{
    QStringList pathList = dir.split("/", Qt::SkipEmptyParts);

    if (pathList.isEmpty())
    {
        return myDocumentsIndex();
    }

    if (pathList.first() != PersistenceManager::myDocumentsName && pathList.first() != Settings::trashedDocumentGroupNamePrefix &&
        pathList.first() != PersistenceManager::modelsName)
    {
        pathList.prepend(PersistenceManager::myDocumentsName);
    }

    QModelIndex parentIndex;

    bool searchingNode = true;
    while (!pathList.isEmpty())
    {
        QString curLevelName = pathList.takeFirst();
        if (searchingNode)
        {
            searchingNode = false;
            for (int i = 0; i < rowCount(parentIndex); ++i)
            {
                QModelIndex curChildIndex = index(i, 0, parentIndex);
                if (nodeFromIndex(curChildIndex)->nodeName() == curLevelName)
                {
                    searchingNode = true;
                    parentIndex   = curChildIndex;
                    break;
                }
            }
        }

        if (!searchingNode)
        {
            DocumentTreeNode *newChild = new DocumentTreeNode(DocumentTreeNode::Catalog, curLevelName);
            parentIndex                = addNode(newChild, parentIndex);
        }
    }

    return parentIndex;
}

bool DocumentTreeModel::inTrash(const QModelIndex &index) const
{
    return isDescendantOf(index, trashIndex());
}

bool DocumentTreeModel::inMyDocuments(const QModelIndex &index) const
{
    return isDescendantOf(index, myDocumentsIndex());
}

void DocumentTreeModel::addDocument(DocumentProxy *pProxyData, const QModelIndex &pParent)
{
    if (!pProxyData)
    {
        return;
    }
    QString docName      = pProxyData->metaData(Settings::documentName).toString();
    QString docGroupName = pProxyData->metaData(Settings::documentGroupName).toString();

    if (docName.isEmpty())
    {
        return;
    }

    QModelIndex       lParent  = pParent;
    DocumentTreeNode *freeNode = new DocumentTreeNode(DocumentTreeNode::Document, docName, QString(), pProxyData);
    if (!pParent.isValid())
    {
        lParent = goTo(docGroupName);
    }

    addNode(freeNode, lParent);
}

void DocumentTreeModel::addNewDocument(DocumentProxy *pProxyData, const QModelIndex &pParent)
{
    addDocument(pProxyData, pParent);
    mNewDocuments << pProxyData;
}

QModelIndex DocumentTreeModel::addCatalog(const QString &pName, const QModelIndex &pParent)
{
    if (pName.isEmpty() || !pParent.isValid())
    {
        return QModelIndex();
    }

    DocumentTreeNode *catalogNode = new DocumentTreeNode(DocumentTreeNode::Catalog, pName);
    return addNode(catalogNode, pParent);
}

void DocumentTreeModel::setNewName(const QModelIndex &index, const QString &newName)
{
    if (!index.isValid())
    {
        return;
    }

    DocumentTreeNode *indexNode = nodeFromIndex(index);

    // 目录重命名时，需要把新的虚拟路径同步给所有子孙。这里借用内部分隔符
    // “+!##s”把“目录路径 + 子项名”拼成一个字符串传给递归调用，避免为此增加额外参数；
    // 用户输入中的 '/' 会先被替换成 '-'，防止目录名破坏层级。
    QString magicSeparator      = "+!##s";
    if (isCatalog(index))
    {
        QString fullNewName = newName;
        fullNewName.replace('/', '-');
        if (!newName.contains(magicSeparator))
        {
            indexNode->setNodeName(fullNewName);
            QString virtualDir = virtualDirForIndex(index);
            fullNewName.prepend(virtualDir.isEmpty() ? "" : virtualDir + magicSeparator);
        }
        for (int i = 0; i < rowCount(index); i++)
        {
            QModelIndex subIndex = this->index(i, 0, index);
            setNewName(subIndex, fullNewName + magicSeparator + subIndex.data().toString());
        }
    }
    else if (isDocument(index))
    {
        Q_ASSERT(indexNode->proxyData());

        int prefixIndex = newName.lastIndexOf(magicSeparator);
        if (prefixIndex != -1)
        {
            // 带分隔符说明这是目录重命名递归下来的结果：只更新文档的分组路径，
            // 文档自身名称由分隔符后的最后一段保持不变。
            QString newDocumentGroupName = newName.left(prefixIndex).replace(magicSeparator, "/");
            indexNode->proxyData()->setMetaData(Settings::documentGroupName, newDocumentGroupName);
        }
        else
        {
            indexNode->setNodeName(newName);
            indexNode->proxyData()->setMetaData(Settings::documentName, newName);
        }

        PersistenceManager::persistenceManager()->persistDocumentMetadata(indexNode->proxyData());
    }
}

QString DocumentTreeModel::adjustNameForParentIndex(const QString &pName, const QModelIndex &pIndex)
{
    int         i            = 0;
    QString     newName      = pName;
    QStringList siblingNames = nodeNameList(pIndex);
    while (siblingNames.contains(newName))
    {
        newName = pName + " " + QVariant(++i).toString();
    }

    return newName;
}

void DocumentTreeModel::fixNodeName(const QModelIndex &source, const QModelIndex &dest)
{
    // 跨层移动后目标层可能已有同名条目，需要补后缀；移入回收站时保留原名。
    DocumentTreeNode *srcNode = nodeFromIndex(source);
    Q_ASSERT(srcNode);

    QString newName = srcNode->nodeName();
    if (source.parent() != dest && (dest != trashIndex() || !inTrash(dest)))
    {
        newName = adjustNameForParentIndex(newName, dest);
        srcNode->setNodeName(newName);
        nodeFromIndex(source)->setNodeName(newName);
    }
}

void DocumentTreeModel::updateIndexNameBindings(DocumentTreeNode *nd)
{
    Q_ASSERT(nd);

    if (nd->nodeType() == DocumentTreeNode::Catalog)
    {
        foreach (DocumentTreeNode *lnd, nd->children())
        {
            updateIndexNameBindings(lnd);
        }
    }
    else if (nd->proxyData())
    {
        nd->proxyData()->setMetaData(Settings::documentGroupName, virtualPathForIndex(indexForNode(nd->parentNode())));
        nd->proxyData()->setMetaData(Settings::documentName, nd->nodeName());
        nd->proxyData()->setMetaData(Settings::documentUpdatedAt, StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
        PersistenceManager::persistenceManager()->persistDocumentMetadata(nd->proxyData());
    }
}

bool DocumentTreeModel::isDescendantOf(const QModelIndex &pPossibleDescendant, const QModelIndex &pPossibleAncestor) const
{
    if (!pPossibleDescendant.isValid())
    {
        return false;
    }

    QModelIndex ancestor = pPossibleDescendant;
    while (ancestor.parent().isValid())
    {
        ancestor = ancestor.parent();
        if (ancestor == pPossibleAncestor)
        {
            return true;
        }
    }

    return false;
}

QModelIndex DocumentTreeModel::addNode(DocumentTreeNode *pFreeNode, const QModelIndex &pParent, eAddItemMode pMode)
{
    DocumentTreeNode *tstParent = nodeFromIndex(pParent);

    if (!pParent.isValid() || tstParent->nodeType() != DocumentTreeNode::Catalog)
    {
        return QModelIndex();
    }
    int newIndex = pMode == aDetectPosition ? positionForParent(pFreeNode, tstParent) : tstParent->children().size();
    // 视图正在监听模型，插入必须包在 begin/endInsertRows 之间，
    // 否则代理模型无法维护行映射，后续索引会全部错位。
    beginInsertRows(pParent, newIndex, newIndex);
    tstParent->insertChild(newIndex, pFreeNode);
    endInsertRows();

    return createIndex(newIndex, 0, pFreeNode);
}

int DocumentTreeModel::positionForParent(DocumentTreeNode *pFreeNode, DocumentTreeNode *pParentNode)
{
    Q_ASSERT(pFreeNode);
    Q_ASSERT(pParentNode);
    Q_ASSERT(pParentNode->nodeType() == DocumentTreeNode::Catalog);

    int c          = -1;
    int childCount = pParentNode->children().count();
    // 找到第一个“比新节点大”的现有子节点位置；都不大于则追加到末尾。
    while (c <= childCount)
    {
        if (++c == childCount || lessThan(pFreeNode, pParentNode->children().at(c)))
        {
            break;
        }
    }
    return c == -1 ? childCount : c;
}

DocumentTreeNode *DocumentTreeModel::nodeFromIndex(const QModelIndex &pIndex) const
{
    if (pIndex.isValid())
    {
        return static_cast<DocumentTreeNode *>(pIndex.internalPointer());
    }
    else
    {
        return mRootNode;
    }
}

bool DocumentTreeModel::nodeLessThan(const DocumentTreeNode *firstIndex, const DocumentTreeNode *secondIndex)
{
    return firstIndex->nodeName() < secondIndex->nodeName();
}

DocumentTreeModel::~DocumentTreeModel()
{
    // 整棵树（含各节点的 DocumentProxy）由根节点递归释放。
    delete mRootNode;
}
