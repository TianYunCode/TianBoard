#include "whiteboard/BoardController.h"
#include <QRandomGenerator>
#include <QRegularExpression>
#include "core/VirtualDesktop.h"

#include <QtWidgets>
#include <QtWebEngineWidgets>

#include "utils/FileSystemUtils.h"
#include "utils/GeometryUtils.h"
#include "platform/PlatformUtils.h"

#include "core/Application.h"
#include "settings/Settings.h"
#include "settings/Setting.h"
#include "core/PersistenceManager.h"
#include "core/ApplicationController.h"
#include "core/DocumentManager.h"
#include "core/MimeData.h"
#include "core/DownloadManager.h"

#include "core/HttpGet.h"

#include "ui/MessageWindow.h"
#include "ui/Resources.h"
#include "ui/ToolbarButtonGroup.h"
#include "ui/MainWindow.h"
#include "ui/ToolWidget.h"
#include "ui/KeyboardPalette.h"
#include "ui/Magnifer.h"
#include "ui/DockPaletteWidget.h"

#include "whiteboard/GraphicsPixmapItem.h"
#include "whiteboard/GraphicsItemUndoCommand.h"
#include "whiteboard/GraphicsSvgItem.h"
#include "whiteboard/GraphicsWidgetItem.h"
#include "whiteboard/GraphicsMediaItem.h"
#include "whiteboard/GraphicsPDFItem.h"
#include "whiteboard/GraphicsTextItem.h"
#include "whiteboard/PageSizeUndoCommand.h"
#include "whiteboard/GraphicsGroupContainerItem.h"
#include "whiteboard/GraphicsStrokesGroup.h"
#include "whiteboard/Item.h"
#include "whiteboard/FeaturesController.h"

#include "ui/FeaturesWidget.h"

#include "whiteboard/ToolsManager.h"

#include "model/DocumentProxy.h"
#include "model/DocumentController.h"

#include "whiteboard/DrawingController.h"
#include "whiteboard/BoardView.h"

#include "whiteboard/BoardPaletteManager.h"

#include "settings/Settings.h"

BoardController::BoardController(MainWindow *mainWindow)
    : DocumentContainer(mainWindow->centralWidget()), mMainWindow(mainWindow), mActiveScene(0), mActiveSceneIndex(-1), mPaletteManager(0),
      mMessageWindow(0), mControlView(0), mDisplayView(0), mControlContainer(0), mControlLayout(0), mZoomFactor(1.0), mIsClosing(false),
      mSystemScaleFactor(1.0), mCleanupDone(false), mCacheWidgetIsEnabled(false), mDeletingSceneIndex(-1), mMovingSceneIndex(-1),
      mActionGroupText(tr("Group")), mActionUngroupText(tr("Ungroup")), mAutosaveTimer(0)
{
    mZoomFactor                   = Settings::settings()->boardZoomFactor->get().toDouble();

    int penColorIndex             = Settings::settings()->penColorIndex();
    int markerColorIndex          = Settings::settings()->markerColorIndex();

    mPenColorOnDarkBackground     = Settings::settings()->penColors(true).at(penColorIndex);
    mPenColorOnLightBackground    = Settings::settings()->penColors(false).at(penColorIndex);
    mMarkerColorOnDarkBackground  = Settings::settings()->markerColors(true).at(markerColorIndex);
    mMarkerColorOnLightBackground = Settings::settings()->markerColors(false).at(markerColorIndex);
}

void BoardController::init()
{
    setupViews();
    setupToolbar();

    connect(Application::undoStack, &QUndoStack::canUndoChanged, this, &BoardController::undoRedoStateChange);

    connect(Application::undoStack, &QUndoStack::canRedoChanged, this, &BoardController::undoRedoStateChange);

    connect(DrawingController::drawingController(), &DrawingController::stylusToolChanged, this, &BoardController::setToolCursor);

    connect(DrawingController::drawingController(), &DrawingController::stylusToolChanged, this, &BoardController::stylusToolChanged);

    connect(Application::app(), &QApplication::lastWindowClosed, this, &BoardController::lastWindowClosed);

    connect(DownloadManager::downloadManager(), &DownloadManager::downloadModalFinished, this, &BoardController::onDownloadModalFinished);
    connect(DownloadManager::downloadManager(), &DownloadManager::addDownloadedFileToBoard, this,
        [this](bool pSuccess, QUrl sourceUrl, QUrl contentUrl, QString pHeader, QByteArray pData, QPointF pPos, QSize pSize, bool isBackground)
        {
            downloadFinished(pSuccess, sourceUrl, contentUrl, pHeader, pData, pPos, pSize, isBackground);
        });

    // 文档被删掉时，BoardController 里缓存的 selectedDocument()（DocumentContainer 的
    // mCurrentDocument）指向的就是那个正在 deleteLater 的对象——PersistenceManager 一直
    // 在发这个信号，而接收端从来没连上，于是删掉当前文档之后每一次翻页/保存都在读已释放
    // 的代理。补上这条连接，切文档的逻辑就在 documentWillBeDeleted() 里。
    connect(PersistenceManager::persistenceManager(), &PersistenceManager::documentWillBeDeleted, this, &BoardController::documentWillBeDeleted);

    DocumentProxy *doc = PersistenceManager::persistenceManager()->createNewDocument();

    setActiveDocumentScene(doc);

    initBackgroundGridSize();

    undoRedoStateChange(true);
}

BoardController::~BoardController()
{
    delete mDisplayView;
}

/**
 * @brief 把默认背景网格调到屏幕上大约 1 厘米。
 */
void BoardController::initBackgroundGridSize()
{
    // 网格尺寸既要随 DPI 换算，又要按“控制视图像素 / 文档标称尺寸”的比例修正。
    // 构造期控制视图还不可用，这里先用文档高度 / 屏幕高度近似该比例；
    // 之后 updateSystemScaleFactor() 会算出精确值存入 mSystemScaleFactor。

    VirtualDesktop *desktop                = VirtualDesktop::instance();
    qreal           dpi                    = desktop->physicalDpi();

    // 显示管理器此时尚未初始化，只能假定控制视图位于主屏。
    qreal screenY                          = desktop->screenGeometry(mControlView).height();
    qreal documentY                        = mActiveScene->nominalSize().height();
    qreal resolutionRatio                  = documentY / screenY;

    int gridSize                           = (resolutionRatio * 10. * dpi) / GeometryUtils::inchSize;

    Settings::settings()->crossSize        = gridSize;
    Settings::settings()->defaultCrossSize = gridSize;
    mActiveScene->setBackgroundGridSize(gridSize);
}

int BoardController::currentPage()
{
    return mActiveSceneIndex + 1;
}

void BoardController::setupViews()
{
    mControlContainer = new QWidget(mMainWindow->centralWidget());

    mControlLayout    = new QHBoxLayout(mControlContainer);
    mControlLayout->setContentsMargins(0, 0, 0, 0);

    mControlView = new BoardView(this, mControlContainer, true, false);
    mControlView->setObjectName(CONTROLVIEW_OBJ_NAME);
    mControlView->setInteractive(true);
    mControlView->setMouseTracking(true);

    mControlView->grabGesture(Qt::SwipeGesture);

    mControlView->setTransformationAnchor(QGraphicsView::NoAnchor);

    mControlLayout->addWidget(mControlView);
    mControlContainer->setObjectName("boardControlContainer");
    mMainWindow->addBoardWidget(mControlContainer);

    connect(mControlView, &BoardView::resized, this, &BoardController::boardViewResized);

    mDisplayView = new BoardView(this, ItemLayerType::FixedBackground, ItemLayerType::Tool, 0);
    mDisplayView->setInteractive(false);
    mDisplayView->setTransformationAnchor(QGraphicsView::NoAnchor);

    mPaletteManager = new BoardPaletteManager(mControlContainer, this);

    mMessageWindow  = new MessageWindow(mControlContainer);
    mMessageWindow->hide();

    connect(this, &BoardController::activeSceneChanged, mPaletteManager, &BoardPaletteManager::activeSceneChanged);
}

void BoardController::setupLayout()
{
    if (mPaletteManager)
    {
        mPaletteManager->setupLayout();
    }
}

void BoardController::setBoxing(QRect displayRect)
{
    if (displayRect.isNull())
    {
        mControlLayout->setContentsMargins(0, 0, 0, 0);
        return;
    }

    qreal controlWidth  = (qreal)mMainWindow->centralWidget()->width();
    qreal controlHeight = (qreal)mMainWindow->centralWidget()->height();
    qreal displayWidth  = (qreal)displayRect.width();
    qreal displayHeight = (qreal)displayRect.height();

    qreal displayRatio  = displayWidth / displayHeight;
    qreal controlRatio  = controlWidth / controlHeight;

    if (displayRatio < controlRatio)
    {
        int boxWidth = (controlWidth - (displayWidth * (controlHeight / displayHeight))) / 2;
        mControlLayout->setContentsMargins(boxWidth, 0, boxWidth, 0);
    }
    else if (displayRatio > controlRatio)
    {
        int boxHeight = (controlHeight - (displayHeight * (controlWidth / displayWidth))) / 2;
        mControlLayout->setContentsMargins(0, boxHeight, 0, boxHeight);
    }
    else
    {
        mControlLayout->setContentsMargins(0, 0, 0, 0);
    }
}

QSize BoardController::displayViewport()
{
    return mDisplayView->geometry().size();
}

QSize BoardController::controlViewport()
{
    return mControlView->geometry().size();
}

QRectF BoardController::controlGeometry()
{
    return mControlView->geometry();
}

void BoardController::setupToolbar()
{
    Settings *settings = Settings::settings();

    Application::app()->insertSpaceToToolbarBeforeAction(mMainWindow->ui().boardToolBar, mMainWindow->ui().actionBoard);

    Application::app()->decorateActionMenu(mMainWindow->ui().actionMenu);

    mMainWindow->ui().actionBoard->setVisible(false);

    mMainWindow->ui().webToolBar->hide();
    mMainWindow->ui().documentToolBar->hide();

    connectToolbar();
    initToolbarTexts();

    Application::app()->toolBarDisplayTextChanged(QVariant(settings->appToolBarDisplayText->get().toBool()));
}

