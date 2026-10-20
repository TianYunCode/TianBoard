#include <QDomDocument>
#include <QWebEngineProfile>
#include <QRegularExpression>
#include <QRegExp>
#include <QWebEngineView>
#include <QWidget>

#include "ui/FeaturesWidget.h"
#include "ui/ThumbnailWidget.h"
#include "utils/FileSystemUtils.h"
#include "core/Application.h"
#include "core/DownloadManager.h"
#include "utils/Globals.h"
#include "whiteboard/BoardController.h"

const char *FeaturesWidget::objNamePathList = "PathList";
const char *FeaturesWidget::objNameFeatureList = "FeatureList";

const QMargins FeatureListMargins(0, 0, 0, 30);
const int FeatureListBorderOffset = 10;

struct FeaturesWidget::Private {
    FeaturesController *controller{nullptr};
    FeaturesListView *pathListView{nullptr};
    QVBoxLayout *layout{nullptr};
    FeaturesActionBar *mActionBar{nullptr};
    DownloadHttpFile *imageGatherer{nullptr};
    FeaturesCentralWidget *centralWidget{nullptr};
};

struct FeaturesMimeData::Private {
    QList<Feature> mFeatures;
};

struct FeaturesNavigatorWidget::Private {
    FeaturesListView *mListView{nullptr};
    QSlider *mListSlider{nullptr};
};

struct FeaturesCentralWidget::Private {
    QStackedWidget *mStackedWidget{nullptr};
    FeaturesNavigatorWidget *mNavigator{nullptr};
    FeatureProperties *mFeatureProperties{nullptr};
    FeaturesWebWidget *webView{nullptr};
    QStackedWidget *mAdditionalDataContainer{nullptr};
};

struct FeaturesNewFolderDialog::Private {
    QLineEdit *mLineEdit{nullptr};
    QRegularExpressionValidator *mValidator{nullptr};
    QStringList mFileNameList;
    QPushButton *acceptButton{nullptr};
    QString acceptText;
    QString cancelText;
    QString labelText;
};

struct FeaturesProgressInfo::Private {
    QProgressBar *mProgressBar{nullptr};
    QLabel *mCommonInfoLabel{nullptr};
    QLabel *mDetailedInfoLabel{nullptr};
};

struct FeaturesWebWidget::Private {
    QWebEngineView *mpView{nullptr};
    QWebEngineSettings *mpWebSettings{nullptr};
    QVBoxLayout *mpLayout{nullptr};
};

struct FeatureProperties::Private {
    QVBoxLayout *mpLayout{nullptr};
    QHBoxLayout *mpButtonLayout{nullptr};
    FeatureItemButton *mpAddPageButton{nullptr};
    FeatureItemButton *mpAddToLibButton{nullptr};
    FeatureItemButton *mpSetAsBackgroundButton{nullptr};
    QLabel *mpObjInfoLabel{nullptr};
    QTreeWidget *mpObjInfos{nullptr};
    QLabel *mpThumbnail{nullptr};
    QPixmap *mpOrigPixmap{nullptr};
    int maxThumbHeight{0};
    Feature *mpElement{nullptr};
    QTreeWidgetItem *mpItem{nullptr};
};

struct FeaturesModel::Private {
    QList<Feature> *featuresList{nullptr};
};

struct FeaturesSearchProxyModel::Private {
    QString mFilterPrefix;
};

struct FeaturesPathProxyModel::Private {
    QString path;
};

struct FeaturesItemDelegate::Private {
    const QListView *listView{nullptr};
};

struct FeaturesPathItemDelegate::Private {
    QPixmap *arrowPixmap{nullptr};
};

FeaturesWidget::FeaturesWidget(QWidget *parent, const char *name)
    : DockPaletteWidget(parent), d(new Private) {
    d->imageGatherer = NULL;

    setObjectName(name);
    setName("FeaturesWidget");
    setVisibleState(true);

    SET_STYLE_SHEET();

    setIconToLeft(QPixmap(":images/general_open.png"));
    setIconToRight(QPixmap(":images/general_close.png"));
    setAcceptDrops(true);

    // 资源库主控制器，随面板一起创建
    d->controller = new FeaturesController(this);

    // 面板总布局
    d->layout = new QVBoxLayout(this);

    // 顶部路径面包屑
    d->pathListView = new FeaturesListView(this, objNamePathList);
    d->controller->assignPathListView(d->pathListView);

    d->centralWidget = new FeaturesCentralWidget(this);
    d->controller->assignFeaturesListView(d->centralWidget->listView());
    d->centralWidget->setSliderPosition(Settings::settings()->featureSliderPosition->get().toInt());

    // 底部动作条，负责拖放与快速搜索
    d->mActionBar = new FeaturesActionBar(d->controller, this);

    d->layout->addWidget(d->pathListView);
    d->layout->addWidget(d->centralWidget);
    d->layout->addWidget(d->mActionBar);

    connect(d->centralWidget->listView(), &FeaturesListView::clicked, this, &FeaturesWidget::currentSelected);
    connect(this, &FeaturesWidget::sendFileNameList, d->centralWidget,
            &FeaturesCentralWidget::sendFileNameList);
    connect(d->mActionBar, &FeaturesActionBar::searchElement, this, &FeaturesWidget::searchStarted);
    connect(d->mActionBar, &FeaturesActionBar::newFolderToCreate, this, &FeaturesWidget::createNewFolder);
    connect(d->mActionBar, &FeaturesActionBar::deleteElements, this, &FeaturesWidget::deleteElements);
    connect(d->mActionBar, &FeaturesActionBar::deleteSelectedElements, this,
            &FeaturesWidget::deleteSelectedElements);
    connect(d->mActionBar, &FeaturesActionBar::addToFavorite, this, &FeaturesWidget::addToFavorite);
    connect(d->mActionBar, &FeaturesActionBar::removeFromFavorite, this, &FeaturesWidget::removeFromFavorite);
    connect(d->mActionBar, &FeaturesActionBar::addElementsToFavorite, this,
            &FeaturesWidget::addElementsToFavorite);
    connect(d->mActionBar, &FeaturesActionBar::removeElementsFromFavorite, this,
            &FeaturesWidget::removeElementsFromFavorite);

    connect(d->mActionBar, &FeaturesActionBar::rescanModel, this, &FeaturesWidget::rescanModel);
    connect(d->pathListView, &FeaturesListView::clicked, this, &FeaturesWidget::currentSelected);
    connect(Application::boardController, &BoardController::displayMetadata, this,
            &FeaturesWidget::onDisplayMetadata);
    connect(DownloadManager::downloadManager(), &DownloadManager::addDownloadedFileToLibrary, this,
            &FeaturesWidget::onAddDownloadedFileToLibrary);
    connect(d->centralWidget, &FeaturesCentralWidget::lockMainWidget, this, &FeaturesWidget::lockIt);
    connect(d->centralWidget, &FeaturesCentralWidget::createNewFolderSignal, d->controller,
            &FeaturesController::addNewFolder);
    connect(d->controller, &FeaturesController::scanStarted, d->centralWidget,
            &FeaturesCentralWidget::scanStarted);
    connect(d->controller, &FeaturesController::scanFinished, d->centralWidget,
            &FeaturesCentralWidget::scanFinished);
    connect(d->controller, &FeaturesController::scanStarted, d->mActionBar, &FeaturesActionBar::lockIt);
    connect(d->controller, &FeaturesController::scanFinished, d->mActionBar, &FeaturesActionBar::unlockIt);
    connect(d->controller, &FeaturesController::maxFilesCountEvaluated, d->centralWidget,
            &FeaturesCentralWidget::maxFilesCountEvaluated);
    connect(d->controller, &FeaturesController::featureAddedFromThread, d->centralWidget,
            &FeaturesCentralWidget::increaseStatusBarValue);
    connect(d->controller, &FeaturesController::scanCategory, d->centralWidget,
            &FeaturesCentralWidget::scanCategory);
    connect(d->controller, &FeaturesController::scanPath, d->centralWidget, &FeaturesCentralWidget::scanPath);
}

