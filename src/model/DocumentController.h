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

class DocumentReplaceDialog : public QDialog
{
    Q_OBJECT

public:
    DocumentReplaceDialog(const QString &pIncommingName, const QStringList &pFileList, QWidget *parent = 0, Qt::WindowFlags pFlags = Qt::WindowFlags());
    void setRegexp(const QRegularExpression &pRegExp);
    bool validString(const QString &pStr);
    void setFileNameAndList(const QString &fileName, const QStringList &pLst);
    QString  labelTextWithName(const QString &documentName) const;
    QString lineEditText() const {return mLineEdit->text();}

signals:
    void createNewFolder(QString str);
    void closeDialog();

private slots:
    void accept();
    void reject();

    void reactOnTextChanged(const QString &pStr);

private:
    QLineEdit *mLineEdit;
    QRegularExpressionValidator *mValidator;
    QStringList mFileNameList;
    QString mIncommingName;
    QPushButton *acceptButton;
    const QString acceptText;
    const QString replaceText;
    const QString cancelText;
    QLabel *mLabelText;
};


class DocumentTreeNode
{
public:
    friend class DocumentTreeModel;

    enum Type {
        Catalog = 0
        , Document
    };

    DocumentTreeNode(Type pType, const QString &pName, const QString &pDisplayName = QString(), DocumentProxy *pProxy = 0);
    DocumentTreeNode() : mType(Catalog), mParent(0), mProxy(0) {;}
    ~DocumentTreeNode();

    QList<DocumentTreeNode*> children() const {return mChildren;}
    DocumentTreeNode *parentNode() {return mParent;}
    Type nodeType() const {return mType;}
    QString nodeName() const {return mName;}
    QString displayName() const {return mDisplayName;}
    void setNodeName(const QString &str) {mName = str; mDisplayName = str;}
    void addChild(DocumentTreeNode *pChild);
    void insertChild(int pIndex, DocumentTreeNode *pChild);
    void moveChild(DocumentTreeNode *child, int index, DocumentTreeNode *newParent);
    void removeChild(int index);
    DocumentProxy *proxyData() const {return mProxy;}
    bool isRoot() {return !mParent;}
    bool isTopLevel()
    {
        if (mParent)
        {
            return !mParent->mParent;
        }
        else
			return false;
    }
    DocumentTreeNode *clone();
    QString dirPathInHierarchy();

    bool findNode(DocumentTreeNode *node);
    DocumentTreeNode *nextSibling();
    DocumentTreeNode *previousSibling();

private:
    Type mType;
    QString mName;
    QString mDisplayName;
    DocumentTreeNode *mParent;
    QList<DocumentTreeNode*> mChildren;
    QPointer<DocumentProxy> mProxy;
};
Q_DECLARE_METATYPE(DocumentTreeNode*)

class DocumentTreeModel : public QAbstractItemModel {
    Q_OBJECT

public:
    enum eAncestors {
        aMyDocuments
        , aModel
        , aTrash
    };

    enum eCopyMode {
        aReference
        , aContentCopy
    };

    enum eAddItemMode {
        aEnd = 0         
        , aDetectPosition 
    };

    enum eDocumentData{
        DataNode = Qt::UserRole +1,
        CreationDate,
        UpdateDate
    };

    DocumentTreeModel(QObject *parent = 0);
    ~DocumentTreeModel();

    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags ( const QModelIndex & index ) const;
    Qt::DropActions supportedDropActions() const {return Qt::MoveAction | Qt::CopyAction;}
    QStringList mimeTypes() const;
    QMimeData *mimeData (const QModelIndexList &indexes) const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    bool removeRows(int row, int count, const QModelIndex &parent);

    bool containsDocuments(const QModelIndex& index);

    QModelIndex indexForNode(DocumentTreeNode *pNode) const;
    QPersistentModelIndex persistentIndexForNode(DocumentTreeNode *pNode);
//    bool insertRow(int row, const QModelIndex &parent);

    QPersistentModelIndex copyIndexToNewParent(const QModelIndex &source, const QModelIndex &newParent, eCopyMode pMode = aReference);

    void copyIndexToNewParent(const QModelIndexList &list, const QModelIndex &newParent, eCopyMode pMode);

