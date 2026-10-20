#include <QButtonGroup>
#include <QAction>

#include "ui/FeaturesActionBar.h"
#include "ui/FeaturesWidget.h"

struct FeaturesActionBar::Private {
    FeaturesController *featuresController{nullptr};
    FeaturesActionBarState currentState{IN_ROOT};
    eButtonSet mCrntButtonSet{eButtonSet_Default};
    eButtonSet mPreviousButtonSet{eButtonSet_Default};
    QButtonGroup *mButtonGroup{nullptr};
    QLineEdit *mSearchBar{nullptr};
    QHBoxLayout *mLayout{nullptr};
    QAction *mpFavoriteAction{nullptr};
    QAction *mpSocialAction{nullptr};
    QAction *mpRescanModelAction{nullptr};
    QAction *mpDeleteAction{nullptr};
    QAction *mpSearchAction{nullptr};
    QAction *mpCloseAction{nullptr};
    QAction *mpRemoveFavorite{nullptr};
    QAction *mpNewFolderAction{nullptr};
    ActionButton *mpFavoriteBtn{nullptr};
    ActionButton *mpSocialBtn{nullptr};
    ActionButton *mpRescanModelBtn{nullptr};
    ActionButton *mpDeleteBtn{nullptr};
    ActionButton *mpCloseBtn{nullptr};
    ActionButton *mpRemoveFavoriteBtn{nullptr};
    ActionButton *mpNewFolderBtn{nullptr};
};

FeaturesActionBar::FeaturesActionBar(FeaturesController *controller, QWidget *parent, const char *name)
    : QWidget(parent), d(new Private) {
    d->featuresController = controller;

    setObjectName(name);
    setStyleSheet(QString("background: #EEEEEE; border-radius : 10px; border : 2px solid #999999;"));

    setAcceptDrops(true);

    d->mButtonGroup = new QButtonGroup(this);
    d->mSearchBar = new QLineEdit(this);
    d->mSearchBar->setStyleSheet(QString("background-color:white; border-radius : 10px; padding : 2px;"));

    d->mLayout = new QHBoxLayout();
    setLayout(d->mLayout);

    setMaximumHeight(ACTIONBAR_HEIGHT);

    // 创建动作
    d->mpFavoriteAction =
        new QAction(QIcon(":/images/libpalette/miniFavorite.png"), tr("Add to favorites"), this);
    d->mpSocialAction = new QAction(QIcon(":/images/libpalette/social.png"), tr("Share"), this);
    d->mpSearchAction = new QAction(QIcon(":/images/libpalette/miniSearch.png"), tr("Search"), this);
    d->mpRescanModelAction =
        new QAction(QIcon(":/images/cursors/rotate.png"), tr("Rescan file system"), this);
    d->mpDeleteAction = new QAction(QIcon(":/images/libpalette/miniTrash.png"), tr("Delete"), this);
    d->mpCloseAction = new QAction(QIcon(":/images/close.svg"), tr("Back to folder"), this);
    d->mpRemoveFavorite =
        new QAction(QIcon(":/images/libpalette/trash_favorite.svg"), tr("Remove from favorites"), this);
    d->mpNewFolderAction =
        new QAction(QIcon(":/images/libpalette/miniNewFolder.png"), tr("Create new folder"), this);

    // 创建按钮
    d->mpFavoriteBtn = new ActionButton(this, d->mpFavoriteAction);
    d->mpSocialBtn = new ActionButton(this, d->mpSocialAction);

    d->mpRescanModelBtn = new ActionButton(this, d->mpRescanModelAction);
    // 重新扫描入口暂不展示，仅保留对象。
    d->mpRescanModelBtn->hide();

    d->mpDeleteBtn = new ActionButton(this, d->mpDeleteAction);
    d->mpCloseBtn = new ActionButton(this, d->mpCloseAction);
    d->mpRemoveFavoriteBtn = new ActionButton(this, d->mpRemoveFavorite);
    d->mpNewFolderBtn = new ActionButton(this, d->mpNewFolderAction);

    // 初始状态下不允许新建文件夹，进入文件夹后由 setButtons() 打开。
    d->mpNewFolderBtn->setEnabled(false);

    // 加入按钮组，便于统一管理按钮状态
    d->mButtonGroup->addButton(d->mpFavoriteBtn);
    d->mButtonGroup->addButton(d->mpSocialBtn);
    d->mButtonGroup->addButton(d->mpDeleteBtn);
    d->mButtonGroup->addButton(d->mpCloseBtn);
    d->mButtonGroup->addButton(d->mpRemoveFavoriteBtn);
    d->mButtonGroup->addButton(d->mpNewFolderBtn);

    connect(d->mpFavoriteAction, &QAction::triggered, this, &FeaturesActionBar::onActionFavorite);
    connect(d->mSearchBar, &QLineEdit::textChanged, this, &FeaturesActionBar::onSearchTextChanged);
    connect(d->mpNewFolderAction, &QAction::triggered, this, &FeaturesActionBar::onActionNewFolder);
    connect(d->mpRemoveFavorite, &QAction::triggered, this, &FeaturesActionBar::onActionRemoveFavorite);
    connect(d->mpRescanModelAction, &QAction::triggered, this, &FeaturesActionBar::onActionRescanModel);
    connect(d->mpDeleteAction, &QAction::triggered, this, &FeaturesActionBar::onActionTrash);


    // 组装默认动作条；搜索框代替了独立的搜索按钮。
    d->mLayout->addWidget(d->mpFavoriteBtn);
    d->mLayout->addWidget(d->mpSocialBtn);
    d->mLayout->addWidget(d->mpNewFolderBtn);
    d->mLayout->addWidget(d->mSearchBar);
    d->mLayout->addWidget(d->mpRescanModelBtn);
    d->mLayout->addWidget(d->mpDeleteBtn);
    d->mLayout->addWidget(d->mpCloseBtn);
    d->mLayout->addWidget(d->mpRemoveFavoriteBtn);
    setCurrentState(IN_ROOT);
    d->mpDeleteBtn->setAcceptDrops(true);
    setAcceptDrops(true);
}

