#include "model/DocumentController.h"
#include <QRegExp>

#include <QtCore>
#include <QtWidgets>

#include "utils/FileSystemUtils.h"
#include "utils/StringUtils.h"
#include "utils/Version.h"
#include "platform/PlatformUtils.h"

#include "core/Application.h"
#include "core/PersistenceManager.h"
#include "core/DocumentManager.h"
#include "core/ApplicationController.h"
#include "settings/Settings.h"
#include "settings/Setting.h"
#include "core/MimeData.h"
#include "core/ForeignObjectsHandler.h"

#include "content/ExportPDF.h"
#include "content/ThumbnailAdaptor.h"

#include "content/MetadataDcSubsetAdaptor.h"

#include "whiteboard/BoardController.h"
#include "whiteboard/BoardPaletteManager.h"
#include "whiteboard/DrawingController.h"


#include "ui/ThumbnailView.h"
#include "ui/MousePressFilter.h"
#include "ui/MessageWindow.h"
#include "ui/MainWindow.h"
#include "ui/DocumentThumbnailWidget.h"
#include "ui/DocumentsWidget.h"
#include "ui/DocumentToolsPalette.h"

#include "whiteboard/GraphicsScene.h"
#include "whiteboard/GraphicsSvgItem.h"
#include "whiteboard/GraphicsPixmapItem.h"

#include "model/DocumentProxy.h"


static bool lessThan(DocumentTreeNode *lValue, DocumentTreeNode *rValue) {
    if (lValue->nodeType() == DocumentTreeNode::Catalog) {
        if (rValue->nodeType() == DocumentTreeNode::Catalog) {
            return lValue->nodeName() < rValue->nodeName();
        } else {
            return true;
        }
    } else {
        if (rValue->nodeType() == DocumentTreeNode::Catalog) {
            return false;
        } else {
            Q_ASSERT(lValue->proxyData());
            Q_ASSERT(rValue->proxyData());

            //return lValue->nodeName() < rValue->nodeName();

            QDateTime lTime = lValue->proxyData()->documentDate();
            QDateTime rTime = rValue->proxyData()->documentDate();

            return lTime > rTime;
        }
    }

    return false;
}


DocumentReplaceDialog::DocumentReplaceDialog(const QString &pIncommingName, const QStringList &pFileList,
                                             QWidget *parent, Qt::WindowFlags pFlags)
    : QDialog(parent, pFlags),
      mFileNameList(pFileList),
      mIncommingName(pIncommingName),
      acceptText(tr("Accept")),
      replaceText(tr("Replace")),
      cancelText(tr("Cancel")),
      mLabelText(0) {
    this->setStyleSheet("background:white;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QVBoxLayout *labelLayout = new QVBoxLayout();

    mLabelText = new QLabel(labelTextWithName(pIncommingName), this);
    mLineEdit = new QLineEdit(this);
    mLineEdit->setText(pIncommingName);
    mLineEdit->selectedText();

    labelLayout->addWidget(mLabelText);
    labelLayout->addWidget(mLineEdit);

    QHBoxLayout *buttonLayout = new QHBoxLayout();

    acceptButton = new QPushButton(acceptText, this);
    QPushButton *cancelButton = new QPushButton(cancelText, this);
    buttonLayout->addWidget(acceptButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(labelLayout);
    mainLayout->addLayout(buttonLayout);

    acceptButton->setEnabled(false);

    connect(acceptButton, &QPushButton::clicked, this, &DocumentReplaceDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &DocumentReplaceDialog::reject);
    connect(mLineEdit, &QLineEdit::textEdited, this, &DocumentReplaceDialog::reactOnTextChanged);

    reactOnTextChanged(mIncommingName);
}

void DocumentReplaceDialog::setRegexp(const QRegularExpression &pRegExp) {
    mValidator->setRegularExpression(pRegExp);
}
bool DocumentReplaceDialog::validString(const QString &pStr) {
    Q_UNUSED(pStr);
    return mLineEdit->hasAcceptableInput();
}

void DocumentReplaceDialog::setFileNameAndList(const QString &fileName, const QStringList &pLst) {
    mFileNameList = pLst;
    mIncommingName = fileName;
    mLabelText->setText(labelTextWithName(fileName));
    mLineEdit->setText(fileName);
    mLineEdit->selectAll();
    mLineEdit->selectedText();
}

QString DocumentReplaceDialog::labelTextWithName(const QString &documentName) const {
    return tr("The name %1 is allready used.\nKeeping this name will replace the document.\nProviding a new "
              "name will create a new document.")
        .arg(documentName);
}

void DocumentReplaceDialog::accept() {
    QDialog::accept();
}
void DocumentReplaceDialog::reject() {
    mLineEdit->clear();
    emit closeDialog();

    QDialog::reject();
}

void DocumentReplaceDialog::reactOnTextChanged(const QString &pStr) {
    //     if !mFileNameList.contains(pStr.trimmed(), Qt::CaseSensitive)

    if (!validString(pStr)) {
        acceptButton->setEnabled(false);
        mLineEdit->setStyleSheet("background:#FFB3C8;");
        acceptButton->setEnabled(false);

    } else if (mFileNameList.contains(pStr.trimmed(), Qt::CaseSensitive)) {
        acceptButton->setEnabled(true);
        mLineEdit->setStyleSheet("background:#FFB3C8;");
        acceptButton->setText(replaceText);

    } else {
        acceptButton->setEnabled(true);
        mLineEdit->setStyleSheet("background:white;");
        acceptButton->setText(acceptText);
    }
}

DocumentTreeNode::DocumentTreeNode(Type pType, const QString &pName, const QString &pDisplayName,
                                   DocumentProxy *pProxy)
    : mType(pType), mName(pName), mDisplayName(pDisplayName), mProxy(pProxy) {
    if (pDisplayName.isEmpty()) {
        mDisplayName = mName;
    }
    mParent = 0;
}

void DocumentTreeNode::addChild(DocumentTreeNode *pChild) {
    if (pChild) {
        mChildren += pChild;
        pChild->mParent = this;
    }
}

void DocumentTreeNode::insertChild(int pIndex, DocumentTreeNode *pChild) {
    if (pChild) {
        mChildren.insert(pIndex, pChild);
        pChild->mParent = this;
    }
}

void DocumentTreeNode::moveChild(DocumentTreeNode *child, int index, DocumentTreeNode *newParent) {
    int childIndex = mChildren.indexOf(child);
    if (childIndex == -1) {
        return;
    }

    newParent->insertChild(index, child);
    mChildren.removeAt(childIndex);
}

void DocumentTreeNode::removeChild(int index) {
    if (index < 0 || index > mChildren.count() - 1) {
        return;
    }

    DocumentTreeNode *curChild = mChildren[index];
    while (curChild->mChildren.count()) {
        curChild->removeChild(0);
    }

    mChildren.removeAt(index);
    delete curChild;
}

DocumentTreeNode *DocumentTreeNode::clone() {
    return new DocumentTreeNode(this->mType, this->mName, this->mDisplayName,
                                this->mProxy ? new DocumentProxy(*this->mProxy) : 0);
}

QString DocumentTreeNode::dirPathInHierarchy() {
    QString result;
    DocumentTreeNode *curNode = this;
    //protect the 2nd level items
    while (curNode->parentNode() && !curNode->isTopLevel()) {
        result.prepend(curNode->parentNode()->nodeName() + "/");
        curNode = curNode->parentNode();
    }

    if (result.endsWith("/")) {
        result.truncate(result.count() - 1);
    }

    return result;
}

DocumentTreeNode::~DocumentTreeNode() {
    foreach (DocumentTreeNode *curChildren, mChildren) {
        delete (curChildren);
        curChildren = 0;
    }
    if (mProxy)
        delete mProxy;
}

bool DocumentTreeNode::findNode(DocumentTreeNode *node) {
    DocumentTreeNode *parent = node->parentNode();

    bool hasFound = false;

    while (parent) {
        if (parent == this) {
            hasFound = true;
            break;
        }

        parent = parent->parentNode();
    }

    return hasFound;
}

DocumentTreeNode *DocumentTreeNode::nextSibling() {
    DocumentTreeNode *parent = this->parentNode();
    DocumentTreeNode *nextSibling = NULL;

    int myIndex = parent->children().indexOf(this);
    int indexOfNextSibling = myIndex + 1;

    if (indexOfNextSibling < parent->children().size()) {
        nextSibling = parent->children().at(indexOfNextSibling);
    }

    return nextSibling;
}

DocumentTreeNode *DocumentTreeNode::previousSibling() {
    DocumentTreeNode *parent = this->parentNode();
    DocumentTreeNode *previousSibling = NULL;

    int myIndex = parent->children().indexOf(this);
    int indexOfPreviousSibling = myIndex - 1;

    if (indexOfPreviousSibling >= 0) {
        previousSibling = parent->children().at(indexOfPreviousSibling);
    }

    return previousSibling;
}


DocumentTreeModel::DocumentTreeModel(QObject *parent) : QAbstractItemModel(parent), mRootNode(0) {
    DocumentTreeNode *rootNode = new DocumentTreeNode(DocumentTreeNode::Catalog, "root");

    QString trashName = Settings::trashedDocumentGroupNamePrefix;

    DocumentTreeNode *myDocsNode = new DocumentTreeNode(
        DocumentTreeNode::Catalog, PersistenceManager::myDocumentsName, tr("My documents"));
    rootNode->addChild(myDocsNode);
    //DocumentTreeNode *modelsNode = new DocumentTreeNode(DocumentTreeNode::Catalog, PersistenceManager::modelsName, tr("Models"));
    //rootNode->addChild(modelsNode);
    DocumentTreeNode *trashNode = new DocumentTreeNode(DocumentTreeNode::Catalog, trashName, tr("Trash"));
    rootNode->addChild(trashNode);

    setRootNode(rootNode);

    mRoot = index(0, 0, QModelIndex());
    mMyDocuments = index(0, 0, QModelIndex());
    mTrash = index(1, 0, QModelIndex());
    mAscendingOrder = true;
}

QModelIndex DocumentTreeModel::index(int row, int column, const QModelIndex &parent) const {
    if (!mRootNode || row < 0 || column < 0) {
        return QModelIndex();
    }

    DocumentTreeNode *nodeParent = nodeFromIndex(parent);
    if (!nodeParent || row > nodeParent->children().count() - 1) {
        return QModelIndex();
    }

    DocumentTreeNode *requiredNode = nodeParent->children().at(row);
    if (!requiredNode) {
        return QModelIndex();
    }

    QModelIndex resIndex = createIndex(row, column, requiredNode);

    return resIndex;
}

QModelIndex DocumentTreeModel::parent(const QModelIndex &child) const {
    DocumentTreeNode *nodeChild = nodeFromIndex(child);
    if (!nodeChild) {
        return QModelIndex();
    }

    DocumentTreeNode *nodeParent = nodeChild->parentNode();
    if (!nodeParent) {
        return QModelIndex();
    }

    DocumentTreeNode *nodePreParent = nodeParent->parentNode();
    if (!nodePreParent) {
        return QModelIndex();
    }

    int row = nodePreParent->children().indexOf(nodeParent);

    QModelIndex resIndex = createIndex(row, 0, nodeParent);

    return resIndex;
}

int DocumentTreeModel::rowCount(const QModelIndex &parent) const {
    if (parent.column() > 0) {
        return 0;
    }

    DocumentTreeNode *nodeParent = nodeFromIndex(parent);
    if (!nodeParent) {
        return 0;
    }

    return nodeParent->children().count();
}

int DocumentTreeModel::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)

    return 3;
}

QVariant DocumentTreeModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) {
        return QVariant();
    }

    DocumentTreeNode *dataNode = nodeFromIndex(index);

    if (!dataNode)
        return QVariant();


    if (role == Qt::DisplayRole) {
        if (index.column() == 0) {
            return dataNode->displayName();
        } else {
            DocumentProxy *proxy = proxyForIndex(index);

            QString displayText = "";

            if (proxy) {
                QDateTime d;

                if (index.column() == 1) {
                    d = proxy->documentDate();
                } else if (index.column() == 2) {
                    d = proxy->lastUpdate();
                }

                displayText = d.toString("dd/MM/yyyy hh:mm");
            }

            return displayText;
        }
    }

    if (role == DocumentTreeModel::CreationDate) {
        return findNodeDate(dataNode, Settings::documentDate);
    }

    if (role == DocumentTreeModel::UpdateDate) {
        return findNodeDate(dataNode, Settings::documentUpdatedAt);
    }

    if (role == Qt::BackgroundRole) {
        if (isConstant(index)) {
            return QBrush(0xD9DFEB);
        }

        if (mHighLighted.isValid() && index == mHighLighted) {
            return QBrush(0x6682B5);
        }
    }

    if (role == Qt::UserRole + 1) {
        return QVariant::fromValue(dataNode);
    }

    if (index.column() == 0) {
        switch (role) {
            case (Qt::DecorationRole):
                if (mCurrentNode && mCurrentNode == dataNode) {
                    return QIcon(":images/currentDocument.png");
                } else {
                    if (index == trashIndex()) {
                        return QIcon(":images/trash.png");
                    } else if (isConstant(index)) {
                        return QIcon(":images/libpalette/ApplicationsCategory.svg");
                    }
                    switch (static_cast<int>(dataNode->nodeType())) {
                        case DocumentTreeNode::Catalog:
                            return QIcon(":images/folder.png");
                        case DocumentTreeNode::Document:
                            return QIcon(":images/toolbar/board.png");
                    }
                }
                break;
            case (Qt::FontRole):
                if (isConstant(index)) {
                    QFont font;
                    font.setBold(true);
                    return font;
                }
                break;
            case (Qt::ForegroundRole):
                if (isConstant(index)) {
                    return QColor(Qt::darkGray);
                }
                break;
        }
    }

    return QVariant();
}

bool DocumentTreeModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    switch (role) {
        case Qt::EditRole:
            if (!index.isValid() || value.toString().isEmpty()) {
                return false;
            }
            setNewName(index, value.toString());
            return true;
    }
    return QAbstractItemModel::setData(index, value, role);
}