FeaturesWidget::~FeaturesWidget() {
    if (NULL != d->imageGatherer)
        delete d->imageGatherer;
}

FeaturesController *FeaturesWidget::getFeaturesController() const {
    return d->controller;
}

void FeaturesWidget::searchStarted(const QString &pattern) {
    d->controller->searchStarted(pattern, d->centralWidget->listView());
}

void FeaturesWidget::currentSelected(const QModelIndex &current) {
    if (!current.isValid()) {
        qWarning() << "SLOT:currentSelected, invalid index catched";
        return;
    }

    QString objName = sender()->objectName();

    if (objName.isEmpty()) {
        qWarning() << "incorrect sender";
    } else if (objName == objNamePathList) {
        // 点击路径面包屑相当于清空搜索并切回该位置。
        d->controller->searchStarted("", d->centralWidget->listView());
    }

    Feature feature = d->controller->getFeature(current, objName);

    if (feature.isFolder()) {
        QString newPath = feature.getFullVirtualPath();

        d->controller->setCurrentElement(feature);
        d->controller->siftElements(newPath);

        d->centralWidget->switchTo(FeaturesCentralWidget::MainList);

        if (feature.getType() == FEATURE_FAVORITE) {
            d->mActionBar->setCurrentState(IN_FAVORITE);

        } else if (feature.getType() == FEATURE_CATEGORY && feature.getName() == "root") {
            d->mActionBar->setCurrentState(IN_ROOT);

        } else if (feature.getType() == FEATURE_TRASH) {
            d->mActionBar->setCurrentState(IN_TRASH);

        } else if (feature.getType() == FEATURE_SEARCH) {
            // 搜索条目的行为与其他条目不同：点击列表里的搜索条目显示空白元素，
            // 点击路径列表里的搜索条目则切到已保存的搜索网页视图。
            if (sender()->objectName() == objNameFeatureList) {
                d->centralWidget->showElement(feature, FeaturesCentralWidget::FeaturesWebView);
            } else if (sender()->objectName() == objNamePathList) {
                d->centralWidget->switchTo(FeaturesCentralWidget::FeaturesWebView);
            }

        } else {
            d->mActionBar->setCurrentState(IN_FOLDER);
        }
    }

    else if (Settings::settings()->libraryShowDetailsForLocalItems->get().toBool() == true) {
        d->centralWidget->showElement(feature, FeaturesCentralWidget::FeaturePropertiesList);
        d->mActionBar->setCurrentState(IN_PROPERTIES);
    }
    d->mActionBar->cleanText();
}

void FeaturesWidget::createNewFolder() {
    d->centralWidget->showAdditionalData(FeaturesCentralWidget::NewFolderDialog,
                                         FeaturesCentralWidget::Modal);
    emit sendFileNameList(d->controller->getFileNamesInFolders());
}

void FeaturesWidget::deleteElements(const FeaturesMimeData *mimeData) {
    if (!mimeData->features().count())
        return;

    QList<Feature> featuresList = mimeData->features();

    foreach (Feature curFeature, featuresList) {
        if (curFeature.inTrash()) {
            d->controller->deleteItem(curFeature.getFullPath());

        } else {
            d->controller->moveToTrash(curFeature);
        }
    }

    d->controller->refreshModels();
}

void FeaturesWidget::deleteSelectedElements() {
    QModelIndexList selected = d->centralWidget->listView()->selectionModel()->selectedIndexes();

    QList<Feature> featureasToMove;
    for (int i = 0; i < selected.count(); i++) {
        featureasToMove.append(d->controller->getFeature(selected.at(i), objNameFeatureList));
    }

    foreach (Feature feature, featureasToMove) {
        if (feature.isDeletable()) {
            if (feature.inTrash()) {
                d->controller->deleteItem(feature);
            } else {
                d->controller->moveToTrash(feature, true);
            }
        }
    }

    d->controller->refreshModels();
}

void FeaturesWidget::rescanModel() {
    d->controller->rescanModel();
}

void FeaturesWidget::lockIt(bool pLock) {
    d->mActionBar->setEnabled(!pLock);
    d->pathListView->setEnabled(!pLock);
    d->centralWidget->setLockedExcludingAdditional(pLock);
}

void FeaturesWidget::addToFavorite(const FeaturesMimeData *mimeData) {
    if (!mimeData->hasUrls())
        return;

    QList<QUrl> urls = mimeData->urls();
    foreach (QUrl url, urls) {
        d->controller->addToFavorite(url);
    }

    d->controller->refreshModels();
}

void FeaturesWidget::removeFromFavorite(const FeaturesMimeData *mimeData) {
    if (!mimeData->hasUrls())
        return;

    QList<QUrl> urls = mimeData->urls();

    foreach (QUrl url, urls) {
        d->controller->removeFromFavorite(url);
    }
}

void FeaturesWidget::onDisplayMetadata(QMap<QString, QString> metadata) {
    QString previewImageUrl = ":images/libpalette/notFound.png";

    QString widgetsUrl = QUrl::fromEncoded(metadata["Url"].toLatin1()).toString();
    QString widgetsThumbsUrl = QUrl::fromEncoded(metadata["thumbnailUrl"].toLatin1()).toString();

    QString strType = FileSystemUtils::mimeTypeFromFileName(widgetsUrl);
    MimeType::Enum thumbType = FileSystemUtils::mimeTypeFromString(strType);

    switch (static_cast<int>(thumbType)) {
        case MimeType::Audio:
            // 音视频没有缩略图时用固定图标占位。
            previewImageUrl = ":images/libpalette/soundIcon.svg";
            break;

        case MimeType::Video:
            previewImageUrl = ":images/libpalette/movieIcon.svg";
            break;

        case MimeType::RasterImage:
        case MimeType::VectorImage:
            previewImageUrl = widgetsUrl;
            break;
    }

    if (!widgetsThumbsUrl.isNull()) {
        previewImageUrl = ":/images/libpalette/loading.png";
        if (!d->imageGatherer) {
            d->imageGatherer = new DownloadHttpFile(0, this);
            // 连接只在对象新建时做一次：imageGatherer 是复用的，每看一次部件元数据都
            // connect 一遍的话，一次下载完成会把 onPreviewLoaded 回调多次。
            connect(d->imageGatherer, &DownloadHttpFile::downloadFinished, this,
                    &FeaturesWidget::onPreviewLoaded);
        }

        // 抓取期间先显示加载占位图，下载完成后由 onPreviewLoaded() 替换。
        d->imageGatherer->get(QUrl(widgetsThumbsUrl), QPoint(0, 0), QSize(), false);
    }

    Feature feature("/root", QImage(previewImageUrl), QString(), widgetsUrl, FEATURE_ITEM);
    feature.setMetadata(metadata);

    d->centralWidget->showElement(feature, FeaturesCentralWidget::FeaturePropertiesList);
    d->mActionBar->setCurrentState(IN_PROPERTIES);
}


