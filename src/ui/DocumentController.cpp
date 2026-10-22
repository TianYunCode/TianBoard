#include "ui/DocumentController.h"
#include "model/MimeTypes.hpp"
#include "utils/UserNotices.h"

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
#include "model/MimeData.h"
#include "model/ForeignObjectsHandler.h"

#include "model/IDocumentFormatServices.h"

#include "ui/MessageWindow.h"
#include "ui/MainWindow.h"
#include "ui/DocumentThumbnailWidget.h"
#include "ui/DocumentsWidget.h"
#include "ui/DocumentToolsPalette.h"

#include "whiteboard/BoardController.h"
#include "whiteboard/GraphicsScene.h"

#include "model/DocumentProxy.h"

DocumentTreeView::DocumentTreeView(QWidget *parent) : QTreeView(parent)
{
    setObjectName("DocumentTreeView");
    setRootIsDecorated(true);
}

void DocumentTreeView::setSelectedAndExpanded(const QModelIndex &pIndex, bool pExpand, bool pEdit)
{
    if (!pIndex.isValid())
    {
        return;
    }

    SortFilterProxyModel *proxy = dynamic_cast<SortFilterProxyModel *>(model());

    // pIndex 来自 DocumentTreeModel；视图的 model 是代理，setCurrentIndex/setExpanded/
    // scrollTo/edit 只吃代理索引。此前直接传源索引，Qt6 的 QSortFilterProxyModel 会在
    // setExpanded → hasChildren → mapToSource 这条链上断言
    // "index from wrong model passed to mapToSource"。
    const QModelIndex viewIndex = proxy ? proxy->mapFromSource(pIndex) : pIndex;

    if (!viewIndex.isValid())
    {
        return;
    }

    clearSelection();

    QItemSelectionModel::SelectionFlags sel = pExpand ? QItemSelectionModel::Select : QItemSelectionModel::Deselect;

    setCurrentIndex(pExpand ? viewIndex : QModelIndex());

    selectionModel()->select(viewIndex, QItemSelectionModel::Rows | sel);

    for (QModelIndex parent = viewIndex.parent(); parent.isValid(); parent = parent.parent())
    {
        setExpanded(parent, pExpand);
    }

    scrollTo(viewIndex, QAbstractItemView::PositionAtCenter);

    if (pEdit)
    {
        edit(viewIndex);
    }
}

void DocumentTreeView::hSliderRangeChanged(int min, int max)
{
    Q_UNUSED(min);
    Q_UNUSED(max);

    QScrollBar *hScroller = horizontalScrollBar();
    if (hScroller)
    {
        hScroller->triggerAction(QAbstractSlider::SliderToMaximum);
    }
}

void DocumentTreeView::dragEnterEvent(QDragEnterEvent *event)
{
    QTreeView::dragEnterEvent(event);
    event->accept();
    event->acceptProposedAction();
}

void DocumentTreeView::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);

    DocumentTreeModel *docModel = documentModel();

    if (docModel)
    {
        docModel->setHighLighted(QModelIndex());
    }

    update();
}

void DocumentTreeView::dragMoveEvent(QDragMoveEvent *event)
{
    QModelIndex index;
    if (selectedIndexes().count() > 0)
    {
        index = selectedIndexes().first();
    }

    bool acceptIt = isAcceptable(index, indexAt(event->position().toPoint()));

    if (event->mimeData()->hasFormat(MimeTypes::boardPage))
    {
        DocumentTreeModel *docModel    = documentModel();
        QModelIndex        targetIndex = mapIndexToSource(indexAt(event->position().toPoint()));

        // 认不出文档模型就没有"目标是不是文档、在不在回收站"可谈，直接不收这趟拖放。
        // 原来这条分支自己也写在 !docModel 里却去解引用 docModel。
        if (!docModel)
        {
            acceptIt = false;
        }
        else if (!docModel->isDocument(targetIndex) || docModel->inTrash(targetIndex))
        {
            event->ignore();
            event->setDropAction(Qt::IgnoreAction);
            docModel->setHighLighted(QModelIndex());
            acceptIt = false;
        }
        else
        {
            docModel->setHighLighted(targetIndex);
            acceptIt = true;
        }
        updateIndexEnvirons(indexAt(event->position().toPoint()));
    }
    QTreeView::dragMoveEvent(event);

    event->setAccepted(acceptIt);
}

void DocumentTreeView::dropEvent(QDropEvent *event)
{
    event->ignore();
    event->setDropAction(Qt::IgnoreAction);
    DocumentTreeModel *docModel = documentModel();

    // 没有文档模型就什么都做不了。原来这里只在有代理时才赋值，
    // 紧接着就无条件 docModel->inTrash(...)，代理缺失时是空指针解引用。
    if (!docModel)
    {
        return;
    }

    QModelIndex     targetIndex = mapIndexToSource(indexAt(event->position().toPoint()));
    QModelIndexList dropIndex   = mapIndexesToSource(selectedIndexes());

    bool isBoardPage            = event->mimeData()->hasFormat(MimeTypes::boardPage);

    bool targetIsInTrash        = docModel->inTrash(targetIndex) || docModel->trashIndex() == targetIndex;
    bool targetIsInMyDocuments  = docModel->inMyDocuments(targetIndex) || docModel->myDocumentsIndex() == targetIndex;

    if (!targetIsInMyDocuments && !targetIsInTrash)
    {
        return;
    }

    if (isBoardPage)
    {
        DocumentProxy *targetDocProxy = docModel->proxyData(targetIndex);

        const MimeData *mimeValue     = qobject_cast<const MimeData *>(event->mimeData());
        if (!targetDocProxy || !mimeValue || !mimeValue->items().count())
        {
            qDebug() << "解析时出错: " << MimeTypes::boardPage;
            QTreeView::dropEvent(event);
            return;
        }

        int count = 0;
        int total = mimeValue->items().size();
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        // 页面拖到某份文档上：逐页深拷贝场景，并把场景引用的相对文件一并复制过去，
        // 然后按当前页数追加到目标文档末尾。
        foreach (MimeDataItem sourceItem, mimeValue->items())
        {
            DocumentProxy *fromProxy = sourceItem.documentProxy();
            int            fromIndex = sourceItem.sceneIndex();
            int            toIndex   = targetDocProxy->pageCount();

            count++;

            Application::applicationController->showMessage(tr("Copying page %1/%2").arg(count).arg(total), true);

            GraphicsScene *scene = PersistenceManager::persistenceManager()->loadDocumentScene(sourceItem.documentProxy(), sourceItem.sceneIndex());
            if (scene)
            {
                GraphicsScene *sceneClone       = scene->sceneDeepCopy();

                // 与上面那个 targetDocProxy（proxyData 拿到的）不同，这里要的是索引重新解析出来的代理；
                // 换个名字，免得后面的相对路径搬运看不出用的是哪一个。
                DocumentProxy *targetSceneProxy = docModel->proxyForIndex(targetIndex);

                // 场景里的图片/音视频是相对路径，必须随场景一起搬到目标文档目录，
                // 否则复制出的页会丢资源。
                foreach (QUrl relativeFile, scene->relativeDependencies())
                {
                    QString source        = scene->document()->persistencePath() + "/" + relativeFile.toString();
                    QString target        = targetSceneProxy->persistencePath() + "/" + relativeFile.toString();

                    QString sourceDecoded = scene->document()->persistencePath() + "/" + relativeFile.toString(QUrl::DecodeReserved);
                    QString targetDecoded = targetSceneProxy->persistencePath() + "/" + relativeFile.toString(QUrl::DecodeReserved);

                    if (QFileInfo(source).isDir())
                    {
                        FileSystemUtils::copyDir(source, target);
                    }
                    else
                    {
                        QFileInfo fi(targetDecoded);
                        QDir      d = fi.dir();
                        d.mkpath(d.absolutePath());
                        QFile::copy(sourceDecoded, targetDecoded);
                    }
                }

                PersistenceManager::persistenceManager()->insertDocumentSceneAt(targetDocProxy, sceneClone, targetDocProxy->pageCount());

                QString thumbTmp(fromProxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", fromIndex));
                QString thumbTo(targetDocProxy->persistencePath() + FileSystemUtils::digitFileFormat("/page%1.thumbnail.jpg", toIndex));

                QFile::remove(thumbTo);
                QFile::copy(thumbTmp, thumbTo);

                Q_ASSERT(QFileInfo(thumbTmp).exists());
                Q_ASSERT(QFileInfo(thumbTo).exists());
                const QPixmap      *pix  = new QPixmap(thumbTmp);
                DocumentController *ctrl = Application::documentController;
                ctrl->addPixmapAt(pix, toIndex);
                ctrl->TreeViewSelectionChanged(ctrl->firstSelectedTreeIndex(), QModelIndex());
            }

            QApplication::restoreOverrideCursor();
            Application::applicationController->showMessage(tr("%1 pages copied", "", total).arg(total), false);

            docModel->setHighLighted(QModelIndex());
        }
    }
    else
    {
        if (targetIsInTrash)
        {
            // 拖到回收站等同删除，先确认；确认后走与删除命令相同的移入回收站流程。
            if (!Application::mainWindow->yesNoQuestion(tr("Remove Item"), tr("Are you sure you want to remove the selected item(s) ?")))
            {
                return;
            }

            Application::documentController->moveIndexesToTrash(dropIndex, docModel);
        }
        else
        {
            docModel->moveIndexes(dropIndex, targetIndex);
        }
    }

    // expand 收的是视图侧（代理）索引；这个视图没套代理时，源索引本身就是视图索引。
    SortFilterProxyModel *proxy = dynamic_cast<SortFilterProxyModel *>(model());
    expand(proxy ? proxy->mapFromSource(targetIndex) : targetIndex);

    QTreeView::dropEvent(event);

    Application::documentController->updateActions();
}

void DocumentTreeView::paintEvent(QPaintEvent *event)
{
    QTreeView::paintEvent(event);
}

void DocumentTreeView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    QTreeView::rowsAboutToBeRemoved(parent, start, end);
}

