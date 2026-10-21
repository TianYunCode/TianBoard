#include <QtWidgets>

#include "ui/DocumentsWidget.h"

#include "ui/DocumentController.h"
#include "ui/DocumentThumbnailWidget.h"

struct DocumentsWidget::Private
{
    QGridLayout             *gridLayout{nullptr};
    QWidget                 *topWidget{nullptr};
    QHBoxLayout             *horizontalLayout{nullptr};
    QSplitter               *splitter{nullptr};
    QWidget                 *topLeftWidget{nullptr};
    QVBoxLayout             *verticalLayout_2{nullptr};
    QHBoxLayout             *horizontalLayout_3{nullptr};
    QComboBox               *sortKind{nullptr};
    QSpacerItem             *horizontalSpacer{nullptr};
    QToolButton             *sortOrder{nullptr};
    QSpacerItem             *horizontalSpacer_2{nullptr};
    DocumentTreeView        *documentTreeView{nullptr};
    QFrame                  *topRightFrame{nullptr};
    QVBoxLayout             *verticalLayout_3{nullptr};
    DocumentThumbnailWidget *thumbnailWidget{nullptr};
    QWidget                 *bottomRightFrame{nullptr};
    QHBoxLayout             *horizontalLayout_2{nullptr};
    QSpacerItem             *horizontalSpacer2{nullptr};
    QSlider                 *documentZoomSlider{nullptr};
    QSpacerItem             *horizontalSpacer1{nullptr};
};