void FeaturesWidget::onPreviewLoaded(int id, bool pSuccess, QUrl sourceUrl, QUrl originalUrl,
                                     QString pContentTypeHeader, QByteArray pData, QPointF pPos, QSize pSize,
                                     bool isBackground) {
    Q_UNUSED(id);
    Q_UNUSED(pSuccess);
    Q_UNUSED(originalUrl);
    Q_UNUSED(isBackground);
    Q_UNUSED(pSize);
    Q_UNUSED(pPos);
    Q_UNUSED(sourceUrl);
    Q_UNUSED(pContentTypeHeader)

    QImage img;
    img.loadFromData(pData);
    QPixmap pix = QPixmap::fromImage(img);
    d->centralWidget->setPropertiesPixmap(pix);
    d->centralWidget->setPropertiesThumbnail(pix);
}

void FeaturesWidget::onAddDownloadedFileToLibrary(bool pSuccess, QUrl sourceUrl, QString pContentHeader,
                                                  QByteArray pData, QString pTitle) {
    if (pSuccess) {
        qDebug() << pData.length();
        d->controller->addDownloadedFile(sourceUrl, pData, pContentHeader, pTitle);
        d->controller->refreshModels();
    }
}

void FeaturesWidget::addElementsToFavorite() {
    if (d->centralWidget->currentView() == FeaturesCentralWidget::FeaturePropertiesList) {
        Feature feature = d->centralWidget->getCurElementFromProperties();
        if (feature != Feature() && !Application::isFromWeb(feature.getFullPath().toString())) {
            d->controller->addToFavorite(feature.getFullPath());
        }

    } else if (d->centralWidget->currentView() == FeaturesCentralWidget::MainList) {
        QModelIndexList selected = d->centralWidget->listView()->selectionModel()->selectedIndexes();
        for (int i = 0; i < selected.size(); ++i) {
            Feature feature = selected.at(i).data(Qt::UserRole + 1).value<Feature>();
            d->controller->addToFavorite(feature.getFullPath());
        }
    }

    d->controller->refreshModels();
}

void FeaturesWidget::removeElementsFromFavorite() {
    QModelIndexList selected = d->centralWidget->listView()->selectionModel()->selectedIndexes();
    QList<QUrl> items;
    for (int i = 0; i < selected.size(); ++i) {
        Feature feature = selected.at(i).data(Qt::UserRole + 1).value<Feature>();
        items.append(feature.getFullPath());
    }

    foreach (QUrl url, items) {
        d->controller->removeFromFavorite(url, true);
    }

    d->controller->refreshModels();
}

void FeaturesWidget::switchToListView() {
    // 视图切换已由 FeaturesCentralWidget 负责，这里保留空实现以兼容旧调用点。
}

void FeaturesWidget::switchToProperties() {
    // 同上：保留空实现，避免旧调用点失效。
}

void FeaturesWidget::switchToWebView() {
    // 同上：保留空实现，避免旧调用点失效。
}

FeaturesMimeData::FeaturesMimeData() : d(new Private) {
    // 空实现：条目列表由调用方通过 setUrls()/setFeatures() 填充。
}

FeaturesMimeData::~FeaturesMimeData() = default;

QList<Feature> FeaturesMimeData::features() const {
    return d->mFeatures;
}

void FeaturesMimeData::setFeatures(const QList<Feature> &fList) {
    d->mFeatures = fList;
}

QStringList FeaturesMimeData::formats() const {
    return QMimeData::formats();
}

void FeaturesWidget::importImage(const QImage &image, const QString &fileName) {
    d->controller->importImage(image, fileName);
}

FeaturesListView::FeaturesListView(QWidget *parent, const char *name) : QListView(parent) {
    setObjectName(name);
}

void FeaturesListView::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls() || event->mimeData()->hasImage())
        event->acceptProposedAction();
}