Qt::ItemFlags DocumentTreeModel::flags(const QModelIndex &index) const {
    Qt::ItemFlags resultFlags = QAbstractItemModel::flags(index);
    DocumentTreeNode *indexNode = nodeFromIndex(index);

    if (index.isValid()) {
        if (!indexNode->isRoot() && !isConstant(index)) {
            if (!inTrash(index)) {
                resultFlags |= Qt::ItemIsEditable;
            }
            resultFlags |= Qt::ItemIsDragEnabled;
        }
        if (indexNode->nodeType() == DocumentTreeNode::Catalog) {
            resultFlags |= Qt::ItemIsDropEnabled;
        }
    }

    return resultFlags;
}

QDateTime DocumentTreeModel::findNodeDate(DocumentTreeNode *node, QString type) const {
    if (type == Settings::documentDate) {
        return findCatalogCreationDate(node);
    } else if (type == Settings::documentUpdatedAt) {
        return findCatalogUpdatedDate(node);
    }

    return QDateTime();
}

QDateTime DocumentTreeModel::findCatalogUpdatedDate(DocumentTreeNode *node) const {
    DocumentProxy *proxy = node->proxyData();

    if (proxy) {
        return proxy->metaData(Settings::documentUpdatedAt).toDateTime();
    } else if (node->children().size() > 0) {
        QDateTime d = findCatalogUpdatedDate(node->children().at(0));

        for (int i = 1; i < node->children().size(); i++) {
            QDateTime dChild = findCatalogUpdatedDate(node->children().at(i));

            if (dChild != QDateTime()) {
                if (mAscendingOrder) {
                    d = qMin(d, dChild);
                } else {
                    d = qMax(d, dChild);
                }
            }
        }

        return d;
    }

    return QDateTime();
}

QDateTime DocumentTreeModel::findCatalogCreationDate(DocumentTreeNode *node) const {
    DocumentProxy *proxy = node->proxyData();

    if (proxy) {
        return proxy->metaData(Settings::documentDate).toDateTime();
    } else if (node->children().size() > 0) {
        QDateTime d = findCatalogCreationDate(node->children().at(0));

        for (int i = 1; i < node->children().size(); i++) {
            QDateTime dChild = findCatalogCreationDate(node->children().at(i));

            if (dChild != QDateTime()) {
                if (mAscendingOrder) {
                    d = qMin(d, dChild);
                } else {
                    d = qMax(d, dChild);
                }
            }
        }

        return d;
    }

    return QDateTime();
}

QStringList DocumentTreeModel::mimeTypes() const {
    QStringList types;
    types << "text/uri-list" << "image/png" << "image/tiff" << "image/gif" << "image/jpeg";
    return types;
}

QMimeData *DocumentTreeModel::mimeData(const QModelIndexList &indexes) const {
    DocumentTreeMimeData *mimeData = new DocumentTreeMimeData();
    QList<QModelIndex> indexList;
    QList<QUrl> urlList;

    foreach (QModelIndex index, indexes) {
        if (index.isValid()) {
            indexList.append(index);
            urlList.append(QUrl());
        }
    }

    mimeData->setUrls(urlList);
    mimeData->setIndexes(indexList);

    return mimeData;
}