bool DocumentTreeView::isAcceptable(const QModelIndex &dragIndex, const QModelIndex &atIndex)
{
    QModelIndex dragIndexSource = mapIndexToSource(dragIndex);
    QModelIndex atIndexSource   = mapIndexToSource(atIndex);

    if (!dragIndexSource.isValid())
    {
        return false;
    }

    return true;
}

Qt::DropAction DocumentTreeView::acceptableAction(const QModelIndex &dragIndex, const QModelIndex &atIndex)
{
    return Qt::MoveAction;
}

void DocumentTreeView::updateIndexEnvirons(const QModelIndex &index)
{
    QRect     updateRect = visualRect(index);
    const int multipler  = 3;
    updateRect.adjust(0, -updateRect.height() * multipler, 0, updateRect.height() * multipler);
    update(updateRect);
}

DocumentTreeModel *DocumentTreeView::documentModel() const
{
    SortFilterProxyModel *proxy = dynamic_cast<SortFilterProxyModel *>(model());

    if (proxy)
    {
        return dynamic_cast<DocumentTreeModel *>(proxy->sourceModel());
    }

    return dynamic_cast<DocumentTreeModel *>(model());
}

QModelIndex DocumentTreeView::mapIndexToSource(const QModelIndex &index)
{
    SortFilterProxyModel *proxy = dynamic_cast<SortFilterProxyModel *>(model());

    if (proxy)
    {
        return proxy->mapToSource(index);
    }

    return index;
}

QModelIndexList DocumentTreeView::mapIndexesToSource(const QModelIndexList &indexes)
{
    SortFilterProxyModel *proxy = dynamic_cast<SortFilterProxyModel *>(model());

    if (proxy)
    {
        QModelIndexList list;

        for (int i = 0; i < indexes.size(); i++)
        {
            list.push_back(proxy->mapToSource(indexes.at(i)));
        }

        return list;
    }

    return indexes;
}

DocumentTreeItemDelegate::DocumentTreeItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

void DocumentTreeItemDelegate::commitAndCloseEditor()
{
    QLineEdit *lineEditor = qobject_cast<QLineEdit *>(sender());
    if (lineEditor)
    {
        // 重命名后文档可能改变排序位置，提交数据后请求控制器重新排序；
        // 编辑器本身由视图在 commitData 后关闭，这里不重复发 closeEditor。
        emit commitData(lineEditor);
        emit Application::documentController->reorderDocumentsRequested();
    }
}

void DocumentTreeItemDelegate::processChangedText(const QString &str) const
{
    QLineEdit *editor = qobject_cast<QLineEdit *>(sender());
    if (!editor)
    {
        return;
    }

    if (!validateString(str))
    {
        editor->setStyleSheet("background-color: #FFB3C8;");
    }
    else
    {
        editor->setStyleSheet("background-color: #FFFFFF;");
    }
}

bool DocumentTreeItemDelegate::validateString(const QString &str) const
{
    return !mExistingFileNames.contains(str);
}

QWidget *DocumentTreeItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);

    if (index.column() == 0)
    {
        mExistingFileNames.clear();
        const DocumentTreeModel *indexModel = qobject_cast<const DocumentTreeModel *>(index.model());
        if (indexModel)
        {
            mExistingFileNames = indexModel->nodeNameList(index.parent());
            mExistingFileNames.removeOne(index.data().toString());
        }

        QLineEdit *nameEditor = new QLineEdit(parent);
        connect(nameEditor, &QLineEdit::editingFinished, this, &DocumentTreeItemDelegate::commitAndCloseEditor);
        connect(nameEditor, &QLineEdit::textChanged, this, &DocumentTreeItemDelegate::processChangedText);
        return nameEditor;
    }

    // 只有名称列可编辑，日期列不提供编辑器。
    return 0;
}

void DocumentTreeItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if (index.column() == 0)
    {
        QLineEdit *lineEditor = qobject_cast<QLineEdit *>(editor);
        lineEditor->setText(index.data().toString());
        lineEditor->selectAll();
    }
}

void DocumentTreeItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QLineEdit *lineEditor = qobject_cast<QLineEdit *>(editor);
    if (validateString(lineEditor->text()))
    {
        model->setData(index, lineEditor->text());
    }
}

void DocumentTreeItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);
}

DocumentController::DocumentController(MainWindow *mainWindow)
    : DocumentContainer(mainWindow->centralWidget()), mSelectionType(None), mParentWidget(mainWindow->centralWidget()),
      mBoardController(BoardController::instance()), mDocumentsView(0), mMainWindow(mainWindow), mDocumentWidget(0), mIsClosing(false),
      mToolsPalette(0), mToolsPalettePositionned(false), mDocumentTrashGroupName(tr("Trash")), mDefaultDocumentGroupName(tr("Untitled Documents")),
      mCurrentIndexMoved(false)
{
    setupViews();
    setupToolbar();
    connect(this, &DocumentController::exportDone, mMainWindow, &MainWindow::onExportDone);
    connect(this, &DocumentContainer::documentThumbnailsUpdated, this, &DocumentController::refreshDocumentThumbnailsView);
    connect(this, &DocumentController::reorderDocumentsRequested, this, &DocumentController::reorderDocuments);
    // 导入的同名文档顶掉了画板上正在展示的那一份：核心只回报这个事实，换谁上画板由界面决定。
    connect(PersistenceManager::persistenceManager(), &PersistenceManager::currentDocumentReplaced, this,
        [this](DocumentProxy *incomingProxy, DocumentProxy *)
        {
            selectDocument(incomingProxy, true, true);
        });
}

DocumentController::~DocumentController()
{
    // mDocumentsView 挂在 mDocumentWidget 的控件树里，随该容器一起由 Qt 销毁，
    // 这里不能再次 delete。
}

void DocumentController::closing()
{
    // 关闭流程里还要删文档、动选择树；这些都会把信号打回 pageSelectionChanged() 与
    // updateActions()，而那时界面正在拆。置了这个标记，那两处就在此收工。
    mIsClosing = true;
}

void DocumentController::createNewDocument()
{
    PersistenceManager *pManager      = PersistenceManager::persistenceManager();
    DocumentTreeModel  *docModel      = pManager->mDocumentTreeStructureModel;
    QModelIndex         selectedIndex = firstSelectedTreeIndex();

    if (!selectedIndex.isValid())
    {
        selectedIndex = docModel->myDocumentsIndex();
    }

    QString groupName =
        docModel->isCatalog(selectedIndex) ? docModel->virtualPathForIndex(selectedIndex) : docModel->virtualDirForIndex(selectedIndex);

    DocumentProxy *document = pManager->createDocument(groupName);
    selectDocument(document, true, false, true);

    if (document)
    {
        pManager->mDocumentTreeStructureModel->markDocumentAsNew(document);
    }
}

void DocumentController::selectDocument(DocumentProxy *proxy, bool setAsCurrentDocument, const bool onImport, const bool editMode)
{
    if (proxy == NULL)
    {
        setDocument(NULL);
        return;
    }

    if (setAsCurrentDocument)
    {
        PersistenceManager::persistenceManager()->mDocumentTreeStructureModel->setCurrentDocument(proxy);
        QModelIndex indexCurrentDoc = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel->indexForProxy(proxy);
        mDocumentsView->documentTreeView()->setSelectedAndExpanded(indexCurrentDoc, true, editMode);

        if (proxy != mBoardController->selectedDocument()) // 只有目标文档与画板上那份不同才重新装载，避免把当前文档再读一遍
        {
            mBoardController->setActiveDocumentScene(proxy, 0, true, onImport);
        }
    }

    mSelectionType = Document;
    setDocument(proxy);
}

void DocumentController::createNewDocumentGroup()
{
    PersistenceManager *pManager      = PersistenceManager::persistenceManager();
    DocumentTreeModel  *docModel      = pManager->mDocumentTreeStructureModel;
    QModelIndex         selectedIndex = firstSelectedTreeIndex();
    if (!selectedIndex.isValid())
    {
        selectedIndex = docModel->myDocumentsIndex();
    }
    QModelIndex parentIndex = docModel->isCatalog(selectedIndex) ? selectedIndex : selectedIndex.parent();

    QString newFolderName   = docModel->adjustNameForParentIndex(tr("New Folder"), parentIndex);

    QModelIndex newIndex    = docModel->addCatalog(newFolderName, parentIndex);
    mDocumentsView->documentTreeView()->setSelectedAndExpanded(newIndex, true, true);
}

DocumentProxy *DocumentController::selectedDocumentProxy()
{
    return PersistenceManager::persistenceManager()->mDocumentTreeStructureModel->proxyForIndex(firstSelectedTreeIndex());
}

QList<DocumentProxy *> DocumentController::selectedProxies()
{
    QList<DocumentProxy *> result;

    foreach (QModelIndex curIndex, mapIndexesToSource(mDocumentsView->documentTreeView()->selectionModel()->selectedIndexes()))
    {
        result << PersistenceManager::persistenceManager()->mDocumentTreeStructureModel->proxyForIndex(curIndex);
    }

    return result;
}

QModelIndexList DocumentController::selectedTreeIndexes()
{
    return mapIndexesToSource(mDocumentsView->documentTreeView()->selectionModel()->selectedRows(0));
}

DocumentProxy *DocumentController::firstSelectedTreeProxy()
{
    return selectedProxies().count() ? selectedProxies().first() : 0;
}

void DocumentController::TreeViewSelectionChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(previous)

    DocumentTreeModel *docModel         = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

    QModelIndex current_index           = mapIndexToSource(current);

    // 多选时不显示任何页面的缩略图：把代理留空即可让缩略图区清空，
    // 避免用户误以为当前显示的页面属于整个选择。
    DocumentProxy *currentDocumentProxy = 0;

    if (current_index.isValid() && mDocumentsView->documentTreeView()->selectionModel()->selectedRows(0).size() == 1)
    {
        currentDocumentProxy = docModel->proxyData(current_index);
        setDocument(currentDocumentProxy, false);
    }

    if (mCurrentIndexMoved)
    {
        if (docModel->isDocument(current_index))
        {
            docModel->setCurrentDocument(currentDocumentProxy);
        }
        else if (docModel->isCatalog(current_index))
        {
            docModel->setCurrentDocument(0);
        }
        mCurrentIndexMoved = false;
    }

    itemSelectionChanged(docModel->isCatalog(current_index) ? Folder : Document);
}