void FeaturesListView::dragMoveEvent(QDragMoveEvent *event) {
    const FeaturesMimeData *fMimeData = qobject_cast<const FeaturesMimeData *>(event->mimeData());
    QModelIndex index = indexAt(event->pos());
    Feature onFeature = model()->data(index, Qt::UserRole + 1).value<Feature>();
    if (fMimeData) {
        if (!index.isValid() || !onFeature.isFolder()) {
            event->ignore();
            return;
        }
        foreach (Feature curFeature, fMimeData->features()) {
            if (curFeature == onFeature) {
                event->ignore();
                return;
            }
        }
    }

    if (event->mimeData()->hasUrls() || event->mimeData()->hasImage()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void FeaturesListView::dropEvent(QDropEvent *event) {
    QObject *eventSource = event->source();
    if (eventSource && eventSource->objectName() == FeaturesWidget::objNameFeatureList) {
        event->setDropAction(Qt::MoveAction);
    }

    QListView::dropEvent(event);
}

void FeaturesListView::thumbnailSizeChanged(int value) {
    setIconSize(QSize(value, value));
    setGridSize(QSize(value + 20, value + 20));

    Settings::settings()->featureSliderPosition->set(value);
}

FeaturesNavigatorWidget::FeaturesNavigatorWidget(QWidget *parent, const char *name)
    : QWidget(parent),
      d(new Private)

{
    d->mListView = 0;
    d->mListSlider = 0;

    // 传入的 name 被忽略，固定对象名，方便按名称查找列表。
    name = "FeaturesNavigatorWidget";

    setObjectName(name);

    d->mListView = new FeaturesListView(this, FeaturesWidget::objNameFeatureList);

    d->mListSlider = new QSlider(Qt::Horizontal, this);

    d->mListSlider->setMinimum(FeaturesWidget::minThumbnailSize);
    d->mListSlider->setMaximum(FeaturesWidget::maxThumbnailSize);
    d->mListSlider->setValue(FeaturesWidget::minThumbnailSize);
    d->mListSlider->setMinimumHeight(20);

    d->mListView->setParent(this);
    QVBoxLayout *mainLayer = new QVBoxLayout(this);

    mainLayer->addWidget(d->mListView, 1);
    mainLayer->addWidget(d->mListSlider, 0);
    mainLayer->setContentsMargins(0, 0, 0, 0);

    connect(d->mListSlider, &QSlider::valueChanged, d->mListView, &FeaturesListView::thumbnailSizeChanged);
}

FeaturesListView *FeaturesNavigatorWidget::listView() {
    return d->mListView;
}

void FeaturesNavigatorWidget::setSliderPosition(int pValue) {
    d->mListSlider->setValue(pValue);
}

FeaturesCentralWidget::FeaturesCentralWidget(QWidget *parent) : QWidget(parent), d(new Private) {
    setObjectName("FeaturesCentralWidget");
    SET_STYLE_SHEET();

    QVBoxLayout *mLayout = new QVBoxLayout(this);
    setLayout(mLayout);

    // 中央区域按页面切换：主列表、属性页、网页视图
    d->mStackedWidget = new QStackedWidget(this);

    // 主列表，底部带缩略图尺寸滑块
    d->mNavigator = new FeaturesNavigatorWidget(this);

    // 单个条目的属性页
    d->mFeatureProperties = new FeatureProperties(this);

    // 搜索等网页内容
    d->webView = new FeaturesWebWidget(this);

    d->mStackedWidget->addWidget(d->mNavigator);
    d->mStackedWidget->addWidget(d->mFeatureProperties);
    d->mStackedWidget->addWidget(d->webView);
    d->mStackedWidget->setCurrentIndex(MainList);
    d->mStackedWidget->setContentsMargins(0, 0, 0, 0);


    d->mAdditionalDataContainer = new QStackedWidget(this);
    d->mAdditionalDataContainer->setObjectName("mAdditionalDataContainer");

    // 新建文件夹输入区
    FeaturesNewFolderDialog *dlg = new FeaturesNewFolderDialog(d->mAdditionalDataContainer);
    d->mAdditionalDataContainer->addWidget(dlg);
    d->mAdditionalDataContainer->setCurrentIndex(NewFolderDialog);

    connect(dlg, &FeaturesNewFolderDialog::createNewFolder, this,
            &FeaturesCentralWidget::createNewFolderSlot);
    connect(dlg, &FeaturesNewFolderDialog::closeDialog, this, &FeaturesCentralWidget::hideAdditionalData);
    connect(this, &FeaturesCentralWidget::sendFileNameList, dlg, &FeaturesNewFolderDialog::setFileNameList);

    // 扫描进度区
    FeaturesProgressInfo *progressBar = new FeaturesProgressInfo();
    d->mAdditionalDataContainer->addWidget(progressBar);
    d->mAdditionalDataContainer->setCurrentIndex(ProgressBarWidget);

    connect(this, &FeaturesCentralWidget::maxFilesCountEvaluated, progressBar,
            &FeaturesProgressInfo::setProgressMax);
    connect(this, &FeaturesCentralWidget::increaseStatusBarValue, progressBar,
            &FeaturesProgressInfo::increaseProgressValue);
    connect(this, &FeaturesCentralWidget::scanCategory, progressBar,
            &FeaturesProgressInfo::setCommonInfoText);
    connect(this, &FeaturesCentralWidget::scanPath, progressBar, &FeaturesProgressInfo::setDetailedInfoText);

    mLayout->addWidget(d->mStackedWidget, 1);
    mLayout->addWidget(d->mAdditionalDataContainer, 0);

    d->mAdditionalDataContainer->hide();
}

FeaturesListView *FeaturesCentralWidget::listView() {
    return d->mNavigator->listView();
}

void FeaturesCentralWidget::setSliderPosition(int pValue) {
    d->mNavigator->setSliderPosition(pValue);
}

FeaturesCentralWidget::StackElement FeaturesCentralWidget::currentView() const {
    return static_cast<StackElement>(d->mStackedWidget->currentIndex());
}

void FeaturesCentralWidget::showElement(const Feature &feature, StackElement pView) {
    if (pView == FeaturesWebView) {
        d->webView->showElement(feature);
        d->mStackedWidget->setCurrentIndex(FeaturesWebView);
    } else if (pView == FeaturePropertiesList) {
        d->mFeatureProperties->showElement(feature);
        d->mStackedWidget->setCurrentIndex(FeaturePropertiesList);
    }
}

void FeaturesCentralWidget::switchTo(StackElement pView) {
    d->mStackedWidget->setCurrentIndex(pView);
}

void FeaturesCentralWidget::setPropertiesPixmap(const QPixmap &pix) {
    d->mFeatureProperties->setOrigPixmap(pix);
}

void FeaturesCentralWidget::setPropertiesThumbnail(const QPixmap &pix) {
    d->mFeatureProperties->setThumbnail(pix);
}

Feature FeaturesCentralWidget::getCurElementFromProperties() {
    return d->mFeatureProperties->getCurrentElement();
}

void FeaturesCentralWidget::showAdditionalData(AddWidget pWidgetType, AddWidgetState pState) {
    if (!d->mAdditionalDataContainer->widget(pWidgetType)) {
        qDebug() << "can't find widget specified by FeaturesCentralWidget::showAdditionalData(AddWidget "
                    "pWidgetType, AddWidgetState pState)";
        return;
    }

    d->mAdditionalDataContainer->setMaximumHeight(
        d->mAdditionalDataContainer->widget(pWidgetType)->sizeHint().height());

    d->mAdditionalDataContainer->setCurrentIndex(pWidgetType);
    d->mAdditionalDataContainer->show();
    emit lockMainWidget(pState == Modal ? true : false);
}

void FeaturesCentralWidget::setLockedExcludingAdditional(bool pLock) {
    // 只锁主视图，附加信息区（对话框/进度条）仍需可交互。
    d->mStackedWidget->setEnabled(!pLock);
}

void FeaturesCentralWidget::createNewFolderSlot(QString pStr) {
    emit createNewFolderSignal(pStr);
    hideAdditionalData();
}

void FeaturesCentralWidget::hideAdditionalData() {
    emit lockMainWidget(false);
    d->mAdditionalDataContainer->hide();
}

void FeaturesCentralWidget::scanStarted() {
    showAdditionalData(ProgressBarWidget);
}

void FeaturesCentralWidget::scanFinished() {
    hideAdditionalData();
}

FeaturesNewFolderDialog::FeaturesNewFolderDialog(QWidget *parent) : QWidget(parent), d(new Private) {
    d->acceptText = tr("Accept");
    d->cancelText = tr("Cancel");
    d->labelText = tr("Enter a new folder name");

    this->setStyleSheet("QPushButton { background:white; }");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QVBoxLayout *labelLayout = new QVBoxLayout();
    labelLayout->setSizeConstraint(QLayout::SetMinimumSize);

    QLabel *mLabel = new QLabel(d->labelText, this);
    mLabel->setWordWrap(true);
    mLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    d->mLineEdit = new QLineEdit(this);

    d->mValidator =
        new QRegularExpressionValidator(QRegularExpression("[^\\/\\:\\?\\*\\|\\<\\>\\\"]{2,}"), this);
    d->mLineEdit->setValidator(d->mValidator);
    labelLayout->addWidget(mLabel);
    labelLayout->addWidget(d->mLineEdit);

    QHBoxLayout *buttonLayout = new QHBoxLayout();

    d->acceptButton = new QPushButton(d->acceptText, this);
    QPushButton *cancelButton = new QPushButton(d->cancelText, this);
    buttonLayout->addWidget(d->acceptButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(labelLayout);
    mainLayout->addLayout(buttonLayout);

    d->acceptButton->setEnabled(false);

    connect(d->acceptButton, &QPushButton::clicked, this, &FeaturesNewFolderDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &FeaturesNewFolderDialog::reject);
    connect(d->mLineEdit, &QLineEdit::textEdited, this, &FeaturesNewFolderDialog::reactOnTextChanged);

    reactOnTextChanged(QString());
}

void FeaturesNewFolderDialog::setRegexp(const QRegularExpression &pRegExp) {
    d->mValidator->setRegularExpression(pRegExp);
}
bool FeaturesNewFolderDialog::validString(const QString &pStr) {
    return d->mLineEdit->hasAcceptableInput() && !d->mFileNameList.contains(pStr, Qt::CaseSensitive);
}

void FeaturesNewFolderDialog::accept() {
    // 名称合法性已由行编辑上的校验器与 reactOnTextChanged() 把关，
    // 这里只负责把结果发给控制器并清空输入框。
    emit createNewFolder(d->mLineEdit->text());
    d->mLineEdit->clear();
}
void FeaturesNewFolderDialog::reject() {
    d->mLineEdit->clear();
    emit closeDialog();
}
void FeaturesNewFolderDialog::setFileNameList(const QStringList &pLst) {
    d->mFileNameList = pLst;
}
void FeaturesNewFolderDialog::reactOnTextChanged(const QString &pStr) {
    if (validString(pStr)) {
        d->acceptButton->setEnabled(true);
        d->mLineEdit->setStyleSheet("background:white;");
    } else {
        d->acceptButton->setEnabled(false);
        d->mLineEdit->setStyleSheet("background:#FFB3C8;");
    }
}

FeaturesProgressInfo::FeaturesProgressInfo(QWidget *parent) : QWidget(parent), d(new Private) {
    d->mProgressBar = 0;
    d->mCommonInfoLabel = 0;
    d->mDetailedInfoLabel = 0;

    QVBoxLayout *mainLayer = new QVBoxLayout(this);

    d->mProgressBar = new QProgressBar(this);
    // 进度条的初始区间只是占位，真实总量由扫描信号驱动 setProgressMax() 覆盖。
    d->mProgressBar->setMinimum(0);
    d->mProgressBar->setMaximum(100000);
    d->mProgressBar->setValue(0);

    d->mProgressBar->setStyleSheet("background:white");

    d->mCommonInfoLabel = new QLabel(this);
    d->mDetailedInfoLabel = new QLabel(this);
    d->mDetailedInfoLabel->setAlignment(Qt::AlignRight);
    d->mCommonInfoLabel->hide();
    d->mDetailedInfoLabel->hide();

    mainLayer->addWidget(d->mCommonInfoLabel);
    mainLayer->addWidget(d->mDetailedInfoLabel);
    mainLayer->addWidget(d->mProgressBar);
}

void FeaturesProgressInfo::setCommonInfoText(const QString &str) {
    d->mProgressBar->setFormat(tr("Loading ") + str + " (%p%)");
}

void FeaturesProgressInfo::setDetailedInfoText(const QString &str) {
    d->mDetailedInfoLabel->setText(str);
}

void FeaturesProgressInfo::setProgressMax(int pValue) {
    d->mProgressBar->setMaximum(pValue);
}

void FeaturesProgressInfo::setProgressMin(int pValue) {
    d->mProgressBar->setMinimum(pValue);
}

void FeaturesProgressInfo::increaseProgressValue() {
    d->mProgressBar->setValue(d->mProgressBar->value() + 1);
}

void FeaturesProgressInfo::sendFeature(Feature pFeature) {
    Q_UNUSED(pFeature);
}


FeaturesWebWidget::FeaturesWebWidget(QWidget *parent, const char *name) : QWidget(parent), d(new Private) {
    d->mpView = NULL;
    d->mpWebSettings = NULL;
    d->mpLayout = NULL;

    setObjectName(name);

    SET_STYLE_SHEET();

    d->mpLayout = new QVBoxLayout();
    setLayout(d->mpLayout);

    d->mpView = new QWebEngineView(this);
    d->mpView->setObjectName("SearchEngineView");
    d->mpWebSettings = QWebEngineProfile::defaultProfile()->settings();
    d->mpWebSettings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    d->mpWebSettings->setAttribute(QWebEngineSettings::PluginsEnabled, true);
    d->mpWebSettings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    d->mpWebSettings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
    d->mpWebSettings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

    d->mpLayout->addWidget(d->mpView);
    d->mpLayout->setContentsMargins(0, 0, 0, 0);
}

FeaturesWebWidget::~FeaturesWebWidget() {
    if (NULL != d->mpView) {
        delete d->mpView;
        d->mpView = NULL;
    }
    if (NULL != d->mpLayout) {
        delete d->mpLayout;
        d->mpLayout = NULL;
    }
}

void FeaturesWebWidget::showElement(const Feature &elem) {
    QString qsWidgetName;
    QString path = elem.getFullPath().toLocalFile();

    QString qsConfigPath = QString("%0/config.xml").arg(path);

    if (QFile::exists(qsConfigPath)) {
        QFile f(qsConfigPath);
        if (f.open(QIODevice::ReadOnly)) {
            QDomDocument domDoc;
            domDoc.setContent(QString(f.readAll()));
            QDomElement root = domDoc.documentElement();

            QDomNode node = root.firstChild();
            while (!node.isNull()) {
                if (node.toElement().tagName() == "content") {
                    QDomAttr srcAttr = node.toElement().attributeNode("src");
                    qsWidgetName = srcAttr.value();
                    break;
                }
                node = node.nextSibling();
            }
            f.close();
        }
    }

    d->mpView->load(QUrl::fromLocalFile(QString("%0/%1").arg(path).arg(qsWidgetName)));
}

FeatureProperties::FeatureProperties(QWidget *parent, const char *name) : QWidget(parent), d(new Private) {
    d->mpLayout = NULL;
    d->mpButtonLayout = NULL;
    d->mpAddPageButton = NULL;
    d->mpAddToLibButton = NULL;
    d->mpObjInfoLabel = NULL;
    d->mpObjInfos = NULL;
    d->mpThumbnail = NULL;
    d->mpOrigPixmap = NULL;
    d->mpElement = NULL;

    setObjectName(name);

    // 属性页控件树：缩略图、操作按钮条、信息标题与两列信息表。
    d->mpLayout = new QVBoxLayout(this);
    setLayout(d->mpLayout);

    d->maxThumbHeight = height() / 4;

    d->mpThumbnail = new QLabel();
    QPixmap icon(":images/libpalette/notFound.png");
    icon.scaledToWidth(THUMBNAIL_WIDTH);

    d->mpThumbnail->setPixmap(icon);
    d->mpThumbnail->setObjectName("DockPaletteWidgetBox");
    d->mpThumbnail->setStyleSheet("background:white;");
    d->mpThumbnail->setAlignment(Qt::AlignHCenter);
    d->mpLayout->addWidget(d->mpThumbnail, 0);

    d->mpButtonLayout = new QHBoxLayout();
    d->mpLayout->addLayout(d->mpButtonLayout, 0);

    d->mpAddPageButton = new FeatureItemButton();
    d->mpAddPageButton->setText(tr("Add to page"));
    d->mpButtonLayout->addWidget(d->mpAddPageButton);

    d->mpAddToLibButton = new FeatureItemButton();
    d->mpAddToLibButton->setText(tr("Add to library"));
    d->mpButtonLayout->addWidget(d->mpAddToLibButton);

    d->mpButtonLayout->addStretch(1);

    d->mpObjInfoLabel = new QLabel(tr("Object informations"));
    d->mpObjInfoLabel->setStyleSheet(QString("color: #888888; font-size : 18px; font-weight:bold;"));
    d->mpLayout->addWidget(d->mpObjInfoLabel, 0);

    d->mpObjInfos = new QTreeWidget(this);
    d->mpObjInfos->setColumnCount(2);
    d->mpObjInfos->header()->hide();
    d->mpObjInfos->setAlternatingRowColors(true);
    d->mpObjInfos->setRootIsDecorated(false);
    d->mpObjInfos->setObjectName("DockPaletteWidgetBox");
    d->mpObjInfos->setStyleSheet("background:white;");
    d->mpLayout->addWidget(d->mpObjInfos, 1);
    d->mpLayout->setContentsMargins(0, 0, 0, 0);

    connect(d->mpAddPageButton, &QPushButton::clicked, this, &FeatureProperties::onAddToPage);
    connect(d->mpAddToLibButton, &QPushButton::clicked, this, &FeatureProperties::onAddToLib);
}

FeatureProperties::~FeatureProperties() {
    if (d->mpOrigPixmap) {
        delete d->mpOrigPixmap;
        d->mpOrigPixmap = NULL;
    }
    if (d->mpElement) {
        delete d->mpElement;
        d->mpElement = NULL;
    }
    if (d->mpThumbnail) {
        delete d->mpThumbnail;
        d->mpThumbnail = NULL;
    }
    if (d->mpButtonLayout) {
        delete d->mpButtonLayout;
        d->mpButtonLayout = NULL;
    }
    if (d->mpAddPageButton) {
        delete d->mpAddPageButton;
        d->mpAddPageButton = NULL;
    }
    if (d->mpAddToLibButton) {
        delete d->mpAddToLibButton;
        d->mpAddToLibButton = NULL;
    }
    if (d->mpObjInfoLabel) {
        delete d->mpObjInfoLabel;
        d->mpObjInfoLabel = NULL;
    }
    if (d->mpObjInfos) {
        delete d->mpObjInfos;
        d->mpObjInfos = NULL;
    }
}

void FeatureProperties::resizeEvent(QResizeEvent *event) {
    Q_UNUSED(event);
    adaptSize();
}

void FeatureProperties::showEvent(QShowEvent *event) {
    Q_UNUSED(event);
    adaptSize();
}

Feature FeatureProperties::getCurrentElement() const {
    if (d->mpElement)
        return *d->mpElement;

    return Feature();
}

void FeatureProperties::setOrigPixmap(const QPixmap &pix) {
    if (d->mpOrigPixmap)
        delete d->mpOrigPixmap;

    d->mpOrigPixmap = new QPixmap(pix);
}

void FeatureProperties::setThumbnail(const QPixmap &pix) {
    d->mpThumbnail->setPixmap(pix.scaledToWidth(THUMBNAIL_WIDTH));
    adaptSize();
}

void FeatureProperties::adaptSize() {
    if (NULL != d->mpOrigPixmap) {
        if (width() < THUMBNAIL_WIDTH + 40) {
            d->mpThumbnail->setPixmap(d->mpOrigPixmap->scaledToWidth(width() - 40));
        } else {
            d->mpThumbnail->setPixmap(d->mpOrigPixmap->scaledToWidth(THUMBNAIL_WIDTH));
        }
    }
}

void FeatureProperties::showElement(const Feature &elem) {
    if (d->mpOrigPixmap) {
        delete d->mpOrigPixmap;
        d->mpOrigPixmap = NULL;
    }
    if (d->mpElement) {
        delete d->mpElement;
        d->mpElement = NULL;
    }
    d->mpElement = new Feature(elem);
    d->mpOrigPixmap = new QPixmap(QPixmap::fromImage(elem.getThumbnail()));
    d->mpThumbnail->setPixmap(QPixmap::fromImage(elem.getThumbnail()).scaledToWidth(THUMBNAIL_WIDTH));
    populateMetadata();

    if (Application::isFromWeb(elem.getFullPath().toString())) {
        d->mpAddToLibButton->show();
    } else {
        d->mpAddToLibButton->hide();
    }
}

void FeatureProperties::populateMetadata() {
    if (NULL != d->mpObjInfos) {
        d->mpObjInfos->clear();
        QMap<QString, QString> metas = d->mpElement->getMetadata();
        QList<QString> lKeys = metas.keys();
        QList<QString> lValues = metas.values();

        for (int i = 0; i < metas.size(); i++) {
            QStringList values;
            values << lKeys.at(i);
            values << lValues.at(i);
            d->mpItem = new QTreeWidgetItem(values);
            d->mpObjInfos->addTopLevelItem(d->mpItem);
        }
        d->mpObjInfos->resizeColumnToContents(0);
    }
}

void FeatureProperties::onAddToPage() {
    QWidget *w = parentWidget()->parentWidget()->parentWidget();
    FeaturesWidget *featuresWidget = qobject_cast<FeaturesWidget *>(w);
    if (featuresWidget)
        featuresWidget->getFeaturesController()->addItemToPage(*d->mpElement);
}

void FeatureProperties::onAddToLib() {
    if (Application::isFromWeb(d->mpElement->getFullPath().toString())) {
        sDownloadFileDesc desc;
        desc.isBackground = false;
        desc.modal = false;
        desc.dest = sDownloadFileDesc::library;
        desc.name = d->mpElement->getMetadata().value("Title", QString());
        qDebug() << desc.name;
        desc.srcUrl = d->mpElement->getFullPath().toString();
        QString str1 = d->mpElement->getFullPath().toString().normalized(QString::NormalizationForm_C);
        QString str2 = d->mpElement->getFullPath().toString().normalized(QString::NormalizationForm_D);
        QString str3 = d->mpElement->getFullPath().toString().normalized(QString::NormalizationForm_KC);
        QString str4 = d->mpElement->getFullPath().toString().normalized(QString::NormalizationForm_KD);
        qDebug() << desc.srcUrl << Qt::endl
                 << "str1" << str1 << Qt::endl
                 << "str2" << str2 << Qt::endl
                 << "str3" << str3 << Qt::endl
                 << "str4" << str4 << Qt::endl;
        DownloadManager::downloadManager()->addFileToDownload(desc);
    }
}


void FeatureProperties::onSetAsBackground() {
    QWidget *w = parentWidget()->parentWidget()->parentWidget();
    FeaturesWidget *featuresWidget = qobject_cast<FeaturesWidget *>(w);
    if (!featuresWidget) {
        qWarning() << "FeatureProperties is not nested under FeaturesWidget; ignoring background request";
        return;
    }
    featuresWidget->getFeaturesController()->addItemAsBackground(*d->mpElement);
}


FeatureItemButton::FeatureItemButton(QWidget *parent, const char *name) : QPushButton(parent) {
    setObjectName(name);
    setStyleSheet(QString("background-color : #DDDDDD; color : #555555; border-radius : 6px; padding : 5px; "
                          "font-weight : bold; font-size : 12px;"));
}

FeatureItemButton::~FeatureItemButton() {}

FeaturesModel::FeaturesModel(QList<Feature> *pFeaturesList, QObject *parent)
    : QAbstractListModel(parent), d(new Private) {
    d->featuresList = pFeaturesList;
}

FeaturesModel::~FeaturesModel() = default;

QVariant FeaturesModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole) {
        return d->featuresList->at(index.row()).getDisplayName();
    }

    else if (role == Qt::DecorationRole) {
        return QIcon(QPixmap::fromImage(d->featuresList->at(index.row()).getThumbnail()));

    } else if (role == Qt::UserRole) {
        return d->featuresList->at(index.row()).getVirtualPath();

    } else if (role == Qt::UserRole + 1) {
        // 该角色按值交出整个 Feature：mimeData、收藏操作与代理模型都要拿完整条目，
        // 只回传 getType() 不够用。
        Feature f = d->featuresList->at(index.row());
        return QVariant::fromValue(f);
    }

    return QVariant();
}

QMimeData *FeaturesModel::mimeData(const QModelIndexList &indexes) const {
    FeaturesMimeData *mimeData = new FeaturesMimeData();
    QList<QUrl> urlList;
    QList<Feature> featuresList;

    foreach (QModelIndex index, indexes) {
        if (index.isValid()) {
            Feature element = data(index, Qt::UserRole + 1).value<Feature>();
            urlList.push_back(element.getFullPath());
            featuresList.append(element);
        }
    }

    mimeData->setUrls(urlList);
    mimeData->setFeatures(featuresList);

    return mimeData;
}

bool FeaturesModel::dropMimeData(const QMimeData *mimeData, Qt::DropAction action, int row, int column,
                                 const QModelIndex &parent) {
    Q_UNUSED(row)

    const FeaturesMimeData *fMimeData = qobject_cast<const FeaturesMimeData *>(mimeData);
    FeaturesController *curController = qobject_cast<FeaturesController *>(QObject::parent());

    bool dataFromSameModel = false;

    if (fMimeData)
        dataFromSameModel = true;

    if ((!mimeData->hasUrls() && !mimeData->hasImage()))
        return false;
    if (action == Qt::IgnoreAction)
        return true;
    if (column > 0)
        return false;

    Feature parentFeature;
    if (!parent.isValid()) {
        parentFeature = curController->getCurrentElement();
    } else {
        parentFeature = parent.data(Qt::UserRole + 1).value<Feature>();
    }

    if (dataFromSameModel) {
        QList<Feature> featList = fMimeData->features();
        for (int i = 0; i < featList.count(); i++) {
            Feature sourceElement;
            if (dataFromSameModel) {
                sourceElement = featList.at(i);
                moveData(sourceElement, parentFeature, Qt::MoveAction, true);
            }
        }
    } else if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();
        foreach (QUrl curUrl, urlList) {
            qDebug() << "URl catched is " << curUrl.toLocalFile();
            curController->moveExternalData(curUrl, parentFeature);
        }
    } else if (mimeData->hasImage()) {
        QImage image = qvariant_cast<QImage>(mimeData->imageData());
        curController->importImage(image, parentFeature);
    }

    return true;
}