void BoardController::setToolCursor(int tool)
{
    if (mActiveScene)
    {
        mActiveScene->setToolCursor(tool);
    }

    mControlView->setToolCursor(tool);
}

void BoardController::connectToolbar()
{
    connect(mMainWindow->ui().actionAdd, &QAction::triggered, this, &BoardController::addItem);
    connect(mMainWindow->ui().actionNewPage, &QAction::triggered, this, qOverload<>(&BoardController::addScene));
    connect(mMainWindow->ui().actionDuplicatePage, &QAction::triggered, this, qOverload<>(&BoardController::duplicateScene));

    connect(mMainWindow->ui().actionClearPage, &QAction::triggered, this, &BoardController::clearScene);
    connect(mMainWindow->ui().actionEraseItems, &QAction::triggered, this, &BoardController::clearSceneItems);
    connect(mMainWindow->ui().actionEraseAnnotations, &QAction::triggered, this, &BoardController::clearSceneAnnotation);
    connect(mMainWindow->ui().actionEraseBackground, &QAction::triggered, this, &BoardController::clearSceneBackground);

    connect(mMainWindow->ui().actionUndo, &QAction::triggered, Application::undoStack, &QUndoStack::undo);
    connect(mMainWindow->ui().actionRedo, &QAction::triggered, Application::undoStack, &QUndoStack::redo);
    connect(mMainWindow->ui().actionRedo, &QAction::triggered, this, &BoardController::startScript);
    connect(mMainWindow->ui().actionBack, &QAction::triggered, this, &BoardController::previousScene);
    connect(mMainWindow->ui().actionForward, &QAction::triggered, this, &BoardController::nextScene);
    connect(mMainWindow->ui().actionSleep, &QAction::triggered, this, &BoardController::stopScript);
    connect(mMainWindow->ui().actionSleep, &QAction::triggered, this, &BoardController::blackout);
    connect(mMainWindow->ui().actionVirtualKeyboard, &QAction::triggered, this, &BoardController::showKeyboard);
    connect(mMainWindow->ui().actionImportPage, &QAction::triggered, this, &BoardController::importPage);
}

void BoardController::startScript()
{
    freezeWebWidgets(false);
}

void BoardController::stopScript()
{
    freezeWebWidgets(true);
}

void BoardController::saveData(SaveFlags fls)
{
    bool verbose = fls.testFlag(sf_showProgress);
    if (verbose)
    {
        Application::showMessage(tr("Saving document..."));
    }
    if (mActiveScene && mActiveScene->isModified())
    {
        persistCurrentScene(true);
    }
    if (verbose)
    {
        Application::showMessage(tr("Document has just been saved..."));
    }
}

void BoardController::initToolbarTexts()
{
    QList<QAction *> allToolbarActions;

    allToolbarActions << mMainWindow->ui().boardToolBar->actions();
    allToolbarActions << mMainWindow->ui().webToolBar->actions();
    allToolbarActions << mMainWindow->ui().documentToolBar->actions();

    foreach (QAction *action, allToolbarActions)
    {
        QString                 nominalText = action->text();
        QString                 shortText   = truncate(nominalText, 48);
        QPair<QString, QString> texts(nominalText, shortText);

        mActionTexts.insert(action, texts);
    }
}

void BoardController::setToolbarTexts()
{
    bool  highResolution = mMainWindow->width() > 1024;
    QSize iconSize;

    if (highResolution)
    {
        iconSize = QSize(48, 32);
    }
    else
    {
        iconSize = QSize(32, 32);
    }

    mMainWindow->ui().boardToolBar->setIconSize(iconSize);
    mMainWindow->ui().webToolBar->setIconSize(iconSize);
    mMainWindow->ui().documentToolBar->setIconSize(iconSize);

    foreach (QAction *action, mActionTexts.keys())
    {
        QPair<QString, QString> texts = mActionTexts.value(action);

        if (highResolution)
        {
            action->setText(texts.first);
        }
        else
        {
            action->setText(texts.second);
        }

        action->setToolTip(texts.first);
    }
}

QString BoardController::truncate(QString text, int maxWidth)
{
    QFontMetricsF fontMetrics(mMainWindow->font());
    return fontMetrics.elidedText(text, Qt::ElideRight, maxWidth);
}

void BoardController::stylusToolDoubleClicked(int tool)
{
    if (tool == StylusTool::ZoomIn || tool == StylusTool::ZoomOut)
    {
        zoomRestore();
    }
    else if (tool == StylusTool::Hand)
    {
        centerRestore();
        mActiveScene->setLastCenter(QPointF(0, 0));
    }
}

void BoardController::addScene()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    persistViewPositionOnCurrentScene();
    persistCurrentScene(false, true);

    DocumentContainer::addPage(mActiveSceneIndex + 1);

    selectedDocument()->setMetaData(Settings::documentUpdatedAt, StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));

    setActiveDocumentScene(mActiveSceneIndex + 1);
    QApplication::restoreOverrideCursor();
}

void BoardController::addScene(GraphicsScene *scene, bool replaceActiveIfEmpty)
{
    if (scene)
    {
        GraphicsScene *clone = scene->sceneDeepCopy();

        if (scene->document() && (scene->document() != selectedDocument()))
        {
            foreach (QUrl relativeFile, scene->relativeDependencies())
            {
                QString source      = scene->document()->persistencePath() + "/" + relativeFile.path();
                QString destination = selectedDocument()->persistencePath() + "/" + relativeFile.path();

                FileSystemUtils::copy(source, destination, true);
            }
        }

        if (replaceActiveIfEmpty && mActiveScene->isEmpty())
        {
            PersistenceManager::persistenceManager()->insertDocumentSceneAt(selectedDocument(), clone, mActiveSceneIndex);
            setActiveDocumentScene(mActiveSceneIndex);
            deleteScene(mActiveSceneIndex + 1);
        }
        else
        {
            persistCurrentScene(false, true);
            PersistenceManager::persistenceManager()->insertDocumentSceneAt(selectedDocument(), clone, mActiveSceneIndex + 1);
            setActiveDocumentScene(mActiveSceneIndex + 1);
        }

        selectedDocument()->setMetaData(Settings::documentUpdatedAt, StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
    }
}

void BoardController::addScene(DocumentProxy *proxy, int sceneIndex, bool replaceActiveIfEmpty)
{
    GraphicsScene *scene = PersistenceManager::persistenceManager()->loadDocumentScene(proxy, sceneIndex);

    if (scene)
    {
        addScene(scene, replaceActiveIfEmpty);
    }
}

void BoardController::duplicateScene(int nIndex)
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    persistCurrentScene(false, true);

    QList<int> scIndexes;
    scIndexes << nIndex;
    duplicatePages(scIndexes);
    insertThumbPage(nIndex);
    emit documentThumbnailsUpdated(this);
    selectedDocument()->setMetaData(Settings::documentUpdatedAt, StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));

    setActiveDocumentScene(nIndex + 1);
    QApplication::restoreOverrideCursor();
}

void BoardController::duplicateScene()
{
    if (Application::applicationController->displayMode() != ApplicationController::Board)
    {
        return;
    }
    duplicateScene(mActiveSceneIndex);
}