    void moveIndex(const QModelIndex &what, const QModelIndex &destination);
    DocumentTreeNode *currentNode() const {return mCurrentNode;} //work around for sorting model.
    void setCurrentNode(DocumentTreeNode *pNode) {mCurrentNode = pNode;}
    QModelIndex currentIndex() {return indexForNode(mCurrentNode);} //index representing a current document
    QModelIndex indexForProxy(DocumentProxy *pSearch) const;
    void setCurrentDocument(DocumentProxy *pDocument);
    void setRootNode(DocumentTreeNode *pRoot);
    DocumentTreeNode *rootNode() const {return mRootNode;}
    DocumentProxy *proxyForIndex(const QModelIndex &pIndex) const;
    QString virtualDirForIndex(const QModelIndex &pIndex) const;
    QString virtualPathForIndex(const QModelIndex &pIndex) const;
    QStringList nodeNameList(const QModelIndex &pIndex) const;
    bool newNodeAllowed(const QModelIndex &pSelectedIndex)  const;
    QModelIndex goTo(const QString &dir);
    bool inTrash(const QModelIndex &index) const;
    bool isCatalog(const QModelIndex &index) const {return nodeFromIndex(index)->nodeType() == DocumentTreeNode::Catalog;}
    bool isDocument(const QModelIndex &index) const {return nodeFromIndex(index)->nodeType() == DocumentTreeNode::Document;}
    bool isToplevel(const QModelIndex &index) const {return nodeFromIndex(index) ? nodeFromIndex(index)->isTopLevel() : false;}
    bool isConstant(const QModelIndex &index) const {return isToplevel(index);}
    bool isOkToRename(const QModelIndex &index) const {return flags(index) & Qt::ItemIsEditable;}
    DocumentProxy *proxyData(const QModelIndex &index) const {return nodeFromIndex(index)->proxyData();}
    void addDocument(DocumentProxy *pProxyData, const QModelIndex &pParent = QModelIndex());
    void addNewDocument(DocumentProxy *pProxyData, const QModelIndex &pParent = QModelIndex());
    QModelIndex addCatalog(const QString &pName, const QModelIndex &pParent);
    QList<DocumentProxy*> newDocuments() {return mNewDocuments;}
    void markDocumentAsNew(DocumentProxy *pDoc) {if (indexForProxy(pDoc).isValid()) mNewDocuments << pDoc;}
    void setNewName(const QModelIndex &index, const QString &newName);
    QString adjustNameForParentIndex(const QString &pName, const QModelIndex &pIndex);

    QPersistentModelIndex myDocumentsIndex() const {return mMyDocuments;}
    QPersistentModelIndex trashIndex() const {return mTrash;}
    DocumentTreeNode *nodeFromIndex(const QModelIndex &pIndex) const;
    static bool nodeLessThan(const DocumentTreeNode *firstIndex, const DocumentTreeNode *secondIndex);
    void setHighLighted(const QModelIndex &newHighLighted) {mHighLighted = newHighLighted;}
    QModelIndex highLighted() {return mHighLighted;}

    bool ascendingOrder() const{ return mAscendingOrder; }
    QDateTime findNodeDate(DocumentTreeNode *node, QString type) const;
    bool inMyDocuments(const QModelIndex &index) const;
    void moveIndexes(const QModelIndexList &source, const QModelIndex &destination);
    bool isDescendantOf(const QModelIndex &pPossibleDescendant, const QModelIndex &pPossibleAncestor) const;

signals:
    void indexChanged(const QModelIndex &newIndex, const QModelIndex &oldIndex);
    void currentIndexMoved(const QModelIndex &newIndex, const QModelIndex &previous); 

private:
    DocumentTreeNode *mRootNode;
    DocumentTreeNode *mCurrentNode;

    DocumentTreeNode *findProxy(DocumentProxy *pSearch, DocumentTreeNode *pParent) const;
    QModelIndex pIndexForNode(const QModelIndex &parent, DocumentTreeNode *pNode) const;
    QModelIndex addNode(DocumentTreeNode *pFreeNode, const QModelIndex &pParent, eAddItemMode pMode = aDetectPosition);
    int positionForParent(DocumentTreeNode *pFreeNode, DocumentTreeNode *pParentNode);
    void fixNodeName(const QModelIndex &source, const QModelIndex &dest);
    void updateIndexNameBindings(DocumentTreeNode *nd);
    QPersistentModelIndex mRoot;
    QPersistentModelIndex mMyDocuments;
    QPersistentModelIndex mTrash;
    QList<DocumentProxy*> mNewDocuments;
    QModelIndex mHighLighted;