void FeaturesModel::addItem(const Feature &item) {
    beginInsertRows(QModelIndex(), d->featuresList->size(), d->featuresList->size());
    d->featuresList->append(item);
    endInsertRows();
}

void FeaturesModel::deleteFavoriteItem(const QString &path) {
    for (int i = 0; i < d->featuresList->size(); ++i) {
        if (!QString::compare(d->featuresList->at(i).getFullPath().toString(), path, Qt::CaseInsensitive) &&
            !QString::compare(d->featuresList->at(i).getVirtualPath(), "/root/favorites",
                              Qt::CaseInsensitive)) {
            removeRow(i, QModelIndex());
            return;
        }
    }
}

void FeaturesModel::deleteItem(const QString &path) {
    for (int i = 0; i < d->featuresList->size(); ++i) {
        if (!QString::compare(d->featuresList->at(i).getFullPath().toString(), path, Qt::CaseInsensitive)) {
            removeRow(i, QModelIndex());
            return;
        }
    }
}

void FeaturesModel::deleteItem(const Feature &feature) {
    int i = d->featuresList->indexOf(feature);
    if (i == -1) {
        qDebug() << "library item vanished while deleting";
        return;
    }
    removeRow(i, QModelIndex());
}

bool FeaturesModel::removeRows(int row, int count, const QModelIndex &parent) {
    if (row < 0)
        return false;
    if (row + count > d->featuresList->size())
        return false;
    beginRemoveRows(parent, row, row + count - 1);
    d->featuresList->erase(d->featuresList->begin() + row, d->featuresList->begin() + row + count);
    endRemoveRows();
    return true;
}