GraphicsItem *BoardController::duplicateItem(Item *item)
{
    if (!item)
    {
        return NULL;
    }

    GraphicsItem *retItem = NULL;

    mLastCreatedItem      = NULL;

    QUrl       sourceUrl;
    QByteArray pData;

    // 统一计算插入位置与尺寸：复用图元原始几何，避免类型分派处重复处理。
    QPointF itemPos;
    QSizeF  itemSize;

    QGraphicsItem *commonItem = dynamic_cast<QGraphicsItem *>(item);
    if (commonItem)
    {
        qreal shifting = Settings::settings()->objectFrameWidth;
        itemPos        = commonItem->pos() + QPointF(shifting, shifting);
        itemSize       = commonItem->boundingRect().size();
        commonItem->setSelected(false);
    }

    MimeType::Enum itemMimeType;

    QString srcFile = item->sourceUrl().toLocalFile();
    if (srcFile.isEmpty())
    {
        srcFile = item->sourceUrl().toString();
    }

    QString contentTypeHeader;
    if (!srcFile.isEmpty())
    {
        contentTypeHeader = FileSystemUtils::mimeTypeFromFileName(srcFile);
    }

    if (NULL != qgraphicsitem_cast<GraphicsGroupContainerItem *>(commonItem))
    {
        itemMimeType = MimeType::Group;
    }
    else
    {
        itemMimeType = FileSystemUtils::mimeTypeFromString(contentTypeHeader);
    }

    switch (static_cast<int>(itemMimeType))
    {
        case MimeType::AppleWidget:
        case MimeType::WebWidget:
        {
            GraphicsWidgetItem *witem = dynamic_cast<GraphicsWidgetItem *>(item);
            if (witem)
            {
                sourceUrl = witem->getOwnFolder();
            }
        }
        break;

        case MimeType::Video:
        case MimeType::Audio:
        {
            GraphicsMediaItem *mitem = dynamic_cast<GraphicsMediaItem *>(item);
            if (mitem)
            {
                sourceUrl = mitem->mediaFileUrl();
                downloadURL(sourceUrl, srcFile, itemPos, QSize(itemSize.width(), itemSize.height()), false, false);
                return NULL; // async operation
            }
        }
        break;

        case MimeType::VectorImage:
        {
            GraphicsSvgItem *viitem = dynamic_cast<GraphicsSvgItem *>(item);
            if (viitem)
            {
                pData     = viitem->fileData();
                sourceUrl = item->sourceUrl();
            }
        }
        break;

        case MimeType::RasterImage:
        {
            GraphicsPixmapItem *pixitem = dynamic_cast<GraphicsPixmapItem *>(item);
            if (pixitem)
            {
                QBuffer buffer(&pData);
                buffer.open(QIODevice::WriteOnly);
                QString format = FileSystemUtils::extension(item->sourceUrl().toString(QUrl::DecodeReserved));
                pixitem->pixmap().save(&buffer, format.toLatin1());
            }
        }
        break;

        case MimeType::Group:
        {
            GraphicsGroupContainerItem *groupItem       = dynamic_cast<GraphicsGroupContainerItem *>(item);
            GraphicsGroupContainerItem *duplicatedGroup = NULL;

            QList<QGraphicsItem *> duplicatedItems;
            QList<QGraphicsItem *> children = groupItem->childItems();

            mActiveScene->setURStackEnable(false);
            foreach (QGraphicsItem *pIt, children)
            {
                Item *pItem = dynamic_cast<Item *>(pIt);
                if (pItem)
                {
                    QGraphicsItem *itemToGroup = dynamic_cast<QGraphicsItem *>(duplicateItem(pItem));
                    if (itemToGroup)
                    {
                        itemToGroup->setZValue(pIt->zValue());
                        itemToGroup->setData(GraphicsItemData::ItemOwnZValue, pIt->data(GraphicsItemData::ItemOwnZValue).toReal());
                        duplicatedItems.append(itemToGroup);
                    }
                }
            }
            duplicatedGroup = mActiveScene->createGroup(duplicatedItems);
            duplicatedGroup->setTransform(groupItem->transform());
            groupItem->setSelected(false);

            retItem = dynamic_cast<GraphicsItem *>(duplicatedGroup);

            // createGroup() 内部已经 addItem 并处理了 z/undo；再 add 一次会让同一个组
            // 在 mFastAccessItems 里出现两次（复制页面时被克隆两遍），z 值也会被重排。
            if (QGraphicsItem *itemToSelect = dynamic_cast<QGraphicsItem *>(retItem))
            {
                itemToSelect->setSelected(true);
            }
            mActiveScene->setURStackEnable(true);
        }
        break;

        case MimeType::UNKNOWN:
        {
            QGraphicsItem *gitem = dynamic_cast<QGraphicsItem *>(item->deepCopy());
            if (gitem)
            {
                mActiveScene->addItem(gitem);

                gitem->setPos(itemPos);

                mLastCreatedItem = gitem;
                gitem->setSelected(true);
            }
            retItem = dynamic_cast<GraphicsItem *>(gitem);
        }
        break;
    }

    if (retItem)
    {
        QGraphicsItem *graphicsRetItem = dynamic_cast<QGraphicsItem *>(retItem);
        if (mActiveScene->isURStackIsEnabled())
        { // 等场景自带撤销栈之后这段就该整体删掉
            GraphicsItemUndoCommand *uc = new GraphicsItemUndoCommand(mActiveScene, 0, graphicsRetItem);
            Application::undoStack->push(uc);
        }
        return retItem;
    }

    Item *createdItem = downloadFinished(
        true, sourceUrl, QUrl::fromLocalFile(srcFile), contentTypeHeader, pData, itemPos, QSize(itemSize.width(), itemSize.height()), false);
    if (createdItem)
    {
        createdItem->setSourceUrl(item->sourceUrl());
        item->copyItemParameters(createdItem);

        QGraphicsItem *createdGitem = dynamic_cast<QGraphicsItem *>(createdItem);
        if (createdGitem)
        {
            createdGitem->setPos(itemPos);
        }
        mLastCreatedItem = dynamic_cast<QGraphicsItem *>(createdItem);
        mLastCreatedItem->setSelected(true);

        retItem = dynamic_cast<GraphicsItem *>(createdItem);
    }

    return retItem;
}

void BoardController::deleteScene(int nIndex)
{
    if (selectedDocument()->pageCount() >= 2)
    {
        mDeletingSceneIndex = nIndex;
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        persistCurrentScene();
        showMessage(tr("Deleting page %1").arg(nIndex + 1), true);

        QList<int> scIndexes;
        scIndexes << nIndex;
        deletePages(scIndexes);
        selectedDocument()->setMetaData(Settings::documentUpdatedAt, StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));

        if (nIndex >= pageCount())
        {
            nIndex = pageCount() - 1;
        }
        setActiveDocumentScene(nIndex);
        showMessage(tr("Page %1 deleted").arg(nIndex + 1));
        QApplication::restoreOverrideCursor();
        mDeletingSceneIndex = -1;
    }
}

void BoardController::clearScene()
{
    if (mActiveScene)
    {
        freezeWebWidgets(true);
        mActiveScene->clearContent(GraphicsScene::clearItemsAndAnnotations);
        mActiveScene->setLastCenter(QPointF(0, 0));
        mControlView->centerOn(mActiveScene->lastCenter());
        updateActionStates();
    }
}

void BoardController::clearSceneItems()
{
    if (mActiveScene)
    {
        freezeWebWidgets(true);
        mActiveScene->clearContent(GraphicsScene::clearItems);
        updateActionStates();
    }
}

void BoardController::clearSceneAnnotation()
{
    if (mActiveScene)
    {
        mActiveScene->clearContent(GraphicsScene::clearAnnotations);
        updateActionStates();
    }
}

void BoardController::clearSceneBackground()
{
    if (mActiveScene)
    {
        mActiveScene->clearContent(GraphicsScene::clearBackground);
        updateActionStates();
    }
}

void BoardController::showDocumentsDialog()
{
    if (selectedDocument())
    {
        persistCurrentScene();
    }
}

void BoardController::libraryDialogClosed(int ret)
{
    Q_UNUSED(ret);
}

void BoardController::blackout()
{
    Application::applicationController->blackout();
}

void BoardController::showKeyboard(bool show)
{
    if (show)
    {
        DrawingController::drawingController()->setStylusTool(StylusTool::Selector);
    }

    if (Settings::settings()->useSystemOnScreenKeyboard->get().toBool())
    {
        PlatformUtils::showOSK(show);
    }
    else
    {
        mPaletteManager->showVirtualKeyboard(show);
    }
}

void BoardController::zoomIn(QPointF scenePoint)
{
    if (mControlView->transform().m11() > MAX_ZOOM_LEVEL)
    {
        qApp->beep();
        return;
    }
    zoom(mZoomFactor, scenePoint);
}

void BoardController::zoomOut(QPointF scenePoint)
{
    if ((mControlView->horizontalScrollBar()->maximum() == 0) && (mControlView->verticalScrollBar()->maximum() == 0))
    {
        // 两个滚动条都没有可滚动范围，说明已缩到最小。
        qApp->beep();
        return;
    }

    qreal newZoomFactor = 1 / mZoomFactor;

    zoom(newZoomFactor, scenePoint);
}

void BoardController::zoomRestore()
{
    QTransform tr;

    tr.scale(mSystemScaleFactor, mSystemScaleFactor);
    mControlView->setTransform(tr);

    centerRestore();

    foreach (QGraphicsItem *gi, mActiveScene->selectedItems())
    {
        // 先取消再恢复选中，强制选中框按新的反缩放系数重绘。
        gi->setSelected(false);
        gi->setSelected(true);
    }

    emit zoomChanged(1.0);
}

void BoardController::centerRestore()
{
    centerOn(QPointF(0, 0));
}

void BoardController::centerOn(QPointF scenePoint)
{
    mControlView->centerOn(scenePoint);
    Application::applicationController->adjustDisplayView();
}

void BoardController::zoom(const qreal ratio, QPointF scenePoint)
{
    QPointF viewCenter    = mControlView->mapToScene(QRect(0, 0, mControlView->width(), mControlView->height()).center());
    QPointF offset        = scenePoint - viewCenter;
    QPointF scalledOffset = offset / ratio;

    qreal currentZoom     = ratio * mControlView->viewportTransform().m11() / mSystemScaleFactor;

    qreal usedRatio       = ratio;
    if (currentZoom > MAX_ZOOM_LEVEL)
    {
        currentZoom = MAX_ZOOM_LEVEL;
        usedRatio   = currentZoom * mSystemScaleFactor / mControlView->viewportTransform().m11();
    }

    mControlView->scale(usedRatio, usedRatio);

    QPointF newCenter = scenePoint - scalledOffset;

    mControlView->centerOn(newCenter);

    emit zoomChanged(currentZoom);
    Application::applicationController->adjustDisplayView();

    emit controlViewportChanged();
    mActiveScene->setBackgroundZoomFactor(mControlView->transform().m11());
}

void BoardController::handScroll(qreal dx, qreal dy)
{
    qreal antiScaleRatio = 1 / (mSystemScaleFactor * currentZoom());
    mControlView->translate(dx * antiScaleRatio, dy * antiScaleRatio);

    Application::applicationController->adjustDisplayView();

    emit controlViewportChanged();
}

void BoardController::persistViewPositionOnCurrentScene()
{
    QRect   rect = mControlView->rect();
    QPoint  center(rect.x() + rect.width() / 2, rect.y() + rect.height() / 2);
    QPointF viewRelativeCenter = mControlView->mapToScene(center);
    mActiveScene->setLastCenter(viewRelativeCenter);
}

void BoardController::previousScene()
{
    if (mActiveSceneIndex > 0)
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        persistViewPositionOnCurrentScene();
        persistCurrentScene();
        setActiveDocumentScene(mActiveSceneIndex - 1);
        mControlView->centerOn(mActiveScene->lastCenter());
        QApplication::restoreOverrideCursor();
    }

    updateActionStates();
}

void BoardController::nextScene()
{
    if (mActiveSceneIndex < selectedDocument()->pageCount() - 1)
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        persistViewPositionOnCurrentScene();
        persistCurrentScene();
        setActiveDocumentScene(mActiveSceneIndex + 1);
        mControlView->centerOn(mActiveScene->lastCenter());
        QApplication::restoreOverrideCursor();
    }

    updateActionStates();
}

void BoardController::firstScene()
{
    if (mActiveSceneIndex > 0)
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        persistViewPositionOnCurrentScene();
        persistCurrentScene();
        setActiveDocumentScene(0);
        mControlView->centerOn(mActiveScene->lastCenter());
        QApplication::restoreOverrideCursor();
    }

    updateActionStates();
}

void BoardController::lastScene()
{
    if (mActiveSceneIndex < selectedDocument()->pageCount() - 1)
    {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        persistViewPositionOnCurrentScene();
        persistCurrentScene();
        setActiveDocumentScene(selectedDocument()->pageCount() - 1);
        mControlView->centerOn(mActiveScene->lastCenter());
        QApplication::restoreOverrideCursor();
    }

    updateActionStates();
}

