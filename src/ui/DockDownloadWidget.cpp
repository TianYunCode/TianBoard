#include "ui/DockDownloadWidget.h"
#include "core/Application.h"

#include "utils/Globals.h"


DockDownloadWidget::DockDownloadWidget(QWidget *parent, const char *name)
    : DockPaletteWidget(parent, name), mpLayout(NULL), mpDLWidget(NULL) {
    // 标签名与逻辑可见状态；初始不可见，首次开始下载时由调色板管理器显示。
    setName("DownloadWidget");
    setVisibleState(false);

    SET_STYLE_SHEET();

    setIconToLeft(QPixmap(":images/general_open.png"));
    setIconToRight(QPixmap(":images/general_close.png"));

    mpLayout = new QVBoxLayout(this);
    setLayout(mpLayout);

    mpDLWidget = new DownloadWidget(this);
    mpLayout->addWidget(mpDLWidget);
}

DockDownloadWidget::~DockDownloadWidget() {
    if (NULL != mpDLWidget) {
        delete mpDLWidget;
        mpDLWidget = NULL;
    }
    if (NULL != mpLayout) {
        delete mpLayout;
        mpLayout = NULL;
    }
}