void FeaturesActionBar::setCurrentState(FeaturesActionBarState state) {
    d->currentState = state;
    setButtons();
}

void FeaturesActionBar::cleanText() {
    d->mSearchBar->clear();
}

void FeaturesActionBar::setButtons() {
    // 各状态下按钮的显隐与可用性差异集中在这里维护。
    switch (d->currentState) {
        case IN_FOLDER:
            d->mpFavoriteBtn->show();
            d->mpSocialBtn->hide();
            d->mSearchBar->show();
            d->mpDeleteBtn->show();
            d->mpCloseBtn->hide();
            d->mpRemoveFavoriteBtn->hide();
            d->mpNewFolderBtn->show();
            d->mpNewFolderBtn->setEnabled(true);
            d->mpDeleteBtn->setEnabled(true);
            break;
        case IN_ROOT:
            d->mpFavoriteBtn->show();
            d->mpSocialBtn->hide();
            d->mSearchBar->show();
            d->mpDeleteBtn->show();
            d->mpCloseBtn->hide();
            d->mpRemoveFavoriteBtn->hide();
            d->mpNewFolderBtn->show();
            d->mpNewFolderBtn->setEnabled(false);
            d->mpDeleteBtn->setEnabled(false);
            break;
        case IN_PROPERTIES:
            d->mpFavoriteBtn->show();
            d->mpSocialBtn->hide();
            d->mSearchBar->show();
            d->mpDeleteBtn->hide();
            d->mpCloseBtn->hide();
            d->mpRemoveFavoriteBtn->hide();
            d->mpNewFolderBtn->hide();
            break;
        case IN_FAVORITE:
            d->mpFavoriteBtn->hide();
            d->mpSocialBtn->hide();
            d->mSearchBar->show();
            d->mpDeleteBtn->hide();
            d->mpCloseBtn->hide();
            d->mpRemoveFavoriteBtn->show();
            d->mpNewFolderBtn->hide();
            break;
        case IN_TRASH:
            d->mpFavoriteBtn->hide();
            d->mpSocialBtn->hide();
            d->mSearchBar->show();
            d->mpDeleteBtn->show();
            d->mpDeleteBtn->setEnabled(true);
            d->mpCloseBtn->hide();
            d->mpNewFolderBtn->hide();
            break;
        default:
            break;
    }
}

void FeaturesActionBar::onSearchTextChanged(QString txt) {
    Q_UNUSED(txt)
    emit searchElement(d->mSearchBar->text());
}

void FeaturesActionBar::onActionNewFolder() {
    emit newFolderToCreate();
}

void FeaturesActionBar::onActionFavorite() {
    emit addElementsToFavorite();
}

void FeaturesActionBar::onActionRemoveFavorite() {
    emit removeElementsFromFavorite();
}

void FeaturesActionBar::onActionTrash() {
    emit deleteSelectedElements();
}
void FeaturesActionBar::onActionRescanModel() {
    emit rescanModel();
}

void FeaturesActionBar::lockIt() {
    setEnabled(false);
}

void FeaturesActionBar::unlockIt() {
    setEnabled(true);
}

void FeaturesActionBar::dragEnterEvent(QDragEnterEvent *event) {
    // 只接受资源库自己的拖放数据，外部文件交给其他控件处理。
    const FeaturesMimeData *fMimeData = qobject_cast<const FeaturesMimeData *>(event->mimeData());
    if (fMimeData) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void FeaturesActionBar::dropEvent(QDropEvent *event) {
    const FeaturesMimeData *fMimeData = qobject_cast<const FeaturesMimeData *>(event->mimeData());

    if (!fMimeData) {
        qWarning() << "data came from not supported widget";
        event->ignore();
        return;
    }

    QWidget *dest = childAt(event->pos());
    if (dest == d->mpDeleteBtn) {
        // 删除前先整体校验：只要有一个条目不可删除就放弃整个操作，避免半完成状态。
        QList<Feature> featuresList = fMimeData->features();
        foreach (Feature curFeature, featuresList) {
            if (!curFeature.isDeletable()) {
                qWarning() << "Undeletable feature found, stopping deleting process";
                event->ignore();
                return;
            }
        }
        event->setDropAction(Qt::MoveAction);
        event->accept();

        emit deleteElements(fMimeData);

    } else if (dest == d->mpFavoriteBtn) {
        event->setDropAction(Qt::CopyAction);
        event->accept();

        emit addToFavorite(fMimeData);

    } else if (dest == d->mpRemoveFavoriteBtn) {
        event->setDropAction(Qt::MoveAction);
        event->accept();

        emit removeFromFavorite(fMimeData);
    }
}

FeaturesActionBar::~FeaturesActionBar() {}

ActionButton::ActionButton(QWidget *parent, QAction *action, const char *name) : QToolButton(parent) {
    setObjectName(name);
    addAction(action);
    setDefaultAction(action);
    setIconSize(QSize(BUTTON_SIZE, BUTTON_SIZE));
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setStyleSheet(QString("QToolButton {color: white; font-weight: bold; font-family: Arial; "
                          "background-color: transparent; border: none}"));
    setFocusPolicy(Qt::NoFocus);
}

ActionButton::~ActionButton() {}