void BoardController::downloadURL(
    const QUrl &url, QString contentSourceUrl, const QPointF &pPos, const QSize &pSize, bool isBackground, bool internalData)
{
    qDebug() << "something has been dropped on the board! Url is: " << url.toString();
    QString sUrl                       = url.toString();

    QGraphicsItem *oldBackgroundObject = NULL;
    if (isBackground)
    {
        oldBackgroundObject = mActiveScene->backgroundObject();
    }

    if (sUrl.startsWith("tianboardtool://"))
    {
        downloadFinished(true, url, QUrl(), "application/tianboard-tool", QByteArray(), pPos, pSize, isBackground);
    }
    else if (sUrl.startsWith("file://") || sUrl.startsWith("/"))
    {
        QUrl    formedUrl   = sUrl.startsWith("file://") ? url : QUrl::fromLocalFile(sUrl);
        QString fileName    = formedUrl.toLocalFile();
        QString contentType = FileSystemUtils::mimeTypeFromFileName(fileName);

        bool shouldLoadFileData =
            contentType.startsWith("image") || contentType.startsWith("application/widget") || contentType.startsWith("application/vnd.apple-widget");

        if (shouldLoadFileData)
        {
            QFile file(fileName);
            file.open(QIODevice::ReadOnly);
            downloadFinished(true, formedUrl, QUrl(), contentType, file.readAll(), pPos, pSize, isBackground, internalData);
            file.close();
        }
        else
        {
            // 媒体文件体积大，交给下载管理器在后台线程复制，避免阻塞界面。
            sDownloadFileDesc desc;
            desc.modal          = false;
            desc.srcUrl         = sUrl;
            desc.originalSrcUrl = contentSourceUrl;
            desc.currentSize    = 0;
            desc.name           = QFileInfo(url.toString()).fileName();
            desc.totalSize      = 0; // 总大小要等下载过程中才拿得到，先按未知处理
            desc.pos            = pPos;
            desc.size           = pSize;
            desc.isBackground   = isBackground;

            DownloadManager::downloadManager()->addFileToDownload(desc);
        }
    }
    else
    {
        QString urlString                = url.toString();
        int     parametersStringPosition = urlString.indexOf("?");
        if (parametersStringPosition != -1)
        {
            urlString = urlString.left(parametersStringPosition);
        }

        // 走到这里说明是从网页拖入的远程资源，需要走模态下载流程。
        sDownloadFileDesc desc;
        desc.modal        = true;
        desc.srcUrl       = urlString;
        desc.currentSize  = 0;
        desc.name         = QFileInfo(urlString).fileName();
        desc.totalSize    = 0; // 总大小要等下载过程中才拿得到，先按未知处理
        desc.pos          = pPos;
        desc.size         = pSize;
        desc.isBackground = isBackground;

        DownloadManager::downloadManager()->addFileToDownload(desc);
    }

    if (isBackground && oldBackgroundObject != mActiveScene->backgroundObject())
    {
        if (mActiveScene->isURStackIsEnabled())
        { // 等场景自带撤销栈之后这段就该整体删掉
            GraphicsItemUndoCommand *uc = new GraphicsItemUndoCommand(mActiveScene, oldBackgroundObject, mActiveScene->backgroundObject());
            Application::undoStack->push(uc);
        }
    }
}

