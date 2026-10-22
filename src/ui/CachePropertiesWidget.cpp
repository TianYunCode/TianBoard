#include <QColor>
#include <QPainter>
#include <QPixmap>
#include <QColorDialog>

#include "ui/CachePropertiesWidget.h"

#include "core/Application.h"
#include "core/ApplicationController.h"
#include "utils/Globals.h"
#include "whiteboard/BoardController.h"
#include "whiteboard/GraphicsScene.h"

// 本面板见过的缓存对象；关闭缓存时从这里同步移除，避免留下悬空指针。
static QVector<GraphicsCache *> mCaches;

CachePropertiesWidget::CachePropertiesWidget(QWidget *parent, const char *name)
    : DockPaletteWidget(parent), mpLayout(NULL), mpCachePropertiesLabel(NULL), mpColorLabel(NULL), mpShapeLabel(NULL), mpSizeLabel(NULL),
      mpColor(NULL), mpSquareButton(NULL), mpCircleButton(NULL), mpCloseButton(NULL), mpSizeSlider(NULL), mpColorLayout(NULL), mpShapeLayout(NULL),
      mpSizeLayout(NULL), mpCloseLayout(NULL), mpProperties(NULL), mpPropertiesLayout(NULL), mpCurrentCache(NULL)
{
    setObjectName(name);

    SET_STYLE_SHEET();

    setName("CachePropWidget");
    setVisibleState(false);
    setIconToLeft(QPixmap(":images/general_open.png"));
    setIconToRight(QPixmap(":images/general_close.png"));
    setContentsMargins(10, 10, 10, 10);

    // 搭建界面
    mpLayout = new QVBoxLayout();
    setLayout(mpLayout);

    // 标题
    mpCachePropertiesLabel = new QLabel(tr("Cache Properties"), this);
    mpCachePropertiesLabel->setObjectName("DockPaletteWidgetTitle");
    mpLayout->addWidget(mpCachePropertiesLabel, 0);

    // 属性区容器
    mpProperties = new QWidget(this);
    mpProperties->setObjectName("DockPaletteWidgetBox");
    mpLayout->addWidget(mpProperties, 1);
    mpPropertiesLayout = new QVBoxLayout();
    mpProperties->setLayout(mpPropertiesLayout);

    // 颜色
    mpColorLayout = new QHBoxLayout();
    mpColorLabel  = new QLabel(tr("Color:"), mpProperties);
    mpColor       = new QPushButton(mpProperties);
    mpColor->setObjectName("DockPaletteWidgetButton");
    updateCacheColor(Qt::black);
    mpColorLayout->addWidget(mpColorLabel, 0);
    mpColorLayout->addWidget(mpColor, 0);
    mpColorLayout->addStretch(1);
    mpPropertiesLayout->addLayout(mpColorLayout, 0);

    // 形状
    mpShapeLayout  = new QHBoxLayout();
    mpShapeLabel   = new QLabel(tr("Shape:"), mpProperties);
    mpSquareButton = new QPushButton(mpProperties);
    mpSquareButton->setIcon(QIcon(":images/cache_square.png"));
    mpSquareButton->setObjectName("DockPaletteWidgetButton");
    mpSquareButton->setCheckable(true);
    mpCircleButton = new QPushButton(mpProperties);
    mpCircleButton->setIcon(QIcon(":images/cache_circle.png"));
    mpCircleButton->setObjectName("DockPaletteWidgetButton");
    mpCircleButton->setCheckable(true);
    mpShapeLayout->addWidget(mpShapeLabel, 0);
    mpShapeLayout->addWidget(mpSquareButton, 0);
    mpShapeLayout->addWidget(mpCircleButton, 0);
    mpShapeLayout->addStretch(1);
    mpPropertiesLayout->addLayout(mpShapeLayout, 0);

    mpCircleButton->setChecked(true);

    // 形状尺寸
    mpSizeLayout = new QHBoxLayout();
    mpSizeLabel  = new QLabel(tr("Size:"), mpProperties);
    mpSizeSlider = new QSlider(Qt::Horizontal, mpProperties);
    mpSizeSlider->setMinimumHeight(20);
    mpSizeSlider->setMinimum(50);
    mpSizeSlider->setMaximum(MAX_SHAPE_WIDTH);
    mpSizeLayout->addWidget(mpSizeLabel, 0);
    mpSizeLayout->addWidget(mpSizeSlider, 1);
    mpPropertiesLayout->addLayout(mpSizeLayout, 0);

    // 关闭
    mpCloseLayout = new QHBoxLayout();
    mpCloseButton = new QPushButton(tr("Close"), mpProperties);
    mpCloseButton->setObjectName("DockPaletteWidgetButton");
    mpCloseLayout->addWidget(mpCloseButton, 0);
    mpCloseLayout->addStretch(1);
    mpPropertiesLayout->addLayout(mpCloseLayout, 0);

    // 把剩余空间撑到底部
    mpPropertiesLayout->addStretch(1);

    // 信号与槽
    connect(mpCloseButton, &QPushButton::clicked, this, &CachePropertiesWidget::onCloseClicked);
    connect(mpColor, &QPushButton::clicked, this, &CachePropertiesWidget::onColorClicked);
    connect(mpCircleButton, &QPushButton::clicked, this, &CachePropertiesWidget::updateShapeButtons);
    connect(mpSquareButton, &QPushButton::clicked, this, &CachePropertiesWidget::updateShapeButtons);
    connect(mpSizeSlider, &QSlider::valueChanged, this, &CachePropertiesWidget::onSizeChanged);
    connect(BoardController::instance(), &BoardController::activeSceneChanged, this, &CachePropertiesWidget::updateCurrentCache);
    connect(BoardController::instance(), &BoardController::cacheEnabled, this, &CachePropertiesWidget::onCacheEnabled);
}