bool DocumentTreeModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column,
                                     const QModelIndex &parent) {
    if (action == Qt::IgnoreAction) {
        return false;
    }

    if (data->hasFormat(Application::mimeTypeBoardPage)) {
        DocumentTreeNode *curNode = nodeFromIndex(index(row - 1, column, parent));
        DocumentProxy *targetDocProxy = curNode->proxyData();
        const MimeData *mimeValue = qobject_cast<const MimeData *>(data);
        if (!targetDocProxy || !mimeValue || !mimeValue->items().count()) {
            qDebug() << "an error ocured while parsing " << Application::mimeTypeBoardPage;
            return false;
        }

        //        int count = 0;
        int total = mimeValue->items().size();

        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        foreach (MimeDataItem sourceItem, mimeValue->items()) {
            DocumentProxy *fromProxy = sourceItem.documentProxy();
            int fromIndex = sourceItem.sceneIndex();
            int toIndex = targetDocProxy->pageCount();

            PersistenceManager::persistenceManager()->copyDocumentScene(fromProxy, fromIndex, targetDocProxy,
                                                                        toIndex);
        }

        QApplication::restoreOverrideCursor();

        Application::applicationController->showMessage(tr("%1 pages copied", "", total).arg(total), false);

        return true;
    }

    const DocumentTreeMimeData *mimeData = qobject_cast<const DocumentTreeMimeData *>(data);
    if (!mimeData) {
        qDebug() << "Incorrect mimeData, only internal one supported";
        return false;
    }

    if (!parent.isValid()) {
        return false;
    }

    DocumentTreeNode *newParentNode = nodeFromIndex(parent);

    if (!newParentNode) {
        qDebug() << "incorrect incoming parent node;";
        return false;
    }

    QList<QModelIndex> incomingIndexes = mimeData->indexes();

    foreach (QModelIndex curIndex, incomingIndexes) {
        if (curIndex.column() == 0) {
            QModelIndex clonedTopLevel =
                copyIndexToNewParent(curIndex, parent, action == Qt::MoveAction ? aReference : aContentCopy);
            if (nodeFromIndex(curIndex) == mCurrentNode && action == Qt::MoveAction) {
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

bool DocumentTreeModel::removeRows(int row, int count, const QModelIndex &parent) {
    if (row < 0 || row + count > rowCount(parent))
        return false;

    beginRemoveRows(parent, row, row + count - 1);

    DocumentTreeNode *parentNode = nodeFromIndex(parent);
    for (int i = row; i < row + count; i++) {
        DocumentTreeNode *curChildNode = parentNode->children().at(i);
        QModelIndex curChildIndex = parent.sibling(i, 0);
        if (curChildNode) {
            if (rowCount(curChildIndex)) {
                while (rowCount(curChildIndex)) {
                    removeRows(0, 1, curChildIndex);
                }
            }
        }
        mNewDocuments.removeAll(curChildNode->proxyData());
        parentNode->removeChild(i);
    }

    endRemoveRows();
    return true;
}

bool DocumentTreeModel::containsDocuments(const QModelIndex &index) {
    for (int i = 0; i < rowCount(index); i++) {
        QModelIndex child = this->index(i, 0, index);
        if (isCatalog(child)) {
            if (containsDocuments(child)) {
                return true;
            }
        } else if (isDocument(child)) {
            return true;
        } else {
            qDebug() << "Who the hell are you ?";
        }
    }

    return false;
}

QModelIndex DocumentTreeModel::indexForNode(DocumentTreeNode *pNode) const {
    if (pNode == 0) {
        return QModelIndex();
    }

    return pIndexForNode(QModelIndex(), pNode);
}

QPersistentModelIndex DocumentTreeModel::persistentIndexForNode(DocumentTreeNode *pNode) {
    return QPersistentModelIndex(indexForNode(pNode));
}

DocumentTreeNode *DocumentTreeModel::findProxy(DocumentProxy *pSearch, DocumentTreeNode *pParent) const {
    foreach (DocumentTreeNode *curNode, pParent->children()) {
        if (DocumentTreeNode::Catalog != curNode->nodeType()) {
            if (curNode->proxyData()->theSameDocument(pSearch))
                return curNode;
        } else if (curNode->children().count()) {
            DocumentTreeNode *recursiveDescendResult = findProxy(pSearch, curNode);
            if (recursiveDescendResult)
                return findProxy(pSearch, curNode);
        }
    }

    return 0;
}

QModelIndex DocumentTreeModel::pIndexForNode(const QModelIndex &parent, DocumentTreeNode *pNode) const {
    for (int i = 0; i < rowCount(parent); i++) {
        QModelIndex curIndex = index(i, 0, parent);
        if (curIndex.internalPointer() == pNode) {
            return curIndex;
        } else if (rowCount(curIndex) > 0) {
            QModelIndex recursiveDescendIndex = pIndexForNode(curIndex, pNode);
            if (recursiveDescendIndex.isValid()) {
                return recursiveDescendIndex;
            }
        }
    }
    return QModelIndex();
}

void DocumentTreeModel::copyIndexToNewParent(const QModelIndexList &list, const QModelIndex &newParent,
                                             eCopyMode pMode) {
    for (int i = 0; i < list.size(); i++) {
        if (list.at(i).column() == 0) {
            copyIndexToNewParent(list.at(i), newParent, pMode);
        }
    }
}

QPersistentModelIndex DocumentTreeModel::copyIndexToNewParent(const QModelIndex &source,
                                                              const QModelIndex &newParent, eCopyMode pMode) {
    DocumentTreeNode *nodeParent = nodeFromIndex(newParent);
    DocumentTreeNode *nodeSource = nodeFromIndex(source);

    if (!nodeParent || !nodeSource) {
        return QModelIndex();
    }

    //beginInsertRows(newParent, rowCount(newParent), rowCount(newParent));

    DocumentTreeNode *clonedNodeSource = 0;
    switch (static_cast<int>(pMode)) {
        case aReference:
            clonedNodeSource = nodeSource->clone();
            if (mNewDocuments.contains(nodeSource->proxyData())) { //update references for session documents
                mNewDocuments << clonedNodeSource->proxyData();

                PersistenceManager::persistenceManager()->reassignDocProxy(clonedNodeSource->proxyData(),
                                                                           nodeSource->proxyData());
            }
            break;

        case aContentCopy:
            DocumentProxy *duplicatedProxy = 0;
            if (nodeSource->nodeType() == DocumentTreeNode::Document && nodeSource->proxyData()) {
                duplicatedProxy =
                    PersistenceManager::persistenceManager()->duplicateDocument(nodeSource->proxyData());
                duplicatedProxy->setMetaData(Settings::documentUpdatedAt,
                                             StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
                MetadataDcSubsetAdaptor::persist(duplicatedProxy);
            }
            clonedNodeSource = new DocumentTreeNode(nodeSource->nodeType(), nodeSource->nodeName(),
                                                    nodeSource->displayName(), duplicatedProxy);
            break;
    }

    // Determine whether to provide a name with postfix if the name in current level already exists
    QString newName = clonedNodeSource->nodeName();
    if ((source.parent() != newParent || pMode != aReference) &&
        (newParent != trashIndex() || !inTrash(newParent))) {
        newName = adjustNameForParentIndex(newName, newParent);
        clonedNodeSource->setNodeName(newName);
    }

    if (clonedNodeSource->proxyData()) {
        clonedNodeSource->proxyData()->setMetaData(Settings::documentGroupName,
                                                   virtualPathForIndex(newParent));
        clonedNodeSource->proxyData()->setMetaData(Settings::documentName, newName);
        PersistenceManager::persistenceManager()->persistDocumentMetadata(clonedNodeSource->proxyData());
    }

    addNode(clonedNodeSource, newParent);
    //endInsertRows();

    QPersistentModelIndex newParentIndex = createIndex(rowCount(newParent), 0, clonedNodeSource);

    if (rowCount(source)) {
        for (int i = 0; i < rowCount(source); i++) {
            QModelIndex curNewParentIndexChild = source.sibling(i, 0);
            copyIndexToNewParent(curNewParentIndexChild, newParentIndex, pMode);
        }
    }

    return newParentIndex;
}

void DocumentTreeModel::moveIndexes(const QModelIndexList &source, const QModelIndex &destination) {
    QModelIndex destinationParent = destination;
    while (!isCatalog(destinationParent)) {
        destinationParent = destinationParent.parent();
    }

    DocumentTreeNode *newParentNode = nodeFromIndex(destinationParent);

    bool hasOneInsertion = false;

    for (int i = 0; i < source.size(); i++) {
        DocumentTreeNode *sourceNode = nodeFromIndex(source.at(i));
        QModelIndex s = source.at(i);

        if (newParentNode == sourceNode->parentNode() || sourceNode->findNode(newParentNode))
            continue;

        if (s.internalId() != destinationParent.internalId()) {
            int sourceIndex = source.at(i).row();
            int destIndex = positionForParent(sourceNode, newParentNode);

            beginMoveRows(s.parent(), sourceIndex, sourceIndex, destinationParent, destIndex);
            fixNodeName(s, destinationParent);
            sourceNode->parentNode()->moveChild(sourceNode, destIndex, newParentNode);
            updateIndexNameBindings(sourceNode);
            hasOneInsertion = true;
        }
    }

    if (hasOneInsertion)
        endMoveRows();
}

void DocumentTreeModel::moveIndex(const QModelIndex &what, const QModelIndex &destination) {
    QModelIndexList list;
    list.push_back(what);
    moveIndexes(list, destination);
}

void DocumentTreeModel::setCurrentDocument(DocumentProxy *pDocument) {
    DocumentTreeNode *testCurNode = findProxy(pDocument, mRootNode);

    if (testCurNode) {
        setCurrentNode(testCurNode);
    }
}

QModelIndex DocumentTreeModel::indexForProxy(DocumentProxy *pSearch) const {
    DocumentTreeNode *proxy = findProxy(pSearch, mRootNode);
    if (!proxy) {
        return QModelIndex();
    }

    return indexForNode(proxy);
}

void DocumentTreeModel::setRootNode(DocumentTreeNode *pRoot) {
    mRootNode = pRoot;
    //reset();
}

DocumentProxy *DocumentTreeModel::proxyForIndex(const QModelIndex &pIndex) const {
    DocumentTreeNode *node = nodeFromIndex(pIndex);
    if (!node) {
        return 0;
    }

    return node->proxyData();
}

QString DocumentTreeModel::virtualDirForIndex(const QModelIndex &pIndex) const {
    QString result;
    DocumentTreeNode *curNode = nodeFromIndex(pIndex);
    //protect the 2nd level items
    while (curNode->parentNode() && !curNode->isTopLevel()) {
        result.prepend(curNode->parentNode()->nodeName() + "/");
        curNode = curNode->parentNode();
    }

    if (result.endsWith("/")) {
        result.truncate(result.count() - 1);
    }

    return result;
}

QString DocumentTreeModel::virtualPathForIndex(const QModelIndex &pIndex) const {
    DocumentTreeNode *curNode = nodeFromIndex(pIndex);
    Q_ASSERT(curNode);

    return virtualDirForIndex(pIndex) + "/" + curNode->nodeName();
}

QStringList DocumentTreeModel::nodeNameList(const QModelIndex &pIndex) const {
    QStringList result;

    DocumentTreeNode *catalog = nodeFromIndex(pIndex);
    if (catalog->nodeType() != DocumentTreeNode::Catalog) {
        return QStringList();
    }

    foreach (DocumentTreeNode *curNode, catalog->children()) {
        result << curNode->nodeName();
    }

    return result;
}

bool DocumentTreeModel::newNodeAllowed(const QModelIndex &pSelectedIndex) const {
    if (!pSelectedIndex.isValid()) {
        return false;
    }

    if (inTrash(pSelectedIndex) || pSelectedIndex == trashIndex()) {
        return false;
    }

    return true;
}

QModelIndex DocumentTreeModel::goTo(const QString &dir) {
    QStringList pathList = dir.split("/", Qt::SkipEmptyParts);

    if (pathList.isEmpty()) {
        return myDocumentsIndex();
    }

    if (pathList.first() != PersistenceManager::myDocumentsName &&
        pathList.first() != Settings::trashedDocumentGroupNamePrefix &&
        pathList.first() != PersistenceManager::modelsName) {
        pathList.prepend(PersistenceManager::myDocumentsName);
    }

    QModelIndex parentIndex;

    bool searchingNode = true;
    while (!pathList.isEmpty()) {
        QString curLevelName = pathList.takeFirst();
        if (searchingNode) {
            searchingNode = false;
            for (int i = 0; i < rowCount(parentIndex); ++i) {
                QModelIndex curChildIndex = index(i, 0, parentIndex);
                if (nodeFromIndex(curChildIndex)->nodeName() == curLevelName) {
                    searchingNode = true;
                    parentIndex = curChildIndex;
                    break;
                }
            }
        }

        if (!searchingNode) {
            DocumentTreeNode *newChild = new DocumentTreeNode(DocumentTreeNode::Catalog, curLevelName);
            parentIndex = addNode(newChild, parentIndex);
        }
    }

    return parentIndex;
}

bool DocumentTreeModel::inTrash(const QModelIndex &index) const {
    return isDescendantOf(index, trashIndex());
}

bool DocumentTreeModel::inMyDocuments(const QModelIndex &index) const {
    return isDescendantOf(index, myDocumentsIndex());
}

void DocumentTreeModel::addDocument(DocumentProxy *pProxyData, const QModelIndex &pParent) {
    if (!pProxyData) {
        return;
    }
    QString docName = pProxyData->metaData(Settings::documentName).toString();
    QString docGroupName = pProxyData->metaData(Settings::documentGroupName).toString();

    if (docName.isEmpty()) {
        return;
    }

    QModelIndex lParent = pParent;
    DocumentTreeNode *freeNode =
        new DocumentTreeNode(DocumentTreeNode::Document, docName, QString(), pProxyData);
    if (!pParent.isValid()) {
        lParent = goTo(docGroupName);
    }

    addNode(freeNode, lParent);
}

void DocumentTreeModel::addNewDocument(DocumentProxy *pProxyData, const QModelIndex &pParent) {
    addDocument(pProxyData, pParent);
    mNewDocuments << pProxyData;
}

QModelIndex DocumentTreeModel::addCatalog(const QString &pName, const QModelIndex &pParent) {
    if (pName.isEmpty() || !pParent.isValid()) {
        return QModelIndex();
    }

    DocumentTreeNode *catalogNode = new DocumentTreeNode(DocumentTreeNode::Catalog, pName);
    return addNode(catalogNode, pParent);
}

void DocumentTreeModel::setNewName(const QModelIndex &index, const QString &newName) {
    if (!index.isValid()) {
        return;
    }

    DocumentTreeNode *indexNode = nodeFromIndex(index);

    QString magicSeparator = "+!##s";
    if (isCatalog(index)) {
        QString fullNewName = newName;
        fullNewName.replace('/', '-');
        if (!newName.contains(magicSeparator)) {
            indexNode->setNodeName(fullNewName);
            QString virtualDir = virtualDirForIndex(index);
            fullNewName.prepend(virtualDir.isEmpty() ? "" : virtualDir + magicSeparator);
        }
        for (int i = 0; i < rowCount(index); i++) {
            QModelIndex subIndex = this->index(i, 0, index);
            setNewName(subIndex, fullNewName + magicSeparator + subIndex.data().toString());
        }

    } else if (isDocument(index)) {
        Q_ASSERT(indexNode->proxyData());

        int prefixIndex = newName.lastIndexOf(magicSeparator);
        if (prefixIndex != -1) {
            QString newDocumentGroupName = newName.left(prefixIndex).replace(magicSeparator, "/");
            indexNode->proxyData()->setMetaData(Settings::documentGroupName, newDocumentGroupName);
        } else {
            indexNode->setNodeName(newName);
            indexNode->proxyData()->setMetaData(Settings::documentName, newName);
        }

        PersistenceManager::persistenceManager()->persistDocumentMetadata(indexNode->proxyData());
    }
}

QString DocumentTreeModel::adjustNameForParentIndex(const QString &pName, const QModelIndex &pIndex) {
    int i = 0;
    QString newName = pName;
    QStringList siblingNames = nodeNameList(pIndex);
    while (siblingNames.contains(newName)) {
        newName = pName + " " + QVariant(++i).toString();
    }

    return newName;
}

void DocumentTreeModel::fixNodeName(const QModelIndex &source, const QModelIndex &dest) {
    // Determine whether to provide a name with postfix if the name in current level allready exists
    DocumentTreeNode *srcNode = nodeFromIndex(source);
    Q_ASSERT(srcNode);

    QString newName = srcNode->nodeName();
    if (source.parent() != dest && (dest != trashIndex() || !inTrash(dest))) {
        newName = adjustNameForParentIndex(newName, dest);
        srcNode->setNodeName(newName);
        nodeFromIndex(source)->setNodeName(newName);
    }
}

void DocumentTreeModel::updateIndexNameBindings(DocumentTreeNode *nd) {
    Q_ASSERT(nd);

    if (nd->nodeType() == DocumentTreeNode::Catalog) {
        foreach (DocumentTreeNode *lnd, nd->children()) {
            updateIndexNameBindings(lnd);
        }
    } else if (nd->proxyData()) {
        nd->proxyData()->setMetaData(Settings::documentGroupName,
                                     virtualPathForIndex(indexForNode(nd->parentNode())));
        nd->proxyData()->setMetaData(Settings::documentName, nd->nodeName());
        nd->proxyData()->setMetaData(Settings::documentUpdatedAt,
                                     StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
        PersistenceManager::persistenceManager()->persistDocumentMetadata(nd->proxyData());
    }
}

bool DocumentTreeModel::isDescendantOf(const QModelIndex &pPossibleDescendant,
                                       const QModelIndex &pPossibleAncestor) const {
    if (!pPossibleDescendant.isValid()) {
        return false;
    }

    QModelIndex ancestor = pPossibleDescendant;
    while (ancestor.parent().isValid()) {
        ancestor = ancestor.parent();
        if (ancestor == pPossibleAncestor) {
            return true;
        }
    }

    return false;
}

QModelIndex DocumentTreeModel::addNode(DocumentTreeNode *pFreeNode, const QModelIndex &pParent,
                                       eAddItemMode pMode) {
    DocumentTreeNode *tstParent = nodeFromIndex(pParent);

    if (!pParent.isValid() || tstParent->nodeType() != DocumentTreeNode::Catalog) {
        return QModelIndex();
    }
    int newIndex =
        pMode == aDetectPosition ? positionForParent(pFreeNode, tstParent) : tstParent->children().size();
    beginInsertRows(pParent, newIndex, newIndex);
    tstParent->insertChild(newIndex, pFreeNode);
    endInsertRows();

    return createIndex(newIndex, 0, pFreeNode);
}

int DocumentTreeModel::positionForParent(DocumentTreeNode *pFreeNode, DocumentTreeNode *pParentNode) {
    Q_ASSERT(pFreeNode);
    Q_ASSERT(pParentNode);
    Q_ASSERT(pParentNode->nodeType() == DocumentTreeNode::Catalog);

    int c = -1;
    int childCount = pParentNode->children().count();
    while (c <= childCount) {
        if (++c == childCount || lessThan(pFreeNode, pParentNode->children().at(c))) {
            break;
        }
    }
    return c == -1 ? childCount : c;
}

DocumentTreeNode *DocumentTreeModel::nodeFromIndex(const QModelIndex &pIndex) const {
    if (pIndex.isValid()) {
        return static_cast<DocumentTreeNode *>(pIndex.internalPointer());
    } else {
        return mRootNode;
    }
}

bool DocumentTreeModel::nodeLessThan(const DocumentTreeNode *firstIndex,
                                     const DocumentTreeNode *secondIndex) {
    return firstIndex->nodeName() < secondIndex->nodeName();
}

DocumentTreeModel::~DocumentTreeModel() {
    delete mRootNode;
}

DocumentTreeView::DocumentTreeView(QWidget *parent) : QTreeView(parent) {
    setObjectName("DocumentTreeView");
    setRootIsDecorated(true);
}

void DocumentTreeView::setSelectedAndExpanded(const QModelIndex &pIndex, bool pExpand, bool pEdit) {
    if (!pIndex.isValid()) {
        return;
    }

    QModelIndex indexCurrentDoc = pIndex;
    clearSelection();

    SortFilterProxyModel *proxy = dynamic_cast<SortFilterProxyModel *>(model());

    QItemSelectionModel::SelectionFlags sel =
        pExpand ? QItemSelectionModel::Select : QItemSelectionModel::Deselect;

    setCurrentIndex(pExpand ? indexCurrentDoc : QModelIndex());

    selectionModel()->select(proxy->mapFromSource(indexCurrentDoc), QItemSelectionModel::Rows | sel);

    while (indexCurrentDoc.parent().isValid()) {
        setExpanded(indexCurrentDoc.parent(), pExpand);
        indexCurrentDoc = indexCurrentDoc.parent();
    }

    scrollTo(proxy->mapFromSource(pIndex), QAbstractItemView::PositionAtCenter);

    if (pEdit)
        edit(proxy->mapFromSource(pIndex));
}

void DocumentTreeView::onModelIndexChanged(const QModelIndex &pNewIndex, const QModelIndex &pOldIndex) {
    Q_UNUSED(pOldIndex)

    QModelIndex indexSource = mapIndexToSource(pNewIndex);

    setSelectedAndExpanded(indexSource, true);
}

void DocumentTreeView::hSliderRangeChanged(int min, int max) {
    Q_UNUSED(min);
    Q_UNUSED(max);

    QScrollBar *hScroller = horizontalScrollBar();
    if (hScroller) {
        hScroller->triggerAction(QAbstractSlider::SliderToMaximum);
    }
}

void DocumentTreeView::dragEnterEvent(QDragEnterEvent *event) {
    QTreeView::dragEnterEvent(event);
    event->accept();
    event->acceptProposedAction();
}

void DocumentTreeView::dragLeaveEvent(QDragLeaveEvent *event) {
    Q_UNUSED(event);

    DocumentTreeModel *docModel = 0;

    SortFilterProxyModel *proxy = dynamic_cast<SortFilterProxyModel *>(model());

    if (proxy) {
        docModel = dynamic_cast<DocumentTreeModel *>(proxy->sourceModel());
    } else {
        docModel = dynamic_cast<DocumentTreeModel *>(model());
    }

    docModel->setHighLighted(QModelIndex());
    update();
}

void DocumentTreeView::dragMoveEvent(QDragMoveEvent *event) {
    QModelIndex index;
    if (selectedIndexes().count() > 0) {
        index = selectedIndexes().first();
    }

    bool acceptIt = isAcceptable(index, indexAt(event->pos()));

    if (event->mimeData()->hasFormat(Application::mimeTypeBoardPage)) {
        SortFilterProxyModel *proxy = dynamic_cast<SortFilterProxyModel *>(model());

        DocumentTreeModel *docModel = 0;

        if (proxy) {
            docModel = dynamic_cast<DocumentTreeModel *>(proxy->sourceModel());
        } else {
            docModel = dynamic_cast<DocumentTreeModel *>(model());
        }

        QModelIndex targetIndex = mapIndexToSource(indexAt(event->pos()));

        if (!docModel || !docModel->isDocument(targetIndex) || docModel->inTrash(targetIndex)) {
            event->ignore();
            event->setDropAction(Qt::IgnoreAction);
            docModel->setHighLighted(QModelIndex());
            acceptIt = false;
        } else {
            docModel->setHighLighted(targetIndex);
            acceptIt = true;
        }
        updateIndexEnvirons(indexAt(event->pos()));
    }
    QTreeView::dragMoveEvent(event);

    event->setAccepted(acceptIt);
}

void DocumentTreeView::dropEvent(QDropEvent *event) {
    event->ignore();
    event->setDropAction(Qt::IgnoreAction);
    DocumentTreeModel *docModel = 0;

    SortFilterProxyModel *proxy = dynamic_cast<SortFilterProxyModel *>(model());
    if (proxy) {
        docModel = dynamic_cast<DocumentTreeModel *>(proxy->sourceModel());
    }

    QModelIndex targetIndex = mapIndexToSource(indexAt(event->pos()));
    QModelIndexList dropIndex = mapIndexesToSource(selectedIndexes());

    //clear the selection right after
    //selectionModel()->clearSelection();

    bool isBoardPage = event->mimeData()->hasFormat(Application::mimeTypeBoardPage);

    bool targetIsInTrash = docModel->inTrash(targetIndex) || docModel->trashIndex() == targetIndex;
    bool targetIsInMyDocuments =
        docModel->inMyDocuments(targetIndex) || docModel->myDocumentsIndex() == targetIndex;

    if (!targetIsInMyDocuments && !targetIsInTrash)
        return;

    if (isBoardPage) {
        DocumentProxy *targetDocProxy = docModel->proxyData(targetIndex);

        const MimeData *mimeValue = qobject_cast<const MimeData *>(event->mimeData());
        if (!targetDocProxy || !mimeValue || !mimeValue->items().count()) {
            qDebug() << "an error ocured while parsing " << Application::mimeTypeBoardPage;
            QTreeView::dropEvent(event);
            return;
        }

        int count = 0;
        int total = mimeValue->items().size();
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        foreach (MimeDataItem sourceItem, mimeValue->items()) {
            DocumentProxy *fromProxy = sourceItem.documentProxy();
            int fromIndex = sourceItem.sceneIndex();
            int toIndex = targetDocProxy->pageCount();

            count++;

            Application::applicationController->showMessage(tr("Copying page %1/%2").arg(count).arg(total),
                                                            true);

            GraphicsScene *scene = PersistenceManager::persistenceManager()->loadDocumentScene(
                sourceItem.documentProxy(), sourceItem.sceneIndex());
            if (scene) {
                GraphicsScene *sceneClone = scene->sceneDeepCopy();

                DocumentProxy *targetDocProxy = docModel->proxyForIndex(targetIndex);

                foreach (QUrl relativeFile, scene->relativeDependencies()) {
                    QString source = scene->document()->persistencePath() + "/" + relativeFile.toString();
                    QString target = targetDocProxy->persistencePath() + "/" + relativeFile.toString();

                    QString sourceDecoded = scene->document()->persistencePath() + "/" +
                                            relativeFile.toString(QUrl::DecodeReserved);
                    QString targetDecoded =
                        targetDocProxy->persistencePath() + "/" + relativeFile.toString(QUrl::DecodeReserved);

                    if (QFileInfo(source).isDir())
                        FileSystemUtils::copyDir(source, target);
                    else {
                        QFileInfo fi(targetDecoded);
                        QDir d = fi.dir();
                        d.mkpath(d.absolutePath());
                        QFile::copy(sourceDecoded, targetDecoded);
                    }
                }

                PersistenceManager::persistenceManager()->insertDocumentSceneAt(targetDocProxy, sceneClone,
                                                                                targetDocProxy->pageCount());

                QString thumbTmp(fromProxy->persistencePath() +
                                 FileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", fromIndex));
                QString thumbTo(targetDocProxy->persistencePath() +
                                FileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", toIndex));

                QFile::remove(thumbTo);
                QFile::copy(thumbTmp, thumbTo);

                Q_ASSERT(QFileInfo(thumbTmp).exists());
                Q_ASSERT(QFileInfo(thumbTo).exists());
                const QPixmap *pix = new QPixmap(thumbTmp);
                DocumentController *ctrl = Application::documentController;
                ctrl->addPixmapAt(pix, toIndex);
                ctrl->TreeViewSelectionChanged(ctrl->firstSelectedTreeIndex(), QModelIndex());
            }

            QApplication::restoreOverrideCursor();
            Application::applicationController->showMessage(tr("%1 pages copied", "", total).arg(total),
                                                            false);

            docModel->setHighLighted(QModelIndex());
        }

    } else {
        if (targetIsInTrash) {
            if (!Application::mainWindow->yesNoQuestion(
                    tr("Remove Item"), tr("Are you sure you want to remove the selected item(s) ?")))
                return;

            Application::documentController->moveIndexesToTrash(dropIndex, docModel);
        } else {
            docModel->moveIndexes(dropIndex, targetIndex);
        }
    }

    expand(proxy->mapFromSource(targetIndex));

    QTreeView::dropEvent(event);

    Application::documentController->updateActions();
}

void DocumentTreeView::paintEvent(QPaintEvent *event) {
    QTreeView::paintEvent(event);
}

void DocumentTreeView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end) {
    QTreeView::rowsAboutToBeRemoved(parent, start, end);
}

bool DocumentTreeView::isAcceptable(const QModelIndex &dragIndex, const QModelIndex &atIndex) {
    QModelIndex dragIndexSource = mapIndexToSource(dragIndex);
    QModelIndex atIndexSource = mapIndexToSource(atIndex);

    if (!dragIndexSource.isValid()) {
        return false;
    }

    return true;
}

Qt::DropAction DocumentTreeView::acceptableAction(const QModelIndex &dragIndex, const QModelIndex &atIndex) {
    return Qt::MoveAction;
}

void DocumentTreeView::updateIndexEnvirons(const QModelIndex &index) {
    QRect updateRect = visualRect(index);
    const int multipler = 3;
    updateRect.adjust(0, -updateRect.height() * multipler, 0, updateRect.height() * multipler);
    update(updateRect);
}

QModelIndex DocumentTreeView::mapIndexToSource(const QModelIndex &index) {
    SortFilterProxyModel *proxy = dynamic_cast<SortFilterProxyModel *>(model());

    if (proxy) {
        return proxy->mapToSource(index);
    }

    return index;
}

QModelIndexList DocumentTreeView::mapIndexesToSource(const QModelIndexList &indexes) {
    SortFilterProxyModel *proxy = dynamic_cast<SortFilterProxyModel *>(model());

    if (proxy) {
        QModelIndexList list;

        for (int i = 0; i < indexes.size(); i++) {
            list.push_back(proxy->mapToSource(indexes.at(i)));
        }

        return list;
    }

    return indexes;
}

DocumentTreeItemDelegate::DocumentTreeItemDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

void DocumentTreeItemDelegate::commitAndCloseEditor() {
    QLineEdit *lineEditor = qobject_cast<QLineEdit *>(sender());
    if (lineEditor) {
        emit commitData(lineEditor);
        //emit closeEditor(lineEditor);
        emit Application::documentController->reorderDocumentsRequested();
    }
}

void DocumentTreeItemDelegate::processChangedText(const QString &str) const {
    QLineEdit *editor = qobject_cast<QLineEdit *>(sender());
    if (!editor) {
        return;
    }

    if (!validateString(str)) {
        editor->setStyleSheet("background-color: #FFB3C8;");
    } else {
        editor->setStyleSheet("background-color: #FFFFFF;");
    }
}

bool DocumentTreeItemDelegate::validateString(const QString &str) const {
    return !mExistingFileNames.contains(str);
}

QWidget *DocumentTreeItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                                const QModelIndex &index) const {
    Q_UNUSED(option);

    if (index.column() == 0) {
        mExistingFileNames.clear();
        const DocumentTreeModel *indexModel = qobject_cast<const DocumentTreeModel *>(index.model());
        if (indexModel) {
            mExistingFileNames = indexModel->nodeNameList(index.parent());
            mExistingFileNames.removeOne(index.data().toString());
        }

        QLineEdit *nameEditor = new QLineEdit(parent);
        connect(nameEditor, &QLineEdit::editingFinished, this,
                &DocumentTreeItemDelegate::commitAndCloseEditor);
        connect(nameEditor, &QLineEdit::textChanged, this, &DocumentTreeItemDelegate::processChangedText);
        return nameEditor;
    }

    //N/C - NNe - 20140407 : the other column are not editable.
    return 0;
}

void DocumentTreeItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
    if (index.column() == 0) {
        QLineEdit *lineEditor = qobject_cast<QLineEdit *>(editor);
        lineEditor->setText(index.data().toString());
        lineEditor->selectAll();
    }
}

void DocumentTreeItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                            const QModelIndex &index) const {
    QLineEdit *lineEditor = qobject_cast<QLineEdit *>(editor);
    if (validateString(lineEditor->text())) {
        model->setData(index, lineEditor->text());
    }
}

void DocumentTreeItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const {
    QStyledItemDelegate::paint(painter, option, index);
}

DocumentController::DocumentController(MainWindow *mainWindow)
    : DocumentContainer(mainWindow->centralWidget()),
      mSelectionType(None),
      mParentWidget(mainWindow->centralWidget()),
      mBoardController(Application::boardController),
      mDocumentsView(0),
      mMainWindow(mainWindow),
      mDocumentWidget(0),
      mIsClosing(false),
      mToolsPalette(0),
      mToolsPalettePositionned(false),
      mDocumentTrashGroupName(tr("Trash")),
      mDefaultDocumentGroupName(tr("Untitled Documents")),
      mCurrentIndexMoved(false) {
    setupViews();
    setupToolbar();
    connect(this, &DocumentController::exportDone, mMainWindow, &MainWindow::onExportDone);
    connect(this, &DocumentContainer::documentThumbnailsUpdated, this,
            &DocumentController::refreshDocumentThumbnailsView);
    connect(this, &DocumentController::reorderDocumentsRequested, this,
            &DocumentController::reorderDocuments);
}

DocumentController::~DocumentController() {
    // mDocumentsView belongs to the widget tree rooted at mDocumentWidget;
    // Qt destroys it together with that container.
}

void DocumentController::createNewDocument() {
    PersistenceManager *pManager = PersistenceManager::persistenceManager();
    DocumentTreeModel *docModel = pManager->mDocumentTreeStructureModel;
    QModelIndex selectedIndex = firstSelectedTreeIndex();

    if (!selectedIndex.isValid())
        selectedIndex = docModel->myDocumentsIndex();

    QString groupName = docModel->isCatalog(selectedIndex) ? docModel->virtualPathForIndex(selectedIndex)
                                                           : docModel->virtualDirForIndex(selectedIndex);

    DocumentProxy *document = pManager->createDocument(groupName);
    selectDocument(document, true, false, true);

    if (document)
        pManager->mDocumentTreeStructureModel->markDocumentAsNew(document);
}

void DocumentController::selectDocument(DocumentProxy *proxy, bool setAsCurrentDocument, const bool onImport,
                                        const bool editMode) {
    if (proxy == NULL) {
        setDocument(NULL);
        return;
    }

    if (setAsCurrentDocument) {
        PersistenceManager::persistenceManager()->mDocumentTreeStructureModel->setCurrentDocument(proxy);
        QModelIndex indexCurrentDoc =
            PersistenceManager::persistenceManager()->mDocumentTreeStructureModel->indexForProxy(proxy);
        mDocumentsView->documentTreeView()->setSelectedAndExpanded(indexCurrentDoc, true, editMode);

        if (proxy !=
            mBoardController
                ->selectedDocument()) // only if wanted Document is different from document actually on Board,
        {
            mBoardController->setActiveDocumentScene(proxy, 0, true, onImport);
        }
    }

    mSelectionType = Document;
    setDocument(proxy);
}

void DocumentController::createNewDocumentGroup() {
    PersistenceManager *pManager = PersistenceManager::persistenceManager();
    DocumentTreeModel *docModel = pManager->mDocumentTreeStructureModel;
    QModelIndex selectedIndex = firstSelectedTreeIndex();
    if (!selectedIndex.isValid()) {
        selectedIndex = docModel->myDocumentsIndex();
    }
    QModelIndex parentIndex = docModel->isCatalog(selectedIndex) ? selectedIndex : selectedIndex.parent();

    QString newFolderName = docModel->adjustNameForParentIndex(tr("New Folder"), parentIndex);

    QModelIndex newIndex = docModel->addCatalog(newFolderName, parentIndex);
    mDocumentsView->documentTreeView()->setSelectedAndExpanded(newIndex, true, true);
}


DocumentProxy *DocumentController::selectedDocumentProxy() {
    return PersistenceManager::persistenceManager()->mDocumentTreeStructureModel->proxyForIndex(
        firstSelectedTreeIndex());
}

QList<DocumentProxy *> DocumentController::selectedProxies() {
    QList<DocumentProxy *> result;

    foreach (QModelIndex curIndex,
             mapIndexesToSource(mDocumentsView->documentTreeView()->selectionModel()->selectedIndexes())) {
        result << PersistenceManager::persistenceManager()->mDocumentTreeStructureModel->proxyForIndex(
            curIndex);
    }

    return result;
}

QModelIndexList DocumentController::selectedTreeIndexes() {
    return mapIndexesToSource(mDocumentsView->documentTreeView()->selectionModel()->selectedRows(0));
}

DocumentProxy *DocumentController::firstSelectedTreeProxy() {
    return selectedProxies().count() ? selectedProxies().first() : 0;
}

void DocumentController::TreeViewSelectionChanged(const QModelIndex &current, const QModelIndex &previous) {
    Q_UNUSED(previous)

    DocumentTreeModel *docModel = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

    QModelIndex current_index = mapIndexToSource(current);

    //if the selection contains more than one object, don't show the thumbnail.
    //We have just to pass a null proxy to disable the display of thumbnail
    DocumentProxy *currentDocumentProxy = 0;

    if (current_index.isValid() &&
        mDocumentsView->documentTreeView()->selectionModel()->selectedRows(0).size() == 1) {
        currentDocumentProxy = docModel->proxyData(current_index);
        setDocument(currentDocumentProxy, false);
    }

    if (mCurrentIndexMoved) {
        if (docModel->isDocument(current_index)) {
            docModel->setCurrentDocument(currentDocumentProxy);
        } else if (docModel->isCatalog(current_index)) {
            docModel->setCurrentDocument(0);
        }
        mCurrentIndexMoved = false;
    }

    itemSelectionChanged(docModel->isCatalog(current_index) ? Folder : Document);
}

QModelIndex DocumentController::mapIndexToSource(const QModelIndex &index) {
    SortFilterProxyModel *proxy =
        dynamic_cast<SortFilterProxyModel *>(mDocumentsView->documentTreeView()->model());

    if (proxy) {
        return proxy->mapToSource(index);
    }

    return index;
}

QModelIndexList DocumentController::mapIndexesToSource(const QModelIndexList &indexes) {
    SortFilterProxyModel *proxy =
        dynamic_cast<SortFilterProxyModel *>(mDocumentsView->documentTreeView()->model());

    if (proxy) {
        QModelIndexList list;

        for (int i = 0; i < indexes.size(); i++) {
            list.push_back(proxy->mapToSource(indexes.at(i)));
        }

        return list;
    }

    return indexes;
}

void DocumentController::TreeViewSelectionChanged(const QItemSelection &selected,
                                                  const QItemSelection &deselected) {
    Q_UNUSED(deselected)

    int nbIndexes = selected.indexes().count();

    if (nbIndexes) {
        QModelIndexList list = mDocumentsView->documentTreeView()->selectionModel()->selectedRows();

        //if multi-selection
        if (list.count() > 1) {
            for (int i = 0; i < list.count(); i++) {
                QModelIndex newSelectedRow = list.at(i);
                QModelIndex parent = newSelectedRow.parent();
                bool isParentIsSelected = false;
                while (parent.isValid()) {
                    isParentIsSelected |= (list.indexOf(parent) != -1);

                    if (isParentIsSelected)
                        break;

                    parent = parent.parent();
                }

                if (!isParentIsSelected)
                    TreeViewSelectionChanged(newSelectedRow, QModelIndex());
                else
                    mDocumentsView->documentTreeView()->selectionModel()->select(
                        newSelectedRow, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
            }
        } else {
            TreeViewSelectionChanged(selected.indexes().at(0), QModelIndex());
        }
    }
}

void DocumentController::itemSelectionChanged(LastSelectedElementType newSelection) {
    mSelectionType = newSelection;
    updateActions();
}


void DocumentController::setupViews() {
    if (!mDocumentWidget) {
        mDocumentWidget = new QWidget(mMainWindow->centralWidget());
        mMainWindow->addDocumentsWidget(mDocumentWidget);

        // documents.ui used to be replayed onto mDocumentWidget itself by
        // Ui::documents::setupUi(). The hand written form is now a widget of its own
        // living inside that container, so a margin-less layout makes it fill the
        // container exactly like the generated grid layout did.
        QVBoxLayout *documentWidgetLayout = new QVBoxLayout(mDocumentWidget);
        documentWidgetLayout->setSpacing(0);
        documentWidgetLayout->setContentsMargins(0, 0, 0, 0);

        mDocumentsView = new DocumentsWidget(mDocumentWidget);
        documentWidgetLayout->addWidget(mDocumentsView);

        int thumbWidth = Settings::settings()->documentThumbnailWidth->get().toInt();

        mDocumentsView->documentZoomSlider()->setValue(thumbWidth);
        mDocumentsView->thumbnailWidget()->setThumbnailWidth(thumbWidth);

        connect(mDocumentsView->documentZoomSlider(), &QSlider::valueChanged, this,
                &DocumentController::documentZoomSliderValueChanged);

        connect(mMainWindow->ui().actionOpen, &QAction::triggered, this,
                &DocumentController::openSelectedItem);
        connect(mMainWindow->ui().actionNewFolder, &QAction::triggered, this,
                &DocumentController::createNewDocumentGroup);
        connect(mMainWindow->ui().actionNewDocument, &QAction::triggered, this,
                &DocumentController::createNewDocument);

        connect(mMainWindow->ui().actionImport, &QAction::triggered, this, &DocumentController::importFile);

        QMenu *addMenu = new QMenu(mDocumentWidget);
        mAddFolderOfImagesAction = addMenu->addAction(tr("Add Folder of Images"));
        mAddImagesAction = addMenu->addAction(tr("Add Images"));
        mAddFileToDocumentAction = addMenu->addAction(tr("Add Pages from File"));

        connect(mAddFolderOfImagesAction, &QAction::triggered, this, &DocumentController::addFolderOfImages);
        connect(mAddFileToDocumentAction, &QAction::triggered, this,
                qOverload<>(&DocumentController::addFileToDocument));
        connect(mAddImagesAction, &QAction::triggered, this, &DocumentController::addImages);

        foreach (QWidget *menuWidget, mMainWindow->ui().actionDocumentAdd->associatedWidgets()) {
            QToolButton *tb = qobject_cast<QToolButton *>(menuWidget);

            if (tb && !tb->menu()) {
                tb->setObjectName("buttonMenu");
                tb->setPopupMode(QToolButton::InstantPopup);

                QMenu *menu = new QMenu(mDocumentWidget);

                menu->addAction(mAddFolderOfImagesAction);
                menu->addAction(mAddImagesAction);
                menu->addAction(mAddFileToDocumentAction);

                tb->setMenu(menu);
            }
        }

        QMenu *exportMenu = new QMenu(mDocumentWidget);

        DocumentManager *documentManager = DocumentManager::documentManager();
        for (int i = 0; i < documentManager->supportedExportAdaptors().length(); i++) {
            ExportAdaptor *adaptor = documentManager->supportedExportAdaptors()[i];
            QAction *currentExportAction = exportMenu->addAction(adaptor->exportName());
            currentExportAction->setData(i);
            connect(currentExportAction, &QAction::triggered, this, &DocumentController::exportDocument);
            exportMenu->addAction(currentExportAction);
            adaptor->setAssociatedAction(currentExportAction);
        }

        foreach (QWidget *menuWidget, mMainWindow->ui().actionExport->associatedWidgets()) {
            QToolButton *tb = qobject_cast<QToolButton *>(menuWidget);

            if (tb && !tb->menu()) {
                tb->setObjectName("buttonMenu");
                tb->setPopupMode(QToolButton::InstantPopup);

                tb->setMenu(exportMenu);
            }
        }

#ifdef Q_OS_OSX
        mMainWindow->ui().actionDelete->setShortcut(QKeySequence(Qt::Key_Backspace));
#else
        mMainWindow->ui().actionDelete->setShortcut(QKeySequence(Qt::Key_Delete));
#endif

        connect(mMainWindow->ui().actionDelete, &QAction::triggered, this,
                &DocumentController::deleteSelectedItem);
        connect(mMainWindow->ui().actionDuplicate, &QAction::triggered, this,
                &DocumentController::duplicateSelectedItem);
        connect(mMainWindow->ui().actionRename, &QAction::triggered, this,
                &DocumentController::renameSelectedItem);
        connect(mMainWindow->ui().actionAddToWorkingDocument, &QAction::triggered, this,
                &DocumentController::addToDocument);

        DocumentTreeModel *model = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

        mSortFilterProxyModel = new SortFilterProxyModel();

        mSortFilterProxyModel->setSourceModel(model);

        // sort documents according to preferences
        int sortKind = Settings::settings()->documentSortKind->get().toInt();
        int sortOrder = Settings::settings()->documentSortOrder->get().toInt();

        // update icon and button
        mDocumentsView->sortKind()->setCurrentIndex(sortKind);
        if (sortOrder == DocumentController::DESC)
            mDocumentsView->sortOrder()->setChecked(true);

        mDocumentsView->documentTreeView()->setModel(mSortFilterProxyModel);

        mDocumentsView->documentTreeView()->setItemDelegate(new DocumentTreeItemDelegate(this));
        mDocumentsView->documentTreeView()->setDragEnabled(true);
        mDocumentsView->documentTreeView()->setAcceptDrops(true);
        mDocumentsView->documentTreeView()->viewport()->setAcceptDrops(true);
        mDocumentsView->documentTreeView()->setDropIndicatorShown(true);
        mDocumentsView->documentTreeView()->header()->setStretchLastSection(false);
        mDocumentsView->documentTreeView()->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        mDocumentsView->documentTreeView()->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        mDocumentsView->documentTreeView()->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

        const int splitterLeftSize = Settings::settings()->documentSplitterLeftSize->get().toInt();
        const int splitterRightSize = Settings::settings()->documentSplitterRightSize->get().toInt();
        QList<int> splitterSizes;
        splitterSizes.append(splitterLeftSize);
        splitterSizes.append(splitterRightSize);
        mDocumentsView->splitter()->setSizes(splitterSizes);

        //mDocumentsView->documentTreeView()->hideColumn(1);
        mDocumentsView->documentTreeView()->hideColumn(2);

        sortDocuments(sortKind, sortOrder);

        connect(mDocumentsView->sortKind(), &QComboBox::activated, this,
                &DocumentController::onSortKindChanged);
        connect(mDocumentsView->sortOrder(), &QToolButton::toggled, this,
                &DocumentController::onSortOrderChanged);

        connect(mDocumentsView->splitter(), &QSplitter::splitterMoved, this,
                &DocumentController::onSplitterMoved);

        connect(mDocumentsView->documentTreeView()->selectionModel(), &QItemSelectionModel::selectionChanged,
                this,
                qOverload<const QItemSelection &, const QItemSelection &>(
                    &DocumentController::TreeViewSelectionChanged));
        connect(PersistenceManager::persistenceManager()->mDocumentTreeStructureModel,
                &DocumentTreeModel::indexChanged, mDocumentsView->documentTreeView(),
                &DocumentTreeView::onModelIndexChanged);
        connect(PersistenceManager::persistenceManager()->mDocumentTreeStructureModel,
                &DocumentTreeModel::currentIndexMoved, this, &DocumentController::currentIndexMoved);

        connect(mDocumentsView->thumbnailWidget(), &DocumentThumbnailWidget::sceneDropped, this,
                &DocumentController::moveSceneToIndex);
        connect(mDocumentsView->thumbnailWidget(), &ThumbnailWidget::resized, this,
                &DocumentController::thumbnailViewResized);
        connect(mDocumentsView->thumbnailWidget(), &ThumbnailWidget::mouseDoubleClick, this,
                &DocumentController::thumbnailPageDoubleClicked);
        connect(mDocumentsView->thumbnailWidget(), &ThumbnailWidget::mouseClick, this,
                &DocumentController::pageClicked);

        connect(mDocumentsView->thumbnailWidget()->scene(), &QGraphicsScene::selectionChanged, this,
                &DocumentController::pageSelectionChanged);

        connect(PersistenceManager::persistenceManager(), &PersistenceManager::documentSceneCreated, this,
                &DocumentController::documentSceneChanged);
        connect(PersistenceManager::persistenceManager(), &PersistenceManager::documentSceneWillBeDeleted,
                this, &DocumentController::documentSceneChanged);

        mDocumentsView->thumbnailWidget()->setBackgroundBrush(Settings::documentViewLightColor);

#ifdef Q_WS_MACX
        mMessageWindow = new MessageWindow(NULL);
#else
        mMessageWindow = new MessageWindow(mDocumentsView->thumbnailWidget());
#endif

        mMessageWindow->setCustomPosition(true);
        mMessageWindow->hide();
    }
}

void DocumentController::refreshDateColumns() {
    if (Settings::settings()->documentSortKind->get().toInt() == DocumentController::Alphabetical) {
        if (!Settings::settings()->showDateColumnOnAlphabeticalSort->get().toBool()) {
            mDocumentsView->documentTreeView()->hideColumn(1);
            mDocumentsView->documentTreeView()->hideColumn(2);
        } else {
            mDocumentsView->documentTreeView()->showColumn(1);
            mDocumentsView->documentTreeView()->hideColumn(2);
        }
    }
}

void DocumentController::reorderDocuments() {
    int kindIndex = mDocumentsView->sortKind()->currentIndex();
    int orderIndex =
        mDocumentsView->sortOrder()->isChecked() ? DocumentController::DESC : DocumentController::ASC;

    sortDocuments(kindIndex, orderIndex);
}

void DocumentController::sortDocuments(int kind, int order) {
    Qt::SortOrder sortOrder = Qt::AscendingOrder;
    if (order == DocumentController::DESC)
        sortOrder = Qt::DescendingOrder;

    if (kind == DocumentController::CreationDate) {
        mSortFilterProxyModel->setSortRole(DocumentTreeModel::CreationDate);
        mSortFilterProxyModel->sort(1, sortOrder);
        mDocumentsView->documentTreeView()->showColumn(1);
        mDocumentsView->documentTreeView()->hideColumn(2);
    } else if (kind == DocumentController::UpdateDate) {
        mSortFilterProxyModel->setSortRole(DocumentTreeModel::UpdateDate);
        mSortFilterProxyModel->sort(2, sortOrder);
        mDocumentsView->documentTreeView()->hideColumn(1);
        mDocumentsView->documentTreeView()->showColumn(2);
    } else {
        mSortFilterProxyModel->setSortRole(Qt::DisplayRole);
        mSortFilterProxyModel->sort(0, sortOrder);
        if (!Settings::settings()->showDateColumnOnAlphabeticalSort->get().toBool()) {
            mDocumentsView->documentTreeView()->hideColumn(1);
            mDocumentsView->documentTreeView()->hideColumn(2);
        }
    }
}


void DocumentController::onSortOrderChanged(bool order) {
    int kindIndex = mDocumentsView->sortKind()->currentIndex();
    int orderIndex = order ? DocumentController::DESC : DocumentController::ASC;

    sortDocuments(kindIndex, orderIndex);

    Settings::settings()->documentSortOrder->setInt(orderIndex);
}

void DocumentController::onSortKindChanged(int index) {
    int orderIndex =
        mDocumentsView->sortOrder()->isChecked() ? DocumentController::DESC : DocumentController::ASC;

    sortDocuments(index, orderIndex);

    Settings::settings()->documentSortKind->setInt(index);
}

void DocumentController::onSplitterMoved(int size, int index) {
    Q_UNUSED(index);
    Settings::settings()->documentSplitterLeftSize->setInt(size);
    Settings::settings()->documentSplitterRightSize->setInt(controlView()->size().width() - size);
}

QWidget *DocumentController::controlView() {
    return mDocumentWidget;
}


void DocumentController::setupToolbar() {
    Application::app()->insertSpaceToToolbarBeforeAction(mMainWindow->ui().documentToolBar,
                                                         mMainWindow->ui().actionBoard);
    connect(mMainWindow->ui().actionDocumentTools, &QAction::triggered, this,
            &DocumentController::toggleDocumentToolsPalette);
}

void DocumentController::setupPalettes() {
    mToolsPalette = new DocumentToolsPalette(controlView());

    mToolsPalette->hide();

    bool showToolsPalette = !mToolsPalette->isEmpty();
    mMainWindow->ui().actionDocumentTools->setVisible(showToolsPalette);

    if (showToolsPalette) {
        mMainWindow->ui().actionDocumentTools->trigger();
    }
}

void DocumentController::show() {
    selectDocument(mBoardController->selectedDocument());

    //to be sure thumbnails will be up-to-date
    reloadThumbnails();

    updateActions();

    if (!mToolsPalette)
        setupPalettes();
}


void DocumentController::hide() {
    // NOOP
}


void DocumentController::openSelectedItem() {
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QList<QGraphicsItem *> selectedItems = mDocumentsView->thumbnailWidget()->selectedItems();

    if (selectedItems.count() > 0) {
        SceneThumbnailPixmap *thumb = dynamic_cast<SceneThumbnailPixmap *>(selectedItems.last());

        if (thumb) {
            DocumentProxy *proxy = thumb->proxy();

            if (proxy && isOKToOpenDocument(proxy)) {
                mBoardController->setActiveDocumentScene(proxy, thumb->sceneIndex());
                Application::applicationController->showBoard();
            }
        }
    } else {
        DocumentProxy *proxy = selectedDocumentProxy();

        if (proxy && isOKToOpenDocument(proxy)) {
            mBoardController->setActiveDocumentScene(proxy);
            Application::applicationController->showBoard();
        }
    }

    QApplication::restoreOverrideCursor();
}

void DocumentController::duplicateSelectedItem() {
    if (Application::applicationController->displayMode() != ApplicationController::Document)
        return;

    if (mSelectionType == Page) {
        QList<QGraphicsItem *> selectedItems = mDocumentsView->thumbnailWidget()->selectedItems();
        QList<int> selectedSceneIndexes;
        foreach (QGraphicsItem *item, selectedItems) {
            SceneThumbnailPixmap *thumb = dynamic_cast<SceneThumbnailPixmap *>(item);
            if (thumb) {
                DocumentProxy *proxy = thumb->proxy();

                if (proxy) {
                    int sceneIndex = thumb->sceneIndex();
                    selectedSceneIndexes << sceneIndex;
                }
            }
        }
        if (selectedSceneIndexes.count() > 0) {
            duplicatePages(selectedSceneIndexes);
            emit documentThumbnailsUpdated(this);
            selectedDocument()->setMetaData(Settings::documentUpdatedAt,
                                            StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
            MetadataDcSubsetAdaptor::persist(selectedDocument());
            int selectedThumbnail = selectedSceneIndexes.last() + selectedSceneIndexes.size();
            mDocumentsView->thumbnailWidget()->selectItemAt(selectedThumbnail);
            int sceneCount = selectedSceneIndexes.count();
            showMessage(tr("duplicated %1 page", "duplicated %1 pages", sceneCount).arg(sceneCount), false);

            mBoardController->setActiveDocumentScene(selectedThumbnail);
            mBoardController->reloadThumbnails();
        }
    } else {
        DocumentTreeModel *docModel = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;
        QModelIndex selectedIndex = firstSelectedTreeIndex();

        Q_ASSERT(!docModel->isConstant(selectedIndex) && !docModel->inTrash(selectedIndex));

        showMessage(tr("Duplicating Document %1").arg(""), true);

        docModel->copyIndexToNewParent(selectedIndex, selectedIndex.parent(),
                                       DocumentTreeModel::aContentCopy);

        showMessage(tr("Document %1 copied").arg(""), false);
    }

    emit reorderDocumentsRequested();
}

void DocumentController::deleteSelectedItem() {
    DocumentTreeModel *docModel = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

    if (!Application::mainWindow->yesNoQuestion(tr("Remove Item"),
                                                tr("Are you sure you want to remove the selected item(s) ?")))
        return;

    QModelIndexList indexes = selectedTreeIndexes();

    if (indexes.size() > 1) {
        deleteMultipleItems(indexes, docModel);
    } else if (indexes.size() == 1) {
        deleteSingleItem(indexes.at(0), docModel);
    }

    updateActions();
}

void DocumentController::deleteMultipleItems(QModelIndexList indexes, DocumentTreeModel *docModel) {
    DeletionType deletionForSelection = deletionTypeForSelection(mSelectionType, indexes.at(0), docModel);

    switch (deletionForSelection) {
        case DeletePage: {
            deletePages(mDocumentsView->thumbnailWidget()->selectedItems());
            break;
        }
        case MoveToTrash: {
            moveIndexesToTrash(indexes, docModel);
            break;
        }
        case CompleteDelete: {
            for (int i = 0; i < indexes.size(); i++) {
                DocumentTreeModel *docModel =
                    PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;
                DocumentProxy *newProxy = docModel->proxyData(indexes.at(i));
                if (newProxy) {
                    pureSetDocument(newProxy);
                }
                emit documentThumbnailsUpdated(this);

                deleteIndexAndAssociatedData(indexes.at(i));
            }
            break;
        }
        case EmptyFolder: {
            for (int i = 0; i < indexes.size(); i++) {
                QModelIndex currentIndex = indexes.at(i);
                if (currentIndex ==
                    docModel->myDocumentsIndex()) { //清空“我的文档”：所有条目移入回收站，随后补一个空白文档
                    int startInd = 0;
                    while (docModel->rowCount(currentIndex)) {
                        QModelIndex testSubINdecurrentIndex = docModel->index(startInd, 0, currentIndex);
                        if (!testSubINdecurrentIndex.isValid()) {
                            break;
                        }
                        docModel->moveIndex(testSubINdecurrentIndex, docModel->trashIndex());
                    }
                    //Here, we are sure that the current scene has been deleted
                    createEmptyDocument();
                } else {
                    //Check if we will delete the current scene
                    DocumentTreeNode *currentNode = docModel->nodeFromIndex(currentIndex);
                    bool deleteCurrentScene =
                        currentNode->findNode(docModel->nodeFromIndex(docModel->currentIndex()));

                    emptyFolder(currentIndex, MoveToTrash); //Empty constant folder

                    if (deleteCurrentScene)
                        createEmptyDocument();
                }
            }

            break;
        }
        case EmptyTrash: {
            for (int i = 0; i < indexes.size(); i++) {
                emptyFolder(indexes.at(i), CompleteDelete); // Empty trash folder
            }

            break;
        }
    }
}

void DocumentController::deleteSingleItem(QModelIndex currentIndex, DocumentTreeModel *docModel) {
    DeletionType deletionForSelection = deletionTypeForSelection(mSelectionType, currentIndex, docModel);

    switch (deletionForSelection) {
        case DeletePage: {
            deletePages(mDocumentsView->thumbnailWidget()->selectedItems());
            break;
        }
        case MoveToTrash: {
            moveToTrash(currentIndex, docModel);
            break;
        }
        case CompleteDelete: {
            DocumentTreeModel *docModel =
                PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;
            DocumentProxy *newProxy = docModel->proxyData(currentIndex);
            if (newProxy) {
                pureSetDocument(newProxy);
            }
            emit documentThumbnailsUpdated(this);
            deleteIndexAndAssociatedData(currentIndex);

            break;
        }
        case EmptyFolder: {
            if (currentIndex ==
                docModel->myDocumentsIndex()) { //清空“我的文档”：所有条目移入回收站，随后补一个空白文档
                int startInd = 0;
                while (docModel->rowCount(currentIndex)) {
                    QModelIndex testSubINdecurrentIndex = docModel->index(startInd, 0, currentIndex);
                    if (!testSubINdecurrentIndex.isValid()) {
                        break;
                    }
                    docModel->moveIndex(testSubINdecurrentIndex, docModel->trashIndex());
                }
                //Here, we are sure that the current scene has been deleted
                createEmptyDocument();
            } else {
                //Check if we will delete the current scene
                DocumentTreeNode *currentNode = docModel->nodeFromIndex(currentIndex);
                bool deleteCurrentScene =
                    currentNode->findNode(docModel->nodeFromIndex(docModel->currentIndex()));

                emptyFolder(currentIndex, MoveToTrash); //Empty constant folder

                if (deleteCurrentScene)
                    createEmptyDocument();
            }

            break;
        }
        case EmptyTrash: {
            emptyFolder(currentIndex, CompleteDelete); // Empty trash folder
            break;
        }
    }
}

void DocumentController::moveIndexesToTrash(const QModelIndexList &list, DocumentTreeModel *docModel) {
    QModelIndex currentScene = docModel->indexForNode(docModel->currentNode());

    //check if the current scene is selected
    QItemSelectionModel *selectionModel = mDocumentsView->documentTreeView()->selectionModel();
    bool deleteCurrentScene = selectionModel->isSelected(mSortFilterProxyModel->mapFromSource(currentScene));

    //check if the current scene is in the hierarchy
    if (!deleteCurrentScene) {
        for (int i = 0; i < list.size(); i++) {
            deleteCurrentScene = docModel->isDescendantOf(currentScene, list.at(i));

            if (deleteCurrentScene) {
                break;
            }
        }
    }

    QModelIndex proxyMapCurentScene = mSortFilterProxyModel->mapFromSource(currentScene);

    if (deleteCurrentScene) {
        QModelIndex sibling = findPreviousSiblingNotSelected(proxyMapCurentScene, selectionModel);

        if (sibling.isValid()) {
            QModelIndex sourceSibling = mSortFilterProxyModel->mapToSource(sibling);

            DocumentProxy *proxy = docModel->proxyForIndex(sourceSibling);

            if (proxy) {
                selectDocument(proxy, true);

                deleteCurrentScene = false;
            }
        } else {
            sibling = findNextSiblingNotSelected(proxyMapCurentScene, selectionModel);

            if (sibling.isValid()) {
                QModelIndex sourceSibling = mSortFilterProxyModel->mapToSource(sibling);

                DocumentProxy *proxy = docModel->proxyForIndex(sourceSibling);

                if (proxy) {
                    selectDocument(proxy, true);

                    deleteCurrentScene = false;
                }
            }
        }
    } else {
        DocumentProxy *proxy = docModel->proxyForIndex(currentScene);
        selectDocument(proxy, true);
    }

    docModel->moveIndexes(list, docModel->trashIndex());

    if (deleteCurrentScene) {
        createEmptyDocument();
    }

    //selectionModel->clearSelection();
}

QModelIndex DocumentController::findPreviousSiblingNotSelected(const QModelIndex &index,
                                                               QItemSelectionModel *selectionModel) {
    QModelIndex sibling = index.sibling(index.row() - 1, 0);

    if (sibling.isValid()) {
        if (!parentIsSelected(sibling, selectionModel) && !selectionModel->isSelected(sibling)) {
            QModelIndex model = mSortFilterProxyModel->mapToSource(sibling);

            if (PersistenceManager::persistenceManager()->mDocumentTreeStructureModel->isCatalog(model)) {
                return findPreviousSiblingNotSelected(sibling, selectionModel);
            } else {
                return sibling;
            }
        } else {
            return findPreviousSiblingNotSelected(sibling, selectionModel);
        }
    } else {
        //if the parent exist keep searching, else stop the search
        QModelIndex parent = index.model()->parent(index);

        if (parent.isValid()) {
            return findPreviousSiblingNotSelected(parent, selectionModel);
        } else {
            return QModelIndex();
        }
    }
}

QModelIndex DocumentController::findNextSiblingNotSelected(const QModelIndex &index,
                                                           QItemSelectionModel *selectionModel) {
    QModelIndex sibling = index.sibling(index.row() + 1, 0);

    if (sibling.isValid()) {
        if (!parentIsSelected(sibling, selectionModel) && !selectionModel->isSelected(sibling)) {
            QModelIndex model = mSortFilterProxyModel->mapToSource(sibling);

            if (PersistenceManager::persistenceManager()->mDocumentTreeStructureModel->isCatalog(model)) {
                return findNextSiblingNotSelected(sibling, selectionModel);
            } else {
                return sibling;
            }
        } else {
            return findNextSiblingNotSelected(sibling, selectionModel);
        }
    } else {
        //if the parent exist keep searching, else stop the search
        QModelIndex parent = index.parent();

        if (parent.isValid()) {
            return findNextSiblingNotSelected(parent, selectionModel);
        } else {
            return QModelIndex();
        }
    }
}

bool DocumentController::parentIsSelected(const QModelIndex &child, QItemSelectionModel *selectionModel) {
    QModelIndex parent = child.parent();

    while (parent.isValid()) {
        if (selectionModel->isSelected(parent)) {
            return true;
        }

        parent = parent.parent();
    }

    return false;
}

void DocumentController::moveToTrash(QModelIndex &index, DocumentTreeModel *docModel) {
    QModelIndexList list;
    list.push_back(index);
    moveIndexesToTrash(list, docModel);
}

void DocumentController::deleteDocumentsInFolderOlderThan(const QModelIndex &index, const int days) {
    DocumentTreeModel *docModel = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

    QModelIndexList list;
    for (int i = 0; i < docModel->rowCount(index); i++) {
        list << docModel->index(i, 0, index);
    }

    foreach (QModelIndex child, list) {
        DocumentProxy *documentProxy = docModel->proxyForIndex(child);

        if (documentProxy) {
            if (documentProxy->lastUpdate().date() < QDateTime::currentDateTime().addDays(-days).date()) {
                PersistenceManager::persistenceManager()->deleteDocument(documentProxy);
            }
        } else {
            if (docModel->isCatalog(child)) {
                deleteDocumentsInFolderOlderThan(child, days);
            }
        }
    }
}

void DocumentController::deleteEmptyFolders(const QModelIndex &index) {
    DocumentTreeModel *docModel = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

    QModelIndexList list;
    for (int i = 0; i < docModel->rowCount(index); i++) {
        list << docModel->index(i, 0, index);
    }

    if (list.length() > 0) {
        foreach (QModelIndex child, list) {
            if (docModel->isCatalog(child)) {
                if (!docModel->containsDocuments(child)) {
                    deleteIndexAndAssociatedData(child);
                }
            }
        }
    } else {
        if (docModel->isCatalog(index)) {
            deleteIndexAndAssociatedData(index);
        }
    }
}

void DocumentController::emptyFolder(const QModelIndex &index, DeletionType pDeletionType) {
    // Issue NC - CFA - 20131029 : ajout d'une popup de confirmation pour la suppression definitive
    //    if(pDeletionType == CompleteDelete && !Application::mainWindow->yesNoQuestion(tr("Empty the trash"),tr("You're about to empty the trash.") +"\n\n" + tr("Are you sure ?")))
    //        return;

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    DocumentTreeModel *docModel = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;
    if (!docModel->isCatalog(index)) {
        return;
    }
    while (docModel->rowCount(index)) {
        QModelIndex subIndex = docModel->index(0, 0, index);
        switch (pDeletionType) {
            case MoveToTrash:
                docModel->moveIndex(subIndex, docModel->trashIndex());
                break;

            case CompleteDelete:
                deleteIndexAndAssociatedData(subIndex);
                break;
            default:
                break;
        }
    }

    QApplication::restoreOverrideCursor();
    // Fin issue NC - CFA - 20131029
}

void DocumentController::deleteIndexAndAssociatedData(const QModelIndex &pIndex) {
    DocumentTreeModel *docModel = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;
    while (docModel->rowCount(pIndex)) {
        QModelIndex subIndex = docModel->index(0, 0, pIndex);
        deleteIndexAndAssociatedData(subIndex);
    }

    if (pIndex.column() == 0) {
        if (docModel->isDocument(pIndex)) {
            DocumentProxy *proxyData = docModel->proxyData(pIndex);

            if (proxyData) {
                PersistenceManager::persistenceManager()->deleteDocument(proxyData);
            }
        }
    }

    docModel->removeRow(pIndex.row(), pIndex.parent());
}

void DocumentController::exportDocument() {
    QAction *currentExportAction = qobject_cast<QAction *>(sender());
    QVariant actionData = currentExportAction->data();
    ExportAdaptor *selectedExportAdaptor =
        DocumentManager::documentManager()->supportedExportAdaptors()[actionData.toInt()];

    DocumentProxy *proxy = firstSelectedTreeProxy();

    selectedExportAdaptor->persist(proxy);
    emit exportDone();
}

void DocumentController::exportDocumentSet() {}

void DocumentController::documentZoomSliderValueChanged(int value) {
    mDocumentsView->thumbnailWidget()->setThumbnailWidth(value);

    Settings::settings()->documentThumbnailWidth->set(value);
}

void DocumentController::importFile() {
    DocumentManager *docManager = DocumentManager::documentManager();

    QString defaultPath = Settings::settings()->lastImportFilePath->get().toString();
    if (defaultPath.isDetached())
        defaultPath = Settings::settings()->userDocumentDirectory();
    QString filePath = QFileDialog::getOpenFileName(mParentWidget, tr("Open Supported File"), defaultPath,
                                                    docManager->importFileFilter());

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    QApplication::processEvents();
    QFileInfo fileInfo(filePath);

    if (fileInfo.suffix().toLower() == "tbx") {
        PersistenceManager::persistenceManager()->createDocumentProxiesStructure(
            docManager->importUbx(filePath, Settings::userDocumentDirectory()), true);

    } else {
        Settings::settings()->lastImportFilePath->set(QVariant(fileInfo.absolutePath()));

        if (filePath.length() > 0) {
            DocumentProxy *createdDocument = 0;
            QApplication::processEvents();
            QFile selectedFile(filePath);

            DocumentTreeModel *docModel =
                PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

            QModelIndex selectedIndex = firstSelectedTreeIndex();
            QString groupName = "";
            if (selectedIndex.isValid()) {
                groupName = docModel->isCatalog(selectedIndex) ? docModel->virtualPathForIndex(selectedIndex)
                                                               : docModel->virtualDirForIndex(selectedIndex);
            }

            showMessage(tr("Importing file %1...").arg(fileInfo.baseName()), true);

            createdDocument = docManager->importFile(selectedFile, groupName);

            if (createdDocument) {
                selectDocument(createdDocument, true, true, true);

            } else {
                showMessage(tr("Failed to import file ... "));
            }
        }
    }

    QApplication::restoreOverrideCursor();
}

void DocumentController::addFolderOfImages() {
    DocumentProxy *document = selectedDocumentProxy();

    if (document) {
        QString defaultPath = Settings::settings()->lastImportFolderPath->get().toString();

        QString imagesDir = QFileDialog::getExistingDirectory(
            mParentWidget, tr("Import all Images from Folder"), defaultPath);
        QDir parentImageDir(imagesDir);
        parentImageDir.cdUp();

        Settings::settings()->lastImportFolderPath->set(QVariant(parentImageDir.absolutePath()));

        if (imagesDir.length() > 0) {
            QDir dir(imagesDir);

            int importedImageNumber =
                DocumentManager::documentManager()->addImageDirToDocument(dir, document);

            if (importedImageNumber == 0) {
                showMessage(tr("Folder does not contain any image files"));
                Application::applicationController->showDocument();
            } else {
                document->setMetaData(Settings::documentUpdatedAt,
                                      StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
                MetadataDcSubsetAdaptor::persist(document);
                reloadThumbnails();
            }
        }
    }
}


void DocumentController::addFileToDocument() {
    DocumentProxy *document = selectedDocumentProxy();

    if (document) {
        addFileToDocument(document);
        reloadThumbnails();
    }
}

DocumentProxyTreeItem *DocumentController::findDocument(DocumentProxy *proxy) {
    selectDocument(proxy);
    //DocumentProxyTreeItem* ppp = dynamic_cast<DocumentProxyTreeItem*>(proxy);
    return NULL;
}

bool DocumentController::addFileToDocument(DocumentProxy *document) {
    QString defaultPath = Settings::settings()->lastImportFilePath->get().toString();
    QString filePath =
        QFileDialog::getOpenFileName(mParentWidget, tr("Open Supported File"), defaultPath,
                                     DocumentManager::documentManager()->importFileFilter(true));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    QApplication::processEvents();

    QFileInfo fileInfo(filePath);
    Settings::settings()->lastImportFilePath->set(QVariant(fileInfo.absolutePath()));

    bool success = false;

    if (filePath.length() > 0) {
        QApplication::
            processEvents(); // NOTE: We performed this just a few lines before. Is it really necessary to do it again here??
        QFile selectedFile(filePath);

        showMessage(tr("Importing file %1...").arg(fileInfo.baseName()), true);

        QStringList fileNames;
        fileNames << filePath;
        success = DocumentManager::documentManager()->addFilesToDocument(document, fileNames);

        if (success) {
            document->setMetaData(Settings::documentUpdatedAt,
                                  StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
            MetadataDcSubsetAdaptor::persist(document);
        } else {
            showMessage(tr("Failed to import file ... "));
        }
    }

    QApplication::restoreOverrideCursor();

    return success;
}


void DocumentController::moveSceneToIndex(DocumentProxy *proxy, int source, int target) {
    if (DocumentContainer::movePageToIndex(source, target)) {
        proxy->setMetaData(Settings::documentUpdatedAt,
                           StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
        MetadataDcSubsetAdaptor::persist(proxy);

        mDocumentsView->thumbnailWidget()->hightlightItem(target);

        mBoardController->setActiveDocumentScene(target);
        mBoardController->reloadThumbnails();
    }
}


void DocumentController::thumbnailViewResized() {
    int maxWidth = qMin(Settings::maxThumbnailWidth, mDocumentsView->thumbnailWidget()->width());

    mDocumentsView->documentZoomSlider()->setMaximum(maxWidth);
}


void DocumentController::pageSelectionChanged() {
    if (mIsClosing)
        return;

    DocumentTreeModel *docModel = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

    bool pageSelected = mDocumentsView->thumbnailWidget()->selectedItems().count() > 0;
    bool docSelected = docModel->isDocument(firstSelectedTreeIndex());
    bool folderSelected = docModel->isCatalog(firstSelectedTreeIndex());

    if (pageSelected)
        itemSelectionChanged(Page);
    else if (docSelected)
        itemSelectionChanged(Document);
    else if (folderSelected)
        itemSelectionChanged(Folder);
    else
        itemSelectionChanged(None);

    updateActions();
}

void DocumentController::documentSceneChanged(DocumentProxy *proxy, int pSceneIndex) {
    Q_UNUSED(pSceneIndex);

    if (proxy == selectedDocumentProxy()) {
        reloadThumbnails();
    }

    QModelIndexList sel = mDocumentsView->documentTreeView()->selectionModel()->selectedRows(0);

    QModelIndex selection;
    if (sel.count() > 0) {
        selection = sel.first();
    }

    TreeViewSelectionChanged(selection, QModelIndex());
}

void DocumentController::thumbnailPageDoubleClicked(QGraphicsItem *item, int index) {
    DocumentTreeModel *docModel = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;
    QModelIndex selectedIndex = firstSelectedTreeIndex();

    if (selectedIndex.isValid()) {
        if (docModel->inTrash(selectedIndex)) {
            return;
        }
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    SceneThumbnailPixmap *thumb = qgraphicsitem_cast<SceneThumbnailPixmap *>(item);

    if (thumb) {
        DocumentProxy *proxy = thumb->proxy();
        if (proxy && isOKToOpenDocument(proxy)) {
            mBoardController->setActiveDocumentScene(proxy, index);
            Application::applicationController->showBoard();
        }
    }

    QApplication::restoreOverrideCursor();
}


void DocumentController::pageClicked(QGraphicsItem *item, int index) {
    Q_UNUSED(item);
    Q_UNUSED(index);

    pageSelectionChanged();
}


void DocumentController::addToDocument() {
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QList<QGraphicsItem *> selectedItems = mDocumentsView->thumbnailWidget()->selectedItems();

    if (selectedItems.count() > 0) {
        int oldActiveSceneIndex = mBoardController->activeSceneIndex();

        QList<QPair<DocumentProxy *, int>> pageInfoList;

        foreach (QGraphicsItem *item, selectedItems) {
            SceneThumbnailPixmap *thumb = dynamic_cast<SceneThumbnailPixmap *>(item);

            if (thumb && thumb->proxy()) {
                QPair<DocumentProxy *, int> pageInfo(thumb->proxy(), thumb->sceneIndex());
                pageInfoList << pageInfo;
            }
        }

        for (int i = 0; i < pageInfoList.length(); i++) {
            mBoardController->addScene(pageInfoList.at(i).first, pageInfoList.at(i).second, false);
        }

        int newActiveSceneIndex = selectedItems.count() == mBoardController->selectedDocument()->pageCount()
                                      ? 0
                                      : oldActiveSceneIndex + 1;
        mDocumentsView->thumbnailWidget()->selectItemAt(newActiveSceneIndex, false);
        selectDocument(mBoardController->selectedDocument());
        mBoardController->selectedDocument()->setMetaData(
            Settings::documentUpdatedAt, StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
        MetadataDcSubsetAdaptor::persist(mBoardController->selectedDocument());
        mBoardController->reloadThumbnails();

        emit Application::boardController->documentThumbnailsUpdated(this);
        Application::applicationController->showBoard();

        mBoardController->setActiveDocumentScene(newActiveSceneIndex);
    }

    QApplication::restoreOverrideCursor();
}

void DocumentController::renameSelectedItem() {
    if (mDocumentsView->documentTreeView()->currentIndex().isValid()) {
        mDocumentsView->documentTreeView()->edit(mDocumentsView->documentTreeView()->currentIndex());
    }
}

bool DocumentController::isOKToOpenDocument(DocumentProxy *proxy) {
    //check version
    QString docVersion = proxy->metaData(Settings::documentVersion).toString();

    // 这里曾经写死一份 "1.1"/"1.2" 白名单，而读写器早就在往每个文档盖 4.8.0，
    // 结果自己存的文档也被判成“比本程序还新”，每次打开都弹一次丢失警告。
    // 现在拿真正的版本序比：只有确实更新于当前格式版本时才问。
    if (docVersion.isEmpty() || !(Version(docVersion) > Version(Settings::currentFileVersion))) {
        return true;
    } else {
        if (Application::mainWindow->yesNoQuestion(
                tr("Open Document"),
                tr("The document '%1' has been generated with a newer version of TianBoard (%2). By opening "
                   "it, you may lose some information. Do you want to proceed?")
                    .arg(proxy->metaData(Settings::documentName).toString())
                    .arg(docVersion))) {
            return true;
        } else {
            return false;
        }
    }
}


void DocumentController::showMessage(const QString &message, bool showSpinningWheel) {
    if (mMessageWindow) {
        int margin = Settings::boardMargin;

        QRect newSize = mDocumentsView->thumbnailWidget()->geometry();

#ifdef Q_WS_MACX
        QPoint point(newSize.left() + margin, newSize.bottom() - mMessageWindow->height() - margin);
        mMessageWindow->move(mDocumentsView->thumbnailWidget()->mapToGlobal(point));
#else
        mMessageWindow->move(margin, newSize.height() - mMessageWindow->height() - margin);
#endif

        mMessageWindow->showMessage(message, showSpinningWheel);
    }
}


void DocumentController::hideMessage() {
    if (mMessageWindow)
        mMessageWindow->hideMessage();
}


void DocumentController::addImages() {
    DocumentProxy *document = selectedDocumentProxy();

    if (document) {
        QString defaultPath = Settings::settings()->lastImportFolderPath->get().toString();

        QString extensions;

        foreach (QString ext, Settings::settings()->imageFileExtensions) {
            extensions += " *.";
            extensions += ext;
        }

        QStringList images =
            QFileDialog::getOpenFileNames(mParentWidget, tr("Add all Images to Document"), defaultPath,
                                          tr("All Images (%1)").arg(extensions));

        if (images.length() > 0) {
            QFileInfo firstImage(images.at(0));

            Settings::settings()->lastImportFolderPath->set(
                QVariant(firstImage.absoluteDir().absolutePath()));

            int importedImageNumber =
                DocumentManager::documentManager()->addFilesToDocument(document, images);

            if (importedImageNumber == 0) {
                Application::showMessage(tr("Selection does not contain any image files!"));
                Application::applicationController->showDocument();
            } else {
                document->setMetaData(Settings::documentUpdatedAt,
                                      StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
                MetadataDcSubsetAdaptor::persist(document);
                reloadThumbnails();
            }
        }
    }
}

void DocumentController::toggleDocumentToolsPalette() {
    if (!mToolsPalette->isVisible() && !mToolsPalettePositionned) {
        mToolsPalette->adjustSizeAndPosition();
        int left = controlView()->width() - 20 - mToolsPalette->width();
        int top = (controlView()->height() - mToolsPalette->height()) / 2;

        mToolsPalette->setCustomPosition(true);
        mToolsPalette->move(left, top);

        mToolsPalettePositionned = true;
    }

    bool visible = mToolsPalette->isVisible();
    mToolsPalette->setVisible(!visible);
}


void DocumentController::cut() {
    // TODO - implemented me
}


void DocumentController::copy() {
    // TODO - implemented me
}


void DocumentController::paste() {
    // TODO - implemented me
}


void DocumentController::focusChanged(QWidget *old, QWidget *current) {
    DocumentTreeModel *treeModel = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

    if (current == mDocumentsView->thumbnailWidget()) {
        if (mDocumentsView->thumbnailWidget()->selectedItems().count() > 0)
            mSelectionType = Page;
        else
            mSelectionType = None;
    } else if (current == mDocumentsView->documentTreeView()) {
        if (treeModel->isDocument(firstSelectedTreeIndex()))
            mSelectionType = Document;
        else if (treeModel->isCatalog(firstSelectedTreeIndex()))
            mSelectionType = Folder;
        else
            mSelectionType = None;
    } else if (current == mDocumentsView->documentZoomSlider()) {
        if (mDocumentsView->thumbnailWidget()->selectedItems().count() > 0)
            mSelectionType = Page;
        else
            mSelectionType = None;
    } else {
        if (old != mDocumentsView->thumbnailWidget() && old != mDocumentsView->documentTreeView() &&
            old != mDocumentsView->documentZoomSlider()) {
            if (current && (current->metaObject()->className() != QPushButton::staticMetaObject.className()))
                mSelectionType = None;
        }
    }
}

void DocumentController::updateActions() {
    if (mIsClosing)
        return;

    DocumentTreeModel *docModel = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

    QModelIndexList list = mDocumentsView->documentTreeView()->selectionModel()->selectedRows(0);

    //if in multi selection, juste activate the actionDelete
    if (list.count() > 1) {
        mMainWindow->ui().actionNewDocument->setEnabled(false);
        mMainWindow->ui().actionNewFolder->setEnabled(false);
        mMainWindow->ui().actionExport->setEnabled(false);
        mMainWindow->ui().actionDuplicate->setEnabled(false);
        mMainWindow->ui().actionOpen->setEnabled(false);
        mMainWindow->ui().actionRename->setEnabled(false);

        mMainWindow->ui().actionAddToWorkingDocument->setEnabled(false);
        mMainWindow->ui().actionDocumentAdd->setEnabled(false);
        mMainWindow->ui().actionImport->setEnabled(false);
        mMainWindow->ui().actionDelete->setEnabled(true);

        return;
    }

#ifdef Q_OS_OSX
    mMainWindow->ui().actionDelete->setShortcut(QKeySequence(Qt::Key_Backspace));
#else
    mMainWindow->ui().actionDelete->setShortcut(QKeySequence(Qt::Key_Delete));
#endif

    QModelIndex selectedIndex = firstSelectedTreeIndex();
    DocumentProxy *selectedProxy = docModel->proxyData(selectedIndex);
    int pageCount = -1;
    if (selectedProxy) {
        pageCount = selectedProxy->pageCount();
    }

    bool pageSelected = false;
    bool groupSelected = false;
    bool docSelected = false;

    if (mSelectionType == Page) {
        pageSelected = true;
    } else {
        if (docModel->isDocument(firstSelectedTreeIndex())) {
            docSelected = true;
        } else if (docModel->isCatalog(firstSelectedTreeIndex())) {
            groupSelected = true;
        }
    }

    bool trashSelected =
        docModel->inTrash(selectedIndex) || selectedIndex == docModel->trashIndex() ? true : false;

    mMainWindow->ui().actionNewDocument->setEnabled(docModel->newNodeAllowed(selectedIndex));
    mMainWindow->ui().actionNewFolder->setEnabled(docModel->newNodeAllowed(selectedIndex));
    mMainWindow->ui().actionExport->setEnabled((docSelected || pageSelected || groupSelected) &&
                                               !trashSelected);
    updateExportSubActions(selectedIndex);

    bool firstSceneSelected = false;

    if (docSelected) {
        mMainWindow->ui().actionDuplicate->setEnabled(!trashSelected);

    } else if (pageSelected) {
        QList<QGraphicsItem *> selection = mDocumentsView->thumbnailWidget()->selectedItems();
        if (pageCount == 1) {
            mMainWindow->ui().actionDuplicate->setEnabled(
                !trashSelected && pageCanBeDuplicated(DocumentContainer::pageFromSceneIndex(0)));

        } else {
            for (int i = 0; i < selection.count() && !firstSceneSelected; i += 1) {
                if (qgraphicsitem_cast<SceneThumbnailPixmap *>(selection.at(i))->sceneIndex() == 0) {
                    mMainWindow->ui().actionDuplicate->setEnabled(
                        !trashSelected && pageCanBeDuplicated(DocumentContainer::pageFromSceneIndex(0)));
                    firstSceneSelected = true;
                    break;
                }
            }
            if (!firstSceneSelected) {
                mMainWindow->ui().actionDuplicate->setEnabled(!trashSelected);
            }
        }

    } else {
        mMainWindow->ui().actionDuplicate->setEnabled(false);
    }

    mMainWindow->ui().actionOpen->setEnabled((docSelected || pageSelected) && !trashSelected);
    mMainWindow->ui().actionRename->setEnabled(docModel->isOkToRename(selectedIndex));

    mMainWindow->ui().actionAddToWorkingDocument->setEnabled(
        pageSelected && !(selectedProxy == mBoardController->selectedDocument()) && !trashSelected);

    DeletionType deletionForSelection = deletionTypeForSelection(mSelectionType, selectedIndex, docModel);
    mMainWindow->ui().actionDelete->setEnabled(deletionForSelection != NoDeletion);

    switch (static_cast<int>(deletionForSelection)) {
        case MoveToTrash:
        case DeletePage:
            mMainWindow->ui().actionDelete->setIcon(QIcon(":/images/trash.png"));
            mMainWindow->ui().actionDelete->setText(tr("Trash"));
            break;
        case CompleteDelete:
            mMainWindow->ui().actionDelete->setIcon(QIcon(":/images/toolbar/deleteDocument.png"));
            mMainWindow->ui().actionDelete->setText(tr("Delete"));
            break;
        case EmptyFolder:
            mMainWindow->ui().actionDelete->setIcon(QIcon(":/images/trash.png"));
            mMainWindow->ui().actionDelete->setText(tr("Empty"));
            break;
        case EmptyTrash:
            mMainWindow->ui().actionDelete->setIcon(QIcon(":/images/toolbar/deleteDocument.png"));
            mMainWindow->ui().actionDelete->setText(tr("Empty"));
            break;
    }

    mMainWindow->ui().actionDocumentAdd->setEnabled((docSelected || pageSelected) && !trashSelected);
    mMainWindow->ui().actionImport->setEnabled(!trashSelected);
}

void DocumentController::updateExportSubActions(const QModelIndex &selectedIndex) {
    DocumentManager *documentManager = DocumentManager::documentManager();
    for (int i = 0; i < documentManager->supportedExportAdaptors().length(); i++) {
        ExportAdaptor *adaptor = documentManager->supportedExportAdaptors()[i];
        if (adaptor->associatedAction()) {
            adaptor->associatedAction()->setEnabled(
                adaptor->associatedActionactionAvailableFor(selectedIndex));
        }
    }
}

void DocumentController::currentIndexMoved(const QModelIndex &newIndex, const QModelIndex &PreviousIndex) {
    Q_UNUSED(newIndex);
    Q_UNUSED(PreviousIndex);

    DocumentTreeModel *docModel = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;
    DocumentProxy *newProxy = docModel->proxyData(newIndex);
    if (newProxy) {
        DocumentProxy *cp =
            new DocumentProxy(*newProxy); // we cannot use newProxy because it will be destroyed later
        pureSetDocument(cp);
        mBoardController->pureSetDocument(cp);
        mBoardController->pureSetDocument(newProxy);
    }
    mCurrentIndexMoved = true;
}

void DocumentController::deletePages(QList<QGraphicsItem *> itemsToDelete) {
    if (itemsToDelete.count() > 0) {
        QList<int> sceneIndexes;
        DocumentProxy *proxy = 0;

        foreach (QGraphicsItem *item, itemsToDelete) {
            SceneThumbnailPixmap *thumb = dynamic_cast<SceneThumbnailPixmap *>(item);

            if (thumb) {
                proxy = thumb->proxy();
                if (proxy) {
                    sceneIndexes.append(thumb->sceneIndex());
                }
            }
        }
        DocumentContainer::deletePages(sceneIndexes);
        emit Application::boardController->documentThumbnailsUpdated(this);

        proxy->setMetaData(Settings::documentUpdatedAt,
                           StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
        MetadataDcSubsetAdaptor::persist(proxy);

        int minIndex = proxy->pageCount() - 1;
        foreach (int i, sceneIndexes)
            minIndex = qMin(i, minIndex);

        if (mBoardController->activeSceneIndex() > minIndex) {
            mBoardController->setActiveSceneIndex(minIndex);
        }

        mDocumentsView->thumbnailWidget()->selectItemAt(minIndex);

        mBoardController->setActiveDocumentScene(minIndex);
        mBoardController->reloadThumbnails();
    }
}

int DocumentController::getSelectedItemIndex() {
    QList<QGraphicsItem *> selectedItems = mDocumentsView->thumbnailWidget()->selectedItems();

    if (selectedItems.count() > 0) {
        SceneThumbnailPixmap *thumb = dynamic_cast<SceneThumbnailPixmap *>(selectedItems.last());
        return thumb->sceneIndex();
    } else
        return -1;
}

bool DocumentController::pageCanBeMovedUp(int page) {
    return page >= 1;
}

bool DocumentController::pageCanBeMovedDown(int page) {
    return page < selectedDocument()->pageCount() - 1;
}

bool DocumentController::pageCanBeDuplicated(int page) {
    return page != 0;
}

bool DocumentController::pageCanBeDeleted(int page) {
    return page != 0;
}

void DocumentController::setDocument(DocumentProxy *document, bool forceReload) {
    DocumentContainer::setDocument(document, forceReload);
}

QModelIndex DocumentController::firstSelectedTreeIndex() {
    return selectedTreeIndexes().count() ? selectedTreeIndexes().first() : QModelIndex();
}

DocumentController::DeletionType
DocumentController::deletionTypeForSelection(LastSelectedElementType pTypeSelection,
                                             const QModelIndex &selectedIndex,
                                             DocumentTreeModel *docModel) const {
    if (pTypeSelection == Page) {
        if (!firstAndOnlySceneSelected()) {
            return DeletePage;
        }
    } else if (docModel->isConstant(selectedIndex)) {
        if (selectedIndex == docModel->trashIndex()) {
            if (docModel->rowCount(selectedIndex) > 0)
                return EmptyTrash;
            else
                return NoDeletion;
        }

        if (selectedIndex.isValid())
            return EmptyFolder;
        else
            return NoDeletion;
    } else if (pTypeSelection != None) {
        if (docModel->inTrash(selectedIndex)) {
            return CompleteDelete;
        } else {
            return MoveToTrash;
        }
    }

    return NoDeletion;
}

bool DocumentController::firstAndOnlySceneSelected() const {
    bool firstSceneSelected = false;
    QList<QGraphicsItem *> selection = mDocumentsView->thumbnailWidget()->selectedItems();
    for (int i = 0; i < selection.count() && !firstSceneSelected; i += 1) {
        SceneThumbnailPixmap *p = dynamic_cast<SceneThumbnailPixmap *>(selection.at(i));
        if (p) {
            int pageCount = p->proxy()->pageCount();
            if (pageCount > 1) //not the only scene
            {
                return false;
            } else {
                if (p->sceneIndex() == 0) {
                    return true; //the first and only scene
                }
            }
        }
    }

    return false;
}

void DocumentController::refreshDocumentThumbnailsView(DocumentContainer *) {
    DocumentTreeModel *docModel = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;
    DocumentProxy *currentDocumentProxy = selectedDocument();

    QModelIndex current = docModel->indexForProxy(currentDocumentProxy);

    if (!current.isValid()) {
        mDocumentsView->thumbnailWidget()->setGraphicsItems(QList<QGraphicsItem *>(), QList<QUrl>(),
                                                            QStringList(), Application::mimeTypeBoardPage);
        return;
    }

    QList<const QPixmap *> thumbs;

    if (currentDocumentProxy) {
        ThumbnailAdaptor::load(currentDocumentProxy, thumbs);
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QList<QGraphicsItem *> items;
    QList<QUrl> itemsPath;

    QGraphicsPixmapItem *selection = 0;

    QStringList labels;

    if (currentDocumentProxy) {
        for (int i = 0; i < currentDocumentProxy->pageCount(); i++) {
            const QPixmap *pix = thumbs.at(i);
            QGraphicsPixmapItem *pixmapItem =
                new SceneThumbnailPixmap(*pix, currentDocumentProxy, i); // deleted by the tree widget

            if (currentDocumentProxy == mBoardController->selectedDocument() &&
                mBoardController->activeSceneIndex() == i) {
                selection = pixmapItem;
            }

            items << pixmapItem;
            int pageIndex = pageFromSceneIndex(i);
            if (pageIndex)
                labels << tr("Page %1").arg(pageIndex);
            else
                labels << tr("Title page");

            itemsPath.append(
                QUrl::fromLocalFile(currentDocumentProxy->persistencePath() +
                                    QString("/pages/%1").arg(DocumentContainer::pageFromSceneIndex(i))));
        }
    }

    mDocumentsView->thumbnailWidget()->setGraphicsItems(items, itemsPath, labels,
                                                        Application::mimeTypeBoardPage);

    if (docModel->inTrash(current)) {
        mDocumentsView->thumbnailWidget()->setDragEnabled(false);
    } else {
        mDocumentsView->thumbnailWidget()->setDragEnabled(true);
    }

    mDocumentsView->thumbnailWidget()->ensureVisible(0, 0, 10, 10);

    if (selection) {
        disconnect(mDocumentsView->thumbnailWidget()->scene(), &QGraphicsScene::selectionChanged, this,
                   &DocumentController::pageSelectionChanged);
        SceneThumbnailPixmap *currentScene = dynamic_cast<SceneThumbnailPixmap *>(selection);
        if (currentScene)
            mDocumentsView->thumbnailWidget()->hightlightItem(currentScene->sceneIndex());
        connect(mDocumentsView->thumbnailWidget()->scene(), &QGraphicsScene::selectionChanged, this,
                &DocumentController::pageSelectionChanged);
    }

    QApplication::restoreOverrideCursor();
}

void DocumentController::createEmptyDocument() {
    PersistenceManager *pManager = PersistenceManager::persistenceManager();
    DocumentTreeModel *docModel = pManager->mDocumentTreeStructureModel;

    QString groupName = docModel->virtualPathForIndex(docModel->myDocumentsIndex());

    DocumentProxy *document = pManager->createDocument(groupName);
    selectDocument(document);

    if (document)
        pManager->mDocumentTreeStructureModel->markDocumentAsNew(document);
}

void DocumentController::collapseAll() {
    //disable the animations because the view port will be in a invalid state
    mDocumentsView->documentTreeView()->setAnimated(false);

    mDocumentsView->documentTreeView()->collapseAll();

    QPersistentModelIndex myDocumentIndex =
        PersistenceManager::persistenceManager()->mDocumentTreeStructureModel->myDocumentsIndex();

    SortFilterProxyModel *proxy =
        dynamic_cast<SortFilterProxyModel *>(mDocumentsView->documentTreeView()->model());

    if (proxy) {
        mDocumentsView->documentTreeView()->setExpanded(proxy->mapFromSource(myDocumentIndex), true);
    } else {
        mDocumentsView->documentTreeView()->setExpanded(myDocumentIndex, true);
    }

    mDocumentsView->documentTreeView()->setAnimated(true);
}

void DocumentController::expandAll() {
    mDocumentsView->documentTreeView()->setAnimated(false);

    mDocumentsView->documentTreeView()->expandAll();

    mDocumentsView->documentTreeView()->setAnimated(true);
}