bool FeaturesModel::removeRow(int row, const QModelIndex &parent) {
    if (row < 0)
        return false;
    if (row >= d->featuresList->size())
        return false;
    beginRemoveRows(parent, row, row);
    d->featuresList->erase(d->featuresList->begin() + row);
    endRemoveRows();
    return true;
}

void FeaturesModel::moveData(const Feature &source, const Feature &destination,
                             Qt::DropAction action = Qt::CopyAction, bool deleteManualy) {
    FeaturesController *curController = qobject_cast<FeaturesController *>(QObject::parent());
    if (!curController)
        return;

    QString sourcePath = source.getFullPath().toLocalFile();
    QString sourceVirtualPath = source.getVirtualPath();

    FeatureElementType sourceType = source.getType();
    QImage sourceIcon = source.getThumbnail();

    if (sourceType == FEATURE_INTERNAL) {
        qWarning() << "Built-in tools cannot be moved";
        return;
    }

    Q_ASSERT(QFileInfo(sourcePath).exists());

    QString name = QFileInfo(sourcePath).fileName();
    QString destPath = destination.getFullPath().toLocalFile();

    QString destVirtualPath = destination.getFullVirtualPath();
    QString destFullPath = destPath + "/" + name;

    if (sourcePath.compare(destFullPath, Qt::CaseInsensitive) || destination.getType() != FEATURE_TRASH) {
        FileSystemUtils::copy(sourcePath, destFullPath);
        if (action == Qt::MoveAction) {
            curController->deleteItem(source.getFullPath());
        }
    }

    // 源是文件夹时，模型里所有落在它下面的子条目都要跟着改写路径，否则拖完之后
    // 这些子条目仍指向旧位置。
    if (sourceType == FEATURE_FOLDER) {
        for (int i = 0; i < d->featuresList->count(); i++) {
            Feature &curFeature = (*d->featuresList)[i];

            QString curFeatureFullPath = curFeature.getFullPath().toLocalFile();
            QString curFeatureVirtualPath = curFeature.getVirtualPath();

            if (curFeatureFullPath.contains(sourcePath) && curFeatureFullPath != sourcePath) {
                Feature copyFeature = curFeature;
                QUrl newPath = QUrl::fromLocalFile(curFeatureFullPath.replace(sourcePath, destFullPath));
                QString newVirtualPath = curFeatureVirtualPath.replace(sourceVirtualPath, destVirtualPath);
                // 移入回收站只改虚拟路径：文件本体仍留在磁盘原处。
                if (destination.getType() != FEATURE_TRASH) {
                    // 真实文件系统路径：复制改副本，移动改原条目。
                    if (action == Qt::CopyAction) {
                        copyFeature.setFullPath(newPath);
                    } else {
                        curFeature.setFullPath(newPath);
                    }
                }
                // 虚拟路径同理：复制走新条目，移动就地改。
                if (action == Qt::CopyAction) {
                    copyFeature.setFullVirtualPath(newVirtualPath);
                } else {
                    curFeature.setFullVirtualPath(newVirtualPath);
                }

                if (action == Qt::CopyAction) {
                    addItem(copyFeature);
                }
            }
        }
    }

    Feature newElement(destVirtualPath + "/" + name, sourceIcon, name, QUrl::fromLocalFile(destFullPath),
                       sourceType);
    addItem(newElement);

    if (deleteManualy) {
        deleteItem(source);
    }

    // 此处刻意不发 dataRestructured()：它曾让视图刷新时序变得不可预期（历史上还引发过
    // 崩溃），而 addItem()/deleteItem() 已经逐行通知过模型。若日后出现列表不刷新，
    // 再考虑补发。
}