    bool mAscendingOrder;

    QDateTime findCatalogUpdatedDate(DocumentTreeNode *node) const;
    QDateTime findCatalogCreationDate(DocumentTreeNode *node) const;
};

class DocumentTreeMimeData : public QMimeData
{
    Q_OBJECT

    public:
        QList<QModelIndex> indexes() const {return mIndexes;}
        void setIndexes (const QList<QModelIndex> &pIndexes) {mIndexes = pIndexes;}

    private:
        QList<QModelIndex> mIndexes;
};

class DocumentTreeView : public QTreeView
{
    Q_OBJECT

public:
    DocumentTreeView (QWidget *parent = 0);

    QModelIndex mapIndexToSource(const QModelIndex &index);
    QModelIndexList mapIndexesToSource(const QModelIndexList &indexes);

public slots:
    void setSelectedAndExpanded(const QModelIndex &pIndex, bool pExpand = true, bool pEdit = false);
    void onModelIndexChanged(const QModelIndex &pNewIndex, const QModelIndex &pOldIndex);
    void hSliderRangeChanged(int min, int max);

protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);
    void paintEvent(QPaintEvent *event);

    DocumentTreeModel *fullModel() {return qobject_cast<DocumentTreeModel*>(model());}
    void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);

private:
    bool isAcceptable(const QModelIndex &dragIndex, const QModelIndex &atIndex);
    Qt::DropAction acceptableAction(const QModelIndex &dragIndex, const QModelIndex &atIndex);
    void updateIndexEnvirons(const QModelIndex &index);
};

class DocumentTreeItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    DocumentTreeItemDelegate(QObject *parent = 0);

private slots:
    void commitAndCloseEditor();
    void processChangedText(const QString &str) const;
    bool validateString(const QString &str) const;

protected:
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex &index) const;

private:
    mutable QStringList mExistingFileNames;
};

class DocumentController : public DocumentContainer
{
    Q_OBJECT

public:
    enum DeletionType {
        MoveToTrash = 0
        , CompleteDelete
        , EmptyFolder
        , EmptyTrash
        , DeletePage
        , NoDeletion
    };

    enum LastSelectedElementType
    {
        None = 0, Folder, Document, Page
    };

    enum SortOrder
    {
        ASC = 0,
        DESC
    };

    enum SortKind
    {
        CreationDate,
        UpdateDate,
        Alphabetical
    };

    DocumentController(MainWindow* mainWindow);
    virtual ~DocumentController();

    void closing();
    QWidget* controlView();
    DocumentProxyTreeItem* findDocument(DocumentProxy* proxy);
    bool addFileToDocument(DocumentProxy* document);
    void deletePages(QList<QGraphicsItem*> itemsToDelete);
    int getSelectedItemIndex();

    bool pageCanBeMovedUp(int page);
    bool pageCanBeMovedDown(int page);
    bool pageCanBeDuplicated(int page);
    bool pageCanBeDeleted(int page);
    QString documentTrashGroupName(){ return mDocumentTrashGroupName;}
    QString defaultDocumentGroupName(){ return mDefaultDocumentGroupName;}

    void setDocument(DocumentProxy *document, bool forceReload = false);
    QModelIndex firstSelectedTreeIndex();
    DocumentProxy *firstSelectedTreeProxy();
    inline DeletionType deletionTypeForSelection(LastSelectedElementType pTypeSelection
                                                    , const QModelIndex &selectedIndex
                                                    , DocumentTreeModel *docModel) const;
    bool firstAndOnlySceneSelected() const;
    QWidget *mainWidget() const {return mDocumentWidget;}

    void moveToTrash(QModelIndex &index, DocumentTreeModel* docModel);

    void deleteDocumentsInFolderOlderThan(const QModelIndex &index, const int days);
    void deleteEmptyFolders(const QModelIndex &index);

    QModelIndex mapIndexToSource(const QModelIndex &index);
    QModelIndexList mapIndexesToSource(const QModelIndexList &indexes);