CachePropertiesWidget::~CachePropertiesWidget()
{
    if (NULL != mpCachePropertiesLabel)
    {
        delete mpCachePropertiesLabel;
        mpCachePropertiesLabel = NULL;
    }
    if (NULL != mpColorLabel)
    {
        delete mpColorLabel;
        mpColorLabel = NULL;
    }
    if (NULL != mpShapeLabel)
    {
        delete mpShapeLabel;
        mpShapeLabel = NULL;
    }
    if (NULL != mpSizeLabel)
    {
        delete mpSizeLabel;
        mpSizeLabel = NULL;
    }
    if (NULL != mpColor)
    {
        delete mpColor;
        mpColor = NULL;
    }
    if (NULL != mpSquareButton)
    {
        delete mpSquareButton;
        mpSquareButton = NULL;
    }
    if (NULL != mpCircleButton)
    {
        delete mpCircleButton;
        mpCircleButton = NULL;
    }
    if (NULL != mpCloseButton)
    {
        delete mpCloseButton;
        mpCloseButton = NULL;
    }
    if (NULL != mpSizeSlider)
    {
        delete mpSizeSlider;
        mpSizeSlider = NULL;
    }
    if (NULL != mpColorLayout)
    {
        delete mpColorLayout;
        mpColorLayout = NULL;
    }
    if (NULL != mpShapeLayout)
    {
        delete mpShapeLayout;
        mpShapeLayout = NULL;
    }
    if (NULL != mpSizeLayout)
    {
        delete mpSizeLayout;
        mpSizeLayout = NULL;
    }
    if (NULL != mpCloseLayout)
    {
        delete mpCloseLayout;
        mpCloseLayout = NULL;
    }
    if (NULL != mpPropertiesLayout)
    {
        delete mpPropertiesLayout;
        mpPropertiesLayout = NULL;
    }
    if (NULL != mpProperties)
    {
        delete mpProperties;
        mpProperties = NULL;
    }
    if (NULL != mpLayout)
    {
        delete mpLayout;
        mpLayout = NULL;
    }
}