QModelIndex DocumentController::mapIndexToSource(const QModelIndex &index)
{
    SortFilterProxyModel *proxy = dynamic_cast<SortFilterProxyModel *>(mDocumentsView->documentTreeView()->model());

    if (proxy)
    {
        return proxy->mapToSource(index);
    }

    return index;
}

QModelIndexList DocumentController::mapIndexesToSource(const QModelIndexList &indexes)
{
    SortFilterProxyModel *proxy = dynamic_cast<SortFilterProxyModel *>(mDocumentsView->documentTreeView()->model());

    if (proxy)
    {
        QModelIndexList list;

        for (int i = 0; i < indexes.size(); i++)
        {
            list.push_back(proxy->mapToSource(indexes.at(i)));
        }

        return list;
    }

    return indexes;
}

void DocumentController::TreeViewSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected)

    int nbIndexes = selected.indexes().count();

    if (nbIndexes)
    {
        QModelIndexList list = mDocumentsView->documentTreeView()->selectionModel()->selectedRows();

        // 多选时逐项处理；若某项的祖先也在选择中则跳过，
        // 因为祖先项的处理已经覆盖整棵子树。
        if (list.count() > 1)
        {
            for (int i = 0; i < list.count(); i++)
            {
                QModelIndex newSelectedRow     = list.at(i);
                QModelIndex parent             = newSelectedRow.parent();
                bool        isParentIsSelected = false;
                while (parent.isValid())
                {
                    isParentIsSelected |= (list.indexOf(parent) != -1);

                    if (isParentIsSelected)
                    {
                        break;
                    }

                    parent = parent.parent();
                }

                if (!isParentIsSelected)
                {
                    TreeViewSelectionChanged(newSelectedRow, QModelIndex());
                }
                else
                {
                    mDocumentsView->documentTreeView()->selectionModel()->select(
                        newSelectedRow, QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
                }
            }
        }
        else
        {
            TreeViewSelectionChanged(selected.indexes().at(0), QModelIndex());
        }
    }
}

void DocumentController::itemSelectionChanged(LastSelectedElementType newSelection)
{
    mSelectionType = newSelection;
    updateActions();
}

void DocumentController::setupViews()
{
    if (!mDocumentWidget)
    {
        mDocumentWidget = new QWidget(mMainWindow->centralWidget());
        mMainWindow->addDocumentsWidget(mDocumentWidget);

        // documents.ui 曾由 Ui::documents::setupUi() 直接铺在 mDocumentWidget 上；
        // 现在手写表单是独立控件，套一层无边距布局让它像原来的网格布局一样填满容器。
        QVBoxLayout *documentWidgetLayout = new QVBoxLayout(mDocumentWidget);
        documentWidgetLayout->setSpacing(0);
        documentWidgetLayout->setContentsMargins(0, 0, 0, 0);

        mDocumentsView = new DocumentsWidget(mDocumentWidget);
        documentWidgetLayout->addWidget(mDocumentsView);

        int thumbWidth = Settings::settings()->documentThumbnailWidth->get().toInt();

        mDocumentsView->documentZoomSlider()->setValue(thumbWidth);
        mDocumentsView->thumbnailWidget()->setThumbnailWidth(thumbWidth);

        connect(mDocumentsView->documentZoomSlider(), &QSlider::valueChanged, this, &DocumentController::documentZoomSliderValueChanged);

        connect(mMainWindow->ui().actionOpen, &QAction::triggered, this, &DocumentController::openSelectedItem);
        connect(mMainWindow->ui().actionNewFolder, &QAction::triggered, this, &DocumentController::createNewDocumentGroup);
        connect(mMainWindow->ui().actionNewDocument, &QAction::triggered, this, &DocumentController::createNewDocument);

        connect(mMainWindow->ui().actionImport, &QAction::triggered, this, &DocumentController::importFile);

        QMenu *addMenu           = new QMenu(mDocumentWidget);
        mAddFolderOfImagesAction = addMenu->addAction(tr("Add Folder of Images"));
        mAddImagesAction         = addMenu->addAction(tr("Add Images"));
        mAddFileToDocumentAction = addMenu->addAction(tr("Add Pages from File"));

        connect(mAddFolderOfImagesAction, &QAction::triggered, this, &DocumentController::addFolderOfImages);
        connect(mAddFileToDocumentAction, &QAction::triggered, this, qOverload<>(&DocumentController::addFileToDocument));
        connect(mAddImagesAction, &QAction::triggered, this, &DocumentController::addImages);

        foreach (QObject *associated, mMainWindow->ui().actionDocumentAdd->associatedObjects())
        {
            QToolButton *tb = qobject_cast<QToolButton *>(associated);

            if (tb && !tb->menu())
            {
                tb->setObjectName("buttonMenu");
                tb->setPopupMode(QToolButton::InstantPopup);

                QMenu *menu = new QMenu(mDocumentWidget);

                menu->addAction(mAddFolderOfImagesAction);
                menu->addAction(mAddImagesAction);
                menu->addAction(mAddFileToDocumentAction);

                tb->setMenu(menu);
            }
        }

        QMenu *exportMenu                = new QMenu(mDocumentWidget);

        DocumentManager *documentManager = DocumentManager::documentManager();
        for (int i = 0; i < documentManager->exportAdaptorCount(); ++i)
        {
            QAction *currentExportAction = exportMenu->addAction(documentManager->exportAdaptorName(i));
            currentExportAction->setData(i);
            connect(currentExportAction, &QAction::triggered, this, &DocumentController::exportDocument);
            exportMenu->addAction(currentExportAction);
            documentManager->associateExportAction(i, currentExportAction);
        }

        foreach (QObject *associated, mMainWindow->ui().actionExport->associatedObjects())
        {
            QToolButton *tb = qobject_cast<QToolButton *>(associated);

            if (tb && !tb->menu())
            {
                tb->setObjectName("buttonMenu");
                tb->setPopupMode(QToolButton::InstantPopup);

                tb->setMenu(exportMenu);
            }
        }

        connect(mMainWindow->ui().actionDelete, &QAction::triggered, this, &DocumentController::deleteSelectedItem);
        connect(mMainWindow->ui().actionDuplicate, &QAction::triggered, this, &DocumentController::duplicateSelectedItem);
        connect(mMainWindow->ui().actionRename, &QAction::triggered, this, &DocumentController::renameSelectedItem);
        connect(mMainWindow->ui().actionAddToWorkingDocument, &QAction::triggered, this, &DocumentController::addToDocument);

        DocumentTreeModel *model = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

        mSortFilterProxyModel    = new SortFilterProxyModel();

        mSortFilterProxyModel->setSourceModel(model);

        // 排序依据与方向来自用户设置，界面控件也要回填成同样的状态。
        int sortKind  = Settings::settings()->documentSortKind->get().toInt();
        int sortOrder = Settings::settings()->documentSortOrder->get().toInt();

        // 下拉框与排序按钮反映当前设置，避免首次打开时界面与真实排序不一致。
        mDocumentsView->sortKind()->setCurrentIndex(sortKind);
        if (sortOrder == DocumentController::DESC)
        {
            mDocumentsView->sortOrder()->setChecked(true);
        }

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

        const int  splitterLeftSize  = Settings::settings()->documentSplitterLeftSize->get().toInt();
        const int  splitterRightSize = Settings::settings()->documentSplitterRightSize->get().toInt();
        QList<int> splitterSizes;
        splitterSizes.append(splitterLeftSize);
        splitterSizes.append(splitterRightSize);
        mDocumentsView->splitter()->setSizes(splitterSizes);

        // 日期列按排序方式只显示其中一列；第 1 列创建日期、第 2 列更新日期。
        mDocumentsView->documentTreeView()->hideColumn(2);

        sortDocuments(sortKind, sortOrder);

        connect(mDocumentsView->sortKind(), &QComboBox::activated, this, &DocumentController::onSortKindChanged);
        connect(mDocumentsView->sortOrder(), &QToolButton::toggled, this, &DocumentController::onSortOrderChanged);

        connect(mDocumentsView->splitter(), &QSplitter::splitterMoved, this, &DocumentController::onSplitterMoved);

        connect(mDocumentsView->documentTreeView()->selectionModel(), &QItemSelectionModel::selectionChanged, this,
            qOverload<const QItemSelection &, const QItemSelection &>(&DocumentController::TreeViewSelectionChanged));
        connect(PersistenceManager::persistenceManager()->mDocumentTreeStructureModel, &DocumentTreeModel::currentIndexMoved, this,
            &DocumentController::currentIndexMoved);

        connect(mDocumentsView->thumbnailWidget(), &DocumentThumbnailWidget::sceneDropped, this, &DocumentController::moveSceneToIndex);
        connect(mDocumentsView->thumbnailWidget(), &ThumbnailWidget::resized, this, &DocumentController::thumbnailViewResized);
        connect(mDocumentsView->thumbnailWidget(), &ThumbnailWidget::mouseDoubleClick, this, &DocumentController::thumbnailPageDoubleClicked);
        connect(mDocumentsView->thumbnailWidget(), &ThumbnailWidget::mouseClick, this, &DocumentController::pageClicked);

        connect(mDocumentsView->thumbnailWidget()->scene(), &QGraphicsScene::selectionChanged, this, &DocumentController::pageSelectionChanged);

        connect(PersistenceManager::persistenceManager(), &PersistenceManager::documentSceneCreated, this, &DocumentController::documentSceneChanged);
        connect(PersistenceManager::persistenceManager(), &PersistenceManager::documentSceneWillBeDeleted, this,
            &DocumentController::documentSceneChanged);

        mDocumentsView->thumbnailWidget()->setBackgroundBrush(Settings::documentViewLightColor);

        mMessageWindow = new MessageWindow(mDocumentsView->thumbnailWidget());

        mMessageWindow->setCustomPosition(true);
        mMessageWindow->hide();
    }
}