Qt::ItemFlags FeaturesModel::flags(const QModelIndex &index) const {
    Qt::ItemFlags resultFlags = QAbstractItemModel::flags(index);
    if (index.isValid()) {
        Feature item = index.data(Qt::UserRole + 1).value<Feature>();
        if (item.getType() == FEATURE_INTERACTIVE || item.getType() == FEATURE_ITEM ||
            item.getType() == FEATURE_AUDIO || item.getType() == FEATURE_VIDEO ||
            item.getType() == FEATURE_IMAGE || item.getType() == FEATURE_FLASH ||
            item.getType() == FEATURE_INTERNAL || item.getType() == FEATURE_FOLDER)

            resultFlags |= Qt::ItemIsDragEnabled;

        if (item.isFolder() && !item.getVirtualPath().isNull())
            resultFlags |= Qt::ItemIsDropEnabled;
    }

    return resultFlags;
}


QStringList FeaturesModel::mimeTypes() const {
    QStringList types;
    types << "text/uri-list" << "image/png" << "image/tiff" << "image/gif" << "image/jpeg";
    return types;
}

int FeaturesModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid() || !d->featuresList)
        return 0;
    else
        return d->featuresList->size();
}

// Qt6 的 QRegularExpression 没有 QRegExp::exactMatch 等价物，只能自己判整串命中。
static bool exactMatch(const QRegularExpression &expression, const QString &text) {
    const QRegularExpressionMatch match = expression.match(text);

    return match.hasMatch() && match.capturedStart() == 0 && match.capturedLength() == text.length();
}