    void sortDocuments(int kind, int order);

    void moveIndexesToTrash(const QModelIndexList &list, DocumentTreeModel *docModel);
    QModelIndex findPreviousSiblingNotSelected(const QModelIndex &index, QItemSelectionModel *selectionModel);
    QModelIndex findNextSiblingNotSelected(const QModelIndex &index, QItemSelectionModel *selectionModel);
    bool parentIsSelected(const QModelIndex& child, QItemSelectionModel *selectionModel);

signals:
    void exportDone();
    void reorderDocumentsRequested();

public slots:
    void createNewDocument();
    void refreshDateColumns();
    void reorderDocuments();
    void createEmptyDocument();

    void createNewDocumentGroup();
    void deleteSelectedItem();
    void deleteSingleItem(QModelIndex index, DocumentTreeModel *docModel);
    void deleteMultipleItems(QModelIndexList indexes, DocumentTreeModel *docModel);
    void emptyFolder(const QModelIndex &index, DeletionType pDeletionType = MoveToTrash);
    void deleteIndexAndAssociatedData(const QModelIndex &pIndex);
    void renameSelectedItem();
    void openSelectedItem();
    void duplicateSelectedItem();
    void importFile();
    void moveSceneToIndex(DocumentProxy* proxy, int source, int target);
    void selectDocument(DocumentProxy* proxy, bool setAsCurrentDocument = true, const bool onImport = false, const bool editMode = false);
    void show();
    void hide();
    void showMessage(const QString& message, bool showSpinningWheel = false);
    void hideMessage();
    void toggleDocumentToolsPalette();
    void cut();
    void copy();
    void paste();
    void focusChanged(QWidget *old, QWidget *current);
    void updateActions();
    void updateExportSubActions(const QModelIndex &selectedIndex);
    void currentIndexMoved(const QModelIndex &newIndex, const QModelIndex &PreviousIndex);

    void onSortKindChanged(int index);
    void onSortOrderChanged(bool order);
    void onSplitterMoved(int size, int index);
    void collapseAll();
    void expandAll();

protected:
    virtual void setupViews();
    virtual void setupToolbar();
    void setupPalettes();
    bool isOKToOpenDocument(DocumentProxy* proxy);
    DocumentProxy* selectedDocumentProxy();
    QList<DocumentProxy*> selectedProxies();
    QModelIndexList selectedTreeIndexes();
    DocumentProxyTreeItem* selectedDocumentProxyTreeItem();
    DocumentGroupTreeItem* selectedDocumentGroupTreeItem();
    QStringList allGroupNames();
	void moveDocumentToTrash(DocumentGroupTreeItem* groupTi, DocumentProxyTreeItem *proxyTi);
	void moveFolderToTrash(DocumentGroupTreeItem* groupTi);
    LastSelectedElementType mSelectionType;

private:
    QWidget *mParentWidget;
    BoardController *mBoardController;
    DocumentsWidget *mDocumentsView;
    MainWindow* mMainWindow;
    QWidget *mDocumentWidget;
    QPointer<MessageWindow> mMessageWindow;
    QAction* mAddFolderOfImagesAction;
    QAction* mAddFileToDocumentAction;
    QAction* mAddImagesAction;
    bool mIsClosing;
    DocumentToolsPalette *mToolsPalette;
    bool mToolsPalettePositionned;

    QString mDocumentTrashGroupName;
    QString mDefaultDocumentGroupName;

    bool mCurrentIndexMoved;

    SortFilterProxyModel *mSortFilterProxyModel;

public slots:
    void TreeViewSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
    void TreeViewSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

private slots:
    void documentZoomSliderValueChanged (int value);
    void itemSelectionChanged(LastSelectedElementType newSelection);
    void exportDocument();
    void exportDocumentSet();

    void thumbnailViewResized();
    void pageSelectionChanged();

    void documentSceneChanged(DocumentProxy* proxy, int pSceneIndex);

    void thumbnailPageDoubleClicked(QGraphicsItem* item, int index);
    void pageClicked(QGraphicsItem* item, int index);
    void addToDocument();

    void addFolderOfImages();
    void addFileToDocument();
    void addImages();
    void refreshDocumentThumbnailsView(DocumentContainer* source);
};