void DocumentController::refreshDateColumns()
{
    if (Settings::settings()->documentSortKind->get().toInt() == DocumentController::Alphabetical)
    {
        if (!Settings::settings()->showDateColumnOnAlphabeticalSort->get().toBool())
        {
            mDocumentsView->documentTreeView()->hideColumn(1);
            mDocumentsView->documentTreeView()->hideColumn(2);
        }
        else
        {
            mDocumentsView->documentTreeView()->showColumn(1);
            mDocumentsView->documentTreeView()->hideColumn(2);
        }
    }
}

void DocumentController::reorderDocuments()
{
    int kindIndex  = mDocumentsView->sortKind()->currentIndex();
    int orderIndex = mDocumentsView->sortOrder()->isChecked() ? DocumentController::DESC : DocumentController::ASC;

    sortDocuments(kindIndex, orderIndex);
}

void DocumentController::sortDocuments(int kind, int order)
{
    Qt::SortOrder sortOrder = Qt::AscendingOrder;
    if (order == DocumentController::DESC)
    {
        sortOrder = Qt::DescendingOrder;
    }

    if (kind == DocumentController::CreationDate)
    {
        mSortFilterProxyModel->setSortRole(DocumentTreeModel::CreationDate);
        mSortFilterProxyModel->sort(1, sortOrder);
        mDocumentsView->documentTreeView()->showColumn(1);
        mDocumentsView->documentTreeView()->hideColumn(2);
    }
    else if (kind == DocumentController::UpdateDate)
    {
        mSortFilterProxyModel->setSortRole(DocumentTreeModel::UpdateDate);
        mSortFilterProxyModel->sort(2, sortOrder);
        mDocumentsView->documentTreeView()->hideColumn(1);
        mDocumentsView->documentTreeView()->showColumn(2);
    }
    else
    {
        mSortFilterProxyModel->setSortRole(Qt::DisplayRole);
        mSortFilterProxyModel->sort(0, sortOrder);
        if (!Settings::settings()->showDateColumnOnAlphabeticalSort->get().toBool())
        {
            mDocumentsView->documentTreeView()->hideColumn(1);
            mDocumentsView->documentTreeView()->hideColumn(2);
        }
    }
}

void DocumentController::onSortOrderChanged(bool order)
{
    int kindIndex  = mDocumentsView->sortKind()->currentIndex();
    int orderIndex = order ? DocumentController::DESC : DocumentController::ASC;

    sortDocuments(kindIndex, orderIndex);

    Settings::settings()->documentSortOrder->setInt(orderIndex);
}

void DocumentController::onSortKindChanged(int index)
{
    int orderIndex = mDocumentsView->sortOrder()->isChecked() ? DocumentController::DESC : DocumentController::ASC;

    sortDocuments(index, orderIndex);

    Settings::settings()->documentSortKind->setInt(index);
}

void DocumentController::onSplitterMoved(int size, int index)
{
    Q_UNUSED(index);
    Settings::settings()->documentSplitterLeftSize->setInt(size);
    Settings::settings()->documentSplitterRightSize->setInt(controlView()->size().width() - size);
}

QWidget *DocumentController::controlView()
{
    return mDocumentWidget;
}

void DocumentController::setupToolbar()
{
    Application::app()->insertSpaceToToolbarBeforeAction(mMainWindow->ui().documentToolBar, mMainWindow->ui().actionBoard);
    connect(mMainWindow->ui().actionDocumentTools, &QAction::triggered, this, &DocumentController::toggleDocumentToolsPalette);
}

void DocumentController::setupPalettes()
{
    mToolsPalette = new DocumentToolsPalette(controlView());

    mToolsPalette->hide();

    bool showToolsPalette = !mToolsPalette->isEmpty();
    mMainWindow->ui().actionDocumentTools->setVisible(showToolsPalette);

    if (showToolsPalette)
    {
        mMainWindow->ui().actionDocumentTools->trigger();
    }
}

void DocumentController::show()
{
    selectDocument(mBoardController->selectedDocument());

    // 看板可能在文档模式不可见期间改过页面，重新加载一次保证缩略图与磁盘一致。
    reloadThumbnails();

    updateActions();

    if (!mToolsPalette)
    {
        setupPalettes();
    }
}

void DocumentController::hide()
{
    // 离开文档模式不需要额外处理：缩略图与选择状态保留，下次 show() 会刷新。
}

void DocumentController::openSelectedItem()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QList<QGraphicsItem *> selectedItems = mDocumentsView->thumbnailWidget()->selectedItems();

    if (selectedItems.count() > 0)
    {
        SceneThumbnailPixmap *thumb = dynamic_cast<SceneThumbnailPixmap *>(selectedItems.last());

        if (thumb)
        {
            DocumentProxy *proxy = thumb->proxy();

            if (proxy && isOKToOpenDocument(proxy))
            {
                mBoardController->setActiveDocumentScene(proxy, thumb->sceneIndex());
                Application::applicationController->showBoard();
            }
        }
    }
    else
    {
        DocumentProxy *proxy = selectedDocumentProxy();

        if (proxy && isOKToOpenDocument(proxy))
        {
            mBoardController->setActiveDocumentScene(proxy);
            Application::applicationController->showBoard();
        }
    }

    QApplication::restoreOverrideCursor();
}