bool FeaturesProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    QString path = index.data(Qt::UserRole).toString();

    return exactMatch(filterRegularExpression(), path);
}

FeaturesSearchProxyModel::FeaturesSearchProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent), d(new Private) {
    // 空实现：过滤前缀由 setFilterPrefix() 在搜索时下发，空前缀相当于不限定目录。
}

FeaturesSearchProxyModel::~FeaturesSearchProxyModel() = default;

void FeaturesSearchProxyModel::setFilterPrefix(const QString &newPrefix) {
    d->mFilterPrefix = newPrefix;
}

bool FeaturesSearchProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    Feature feature = sourceModel()->data(index, Qt::UserRole + 1).value<Feature>();
    bool isFile = feature.getType() == FEATURE_INTERACTIVE || feature.getType() == FEATURE_INTERNAL ||
                  feature.getType() == FEATURE_ITEM || feature.getType() == FEATURE_AUDIO ||
                  feature.getType() == FEATURE_VIDEO || feature.getType() == FEATURE_IMAGE;

    return isFile && feature.getFullVirtualPath().contains(d->mFilterPrefix) &&
           exactMatch(filterRegularExpression(), feature.getName());
}

FeaturesPathProxyModel::FeaturesPathProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent), d(new Private) {
    // 空实现：待过滤的虚拟路径由 setPath() 在面包屑点击时下发。
}

FeaturesPathProxyModel::~FeaturesPathProxyModel() = default;

void FeaturesPathProxyModel::setPath(const QString &p) {
    d->path = p;
}

bool FeaturesPathProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    Feature feature = sourceModel()->data(index, Qt::UserRole + 1).value<Feature>();

    // 只显示“当前路径的各级父目录 + 当前这一级”，进了子目录就不该再把兄弟目录列出来
    return (feature.isFolder() && (d->path.startsWith(feature.getFullVirtualPath() + "/") ||
                                   d->path == feature.getFullVirtualPath()));
}

FeaturesItemDelegate::FeaturesItemDelegate(QObject *parent, const QListView *lw)
    : QStyledItemDelegate(parent), d(new Private) {
    d->listView = lw;
}

FeaturesItemDelegate::~FeaturesItemDelegate() = default;

QString FeaturesItemDelegate::displayText(const QVariant &value, const QLocale &locale) const {
    Q_UNUSED(locale)

    QString text = value.toString();
    text = text.replace(".wgt", "");
    text = text.replace(".wgs", "");
    text = text.replace(".swf", "");
    if (d->listView) {
        const QFontMetrics fm = d->listView->fontMetrics();
        const QSize iSize = d->listView->gridSize();
        return fm.elidedText(text, Qt::ElideRight, iSize.width());
    }
    return text;
}

FeaturesPathItemDelegate::FeaturesPathItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent), d(new Private) {
    d->arrowPixmap = new QPixmap(":images/navig_arrow.png");
}

QString FeaturesPathItemDelegate::displayText(const QVariant &value, const QLocale &locale) const {
    Q_UNUSED(value)
    Q_UNUSED(locale)

    return QString();
}

void FeaturesPathItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                                     const QModelIndex &index) const {
    Feature feature = index.data(Qt::UserRole + 1).value<Feature>();
    QRect rect = option.rect;
    if (!feature.getFullPath().isEmpty()) {
        painter->drawPixmap(rect.left() - 10, rect.center().y() - 5, *d->arrowPixmap);
    }
    painter->drawImage(rect.left() + 5, rect.center().y() - 5,
                       feature.getThumbnail().scaledToHeight(30, Qt::SmoothTransformation));
}

FeaturesPathItemDelegate::~FeaturesPathItemDelegate() {
    if (d->arrowPixmap) {
        delete d->arrowPixmap;
        d->arrowPixmap = NULL;
    }
}