void CachePropertiesWidget::onCloseClicked()
{
    if (!mCaches.empty())
    {
        // 先把缓存从跟踪列表移除，再从场景删除，避免列表残留失效指针。
        mCaches.remove(mCaches.indexOf(mpCurrentCache));

        // 从白板页面移除遮罩
        BoardController::instance()->activeScene()->removeItem(mpCurrentCache);
        mpCurrentCache = NULL;

        emit hideTab(this);
    }
}

void CachePropertiesWidget::updateCacheColor(QColor color)
{
    mActualColor = color;

    // 用纯色方块作为按钮图标，直观显示当前颜色
    QPixmap  pix(32, 32);
    QPainter p;

    p.begin(&pix);

    p.setBackground(Qt::transparent);
    p.setBrush(color); // 画当前颜色
    p.drawRect(0, 0, 32, 32);

    p.end();

    mpColor->setIcon(QIcon(pix));

    if (NULL != mpCurrentCache)
    {
        mpCurrentCache->setMaskColor(mActualColor);
    }
}

void CachePropertiesWidget::onColorClicked()
{
    // 打开系统取色对话框，取消时返回无效颜色；updateCacheColor 仍会应用它。
    QColor newColor = QColorDialog::getColor(mActualColor, this);
    updateCacheColor(newColor);
}

void CachePropertiesWidget::updateShapeButtons()
{
    if (mpCircleButton->hasFocus())
    {
        mActualShape = eMaskShape_Circle;
        mpSquareButton->setChecked(false);
    }
    else if (mpSquareButton->hasFocus())
    {
        mActualShape = eMaskShap_Rectangle;
        mpCircleButton->setChecked(false);
    }

    if (NULL != mpCurrentCache)
    {
        mpCurrentCache->setMaskShape(mActualShape);
    }
}

void CachePropertiesWidget::updateCurrentCache()
{
    bool isBoardMode = false;
    // 本面板只在 Board 模式有意义
    if (Application::applicationController != NULL)
    {
        // 有应用控制器时，只有处于 Board 模式且没有进入“显示桌面”才继续处理
        if (Application::applicationController->displayMode() == ApplicationController::Board &&
            !Application::applicationController->isShowingDesktop())
        {
            isBoardMode = true;
        }
    }
    // 没有应用控制器时无法判断当前模式，按 Board 处理，保持旧行为
    else
    {
        isBoardMode = true;
    }

    if (isBoardMode)
    {
        // 取当前页上的缓存条目
        QList<QGraphicsItem *> items = BoardController::instance()->activeScene()->items();
        foreach (QGraphicsItem *it, items)
        {
            if ("Cache" == it->data(Qt::UserRole).toString())
            {
                setEnabled(true);
                emit showTab(this);
                mpCurrentCache = dynamic_cast<GraphicsCache *>(it);
                if (!mpCurrentCache)
                {
                    // 带了 "Cache" 标记但不是 GraphicsCache 的条目：不能继续往下解引用。
                    continue;
                }
                if (!mCaches.contains(mpCurrentCache))
                {
                    mCaches.append(mpCurrentCache);
                }

                // 回填缓存的当前属性
                mpSizeSlider->setValue(mpCurrentCache->shapeWidth());
                updateCacheColor(mpCurrentCache->maskColor());
                switch (mpCurrentCache->maskshape())
                {
                    case eMaskShape_Circle:
                        mpCircleButton->setChecked(true);
                        mpSquareButton->setChecked(false);
                        break;
                    case eMaskShap_Rectangle:
                        mpCircleButton->setChecked(false);
                        mpSquareButton->setChecked(true);
                        break;
                }

                return;
            }
        }
    }

    // 走到这里说明：本页没有缓存、当前不是 Board 模式，或处于“显示桌面”状态。
    emit hideTab(this);
    mpCurrentCache = NULL;
    setDisabled(true);
}

void CachePropertiesWidget::onSizeChanged(int newSize)
{
    if (NULL != mpCurrentCache)
    {
        mpCurrentCache->setShapeWidth(newSize);
    }
}

void CachePropertiesWidget::onCacheEnabled()
{
    emit showTab(this);
}