void DocumentController::duplicateSelectedItem()
{
    if (Application::applicationController->displayMode() != ApplicationController::Document)
    {
        return;
    }

    if (mSelectionType == Page)
    {
        QList<QGraphicsItem *> selectedItems = mDocumentsView->thumbnailWidget()->selectedItems();
        QList<int>             selectedSceneIndexes;
        foreach (QGraphicsItem *item, selectedItems)
        {
            SceneThumbnailPixmap *thumb = dynamic_cast<SceneThumbnailPixmap *>(item);
            if (thumb)
            {
                DocumentProxy *proxy = thumb->proxy();

                if (proxy)
                {
                    int sceneIndex = thumb->sceneIndex();
                    selectedSceneIndexes << sceneIndex;
                }
            }
        }
        if (selectedSceneIndexes.count() > 0)
        {
            duplicatePages(selectedSceneIndexes);
            emit documentThumbnailsUpdated(this);
            selectedDocument()->setMetaData(Settings::documentUpdatedAt, StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
            persistDocumentMetadata(selectedDocument());
            int selectedThumbnail = selectedSceneIndexes.last() + selectedSceneIndexes.size();
            mDocumentsView->thumbnailWidget()->selectItemAt(selectedThumbnail);
            int sceneCount = selectedSceneIndexes.count();
            showMessage(tr("duplicated %1 page", "duplicated %1 pages", sceneCount).arg(sceneCount), false);

            mBoardController->setActiveDocumentScene(selectedThumbnail);
            mBoardController->reloadThumbnails();
        }
    }
    else
    {
        DocumentTreeModel *docModel      = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;
        QModelIndex        selectedIndex = firstSelectedTreeIndex();

        Q_ASSERT(!docModel->isConstant(selectedIndex) && !docModel->inTrash(selectedIndex));

        showMessage(tr("Duplicating Document %1").arg(""), true);

        docModel->copyIndexToNewParent(selectedIndex, selectedIndex.parent(), DocumentTreeModel::aContentCopy);

        showMessage(tr("Document %1 copied").arg(""), false);
    }

    emit reorderDocumentsRequested();
}

void DocumentController::deleteSelectedItem()
{
    DocumentTreeModel *docModel = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

    if (!Application::mainWindow->yesNoQuestion(tr("Remove Item"), tr("Are you sure you want to remove the selected item(s) ?")))
    {
        return;
    }

    QModelIndexList indexes = selectedTreeIndexes();

    if (indexes.size() > 1)
    {
        deleteMultipleItems(indexes, docModel);
    }
    else if (indexes.size() == 1)
    {
        deleteSingleItem(indexes.at(0), docModel);
    }

    updateActions();
}

void DocumentController::deleteMultipleItems(QModelIndexList indexes, DocumentTreeModel *docModel)
{
    if (mSelectionType == Page)
    {
        deletePages(mDocumentsView->thumbnailWidget()->selectedItems());
        return;
    }

    // 每个索引按自己的位置各算各的删除方式。混选“回收站 / 我的文档 / 普通文档”时，
    // 过去拿 indexes.at(0) 的结果套全程：普通文档会被 CompleteDelete 永久删除；
    // 把回收站 MoveToTrash 到它自己则让 emptyFolder 的 while (rowCount()) 永不结束。
    // 用 QPersistentModelIndex 保存选择：每删/移走一项，后面索引的行号会被自动校正，
    // 不会拿着过期的 row 去 removeRow。
    QList<QPersistentModelIndex> pending;
    QSet<QPersistentModelIndex>  selected;
    pending.reserve(indexes.size());
    foreach (const QModelIndex &index, indexes)
    {
        QPersistentModelIndex persistent(index);
        if (persistent.isValid())
        {
            pending.append(persistent);
            selected.insert(persistent);
        }
    }

    for (const QPersistentModelIndex &persistent : pending)
    {
        if (!persistent.isValid())
        {
            continue;
        }

        // 祖先也被选中时跳过：处理祖先时已经把整棵子树搬走或删掉了。
        bool ancestorSelected = false;
        for (QModelIndex ancestor = persistent.parent(); ancestor.isValid(); ancestor = ancestor.parent())
        {
            if (selected.contains(QPersistentModelIndex(ancestor)))
            {
                ancestorSelected = true;
                break;
            }
        }
        if (ancestorSelected)
        {
            continue;
        }

        const QModelIndex index = persistent;
        switch (deletionTypeForSelection(mSelectionType, index, docModel))
        {
            case DeletePage:
                deletePages(mDocumentsView->thumbnailWidget()->selectedItems());
                break;
            case MoveToTrash:
                moveIndexesToTrash(QModelIndexList{index}, docModel);
                break;
            case CompleteDelete:
            {
                DocumentProxy *newProxy = docModel->proxyData(index);
                if (newProxy)
                {
                    pureSetDocument(newProxy);
                }
                emit documentThumbnailsUpdated(this);
                deleteIndexAndAssociatedData(index);
                break;
            }
            case EmptyFolder:
            {
                if (index == docModel->myDocumentsIndex())
                { // 清空“我的文档”：所有条目移入回收站，随后补一个空白文档
                    while (docModel->rowCount(index))
                    {
                        QModelIndex subIndex = docModel->index(0, 0, index);
                        if (!subIndex.isValid())
                        {
                            break;
                        }
                        docModel->moveIndex(subIndex, docModel->trashIndex());
                    }
                    createEmptyDocument();
                }
                else
                {
                    DocumentTreeNode *currentNode        = docModel->nodeFromIndex(index);
                    bool              deleteCurrentScene = currentNode->findNode(docModel->nodeFromIndex(docModel->currentIndex()));

                    emptyFolder(index, MoveToTrash); // Empty constant folder

                    if (deleteCurrentScene)
                    {
                        createEmptyDocument();
                    }
                }
                break;
            }
            case EmptyTrash:
                emptyFolder(index, CompleteDelete); // Empty trash folder
                break;
            case NoDeletion:
                break;
        }
    }
}

void DocumentController::deleteSingleItem(QModelIndex currentIndex, DocumentTreeModel *docModel)
{
    DeletionType deletionForSelection = deletionTypeForSelection(mSelectionType, currentIndex, docModel);

    switch (deletionForSelection)
    {
        case DeletePage:
        {
            deletePages(mDocumentsView->thumbnailWidget()->selectedItems());
            break;
        }
        case MoveToTrash:
        {
            moveToTrash(currentIndex, docModel);
            break;
        }
        case CompleteDelete:
        {
            // 用形参传进来的模型，与上面 MoveToTrash 分支保持一致；
            // 原先在这里又去取全局模型，同名遮蔽了形参，换个调用方就会各算各的。
            DocumentProxy *newProxy = docModel->proxyData(currentIndex);
            if (newProxy)
            {
                pureSetDocument(newProxy);
            }
            emit documentThumbnailsUpdated(this);
            deleteIndexAndAssociatedData(currentIndex);

            break;
        }
        case EmptyFolder:
        {
            if (currentIndex == docModel->myDocumentsIndex())
            { // 清空“我的文档”：所有条目移入回收站，随后补一个空白文档
                int startInd = 0;
                // 每次总删第 0 行：移走一项后后面的行号自动前移，直到目录为空。
                while (docModel->rowCount(currentIndex))
                {
                    QModelIndex testSubINdecurrentIndex = docModel->index(startInd, 0, currentIndex);
                    if (!testSubINdecurrentIndex.isValid())
                    {
                        break;
                    }
                    docModel->moveIndex(testSubINdecurrentIndex, docModel->trashIndex());
                }
                // 走到这里说明当前场景已被移走，补一份空白文档让看板有内容可显示。
                createEmptyDocument();
            }
            else
            {
                // 先判断当前场景是否位于将被清空的目录里，清空后需要新建空白文档顶替。
                DocumentTreeNode *currentNode        = docModel->nodeFromIndex(currentIndex);
                bool              deleteCurrentScene = currentNode->findNode(docModel->nodeFromIndex(docModel->currentIndex()));

                emptyFolder(currentIndex, MoveToTrash); // 清空普通目录：内容移入回收站

                if (deleteCurrentScene)
                {
                    createEmptyDocument();
                }
            }

            break;
        }
        case EmptyTrash:
        {
            emptyFolder(currentIndex, CompleteDelete); // Empty trash folder
            break;
        }
    }
}

void DocumentController::moveIndexesToTrash(const QModelIndexList &list, DocumentTreeModel *docModel)
{
    QModelIndex currentScene                = docModel->indexForNode(docModel->currentNode());

    // 判断当前文档是否在待移走的集合里：先看它自身是否被选中，
    // 再看它是否位于某个被选中的目录之下。
    QItemSelectionModel *selectionModel     = mDocumentsView->documentTreeView()->selectionModel();
    bool                 deleteCurrentScene = selectionModel->isSelected(mSortFilterProxyModel->mapFromSource(currentScene));

    if (!deleteCurrentScene)
    {
        for (int i = 0; i < list.size(); i++)
        {
            deleteCurrentScene = docModel->isDescendantOf(currentScene, list.at(i));

            if (deleteCurrentScene)
            {
                break;
            }
        }
    }

    QModelIndex proxyMapCurentScene = mSortFilterProxyModel->mapFromSource(currentScene);

    if (deleteCurrentScene)
    {
        QModelIndex sibling = findPreviousSiblingNotSelected(proxyMapCurentScene, selectionModel);

        if (sibling.isValid())
        {
            QModelIndex sourceSibling = mSortFilterProxyModel->mapToSource(sibling);

            DocumentProxy *proxy      = docModel->proxyForIndex(sourceSibling);

            if (proxy)
            {
                selectDocument(proxy, true);

                deleteCurrentScene = false;
            }
        }
        else
        {
            sibling = findNextSiblingNotSelected(proxyMapCurentScene, selectionModel);

            if (sibling.isValid())
            {
                QModelIndex sourceSibling = mSortFilterProxyModel->mapToSource(sibling);

                DocumentProxy *proxy      = docModel->proxyForIndex(sourceSibling);

                if (proxy)
                {
                    selectDocument(proxy, true);

                    deleteCurrentScene = false;
                }
            }
        }
    }
    else
    {
        DocumentProxy *proxy = docModel->proxyForIndex(currentScene);
        selectDocument(proxy, true);
    }

    docModel->moveIndexes(list, docModel->trashIndex());

    if (deleteCurrentScene)
    {
        // 当前文档已被移入回收站且没有可回落的兄弟项，补一份空白文档。
        createEmptyDocument();
    }
}

QModelIndex DocumentController::findPreviousSiblingNotSelected(const QModelIndex &index, QItemSelectionModel *selectionModel)
{
    QModelIndex sibling = index.sibling(index.row() - 1, 0);

    if (sibling.isValid())
    {
        if (!parentIsSelected(sibling, selectionModel) && !selectionModel->isSelected(sibling))
        {
            QModelIndex model = mSortFilterProxyModel->mapToSource(sibling);

            if (PersistenceManager::persistenceManager()->mDocumentTreeStructureModel->isCatalog(model))
            {
                return findPreviousSiblingNotSelected(sibling, selectionModel);
            }
            else
            {
                return sibling;
            }
        }
        else
        {
            return findPreviousSiblingNotSelected(sibling, selectionModel);
        }
    }
    else
    {
        // 没有上一个兄弟项时退到父级继续查找，直到顶层；父级无效说明已经找遍。
        QModelIndex parent = index.model()->parent(index);

        if (parent.isValid())
        {
            return findPreviousSiblingNotSelected(parent, selectionModel);
        }
        else
        {
            return QModelIndex();
        }
    }
}

QModelIndex DocumentController::findNextSiblingNotSelected(const QModelIndex &index, QItemSelectionModel *selectionModel)
{
    QModelIndex sibling = index.sibling(index.row() + 1, 0);

    if (sibling.isValid())
    {
        if (!parentIsSelected(sibling, selectionModel) && !selectionModel->isSelected(sibling))
        {
            QModelIndex model = mSortFilterProxyModel->mapToSource(sibling);

            if (PersistenceManager::persistenceManager()->mDocumentTreeStructureModel->isCatalog(model))
            {
                return findNextSiblingNotSelected(sibling, selectionModel);
            }
            else
            {
                return sibling;
            }
        }
        else
        {
            return findNextSiblingNotSelected(sibling, selectionModel);
        }
    }
    else
    {
        // 没有下一个兄弟项时退到父级继续查找，直到顶层；父级无效说明已经找遍。
        QModelIndex parent = index.parent();

        if (parent.isValid())
        {
            return findNextSiblingNotSelected(parent, selectionModel);
        }
        else
        {
            return QModelIndex();
        }
    }
}

bool DocumentController::parentIsSelected(const QModelIndex &child, QItemSelectionModel *selectionModel)
{
    QModelIndex parent = child.parent();

    while (parent.isValid())
    {
        if (selectionModel->isSelected(parent))
        {
            return true;
        }

        parent = parent.parent();
    }

    return false;
}

void DocumentController::moveToTrash(QModelIndex &index, DocumentTreeModel *docModel)
{
    QModelIndexList list;
    list.push_back(index);
    moveIndexesToTrash(list, docModel);
}

void DocumentController::deleteDocumentsInFolderOlderThan(const QModelIndex &index, const int days)
{
    DocumentTreeModel *docModel = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

    // 每一轮都现取索引，并倒着走。这里其实不会错位——deleteDocument 只删盘上的文件和场景，
    // 模型行留着（代理走 deleteLater，行由 deleteEmptyFolders 那一路摘），所以行号在整个循环里
    // 是稳的；倒着走的好处是万一有别的路径往这一层插了新行（删到当前活动文档时
    // documentWillBeDeleted 会造一份新文档），那些行不会被顺手扫到。
    for (int i = docModel->rowCount(index) - 1; i >= 0; i--)
    {
        QModelIndex child            = docModel->index(i, 0, index);

        DocumentProxy *documentProxy = docModel->proxyForIndex(child);

        if (documentProxy)
        {
            if (documentProxy->lastUpdate().date() < QDateTime::currentDateTime().addDays(-days).date())
            {
                PersistenceManager::persistenceManager()->deleteDocument(documentProxy);
            }
        }
        else
        {
            if (docModel->isCatalog(child))
            {
                deleteDocumentsInFolderOlderThan(child, days);
            }
        }
    }
}

void DocumentController::deleteEmptyFolders(const QModelIndex &index)
{
    DocumentTreeModel *docModel = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

    const int childCount        = docModel->rowCount(index);

    // 倒着走，而且每一轮都现取索引：deleteIndexAndAssociatedData 会 removeRow 掉正在处理这一行，
    // 同一父节点下比它靠后的行号全体前移一位。旧写法先把整层快照成 QModelIndexList 再逐个删，
    // 于是 containsDocuments() 拿到的是移过位的过期索引——判"这个目录是空的吗"判错就会
    // 删掉还有文档的目录。倒序保证未处理的行号不会变，现取保证索引不是过期的，两者缺一不可。
    for (int i = childCount - 1; i >= 0; i--)
    {
        QModelIndex child = docModel->index(i, 0, index);

        if (docModel->isCatalog(child))
        {
            if (!docModel->containsDocuments(child))
            {
                deleteIndexAndAssociatedData(child);
            }
        }
    }

    if (childCount == 0 && docModel->isCatalog(index))
    {
        deleteIndexAndAssociatedData(index);
    }
}

void DocumentController::emptyFolder(const QModelIndex &index, DeletionType pDeletionType)
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    DocumentTreeModel *docModel = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;
    if (!docModel->isCatalog(index))
    {
        return;
    }
    // 始终删除第 0 行：每移走/删掉一项，剩余条目的行号都会前移，直到目录清空。
    while (docModel->rowCount(index))
    {
        QModelIndex subIndex = docModel->index(0, 0, index);
        switch (pDeletionType)
        {
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
}

void DocumentController::deleteIndexAndAssociatedData(const QModelIndex &pIndex)
{
    DocumentTreeModel *docModel = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;
    // 自底向上删除：先清空子树，保证删除目录时其下每份文档都走到 deleteDocument。
    while (docModel->rowCount(pIndex))
    {
        QModelIndex subIndex = docModel->index(0, 0, pIndex);
        deleteIndexAndAssociatedData(subIndex);
    }

    if (pIndex.column() == 0)
    {
        if (docModel->isDocument(pIndex))
        {
            DocumentProxy *proxyData = docModel->proxyData(pIndex);

            if (proxyData)
            {
                PersistenceManager::persistenceManager()->deleteDocument(proxyData);
            }
        }
    }

    docModel->removeRow(pIndex.row(), pIndex.parent());
}

void DocumentController::exportDocument()
{
    QAction  *currentExportAction = qobject_cast<QAction *>(sender());
    const int adaptorIndex        = currentExportAction->data().toInt();

    DocumentProxy *proxy          = firstSelectedTreeProxy();

    DocumentManager::documentManager()->exportDocumentWithAdaptor(adaptorIndex, proxy, firstSelectedTreeIndex());
    emit exportDone();
}

void DocumentController::exportDocumentSet()
{
}

void DocumentController::documentZoomSliderValueChanged(int value)
{
    mDocumentsView->thumbnailWidget()->setThumbnailWidth(value);

    Settings::settings()->documentThumbnailWidth->set(value);
}

void DocumentController::importFile()
{
    DocumentManager *docManager = DocumentManager::documentManager();

    QString defaultPath         = Settings::settings()->lastImportFilePath->get().toString();
    if (defaultPath.isDetached())
    {
        defaultPath = Settings::settings()->userDocumentDirectory();
    }
    QString filePath = QFileDialog::getOpenFileName(mParentWidget, tr("Open Supported File"), defaultPath, docManager->importFileFilter());

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    QApplication::processEvents();
    QFileInfo fileInfo(filePath);

    if (fileInfo.suffix().toLower() == "tbx")
    {
        PersistenceManager::persistenceManager()->createDocumentProxiesStructure(
            docManager->importUbx(filePath, Settings::userDocumentDirectory()), true);
    }
    else
    {
        Settings::settings()->lastImportFilePath->set(QVariant(fileInfo.absolutePath()));

        if (filePath.length() > 0)
        {
            DocumentProxy *createdDocument = 0;
            QApplication::processEvents();
            QFile selectedFile(filePath);

            DocumentTreeModel *docModel = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

            QModelIndex selectedIndex   = firstSelectedTreeIndex();
            QString     groupName       = "";
            if (selectedIndex.isValid())
            {
                groupName =
                    docModel->isCatalog(selectedIndex) ? docModel->virtualPathForIndex(selectedIndex) : docModel->virtualDirForIndex(selectedIndex);
            }

            showMessage(tr("Importing file %1...").arg(fileInfo.baseName()), true);

            createdDocument = docManager->importFile(selectedFile, groupName);

            if (createdDocument)
            {
                selectDocument(createdDocument, true, true, true);
            }
            else
            {
                showMessage(tr("Failed to import file ... "));
            }
        }
    }

    QApplication::restoreOverrideCursor();
}

void DocumentController::addFolderOfImages()
{
    DocumentProxy *document = selectedDocumentProxy();

    if (document)
    {
        QString defaultPath = Settings::settings()->lastImportFolderPath->get().toString();

        QString imagesDir   = QFileDialog::getExistingDirectory(mParentWidget, tr("Import all Images from Folder"), defaultPath);
        QDir    parentImageDir(imagesDir);
        parentImageDir.cdUp();

        Settings::settings()->lastImportFolderPath->set(QVariant(parentImageDir.absolutePath()));

        if (imagesDir.length() > 0)
        {
            QDir dir(imagesDir);

            int importedImageNumber = DocumentManager::documentManager()->addImageDirToDocument(dir, document);

            if (importedImageNumber == 0)
            {
                showMessage(tr("Folder does not contain any image files"));
                Application::applicationController->showDocument();
            }
            else
            {
                document->setMetaData(Settings::documentUpdatedAt, StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
                persistDocumentMetadata(document);
                reloadThumbnails();
            }
        }
    }
}

void DocumentController::addFileToDocument()
{
    DocumentProxy *document = selectedDocumentProxy();

    if (document)
    {
        addFileToDocument(document);
        reloadThumbnails();
    }
}

DocumentProxyTreeItem *DocumentController::findDocument(DocumentProxy *proxy)
{
    // 旧树条目类型已不在文档视图中使用，这里保留选中行为并返回空。
    selectDocument(proxy);
    return NULL;
}

bool DocumentController::addFileToDocument(DocumentProxy *document)
{
    QString defaultPath = Settings::settings()->lastImportFilePath->get().toString();
    QString filePath    = QFileDialog::getOpenFileName(
        mParentWidget, tr("Open Supported File"), defaultPath, DocumentManager::documentManager()->importFileFilter(true));

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    QApplication::processEvents();

    QFileInfo fileInfo(filePath);
    Settings::settings()->lastImportFilePath->set(QVariant(fileInfo.absolutePath()));

    bool success = false;

    if (filePath.length() > 0)
    {
        QApplication::processEvents(); // 前面几行刚 processEvents 过一次，这里是否真有必要再来一次没把握；先保持原样?
        QFile selectedFile(filePath);

        showMessage(tr("Importing file %1...").arg(fileInfo.baseName()), true);

        QStringList fileNames;
        fileNames << filePath;
        success = DocumentManager::documentManager()->addFilesToDocument(document, fileNames);

        if (success)
        {
            document->setMetaData(Settings::documentUpdatedAt, StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
            persistDocumentMetadata(document);
        }
        else
        {
            showMessage(tr("Failed to import file ... "));
        }
    }

    QApplication::restoreOverrideCursor();

    return success;
}

void DocumentController::moveSceneToIndex(DocumentProxy *proxy, int source, int target)
{
    if (DocumentContainer::movePageToIndex(source, target))
    {
        proxy->setMetaData(Settings::documentUpdatedAt, StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
        persistDocumentMetadata(proxy);

        mDocumentsView->thumbnailWidget()->hightlightItem(target);

        mBoardController->setActiveDocumentScene(target);
        mBoardController->reloadThumbnails();
    }
}

void DocumentController::thumbnailViewResized()
{
    int maxWidth = qMin(Settings::maxThumbnailWidth, mDocumentsView->thumbnailWidget()->width());

    mDocumentsView->documentZoomSlider()->setMaximum(maxWidth);
}

void DocumentController::pageSelectionChanged()
{
    if (mIsClosing)
    {
        return;
    }

    DocumentTreeModel *docModel = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

    bool pageSelected           = mDocumentsView->thumbnailWidget()->selectedItems().count() > 0;
    bool docSelected            = docModel->isDocument(firstSelectedTreeIndex());
    bool folderSelected         = docModel->isCatalog(firstSelectedTreeIndex());

    if (pageSelected)
    {
        itemSelectionChanged(Page);
    }
    else if (docSelected)
    {
        itemSelectionChanged(Document);
    }
    else if (folderSelected)
    {
        itemSelectionChanged(Folder);
    }
    else
    {
        itemSelectionChanged(None);
    }

    updateActions();
}

void DocumentController::documentSceneChanged(DocumentProxy *proxy, int pSceneIndex)
{
    Q_UNUSED(pSceneIndex);

    if (proxy == selectedDocumentProxy())
    {
        reloadThumbnails();
    }

    QModelIndexList sel = mDocumentsView->documentTreeView()->selectionModel()->selectedRows(0);

    QModelIndex selection;
    if (sel.count() > 0)
    {
        selection = sel.first();
    }

    TreeViewSelectionChanged(selection, QModelIndex());
}

void DocumentController::thumbnailPageDoubleClicked(QGraphicsItem *item, int index)
{
    DocumentTreeModel *docModel      = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;
    QModelIndex        selectedIndex = firstSelectedTreeIndex();

    if (selectedIndex.isValid())
    {
        if (docModel->inTrash(selectedIndex))
        {
            return;
        }
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    SceneThumbnailPixmap *thumb = qgraphicsitem_cast<SceneThumbnailPixmap *>(item);

    if (thumb)
    {
        DocumentProxy *proxy = thumb->proxy();
        if (proxy && isOKToOpenDocument(proxy))
        {
            mBoardController->setActiveDocumentScene(proxy, index);
            Application::applicationController->showBoard();
        }
    }

    QApplication::restoreOverrideCursor();
}

void DocumentController::pageClicked(QGraphicsItem *item, int index)
{
    Q_UNUSED(item);
    Q_UNUSED(index);

    pageSelectionChanged();
}

void DocumentController::addToDocument()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QList<QGraphicsItem *> selectedItems = mDocumentsView->thumbnailWidget()->selectedItems();

    if (selectedItems.count() > 0)
    {
        int oldActiveSceneIndex = mBoardController->activeSceneIndex();

        QList<QPair<DocumentProxy *, int>> pageInfoList;

        foreach (QGraphicsItem *item, selectedItems)
        {
            SceneThumbnailPixmap *thumb = dynamic_cast<SceneThumbnailPixmap *>(item);

            if (thumb && thumb->proxy())
            {
                QPair<DocumentProxy *, int> pageInfo(thumb->proxy(), thumb->sceneIndex());
                pageInfoList << pageInfo;
            }
        }

        for (int i = 0; i < pageInfoList.length(); i++)
        {
            mBoardController->addScene(pageInfoList.at(i).first, pageInfoList.at(i).second, false);
        }

        int newActiveSceneIndex = selectedItems.count() == mBoardController->selectedDocument()->pageCount() ? 0 : oldActiveSceneIndex + 1;
        mDocumentsView->thumbnailWidget()->selectItemAt(newActiveSceneIndex, false);
        selectDocument(mBoardController->selectedDocument());
        mBoardController->selectedDocument()->setMetaData(Settings::documentUpdatedAt, StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
        persistDocumentMetadata(mBoardController->selectedDocument());
        mBoardController->reloadThumbnails();

        emit BoardController::instance() -> documentThumbnailsUpdated(this);
        Application::applicationController->showBoard();

        mBoardController->setActiveDocumentScene(newActiveSceneIndex);
    }

    QApplication::restoreOverrideCursor();
}

void DocumentController::renameSelectedItem()
{
    if (mDocumentsView->documentTreeView()->currentIndex().isValid())
    {
        mDocumentsView->documentTreeView()->edit(mDocumentsView->documentTreeView()->currentIndex());
    }
}

bool DocumentController::isOKToOpenDocument(DocumentProxy *proxy)
{
    QString docVersion = proxy->metaData(Settings::documentVersion).toString();

    // 这里曾经写死一份 "1.1"/"1.2" 白名单，而读写器早就在往每个文档盖 4.8.0，
    // 结果自己存的文档也被判成“比本程序还新”，每次打开都弹一次丢失警告。
    // 现在拿真正的版本序比：只有确实更新于当前格式版本时才问。
    if (docVersion.isEmpty() || !(Version(docVersion) > Version(Settings::currentFileVersion)))
    {
        return true;
    }
    else
    {
        if (Application::mainWindow->yesNoQuestion(
                tr("Open Document"), tr("The document '%1' has been generated with a newer version of TianBoard (%2). By opening "
                                        "it, you may lose some information. Do you want to proceed?")
                                         .arg(proxy->metaData(Settings::documentName).toString())
                                         .arg(docVersion)))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
}

void DocumentController::showMessage(const QString &message, bool showSpinningWheel)
{
    if (mMessageWindow)
    {
        int margin    = Settings::boardMargin;

        QRect newSize = mDocumentsView->thumbnailWidget()->geometry();

        mMessageWindow->move(margin, newSize.height() - mMessageWindow->height() - margin);

        mMessageWindow->showMessage(message, showSpinningWheel);
    }
}

void DocumentController::hideMessage()
{
    if (mMessageWindow)
    {
        mMessageWindow->hideMessage();
    }
}

void DocumentController::addImages()
{
    DocumentProxy *document = selectedDocumentProxy();

    if (document)
    {
        QString defaultPath = Settings::settings()->lastImportFolderPath->get().toString();

        QString extensions;

        foreach (QString ext, Settings::settings()->imageFileExtensions)
        {
            extensions += " *.";
            extensions += ext;
        }

        QStringList images =
            QFileDialog::getOpenFileNames(mParentWidget, tr("Add all Images to Document"), defaultPath, tr("All Images (%1)").arg(extensions));

        if (images.length() > 0)
        {
            QFileInfo firstImage(images.at(0));

            Settings::settings()->lastImportFolderPath->set(QVariant(firstImage.absoluteDir().absolutePath()));

            int importedImageNumber = DocumentManager::documentManager()->addFilesToDocument(document, images);

            if (importedImageNumber == 0)
            {
                UserNotices::presentStatusMessage(tr("Selection does not contain any image files!"));
                Application::applicationController->showDocument();
            }
            else
            {
                document->setMetaData(Settings::documentUpdatedAt, StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
                persistDocumentMetadata(document);
                reloadThumbnails();
            }
        }
    }
}

void DocumentController::toggleDocumentToolsPalette()
{
    if (!mToolsPalette->isVisible() && !mToolsPalettePositionned)
    {
        mToolsPalette->adjustSizeAndPosition();
        int left = controlView()->width() - 20 - mToolsPalette->width();
        int top  = (controlView()->height() - mToolsPalette->height()) / 2;

        mToolsPalette->setCustomPosition(true);
        mToolsPalette->move(left, top);

        mToolsPalettePositionned = true;
    }

    bool visible = mToolsPalette->isVisible();
    mToolsPalette->setVisible(!visible);
}

void DocumentController::focusChanged(QWidget *old, QWidget *current)
{
    DocumentTreeModel *treeModel = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

    if (current == mDocumentsView->thumbnailWidget())
    {
        if (mDocumentsView->thumbnailWidget()->selectedItems().count() > 0)
        {
            mSelectionType = Page;
        }
        else
        {
            mSelectionType = None;
        }
    }
    else if (current == mDocumentsView->documentTreeView())
    {
        if (treeModel->isDocument(firstSelectedTreeIndex()))
        {
            mSelectionType = Document;
        }
        else if (treeModel->isCatalog(firstSelectedTreeIndex()))
        {
            mSelectionType = Folder;
        }
        else
        {
            mSelectionType = None;
        }
    }
    else if (current == mDocumentsView->documentZoomSlider())
    {
        if (mDocumentsView->thumbnailWidget()->selectedItems().count() > 0)
        {
            mSelectionType = Page;
        }
        else
        {
            mSelectionType = None;
        }
    }
    else
    {
        if (old != mDocumentsView->thumbnailWidget() && old != mDocumentsView->documentTreeView() && old != mDocumentsView->documentZoomSlider())
        {
            if (current && (current->metaObject()->className() != QPushButton::staticMetaObject.className()))
            {
                mSelectionType = None;
            }
        }
    }
}

void DocumentController::updateActions()
{
    if (mIsClosing)
    {
        return;
    }

    DocumentTreeModel *docModel = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;

    QModelIndexList list        = mDocumentsView->documentTreeView()->selectionModel()->selectedRows(0);

    // 多选时只保留“删除”可用：重命名、复制、导出等操作都需要唯一目标。
    if (list.count() > 1)
    {
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

    QModelIndex    selectedIndex = firstSelectedTreeIndex();
    DocumentProxy *selectedProxy = docModel->proxyData(selectedIndex);
    int            pageCount     = -1;
    if (selectedProxy)
    {
        pageCount = selectedProxy->pageCount();
    }

    bool pageSelected  = false;
    bool groupSelected = false;
    bool docSelected   = false;

    if (mSelectionType == Page)
    {
        pageSelected = true;
    }
    else
    {
        if (docModel->isDocument(firstSelectedTreeIndex()))
        {
            docSelected = true;
        }
        else if (docModel->isCatalog(firstSelectedTreeIndex()))
        {
            groupSelected = true;
        }
    }

    bool trashSelected = docModel->inTrash(selectedIndex) || selectedIndex == docModel->trashIndex() ? true : false;

    mMainWindow->ui().actionNewDocument->setEnabled(docModel->newNodeAllowed(selectedIndex));
    mMainWindow->ui().actionNewFolder->setEnabled(docModel->newNodeAllowed(selectedIndex));
    mMainWindow->ui().actionExport->setEnabled((docSelected || pageSelected || groupSelected) && !trashSelected);
    DocumentManager::documentManager()->refreshExportActions(selectedIndex);

    bool firstSceneSelected = false;

    if (docSelected)
    {
        mMainWindow->ui().actionDuplicate->setEnabled(!trashSelected);
    }
    else if (pageSelected)
    {
        QList<QGraphicsItem *> selection = mDocumentsView->thumbnailWidget()->selectedItems();
        if (pageCount == 1)
        {
            mMainWindow->ui().actionDuplicate->setEnabled(!trashSelected && pageCanBeDuplicated(DocumentContainer::pageFromSceneIndex(0)));
        }
        else
        {
            for (int i = 0; i < selection.count() && !firstSceneSelected; i += 1)
            {
                if (qgraphicsitem_cast<SceneThumbnailPixmap *>(selection.at(i))->sceneIndex() == 0)
                {
                    mMainWindow->ui().actionDuplicate->setEnabled(!trashSelected && pageCanBeDuplicated(DocumentContainer::pageFromSceneIndex(0)));
                    firstSceneSelected = true;
                    break;
                }
            }
            if (!firstSceneSelected)
            {
                mMainWindow->ui().actionDuplicate->setEnabled(!trashSelected);
            }
        }
    }
    else
    {
        mMainWindow->ui().actionDuplicate->setEnabled(false);
    }

    mMainWindow->ui().actionOpen->setEnabled((docSelected || pageSelected) && !trashSelected);
    mMainWindow->ui().actionRename->setEnabled(docModel->isOkToRename(selectedIndex));

    mMainWindow->ui().actionAddToWorkingDocument->setEnabled(
        pageSelected && !(selectedProxy == mBoardController->selectedDocument()) && !trashSelected);

    DeletionType deletionForSelection = deletionTypeForSelection(mSelectionType, selectedIndex, docModel);
    mMainWindow->ui().actionDelete->setEnabled(deletionForSelection != NoDeletion);

    switch (static_cast<int>(deletionForSelection))
    {
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

void DocumentController::currentIndexMoved(const QModelIndex &newIndex, const QModelIndex &PreviousIndex)
{
    Q_UNUSED(newIndex);
    Q_UNUSED(PreviousIndex);

    DocumentTreeModel *docModel = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;
    DocumentProxy     *newProxy = docModel->proxyData(newIndex);
    if (newProxy)
    {
        // 这个槽只在文档树内部拖拽换父级（MoveAction）时被发，newIndex 指向的是刚克隆到
        // 新位置的那个节点——它的生命周期归树节点，不会马上被销毁。真正即将消失的是
        // 源节点那一份，而 removeRows 会把容器与画板一起带走。
        // 原来这里 new 了一份 DocumentProxy 拷进容器、又把画板改回 newProxy：拷出来的那份
        // 从此没人接管（每拖一次漏一个代理），而容器还留在它上面，等于让文档库控制器
        // 认了一个不在树里的幽灵文档。
        pureSetDocument(newProxy);
        mBoardController->pureSetDocument(newProxy);
    }
    mCurrentIndexMoved = true;
}

void DocumentController::deletePages(QList<QGraphicsItem *> itemsToDelete)
{
    if (itemsToDelete.count() > 0)
    {
        QList<int>     sceneIndexes;
        DocumentProxy *proxy = 0;

        foreach (QGraphicsItem *item, itemsToDelete)
        {
            SceneThumbnailPixmap *thumb = dynamic_cast<SceneThumbnailPixmap *>(item);

            if (thumb)
            {
                proxy = thumb->proxy();
                if (proxy)
                {
                    sceneIndexes.append(thumb->sceneIndex());
                }
            }
        }
        DocumentContainer::deletePages(sceneIndexes);
        emit BoardController::instance() -> documentThumbnailsUpdated(this);

        proxy->setMetaData(Settings::documentUpdatedAt, StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
        persistDocumentMetadata(proxy);

        int minIndex = proxy->pageCount() - 1;
        foreach (int i, sceneIndexes)
        {
            minIndex = qMin(i, minIndex);
        }

        if (mBoardController->activeSceneIndex() > minIndex)
        {
            mBoardController->setActiveSceneIndex(minIndex);
        }

        mDocumentsView->thumbnailWidget()->selectItemAt(minIndex);

        mBoardController->setActiveDocumentScene(minIndex);
        mBoardController->reloadThumbnails();
    }
}

int DocumentController::getSelectedItemIndex()
{
    QList<QGraphicsItem *> selectedItems = mDocumentsView->thumbnailWidget()->selectedItems();

    if (selectedItems.count() > 0)
    {
        SceneThumbnailPixmap *thumb = dynamic_cast<SceneThumbnailPixmap *>(selectedItems.last());
        return thumb->sceneIndex();
    }
    else
    {
        return -1;
    }
}

bool DocumentController::pageCanBeMovedUp(int page)
{
    return page >= 1;
}

bool DocumentController::pageCanBeMovedDown(int page)
{
    return page < selectedDocument()->pageCount() - 1;
}

bool DocumentController::pageCanBeDuplicated(int page)
{
    return page != 0;
}

bool DocumentController::pageCanBeDeleted(int page)
{
    return page != 0;
}

void DocumentController::setDocument(DocumentProxy *document, bool forceReload)
{
    DocumentContainer::setDocument(document, forceReload);
}

QModelIndex DocumentController::firstSelectedTreeIndex()
{
    return selectedTreeIndexes().count() ? selectedTreeIndexes().first() : QModelIndex();
}

DocumentController::DeletionType DocumentController::deletionTypeForSelection(
    LastSelectedElementType pTypeSelection, const QModelIndex &selectedIndex, DocumentTreeModel *docModel) const
{
    if (pTypeSelection == Page)
    {
        if (!firstAndOnlySceneSelected())
        {
            return DeletePage;
        }
    }
    else if (docModel->isConstant(selectedIndex))
    {
        if (selectedIndex == docModel->trashIndex())
        {
            if (docModel->rowCount(selectedIndex) > 0)
            {
                return EmptyTrash;
            }
            else
            {
                return NoDeletion;
            }
        }

        if (selectedIndex.isValid())
        {
            return EmptyFolder;
        }
        else
        {
            return NoDeletion;
        }
    }
    else if (pTypeSelection != None)
    {
        if (docModel->inTrash(selectedIndex))
        {
            return CompleteDelete;
        }
        else
        {
            return MoveToTrash;
        }
    }

    return NoDeletion;
}

bool DocumentController::firstAndOnlySceneSelected() const
{
    bool                   firstSceneSelected = false;
    QList<QGraphicsItem *> selection          = mDocumentsView->thumbnailWidget()->selectedItems();
    for (int i = 0; i < selection.count() && !firstSceneSelected; i += 1)
    {
        SceneThumbnailPixmap *p = dynamic_cast<SceneThumbnailPixmap *>(selection.at(i));
        if (p)
        {
            int pageCount = p->proxy()->pageCount();
            if (pageCount > 1) // 页数大于 1 就不算“唯一的一页”
            {
                return false;
            }
            else
            {
                if (p->sceneIndex() == 0)
                {
                    return true; // 只有一页且就是第 0 页
                }
            }
        }
    }

    return false;
}

void DocumentController::refreshDocumentThumbnailsView(DocumentContainer *)
{
    DocumentTreeModel *docModel             = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel;
    DocumentProxy     *currentDocumentProxy = selectedDocument();

    QModelIndex current                     = docModel->indexForProxy(currentDocumentProxy);

    if (!current.isValid())
    {
        mDocumentsView->thumbnailWidget()->setGraphicsItems(QList<QGraphicsItem *>(), QList<QUrl>(), QStringList(), MimeTypes::boardPage);
        return;
    }

    QList<const QPixmap *> thumbs;

    if (currentDocumentProxy)
    {
        loadDocumentPageThumbnails(currentDocumentProxy, thumbs);
    }

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QList<QGraphicsItem *> items;
    QList<QUrl>            itemsPath;

    QGraphicsPixmapItem *selection = 0;

    QStringList labels;

    if (currentDocumentProxy)
    {
        for (int i = 0; i < currentDocumentProxy->pageCount(); i++)
        {
            const QPixmap       *pix        = thumbs.at(i);
            QGraphicsPixmapItem *pixmapItem = new SceneThumbnailPixmap(*pix, currentDocumentProxy, i); // 归场景（缩略图视图）所有，由它销毁

            if (currentDocumentProxy == mBoardController->selectedDocument() && mBoardController->activeSceneIndex() == i)
            {
                selection = pixmapItem;
            }

            items << pixmapItem;
            int pageIndex = pageFromSceneIndex(i);
            if (pageIndex)
            {
                labels << tr("Page %1").arg(pageIndex);
            }
            else
            {
                labels << tr("Title page");
            }

            itemsPath.append(
                QUrl::fromLocalFile(currentDocumentProxy->persistencePath() + QString("/pages/%1").arg(DocumentContainer::pageFromSceneIndex(i))));
        }
    }

    mDocumentsView->thumbnailWidget()->setGraphicsItems(items, itemsPath, labels, MimeTypes::boardPage);

    // documentPageThumbnail() 返回 new 出来的 QPixmap，SceneThumbnailPixmap 只是按值拷贝；
    // 不在这里释放的话，每刷新一次缩略图就泄漏 pageCount 个 QPixmap。
    qDeleteAll(thumbs);
    thumbs.clear();

    if (docModel->inTrash(current))
    {
        mDocumentsView->thumbnailWidget()->setDragEnabled(false);
    }
    else
    {
        mDocumentsView->thumbnailWidget()->setDragEnabled(true);
    }

    mDocumentsView->thumbnailWidget()->ensureVisible(0, 0, 10, 10);

    if (selection)
    {
        disconnect(mDocumentsView->thumbnailWidget()->scene(), &QGraphicsScene::selectionChanged, this, &DocumentController::pageSelectionChanged);
        SceneThumbnailPixmap *currentScene = dynamic_cast<SceneThumbnailPixmap *>(selection);
        if (currentScene)
        {
            mDocumentsView->thumbnailWidget()->hightlightItem(currentScene->sceneIndex());
        }
        connect(mDocumentsView->thumbnailWidget()->scene(), &QGraphicsScene::selectionChanged, this, &DocumentController::pageSelectionChanged);
    }

    QApplication::restoreOverrideCursor();
}

void DocumentController::createEmptyDocument()
{
    PersistenceManager *pManager = PersistenceManager::persistenceManager();
    DocumentTreeModel  *docModel = pManager->mDocumentTreeStructureModel;

    QString groupName            = docModel->virtualPathForIndex(docModel->myDocumentsIndex());

    DocumentProxy *document      = pManager->createDocument(groupName);
    selectDocument(document);

    if (document)
    {
        pManager->mDocumentTreeStructureModel->markDocumentAsNew(document);
    }
}

void DocumentController::collapseAll()
{
    // 折叠期间关闭动画：动画会访问即将失效的 viewport 状态，在大量条目上容易出问题。
    mDocumentsView->documentTreeView()->setAnimated(false);

    mDocumentsView->documentTreeView()->collapseAll();

    QPersistentModelIndex myDocumentIndex = PersistenceManager::persistenceManager()->mDocumentTreeStructureModel->myDocumentsIndex();

    SortFilterProxyModel *proxy           = dynamic_cast<SortFilterProxyModel *>(mDocumentsView->documentTreeView()->model());

    if (proxy)
    {
        mDocumentsView->documentTreeView()->setExpanded(proxy->mapFromSource(myDocumentIndex), true);
    }
    else
    {
        mDocumentsView->documentTreeView()->setExpanded(myDocumentIndex, true);
    }

    mDocumentsView->documentTreeView()->setAnimated(true);
}

void DocumentController::expandAll()
{
    mDocumentsView->documentTreeView()->setAnimated(false);

    mDocumentsView->documentTreeView()->expandAll();

    mDocumentsView->documentTreeView()->setAnimated(true);
}