Item *BoardController::downloadFinished(bool pSuccess, QUrl sourceUrl, QUrl contentUrl, QString pContentTypeHeader, QByteArray pData, QPointF pPos,
    QSize pSize, bool isBackground, bool internalData)
{
    QString mimeType = pContentTypeHeader;

    // 拖放可能带上 "image/jpeg;charset=..." 这类参数，只取分号前的标准 MIME。
    if (mimeType.isEmpty())
    {
        mimeType = FileSystemUtils::mimeTypeFromFileName(sourceUrl.toString());
    }

    int position = mimeType.indexOf(";");
    if (position != -1)
    {
        mimeType = mimeType.left(position);
    }

    MimeType::Enum itemMimeType = FileSystemUtils::mimeTypeFromString(mimeType);

    if (!pSuccess)
    {
        showMessage(tr("Downloading content %1 failed").arg(sourceUrl.toString()));
        return NULL;
    }

    mActiveScene->deselectAllItems();

    if (!sourceUrl.toString().startsWith("file://") && !sourceUrl.toString().startsWith("tianboardtool://"))
    {
        showMessage(tr("Download finished"));
    }

    if (MimeType::RasterImage == itemMimeType)
    {
        qDebug() << "accepting mime type" << mimeType << "as raster image";

        QPixmap pix;
        if (pData.length() == 0)
        {
            pix.load(sourceUrl.toLocalFile());
        }
        else
        {
            QImage img;
            img.loadFromData(pData);
            pix = QPixmap::fromImage(img);
        }

        GraphicsPixmapItem *pixItem = mActiveScene->addPixmap(pix, NULL, pPos, 1.);
        pixItem->setSourceUrl(sourceUrl);

        if (isBackground)
        {
            mActiveScene->setAsBackgroundObject(pixItem, true);
        }
        else
        {
            mActiveScene->scaleToFitDocumentSize(pixItem, true, Settings::objectInControlViewMargin);
            DrawingController::drawingController()->setStylusTool(StylusTool::Selector);
        }

        return pixItem;
    }
    else if (MimeType::VectorImage == itemMimeType)
    {
        qDebug() << "accepting mime type" << mimeType << "as vecto image";

        GraphicsSvgItem *svgItem = mActiveScene->addSvg(sourceUrl, pPos, pData);
        if (!svgItem)
        {
            qWarning() << "cannot store embedded svg content, dropping it";
            return nullptr;
        }
        svgItem->setSourceUrl(sourceUrl);

        if (isBackground)
        {
            mActiveScene->setAsBackgroundObject(svgItem);
        }
        else
        {
            mActiveScene->scaleToFitDocumentSize(svgItem, true, Settings::objectInControlViewMargin);
            DrawingController::drawingController()->setStylusTool(StylusTool::Selector);
        }

        return svgItem;
    }
    else if (MimeType::AppleWidget == itemMimeType) // mime type invented by us :-(
    {
        qDebug() << "accepting mime type" << mimeType << "as Apple widget";

        QUrl widgetUrl = sourceUrl;

        if (pData.length() > 0)
        {
            widgetUrl = expandWidgetToTempDir(pData, "wdgt");
        }

        GraphicsWidgetItem *appleWidgetItem = mActiveScene->addAppleWidget(widgetUrl, pPos);

        appleWidgetItem->setSourceUrl(sourceUrl);

        if (isBackground)
        {
            mActiveScene->setAsBackgroundObject(appleWidgetItem);
        }
        else
        {
            DrawingController::drawingController()->setStylusTool(StylusTool::Selector);
        }

        return appleWidgetItem;
    }
    else if (MimeType::WebWidget == itemMimeType)
    {
        qDebug() << "accepting mime type" << mimeType << "as W3C widget";
        QUrl widgetUrl = sourceUrl;

        if (pData.length() > 0)
        {
            widgetUrl = expandWidgetToTempDir(pData);
        }

        GraphicsWidgetItem *webWidgetItem = addWebWidget(widgetUrl, pPos);

        if (isBackground)
        {
            mActiveScene->setAsBackgroundObject(webWidgetItem);
        }
        else
        {
            DrawingController::drawingController()->setStylusTool(StylusTool::Selector);
        }

        return webWidgetItem;
    }
    else if (MimeType::Video == itemMimeType)
    {
        qDebug() << "accepting mime type" << mimeType << "as video";

        GraphicsMediaItem *mediaVideoItem = 0;
        QUuid              uuid           = QUuid::createUuid();
        if (pData.length() > 0)
        {
            QString destFile;
            bool    b = PersistenceManager::persistenceManager()->addFileToDocument(
                selectedDocument(), sourceUrl.toString(), PersistenceManager::videoDirectory, uuid, destFile, &pData);
            if (!b)
            {
                showMessage(tr("Add file operation failed: file copying error"));
                return NULL;
            }

            QUrl url       = QUrl::fromLocalFile(destFile);

            mediaVideoItem = mActiveScene->addMedia(url, false, pPos);
        }
        else
        {
            qDebug() << sourceUrl.toString();
            mediaVideoItem = addVideo(sourceUrl, false, pPos, true);
        }

        if (mediaVideoItem)
        {
            if (contentUrl.isEmpty())
            {
                mediaVideoItem->setSourceUrl(sourceUrl);
            }
            else
            {
                mediaVideoItem->setSourceUrl(contentUrl);
            }
            mediaVideoItem->setUuid(uuid);
            connect(this, &BoardController::activeSceneChanged, mediaVideoItem, &GraphicsMediaItem::activeSceneChanged);
        }

        DrawingController::drawingController()->setStylusTool(StylusTool::Selector);

        return mediaVideoItem;
    }
    else if (MimeType::Audio == itemMimeType)
    {
        qDebug() << "accepting mime type" << mimeType << "as audio";

        GraphicsMediaItem *audioMediaItem = 0;

        QUuid uuid                        = QUuid::createUuid();
        if (pData.length() > 0)
        {
            QString destFile;
            bool    b = PersistenceManager::persistenceManager()->addFileToDocument(
                selectedDocument(), sourceUrl.toString(), PersistenceManager::audioDirectory, uuid, destFile, &pData);
            if (!b)
            {
                showMessage(tr("Add file operation failed: file copying error"));
                return NULL;
            }

            QUrl url       = QUrl::fromLocalFile(destFile);

            audioMediaItem = mActiveScene->addMedia(url, false, pPos);
        }
        else
        {
            audioMediaItem = addAudio(sourceUrl, false, pPos, true);
        }

        if (audioMediaItem)
        {
            if (contentUrl.isEmpty())
            {
                audioMediaItem->setSourceUrl(sourceUrl);
            }
            else
            {
                audioMediaItem->setSourceUrl(contentUrl);
            }
            audioMediaItem->setUuid(uuid);
            connect(this, &BoardController::activeSceneChanged, audioMediaItem, &GraphicsMediaItem::activeSceneChanged);
        }

        DrawingController::drawingController()->setStylusTool(StylusTool::Selector);

        return audioMediaItem;
    }
    else if (MimeType::PDF == itemMimeType)
    {
        qDebug() << "accepting mime type" << mimeType << "as PDF";
        qDebug() << "pdf data length: " << pData.size();
        qDebug() << "sourceurl : " + sourceUrl.toString();
        int result = 0;
        if (!sourceUrl.isEmpty())
        {
            QStringList fileNames;
            fileNames << sourceUrl.toLocalFile();
            result = DocumentManager::documentManager()->addFilesToDocument(selectedDocument(), fileNames);
        }
        else if (pData.size())
        {
            QTemporaryFile pdfFile("XXXXXX.pdf");
            if (pdfFile.open())
            {
                pdfFile.write(pData);
                QStringList fileNames;
                fileNames << pdfFile.fileName();
                result = DocumentManager::documentManager()->addFilesToDocument(selectedDocument(), fileNames);
                pdfFile.close();
            }
        }

        if (result)
        {
            selectedDocument()->setMetaData(Settings::documentUpdatedAt, StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
        }
    }
    else if (MimeType::NativeTool == itemMimeType)
    {
        qDebug() << "accepting mime type" << mimeType << "TianBoard Tool";

        if (sourceUrl.toString() == ToolsManager::manager()->compass.id)
        {
            mActiveScene->addCompass(pPos);
            DrawingController::drawingController()->setStylusTool(StylusTool::Selector);
        }
        else if (sourceUrl.toString() == ToolsManager::manager()->ruler.id)
        {
            mActiveScene->addRuler(pPos);
            DrawingController::drawingController()->setStylusTool(StylusTool::Selector);
        }
        else if (sourceUrl.toString() == ToolsManager::manager()->protractor.id)
        {
            mActiveScene->addProtractor(pPos);
            DrawingController::drawingController()->setStylusTool(StylusTool::Selector);
        }
        else if (sourceUrl.toString() == ToolsManager::manager()->axes.id)
        {
            mActiveScene->addAxes(pPos);
            DrawingController::drawingController()->setStylusTool(StylusTool::Selector);
        }
        else if (sourceUrl.toString() == ToolsManager::manager()->triangle.id)
        {
            mActiveScene->addTriangle(pPos);
            DrawingController::drawingController()->setStylusTool(StylusTool::Selector);
        }
        else if (sourceUrl.toString() == ToolsManager::manager()->cache.id)
        {
            mActiveScene->addCache();
            DrawingController::drawingController()->setStylusTool(StylusTool::Selector);
        }
        else if (sourceUrl.toString() == ToolsManager::manager()->magnifier.id)
        {
            MagnifierParams params;
            params.x                    = controlContainer()->geometry().width() / 2;
            params.y                    = controlContainer()->geometry().height() / 2;
            params.zoom                 = 2;
            params.sizePercentFromScene = 20;
            mActiveScene->addMagnifier(params);
            DrawingController::drawingController()->setStylusTool(StylusTool::Selector);
        }
        else if (sourceUrl.toString() == ToolsManager::manager()->mask.id)
        {
            mActiveScene->addMask(pPos);
            DrawingController::drawingController()->setStylusTool(StylusTool::Selector);
        }
        else
        {
            showMessage(tr("Unknown tool type %1").arg(sourceUrl.toString()));
        }
    }
    else if (sourceUrl.toString().contains("edumedia-sciences.com"))
    {
        qDebug() << "accepting url " << sourceUrl.toString() << "as eduMedia content";

        QTemporaryFile eduMediaZipFile("XXXXXX.edumedia");
        if (eduMediaZipFile.open())
        {
            eduMediaZipFile.write(pData);
            eduMediaZipFile.close();

            QString tempDir = FileSystemUtils::createTempDir("tianboard-edumedia");

            FileSystemUtils::expandZipToDir(eduMediaZipFile, tempDir);

            QDir appDir(tempDir);

            foreach (QString subDirName, appDir.entryList(QDir::AllDirs))
            {
                QDir subDir(tempDir + "/" + subDirName + "/contents");

                foreach (QString fileName, subDir.entryList(QDir::Files))
                {
                    if (fileName.toLower().endsWith(".swf"))
                    {
                        QString swfFile = tempDir + "/" + subDirName + "/contents/" + fileName;

                        QSize size;

                        if (pSize.height() > 0 && pSize.width() > 0)
                        {
                            size = pSize;
                        }
                        else
                        {
                            size = mActiveScene->nominalSize() * .8;
                        }

                        QString widgetUrl = GraphicsWebWidgetItem::createNPAPIWrapper(swfFile, "application/x-shockwave-flash", size);

                        if (widgetUrl.length() > 0)
                        {
                            GraphicsWidgetItem *widgetItem = mActiveScene->addWebWidget(QUrl::fromLocalFile(widgetUrl), pPos);

                            widgetItem->setSourceUrl(sourceUrl);

                            DrawingController::drawingController()->setStylusTool(StylusTool::Selector);

                            return widgetItem;
                        }
                    }
                }
            }
        }
    }
    else
    {
        showMessage(tr("Unknown content type %1").arg(pContentTypeHeader));
        qWarning() << "ignoring mime type" << pContentTypeHeader;
    }

    return NULL;
}

void BoardController::setActiveDocumentScene(int pSceneIndex)
{
    setActiveDocumentScene(selectedDocument(), pSceneIndex);
}

void BoardController::setActiveDocumentScene(DocumentProxy *pDocumentProxy, const int pSceneIndex, bool forceReload, bool onImport)
{
    saveViewState();

    bool documentChange = selectedDocument() != pDocumentProxy;

    int index           = pSceneIndex;
    int sceneCount      = pDocumentProxy->pageCount();
    if (index >= sceneCount && sceneCount > 0)
    {
        index = sceneCount - 1;
    }

    GraphicsScene *targetScene = PersistenceManager::persistenceManager()->loadDocumentScene(pDocumentProxy, index);

    bool sceneChange           = targetScene != mActiveScene;

    if (targetScene)
    {
        if (mActiveScene && !onImport)
        {
            persistCurrentScene();
            freezeWebWidgets(true);
            ClearUndoStack();
        }
        else
        {
            Application::undoStack->clear();
        }

        mActiveScene      = targetScene;
        mActiveSceneIndex = index;

        setDocument(pDocumentProxy, forceReload);

        updateSystemScaleFactor();

        mControlView->setScene(mActiveScene);
        disconnect(mControlView, &BoardView::mouseReleased, mActiveScene, &GraphicsScene::updateSelectionFrame);
        connect(mControlView, &BoardView::mouseReleased, mActiveScene, &GraphicsScene::updateSelectionFrame);

        mDisplayView->setScene(mActiveScene);
        mActiveScene->setBackgroundZoomFactor(mControlView->transform().m11());
        pDocumentProxy->setDefaultDocumentSize(mActiveScene->nominalSize());
        updatePageSizeState();

        adjustDisplayViews();

        Settings::settings()->setDarkBackground(mActiveScene->isDarkBackground());
        Settings::settings()->setPageBackground(mActiveScene->pageBackground());

        freezeWebWidgets(false);
    }

    selectionChanged();

    updateBackgroundActionsState(mActiveScene->isDarkBackground(), mActiveScene->pageBackground());

    if (documentChange)
    {
        GraphicsTextItem::lastUsedTextColor = QColor(Qt::black);
    }

    if (sceneChange)
    {
        emit activeSceneChanged();
    }
}

void BoardController::moveSceneToIndex(int source, int target)
{
    if (selectedDocument())
    {
        persistCurrentScene(false, true);

        DocumentContainer::movePageToIndex(source, target);

        selectedDocument()->setMetaData(Settings::documentUpdatedAt, StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
        PersistenceManager::persistenceManager()->persistDocumentMetadata(selectedDocument());
        mMovingSceneIndex = source;
        mActiveSceneIndex = target;
        setActiveDocumentScene(target);
        mMovingSceneIndex = -1;

        emit activeSceneChanged();
    }
}

void BoardController::findUniquesItems(const QUndoCommand *parent, QSet<QGraphicsItem *> &itms)
{
    if (parent->childCount())
    {
        for (int i = 0; i < parent->childCount(); i++)
        {
            findUniquesItems(parent->child(i), itms);
        }
    }

    // 事务宏只负责分组，其子命令已在上面递归处理，这里直接返回。
    if (parent->text() == Settings::undoCommandTransactionName)
    {
        return;
    }

    const UndoCommand *undoCmd = static_cast<const UndoCommand *>(parent);
    if (undoCmd->getType() != UndoType::undotype_GRAPHICITEM)
    {
        return;
    }

    const GraphicsItemUndoCommand *cmd = dynamic_cast<const GraphicsItemUndoCommand *>(parent);

    // 收集新增与移除列表中的图元；组内图元会随组一起删除，不单独收集。
    QSetIterator<QGraphicsItem *> itAdded(cmd->GetAddedList());
    while (itAdded.hasNext())
    {
        QGraphicsItem *item = itAdded.next();
        if (!itms.contains(item) && !(item->parentItem() && GraphicsGroupContainerItem::Type == item->parentItem()->type()))
        {
            itms.insert(item);
        }
    }

    QSetIterator<QGraphicsItem *> itRemoved(cmd->GetRemovedList());
    while (itRemoved.hasNext())
    {
        QGraphicsItem *item = itRemoved.next();
        if (!itms.contains(item) && !(item->parentItem() && GraphicsGroupContainerItem::Type == item->parentItem()->type()))
        {
            itms.insert(item);
        }
    }
}

void BoardController::ClearUndoStack()
{
    QSet<QGraphicsItem *> uniqueItems;
    // 遍历撤销栈里的每条命令，收集其引用到的图元。
    for (int i = 0; i < Application::undoStack->count(); i++)
    {
        findUniquesItems(Application::undoStack->command(i), uniqueItems);
    }

    // 先取得剪贴板中图元的来源 URL：被剪切但尚未粘贴的图元不在场景里，也不能删。
    // 这里用来源 URL 作相等性替身，使媒体、网页小部件等能跨页剪切粘贴。
    QClipboard      *clipboard = QApplication::clipboard();
    const QMimeData *data      = clipboard->mimeData();
    QList<QUrl>      sourceURLs;

    if (data && data->hasFormat(Application::mimeTypeBoardPageItem))
    {
        const MimeDataGraphicsItem *mimeDataGI = qobject_cast<const MimeDataGraphicsItem *>(data);

        if (mimeDataGI)
        {
            foreach (Item *sourceItem, mimeDataGI->items())
            {
                sourceURLs << sourceItem->sourceUrl();
            }
        }
    }

    // 既不在场景、也不在剪贴板中的图元才可以销毁。
    QSetIterator<QGraphicsItem *> itUniq(uniqueItems);
    while (itUniq.hasNext())
    {
        QGraphicsItem *item  = itUniq.next();
        GraphicsScene *scene = NULL;
        if (item->scene())
        {
            scene = dynamic_cast<GraphicsScene *>(item->scene());
        }

        bool  inClipboard = false;
        Item *ubi         = dynamic_cast<Item *>(item);
        if (ubi && sourceURLs.contains(ubi->sourceUrl()))
        {
            inClipboard = true;
        }

        if (!scene && !inClipboard)
        {
            if (!mActiveScene->deleteItem(item))
            {
                delete item;
                item = 0;
            }
        }
    }

    // 清空撤销栈，命令对象随之释放。
    Application::undoStack->clear();
}

void BoardController::adjustDisplayViews()
{
    if (Application::applicationController)
    {
        Application::applicationController->adjustDisplayView();
        Application::applicationController->adjustPreviousViews(mActiveSceneIndex, selectedDocument());
    }
}

int BoardController::autosaveTimeoutFromSettings()
{
    int value  = Settings::settings()->autoSaveInterval->get().toInt();
    int minute = 60 * 1000;

    return value * minute;
}

void BoardController::changeBackground(bool isDark, PageBackground pageBackground)
{
    bool           currentIsDark         = mActiveScene->isDarkBackground();
    PageBackground currentBackgroundType = mActiveScene->pageBackground();

    if ((isDark != currentIsDark) || (currentBackgroundType != pageBackground))
    {
        Settings::settings()->setDarkBackground(isDark);
        Settings::settings()->setPageBackground(pageBackground);

        mActiveScene->setBackground(isDark, pageBackground);

        emit backgroundChanged();
    }
}

void BoardController::boardViewResized(QResizeEvent *event)
{
    Q_UNUSED(event);

    int innerMargin = Settings::boardMargin;
    int userHeight  = mControlContainer->height() - (2 * innerMargin);

    mMessageWindow->move(innerMargin, innerMargin + userHeight - mMessageWindow->height());
    mMessageWindow->adjustSizeAndPosition();

    Application::applicationController->initViewState(mControlView->horizontalScrollBar()->value(), mControlView->verticalScrollBar()->value());

    updateSystemScaleFactor();

    mControlView->centerOn(0, 0);

    if (mDisplayView)
    {
        Application::applicationController->adjustDisplayView();
        mDisplayView->centerOn(0, 0);
    }

    mPaletteManager->containerResized();

    Application::boardController->controlView()->scene()->moveMagnifier();
}

void BoardController::documentWillBeDeleted(DocumentProxy *pProxy)
{
    if (selectedDocument() == pProxy)
    {
        if (!mIsClosing)
        {
            setActiveDocumentScene(PersistenceManager::persistenceManager()->createDocument());
        }
    }
}

void BoardController::showMessage(const QString &message, bool showSpinningWheel)
{
    mMessageWindow->showMessage(message, showSpinningWheel);
}

void BoardController::hideMessage()
{
    mMessageWindow->hideMessage();
}

void BoardController::setDisabled(bool disable)
{
    mMainWindow->ui().boardToolBar->setDisabled(disable);
    mControlView->setDisabled(disable);
}

void BoardController::selectionChanged()
{
    updateActionStates();
    emit pageSelectionChanged(activeSceneIndex());
}

void BoardController::undoRedoStateChange(bool canUndo)
{
    Q_UNUSED(canUndo);

    mMainWindow->ui().actionUndo->setEnabled(Application::undoStack->canUndo());
    mMainWindow->ui().actionRedo->setEnabled(Application::undoStack->canRedo());

    updateActionStates();
}

void BoardController::updateActionStates()
{
    mMainWindow->ui().actionBack->setEnabled(selectedDocument() && (mActiveSceneIndex > 0));
    mMainWindow->ui().actionForward->setEnabled(selectedDocument() && (mActiveSceneIndex < selectedDocument()->pageCount() - 1));
    mMainWindow->ui().actionErase->setEnabled(mActiveScene && !mActiveScene->isEmpty());
}

GraphicsScene *BoardController::activeScene() const
{
    return mActiveScene;
}

int BoardController::activeSceneIndex() const
{
    return mActiveSceneIndex;
}

void BoardController::setActiveSceneIndex(int i)
{
    mActiveSceneIndex = i;
}

void BoardController::documentSceneChanged(DocumentProxy *pDocumentProxy, int pIndex)
{
    Q_UNUSED(pIndex);

    if (selectedDocument() == pDocumentProxy)
    {
        setActiveDocumentScene(mActiveSceneIndex);
        updatePage(pIndex);
    }
}

void BoardController::autosaveTimeout()
{
    if (Application::applicationController->displayMode() != ApplicationController::Board)
    {
        // 只在画板模式下自动保存。
        return;
    }

    saveData(sf_showProgress);
    Settings::settings()->save();
}

void BoardController::appMainModeChanged(ApplicationController::MainMode md)
{
    int autoSaveInterval = autosaveTimeoutFromSettings();
    if (!autoSaveInterval)
    {
        return;
    }

    if (!mAutosaveTimer)
    {
        mAutosaveTimer = new QTimer(this);
        connect(mAutosaveTimer, &QTimer::timeout, this, &BoardController::autosaveTimeout);
    }

    if (md == ApplicationController::Board)
    {
        mAutosaveTimer->start(autoSaveInterval);
    }
    else if (mAutosaveTimer->isActive())
    {
        mAutosaveTimer->stop();
    }
}

void BoardController::closing()
{
    mIsClosing = true;
    lastWindowClosed();
    ClearUndoStack();
    showKeyboard(false);
}

void BoardController::lastWindowClosed()
{
    if (!mCleanupDone)
    {
        if (selectedDocument()->pageCount() == 1 && (!mActiveScene || mActiveScene->isEmpty()))
        {
            PersistenceManager::persistenceManager()->deleteDocument(selectedDocument());
        }
        else
        {
            persistCurrentScene();
        }

        PersistenceManager::persistenceManager()->purgeEmptyDocuments();

        mCleanupDone = true;
    }
}

void BoardController::setColorIndex(int pColorIndex)
{
    DrawingController::drawingController()->setColorIndex(pColorIndex);

    if (DrawingController::drawingController()->stylusTool() != StylusTool::Marker &&
        DrawingController::drawingController()->stylusTool() != StylusTool::Line &&
        DrawingController::drawingController()->stylusTool() != StylusTool::Text &&
        DrawingController::drawingController()->stylusTool() != StylusTool::Selector)
    {
        DrawingController::drawingController()->setStylusTool(StylusTool::Pen);
    }

    if (DrawingController::drawingController()->stylusTool() == StylusTool::Pen ||
        DrawingController::drawingController()->stylusTool() == StylusTool::Line ||
        DrawingController::drawingController()->stylusTool() == StylusTool::Text ||
        DrawingController::drawingController()->stylusTool() == StylusTool::Selector)
    {
        mPenColorOnDarkBackground  = Settings::settings()->penColors(true).at(pColorIndex);
        mPenColorOnLightBackground = Settings::settings()->penColors(false).at(pColorIndex);

        if (DrawingController::drawingController()->stylusTool() == StylusTool::Selector)
        {
            // 画板模式下选择器没有意义，切回画笔。
            if (Application::applicationController->displayMode() == ApplicationController::Board)
            {
                DrawingController::drawingController()->setStylusTool(StylusTool::Pen);
                mMainWindow->ui().actionPen->setChecked(true);
            }
        }

        emit penColorChanged();
    }
    else if (DrawingController::drawingController()->stylusTool() == StylusTool::Marker)
    {
        mMarkerColorOnDarkBackground  = Settings::settings()->markerColors(true).at(pColorIndex);
        mMarkerColorOnLightBackground = Settings::settings()->markerColors(false).at(pColorIndex);
    }
}

void BoardController::colorPaletteChanged()
{
    mPenColorOnDarkBackground     = Settings::settings()->penColor(true);
    mPenColorOnLightBackground    = Settings::settings()->penColor(false);
    mMarkerColorOnDarkBackground  = Settings::settings()->markerColor(true);
    mMarkerColorOnLightBackground = Settings::settings()->markerColor(false);
}

qreal BoardController::currentZoom()
{
    if (mControlView)
    {
        return mControlView->viewportTransform().m11() / mSystemScaleFactor;
    }
    else
    {
        return 1.0;
    }
}

void BoardController::removeTool(ToolWidget *toolWidget)
{
    toolWidget->hide();

    delete toolWidget;
}

void BoardController::hide()
{
}

void BoardController::show()
{
}

void BoardController::persistCurrentScene(bool isAnAutomaticBackup, bool forceImmediateSave)
{
    if (PersistenceManager::persistenceManager() && selectedDocument() && mActiveScene && mActiveSceneIndex != mDeletingSceneIndex &&
        (mActiveSceneIndex >= 0) && mActiveSceneIndex != mMovingSceneIndex && (mActiveScene->isModified()))
    {
        PersistenceManager::persistenceManager()->persistDocumentScene(selectedDocument(), mActiveScene, mActiveSceneIndex);
        updatePage(mActiveSceneIndex);
    }
}

void BoardController::updateSystemScaleFactor()
{
    qreal newScaleFactor = 1.0;

    if (mActiveScene)
    {
        QSize pageNominalSize                         = mActiveScene->nominalSize();
        // 按页面标称尺寸与控制视口的较小比例做适配；自定义尺寸也走同一路径。
        QMap<DocumentSizeRatio::Enum, QSize> sizesMap = Settings::settings()->documentSizes;
        {
            QSize controlSize = controlViewport();

            qreal hFactor     = ((qreal)controlSize.width()) / ((qreal)pageNominalSize.width());
            qreal vFactor     = ((qreal)controlSize.height()) / ((qreal)pageNominalSize.height());

            newScaleFactor    = qMin(hFactor, vFactor);
        }
    }

    if (mSystemScaleFactor != newScaleFactor)
    {
        mSystemScaleFactor = newScaleFactor;
    }

    GraphicsScene::SceneViewState viewState = mActiveScene->viewState();

    QTransform scalingTransform;

    qreal scaleFactor = viewState.zoomFactor * mSystemScaleFactor;
    scalingTransform.scale(scaleFactor, scaleFactor);

    mControlView->setTransform(scalingTransform);
    mControlView->horizontalScrollBar()->setValue(viewState.horizontalPosition);
    mControlView->verticalScrollBar()->setValue(viewState.verticalPostition);
    mActiveScene->setBackgroundZoomFactor(mControlView->transform().m11());
}

void BoardController::setWidePageSize(bool checked)
{
    Q_UNUSED(checked);
    QSize newSize = Settings::settings()->documentSizes.value(DocumentSizeRatio::Ratio16_9);

    if (mActiveScene->nominalSize() != newSize)
    {
        PageSizeUndoCommand *uc = new PageSizeUndoCommand(mActiveScene, mActiveScene->nominalSize(), newSize);
        Application::undoStack->push(uc);

        setPageSize(newSize);
    }
}

void BoardController::setRegularPageSize(bool checked)
{
    Q_UNUSED(checked);
    QSize newSize = Settings::settings()->documentSizes.value(DocumentSizeRatio::Ratio4_3);

    if (mActiveScene->nominalSize() != newSize)
    {
        PageSizeUndoCommand *uc = new PageSizeUndoCommand(mActiveScene, mActiveScene->nominalSize(), newSize);
        Application::undoStack->push(uc);

        setPageSize(newSize);
    }
}

void BoardController::setPageSize(QSize newSize)
{
    if (mActiveScene->nominalSize() != newSize)
    {
        mActiveScene->setNominalSize(newSize);

        saveViewState();

        updateSystemScaleFactor();
        updatePageSizeState();
        adjustDisplayViews();
        selectedDocument()->setMetaData(Settings::documentUpdatedAt, StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));

        Settings::settings()->pageSize->set(newSize);
    }
}

void BoardController::notifyCache(bool visible)
{
    if (visible)
    {
        emit cacheEnabled();
    }

    mCacheWidgetIsEnabled = visible;
}

void BoardController::updatePageSizeState()
{
    if (mActiveScene->nominalSize() == Settings::settings()->documentSizes.value(DocumentSizeRatio::Ratio16_9))
    {
        mMainWindow->ui().actionWidePageSize->setChecked(true);
    }
    else if (mActiveScene->nominalSize() == Settings::settings()->documentSizes.value(DocumentSizeRatio::Ratio4_3))
    {
        mMainWindow->ui().actionRegularPageSize->setChecked(true);
    }
    else
    {
        mMainWindow->ui().actionCustomPageSize->setChecked(true);
    }
}

void BoardController::saveViewState()
{
    if (mActiveScene)
    {
        mActiveScene->setViewState(GraphicsScene::SceneViewState(
            currentZoom(), mControlView->horizontalScrollBar()->value(), mControlView->verticalScrollBar()->value(), mActiveScene->lastCenter()));
    }
}

void BoardController::stylusToolChanged(int tool)
{
    if (PlatformUtils::hasVirtualKeyboard() && mPaletteManager->mKeyboardPalette)
    {
        StylusTool::Enum eTool = (StylusTool::Enum)tool;
        if (eTool != StylusTool::Selector && eTool != StylusTool::Text)
        {
            if (mPaletteManager->mKeyboardPalette->keyboardVisible())
            {
                Application::mainWindow->ui().actionVirtualKeyboard->activate(QAction::Trigger);
            }
        }
    }
}

QUrl BoardController::expandWidgetToTempDir(const QByteArray &pZipedData, const QString &ext)
{
    QUrl           widgetUrl;
    QTemporaryFile tmp;

    if (tmp.open())
    {
        tmp.write(pZipedData);
        tmp.flush();
        tmp.close();

        QString tmpDir = FileSystemUtils::createTempDir() + "." + ext;

        if (FileSystemUtils::expandZipToDir(tmp, tmpDir))
        {
            widgetUrl = QUrl::fromLocalFile(tmpDir);
        }
    }

    return widgetUrl;
}

void BoardController::grabScene(const QRectF &pSceneRect)
{
    if (mActiveScene)
    {
        QImage image(pSceneRect.width(), pSceneRect.height(), QImage::Format_ARGB32);
        image.fill(Qt::transparent);

        QRectF   targetRect(0, 0, pSceneRect.width(), pSceneRect.height());
        QPainter painter(&image);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.setRenderHint(QPainter::Antialiasing);

        mActiveScene->setRenderingContext(GraphicsScene::NonScreen);
        mActiveScene->setRenderingQuality(Item::RenderingQualityHigh);

        mActiveScene->render(&painter, targetRect, pSceneRect);

        mActiveScene->setRenderingContext(GraphicsScene::Screen);
        mActiveScene->setRenderingQuality(Item::RenderingQualityHigh);

        mPaletteManager->addItem(QPixmap::fromImage(image));
        selectedDocument()->setMetaData(Settings::documentUpdatedAt, StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
    }
}

GraphicsMediaItem *BoardController::addVideo(const QUrl &pSourceUrl, bool startPlay, const QPointF &pos, bool bUseSource)
{
    QUuid uuid        = QUuid::createUuid();
    QUrl  concreteUrl = pSourceUrl;

    // 未要求直接复用源文件时，先把媒体复制进文档目录。
    if (!bUseSource)
    {
        QString destFile;
        bool    b = PersistenceManager::persistenceManager()->addFileToDocument(
            selectedDocument(), pSourceUrl.toLocalFile(), PersistenceManager::videoDirectory, uuid, destFile);
        if (!b)
        {
            showMessage(tr("Add file operation failed: file copying error"));
            return NULL;
        }
        concreteUrl = QUrl::fromLocalFile(destFile);
    } // 否则直接沿用源文件 URL。

    GraphicsMediaItem *vi = mActiveScene->addMedia(concreteUrl, startPlay, pos);
    selectedDocument()->setMetaData(Settings::documentUpdatedAt, StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));

    if (vi)
    {
        vi->setUuid(uuid);
        vi->setSourceUrl(pSourceUrl);
    }

    return vi;
}

GraphicsMediaItem *BoardController::addAudio(const QUrl &pSourceUrl, bool startPlay, const QPointF &pos, bool bUseSource)
{
    QUuid uuid        = QUuid::createUuid();
    QUrl  concreteUrl = pSourceUrl;

    // 未要求直接复用源文件时，先把媒体复制进文档目录。
    if (!bUseSource)
    {
        QString destFile;
        bool    b = PersistenceManager::persistenceManager()->addFileToDocument(
            selectedDocument(), pSourceUrl.toLocalFile(), PersistenceManager::audioDirectory, uuid, destFile);
        if (!b)
        {
            showMessage(tr("Add file operation failed: file copying error"));
            return NULL;
        }
        concreteUrl = QUrl::fromLocalFile(destFile);
    } // 否则直接沿用源文件 URL。

    GraphicsMediaItem *ai = mActiveScene->addMedia(concreteUrl, startPlay, pos);
    selectedDocument()->setMetaData(Settings::documentUpdatedAt, StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));

    if (ai)
    {
        ai->setUuid(uuid);
        ai->setSourceUrl(pSourceUrl);
    }

    return ai;
}