DocumentsWidget::DocumentsWidget(QWidget *parent) : QWidget(parent), d(new Private)
{
    if (objectName().isEmpty())
    {
        setObjectName("documents");
    }
    setEnabled(true);
    resize(796, 646);

    QSizePolicy sizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(this->sizePolicy().hasHeightForWidth());
    setSizePolicy(sizePolicy);
    setContextMenuPolicy(Qt::DefaultContextMenu);
    setStyleSheet(QString::fromUtf8(""));

    d->gridLayout = new QGridLayout(this);
    d->gridLayout->setSpacing(0);
    d->gridLayout->setContentsMargins(0, 0, 0, 0);
    d->gridLayout->setObjectName("gridLayout");

    d->topWidget = new QWidget(this);
    d->topWidget->setObjectName("topWidget");
    d->horizontalLayout = new QHBoxLayout(d->topWidget);
    d->horizontalLayout->setSpacing(0);
    d->horizontalLayout->setContentsMargins(0, 0, 0, 0);
    d->horizontalLayout->setObjectName("horizontalLayout");

    d->splitter = new QSplitter(d->topWidget);
    d->splitter->setObjectName("splitter");
    d->splitter->setMinimumSize(QSize(400, 0));
    d->splitter->setOrientation(Qt::Horizontal);
    d->splitter->setOpaqueResize(true);
    d->splitter->setHandleWidth(6);
    d->splitter->setChildrenCollapsible(false);

    d->topLeftWidget = new QWidget(d->splitter);
    d->topLeftWidget->setObjectName("topLeftWidget");
    QSizePolicy sizePolicy1(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(d->topLeftWidget->sizePolicy().hasHeightForWidth());
    d->topLeftWidget->setSizePolicy(sizePolicy1);
    d->verticalLayout_2 = new QVBoxLayout(d->topLeftWidget);
    d->verticalLayout_2->setSpacing(0);
    d->verticalLayout_2->setContentsMargins(0, 0, 0, 0);
    d->verticalLayout_2->setObjectName("verticalLayout_2");

    d->horizontalLayout_3 = new QHBoxLayout();
    d->horizontalLayout_3->setSpacing(0);
    d->horizontalLayout_3->setObjectName("horizontalLayout_3");
    d->horizontalLayout_3->setSizeConstraint(QLayout::SetMinimumSize);

    d->sortKind = new QComboBox(d->topLeftWidget);
    d->sortKind->addItem(QString());
    d->sortKind->addItem(QString());
    d->sortKind->addItem(QString());
    d->sortKind->setObjectName("sortKind");
    QSizePolicy sizePolicy2(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);
    sizePolicy2.setHorizontalStretch(4);
    sizePolicy2.setVerticalStretch(0);
    sizePolicy2.setHeightForWidth(d->sortKind->sizePolicy().hasHeightForWidth());
    d->sortKind->setSizePolicy(sizePolicy2);
    d->sortKind->setMaximumSize(QSize(200, 16777215));
    d->sortKind->setStyleSheet(QString::fromUtf8(""));
    d->sortKind->setMinimumContentsLength(0);
    d->horizontalLayout_3->addWidget(d->sortKind);

    d->horizontalSpacer = new QSpacerItem(10, 20, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);
    d->horizontalLayout_3->addItem(d->horizontalSpacer);

    d->sortOrder = new QToolButton(d->topLeftWidget);
    d->sortOrder->setObjectName("sortOrder");
    QSizePolicy sizePolicy3(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
    sizePolicy3.setHorizontalStretch(0);
    sizePolicy3.setVerticalStretch(0);
    sizePolicy3.setHeightForWidth(d->sortOrder->sizePolicy().hasHeightForWidth());
    d->sortOrder->setSizePolicy(sizePolicy3);
    d->sortOrder->setStyleSheet(QString::fromUtf8("QToolButton { border-style:none; border-width: 0px;margin-left:2px;margin-right:2px}"));
    d->sortOrder->setText(QString::fromUtf8(""));
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/images/asc.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    icon.addFile(QString::fromUtf8(":/images/desc.png"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
    d->sortOrder->setIcon(icon);
    d->sortOrder->setCheckable(true);
    d->horizontalLayout_3->addWidget(d->sortOrder);

    d->horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);
    d->horizontalLayout_3->addItem(d->horizontalSpacer_2);

    d->verticalLayout_2->addLayout(d->horizontalLayout_3);

    d->documentTreeView = new DocumentTreeView(d->topLeftWidget);
    d->documentTreeView->setObjectName("documentTreeView");
    d->documentTreeView->setMinimumSize(QSize(0, 0));
    d->documentTreeView->setMaximumSize(QSize(16777215, 16777215));
    d->documentTreeView->setFrameShape(QFrame::NoFrame);
    d->documentTreeView->setFrameShadow(QFrame::Plain);
    d->documentTreeView->setAutoScroll(true);
    d->documentTreeView->setAutoScrollMargin(65);
    d->documentTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    d->documentTreeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    d->documentTreeView->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    d->documentTreeView->setAnimated(true);
    d->documentTreeView->setHeaderHidden(true);
    d->documentTreeView->header()->setVisible(false);
    d->verticalLayout_2->addWidget(d->documentTreeView);

    d->splitter->addWidget(d->topLeftWidget);

    d->topRightFrame = new QFrame(d->splitter);
    d->topRightFrame->setObjectName("topRightFrame");
    d->topRightFrame->setStyleSheet(QString::fromUtf8(""));
    d->verticalLayout_3 = new QVBoxLayout(d->topRightFrame);
    d->verticalLayout_3->setSpacing(0);
    d->verticalLayout_3->setContentsMargins(0, 0, 0, 0);
    d->verticalLayout_3->setObjectName("verticalLayout_3");

    d->thumbnailWidget = new DocumentThumbnailWidget(d->topRightFrame);
    d->thumbnailWidget->setObjectName("thumbnailWidget");
    d->thumbnailWidget->setContextMenuPolicy(Qt::NoContextMenu);
    d->thumbnailWidget->setStyleSheet(QString::fromUtf8(""));
    d->thumbnailWidget->setFrameShape(QFrame::NoFrame);
    d->thumbnailWidget->setFrameShadow(QFrame::Plain);
    d->thumbnailWidget->setLineWidth(1);
    d->thumbnailWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    d->thumbnailWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->thumbnailWidget->setDragMode(QGraphicsView::NoDrag);
    d->verticalLayout_3->addWidget(d->thumbnailWidget);

    d->bottomRightFrame = new QWidget(d->topRightFrame);
    d->bottomRightFrame->setObjectName("bottomRightFrame");
    d->horizontalLayout_2 = new QHBoxLayout(d->bottomRightFrame);
    d->horizontalLayout_2->setSpacing(0);
    d->horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
    d->horizontalLayout_2->setObjectName("horizontalLayout_2");
    // 表单本身就重复设置过一次；保留第二次，使生成后的几何行为与旧界面完全一致。
    d->horizontalLayout_2->setContentsMargins(0, 0, 0, 0);

    d->horizontalSpacer2 = new QSpacerItem(300, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);
    d->horizontalLayout_2->addItem(d->horizontalSpacer2);

    d->documentZoomSlider = new QSlider(d->bottomRightFrame);
    d->documentZoomSlider->setObjectName("documentZoomSlider");
    // 这里复用 sortOrder 的尺寸策略对象，只刷新 heightForWidth 标志，与 uic 生成代码一致。
    sizePolicy3.setHeightForWidth(d->documentZoomSlider->sizePolicy().hasHeightForWidth());
    d->documentZoomSlider->setSizePolicy(sizePolicy3);
    d->documentZoomSlider->setMinimumSize(QSize(150, 0));
    d->documentZoomSlider->setStyleSheet(QString::fromUtf8(""));
    d->documentZoomSlider->setMinimum(50);
    d->documentZoomSlider->setMaximum(500);
    d->documentZoomSlider->setSingleStep(10);
    d->documentZoomSlider->setValue(150);
    d->documentZoomSlider->setOrientation(Qt::Horizontal);
    d->documentZoomSlider->setInvertedAppearance(false);
    d->documentZoomSlider->setInvertedControls(false);
    d->horizontalLayout_2->addWidget(d->documentZoomSlider);

    d->horizontalSpacer1 = new QSpacerItem(40, 20, QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Minimum);
    d->horizontalLayout_2->addItem(d->horizontalSpacer1);

    d->verticalLayout_3->addWidget(d->bottomRightFrame);

    d->splitter->addWidget(d->topRightFrame);

    d->horizontalLayout->addWidget(d->splitter);

    d->gridLayout->addWidget(d->topWidget, 0, 0, 1, 1);

    setWindowTitle(tr("TianBoard Documents"));
    d->sortKind->setItemText(0, tr("Creation date"));
    d->sortKind->setItemText(1, tr("Update date"));
    d->sortKind->setItemText(2, tr("Alphabetical order"));
#if QT_CONFIG(tooltip)
    d->sortOrder->setToolTip(tr("Sort Order"));
#endif
}

DocumentsWidget::~DocumentsWidget() = default;

DocumentTreeView *DocumentsWidget::documentTreeView() const
{
    return d->documentTreeView;
}

DocumentThumbnailWidget *DocumentsWidget::thumbnailWidget() const
{
    return d->thumbnailWidget;
}

QComboBox *DocumentsWidget::sortKind() const
{
    return d->sortKind;
}

QToolButton *DocumentsWidget::sortOrder() const
{
    return d->sortOrder;
}

QSplitter *DocumentsWidget::splitter() const
{
    return d->splitter;
}

QSlider *DocumentsWidget::documentZoomSlider() const
{
    return d->documentZoomSlider;
}