GraphicsWidgetItem *BoardController::addWebWidget(const QUrl &pUrl, const QPointF &pos)
{
    GraphicsWidgetItem *webWidgetItem = 0;

    QUuid uuid                        = QUuid::createUuid();

    QString destPath;
    if (!PersistenceManager::persistenceManager()->addGraphicsWidgetToDocument(selectedDocument(), pUrl.toLocalFile(), uuid, destPath))
    {
        return NULL;
    }
    QUrl newUrl   = QUrl::fromLocalFile(destPath);

    webWidgetItem = mActiveScene->addWebWidget(newUrl, pos);

    if (webWidgetItem)
    {
        webWidgetItem->setUuid(uuid);
        webWidgetItem->setOwnFolder(newUrl);
        webWidgetItem->setSourceUrl(pUrl);

        QString struuid      = StringUtils::toCanonicalUuid(uuid);
        QString snapshotPath = selectedDocument()->persistencePath() + "/" + PersistenceManager::widgetDirectory + "/" + struuid + ".png";
        webWidgetItem->setSnapshotPath(QUrl::fromLocalFile(snapshotPath));
        GraphicsWidgetItem *tmpItem = dynamic_cast<GraphicsWidgetItem *>(webWidgetItem);
        if (tmpItem && tmpItem->scene())
        {
            tmpItem->takeSnapshot().save(snapshotPath, "PNG");
        }
    }

    return webWidgetItem;
}

void BoardController::cut()
{
    // 先固定选区：下面的循环会把图元从场景移除，直接遍历 selectedItems() 会被修改。
    QList<QGraphicsItem *> selectedItems;
    foreach (QGraphicsItem *gi, mActiveScene->selectedItems())
    {
        selectedItems << gi;
    }

    QList<Item *> selected;
    foreach (QGraphicsItem *gi, selectedItems)
    {
        gi->setSelected(false);

        Item         *boardItem = dynamic_cast<Item *>(gi);
        GraphicsItem *giAsItem  = dynamic_cast<GraphicsItem *>(gi);

        if (boardItem && giAsItem && !mActiveScene->tools().contains(gi))
        {
            selected << boardItem->deepCopy();
            giAsItem->remove();
        }
    }

    if (selected.size() > 0)
    {
        QClipboard *clipboard        = QApplication::clipboard();

        MimeDataGraphicsItem *mimeGi = new MimeDataGraphicsItem(selected);

        mimeGi->setData(Application::mimeTypeBoardPageItem, QByteArray());
        clipboard->setMimeData(mimeGi);

        selectedDocument()->setMetaData(Settings::documentUpdatedAt, StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
    }
}

void BoardController::copy()
{
    QList<Item *> selected;

    foreach (QGraphicsItem *gi, mActiveScene->selectedItems())
    {
        Item *boardItem = dynamic_cast<Item *>(gi);

        // 与 cut() 一致：剪贴板存副本而不是场景里的活对象。存活指针时，源文档一关，
        // 剪贴板里就是悬空指针（QWindowsClipboard 返回的仍是这个对象），之后粘贴会崩。
        if (boardItem && !mActiveScene->tools().contains(gi))
        {
            selected << boardItem->deepCopy();
        }
    }

    if (selected.size() > 0)
    {
        QClipboard *clipboard        = QApplication::clipboard();

        MimeDataGraphicsItem *mimeGi = new MimeDataGraphicsItem(selected);

        mimeGi->setData(Application::mimeTypeBoardPageItem, QByteArray());
        clipboard->setMimeData(mimeGi);
    }
}

void BoardController::paste()
{
    QClipboard *clipboard = QApplication::clipboard();
    qreal       xPosition = QRandomGenerator::global()->generateDouble() * 400;
    qreal       yPosition = QRandomGenerator::global()->generateDouble() * 200;
    QPointF     pos(xPosition - 200, yPosition - 100);
    processMimeData(clipboard->mimeData(), pos);

    selectedDocument()->setMetaData(Settings::documentUpdatedAt, StringUtils::toUtcIsoDateTime(QDateTime::currentDateTime()));
}

bool zLevelLessThan(Item *s1, Item *s2)
{
    qreal s1Zvalue = dynamic_cast<QGraphicsItem *>(s1)->data(GraphicsItemData::ItemOwnZValue).toReal();
    qreal s2Zvalue = dynamic_cast<QGraphicsItem *>(s2)->data(GraphicsItemData::ItemOwnZValue).toReal();
    return s1Zvalue < s2Zvalue;
}

void BoardController::processMimeData(const QMimeData *pMimeData, const QPointF &pPos)
{
    if (pMimeData->hasFormat(Application::mimeTypeBoardPage))
    {
        const MimeData *mimeData = qobject_cast<const MimeData *>(pMimeData);

        if (mimeData)
        {
            int previousActiveSceneIndex = activeSceneIndex();
            int previousPageCount        = selectedDocument()->pageCount();

            foreach (MimeDataItem sourceItem, mimeData->items())
            {
                addScene(sourceItem.documentProxy(), sourceItem.sceneIndex(), true);
            }

            if (selectedDocument()->pageCount() < previousPageCount + mimeData->items().count())
            {
                setActiveDocumentScene(previousActiveSceneIndex);
            }
            else
            {
                setActiveDocumentScene(previousActiveSceneIndex + 1);
            }

            return;
        }
    }

    if (pMimeData->hasFormat(Application::mimeTypeBoardPageItem))
    {
        const MimeDataGraphicsItem *mimeData = qobject_cast<const MimeDataGraphicsItem *>(pMimeData);

        if (mimeData)
        {
            QList<Item *> items = mimeData->items();
            std::stable_sort(items.begin(), items.end(), zLevelLessThan);
            foreach (Item *item, items)
            {
                QGraphicsItem *pItem = dynamic_cast<QGraphicsItem *>(item);
                if (NULL != pItem)
                {
                    duplicateItem(item);
                }
            }

            return;
        }
    }

    if (pMimeData->hasHtml())
    {
        QString qsHtml = pMimeData->html();
        QString url    = Application::urlFromHtml(qsHtml);

        if ("" != url)
        {
            downloadURL(url, QString(), pPos);
            return;
        }
    }

    if (pMimeData->hasUrls())
    {
        QList<QUrl> urls                         = pMimeData->urls();

        int index                                = 0;

        const FeaturesMimeData *internalMimeData = qobject_cast<const FeaturesMimeData *>(pMimeData);
        bool                    internalData     = false;
        if (internalMimeData)
        {
            internalData = true;
        }

        foreach (const QUrl url, urls)
        {
            QPointF pos(pPos + QPointF(index * 15, index * 15));

            downloadURL(url, QString(), pos, QSize(), false, internalData);
            index++;
        }

        return;
    }

    if (pMimeData->hasImage())
    {
        QImage  img = qvariant_cast<QImage>(pMimeData->imageData());
        QPixmap pix = QPixmap::fromImage(img);

        // 校验位图是否有效：网页端提供的图像 MIME 数据可能不完整。
        if (pix.width() != 0 && pix.height() != 0)
        {
            mActiveScene->addPixmap(pix, NULL, pPos, 1.);
            return;
        }
    }

    if (pMimeData->hasText())
    {
        if ("" != pMimeData->text())
        {
            // 文本也可能是一个 URL，先识别出来走下载流程。
            QString qsTmp = pMimeData->text().remove(QRegularExpression("[\\0]"));
            if (qsTmp.startsWith("http"))
            {
                downloadURL(QUrl(qsTmp), QString(), pPos);
            }
            else
            {
                if (mActiveScene->selectedItems().count() && mActiveScene->selectedItems().at(0)->type() == GraphicsItemType::TextItemType)
                {
                    dynamic_cast<GraphicsTextItem *>(mActiveScene->selectedItems().at(0))->setHtml(pMimeData->text());
                }
                else
                {
                    mActiveScene->addTextHtml("", pPos)->setHtml(pMimeData->text());
                }
            }
        }
    }
}

void BoardController::moveGraphicsWidgetToControlView(GraphicsWidgetItem *graphicsWidget)
{
    mActiveScene->setURStackEnable(false);
    GraphicsItem       *toolWidget           = duplicateItem(dynamic_cast<Item *>(graphicsWidget));
    GraphicsWidgetItem *copyedGraphicsWidget = NULL;

    if (toolWidget)
    {
        if (GraphicsWidgetItem::Type == toolWidget->type())
        {
            copyedGraphicsWidget = static_cast<GraphicsWidgetItem *>(toolWidget);
        }

        // 内层不叫 toolWidget：外层那个是场景里被复制出来的图元，同名会让人以为
        // 下面的 centerOn/show 操作的是同一个对象。
        ToolWidget *pinnedToolWidget = new ToolWidget(copyedGraphicsWidget, mControlView);

        graphicsWidget->remove(false);
        mActiveScene->addItemToDeletion(graphicsWidget);

        mActiveScene->setURStackEnable(true);

        QPoint controlViewPos = mControlView->mapFromScene(graphicsWidget->sceneBoundingRect().center());
        pinnedToolWidget->centerOn(mControlView->mapTo(mControlContainer, controlViewPos));
        pinnedToolWidget->show();
    }
}

void BoardController::moveToolWidgetToScene(ToolWidget *toolWidget)
{
    GraphicsWidgetItem *widgetToScene = toolWidget->toolWidget();

    widgetToScene->resetTransform();

    QPoint  mainWindowCenter  = toolWidget->mapTo(mMainWindow, QPoint(toolWidget->width(), toolWidget->height()) / 2);
    QPoint  controlViewCenter = mControlView->mapFrom(mMainWindow, mainWindowCenter);
    QPointF scenePos          = mControlView->mapToScene(controlViewCenter);

    mActiveScene->addGraphicsWidget(widgetToScene, scenePos);

    toolWidget->remove();
}

void BoardController::updateBackgroundActionsState(bool isDark, PageBackground pageBackground)
{
    switch (pageBackground)
    {
        case PageBackground::crossed:
            if (isDark)
            {
                mMainWindow->ui().actionCrossedDarkBackground->setChecked(true);
            }
            else
            {
                mMainWindow->ui().actionCrossedLightBackground->setChecked(true);
            }
            break;

        case PageBackground::ruled:
            if (isDark)
            {
                mMainWindow->ui().actionRuledDarkBackground->setChecked(true);
            }
            else
            {
                mMainWindow->ui().actionRuledLightBackground->setChecked(true);
            }
            break;

        default:
            if (isDark)
            {
                mMainWindow->ui().actionPlainDarkBackground->setChecked(true);
            }
            else
            {
                mMainWindow->ui().actionPlainLightBackground->setChecked(true);
            }
            break;
    }
}

void BoardController::addItem()
{
    QString defaultPath = Settings::settings()->lastImportToLibraryPath->get().toString();

    QString extensions;
    foreach (QString ext, Settings::imageFileExtensions)
    {
        extensions += " *.";
        extensions += ext;
    }

    QString filename = QFileDialog::getOpenFileName(
        mControlContainer, tr("Add Item"), defaultPath, tr("All Supported (%1)").arg(extensions), NULL, QFileDialog::DontUseNativeDialog);

    if (filename.length() > 0)
    {
        mPaletteManager->addItem(QUrl::fromLocalFile(filename));
        QFileInfo source(filename);
        Settings::settings()->lastImportToLibraryPath->set(QVariant(source.absolutePath()));
    }
}

void BoardController::importPage()
{
    int pageCount = selectedDocument()->pageCount();
    if (Application::documentController->addFileToDocument(selectedDocument()))
    {
        setActiveDocumentScene(selectedDocument(), pageCount, true);
    }
}

void BoardController::notifyPageChanged()
{
    emit activeSceneChanged();
}

void BoardController::onDownloadModalFinished()
{
}

void BoardController::displayMetaData(QMap<QString, QString> metadatas)
{
    emit displayMetadata(metadatas);
}

void BoardController::freezeWebWidgets(bool freeze)
{
    if (mActiveSceneIndex >= 0)
    {
        QList<QGraphicsItem *> list = Application::boardController->activeScene()->getFastAccessItems();
        foreach (QGraphicsItem *item, list)
        {
            freezeWebWidget(item, freeze);
        }
    }
}

void BoardController::freezeWebWidget(QGraphicsItem *item, bool freeze)
{
    if (item->type() == GraphicsWebWidgetItem::Type)
    {
        GraphicsWebWidgetItem *item_casted = dynamic_cast<GraphicsWebWidgetItem *>(item);
        if (0 == item_casted)
        {
            return;
        }

        // freeze 参数暂未参与分支：两种状态都重新加载主页面。
        item_casted->loadMainHtml();
    }
}
