#include <QPoint>
#include <QPointF>
#include <QPainterPath>

#include "ui/DockPalette.h"

#include "settings/Settings.h"
#include "ui/PreferencesController.h"
#include "core/DownloadManager.h"

#include "whiteboard/BoardController.h"

constexpr int kTabSize            = 50;
// 按下与松开间隔小于此值才算“点击”。实际取值极大，等于不设时限：
// 只要没有触发拖拽改尺寸，松开就按点击处理。
constexpr int kMaxClickDurationMs = 1000000;

struct TabDockPalette::Private
{
    DockPalette *dock{nullptr};
    int          mVerticalOffset{0};
    bool         mFlotable{false};
};

struct DockPalette::Private
{
    /** 当前界面模式 */
    DockWidgetMode mCurrentMode{DockWidgetMode::Board};
    /** 当前停靠方向 */
    DockOrientation mOrientation{DockOrientation::Left};
    /** 当前背景画刷 */
    QBrush mBackgroundBrush;
    /** 首选宽度 */
    int mPreferredWidth{1000};
    /** 首选高度 */
    int mPreferredHeight{1000};
    /** 是否允许拖拽改宽 */
    bool mCanResize{false};
    /** 一次按下到松开之间是否发生过改宽，用于区分点击与拖拽 */
    bool mResized{false};
    /** 触发折叠的宽度阈值 */
    int mCollapseWidth{1500};
    /** 最近一次非折叠宽度 */
    int mLastWidth{-1};
    /** 按下时刻，用于点击时长判断 */
    QElapsedTimer mClickTime;
    /** 按下位置 */
    QPoint mMousePressPos;
    /** 标签条上下朝向 */
    DockTabOrientation mTabsOrientation{DockTabOrientation::Up};
    /** 标签条的水平位置 */
    int mHTab{0};
    /** 承载页面的堆叠控件 */
    QStackedWidget *mpStackWidget{nullptr};
    /** 主布局 */
    QVBoxLayout *mpLayout{nullptr};
    /** 当前标签下标 */
    int mCurrentTab{0};
    /** 当前已加入标签的页面 */
    QVector<DockPaletteWidget *> mTabWidgets;
    /** 所有已注册页面，含当前隐藏的 */
    QVector<DockPaletteWidget *> mRegisteredWidgets;
    /** 当前标签页名称，下载结束后据此恢复页面 */
    QString mCrntTabWidget;
    /** 各模式下最后打开的标签下标 */
    QMap<DockWidgetMode, int> mLastOpenedTabForMode;

    DockPaletteType mPaletteType{DockPaletteType::Left};
    TabDockPalette *mTabPalette{nullptr};
};

DockPalette::DockPalette(DockPaletteType paletteType, QWidget *parent, const char *name)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint), d(new Private)
{
    d->mPaletteType = paletteType;
    d->mTabPalette  = new TabDockPalette(this, parent);

    setObjectName(name);

    d->mpLayout = new QVBoxLayout();
    setLayout(d->mpLayout);

    d->mpStackWidget = new QStackedWidget(this);
    d->mpLayout->addWidget(d->mpStackWidget);

    // 标签列表从空开始，由 addTab()/switchMode() 逐步填充。
    d->mTabWidgets.clear();

    // 最小宽度设为 0，折叠时可以完全收进边缘。
    setMinimumWidth(0);

    if (parent)
    {
        // 作为容器子控件时不把鼠标事件继续传给白板，否则拖动标签会误触画布。
        setAttribute(Qt::WA_NoMousePropagation);
        setAttribute(Qt::WA_TranslucentBackground);
    }
    else
    {
        // 无父控件时作为独立窗口，同样需要透明背景来画出圆角。
        setAttribute(Qt::WA_TranslucentBackground);
    }

    d->mBackgroundBrush = QBrush(Settings::paletteColor);

    // 只能通过样式表把顶层控件背景设为透明，否则会盖住圆角外的区域。
    setStyleSheet("QWidget {background-color: transparent}");

    // 先按工具栏当前位置确定标签朝上还是朝下。
    onToolbarPosUpdated();
    connect(Settings::settings()->appToolBarPositionedAtTop, &Setting::changed, this, &DockPalette::onToolbarPosUpdated);
    connect(DownloadManager::downloadManager(), &DownloadManager::allDownloadsFinished, this, &DockPalette::onAllDownloadsFinished);

    connect(BoardController::instance(), &DocumentContainer::documentSet, this, &DockPalette::onDocumentSet);
    connect(this, &DockPalette::pageSelectionChangedRequired, BoardController::instance(), &BoardController::selectionChanged);
}

DockPalette::~DockPalette()
{
    if (NULL != d->mpStackWidget)
    {
        delete d->mpStackWidget;
        d->mpStackWidget = NULL;
    }
    if (NULL != d->mpLayout)
    {
        delete d->mpLayout;
        d->mpLayout = NULL;
    }
}

DockWidgetMode DockPalette::currentMode()
{
    return d->mCurrentMode;
}

int DockPalette::lastWidth()
{
    return d->mLastWidth;
}

void DockPalette::setLastWidth(int width)
{
    d->mLastWidth = width;
}

void DockPalette::setCollapseWidth(int width)
{
    d->mCollapseWidth = width;
}

bool DockPalette::canResize()
{
    return d->mCanResize;
}

void DockPalette::setLastOpenedTabForMode(DockWidgetMode mode, int tabIndex)
{
    d->mLastOpenedTabForMode.insert(mode, tabIndex);
}

QVector<DockPaletteWidget *> DockPalette::GetWidgetsList()
{
    return d->mRegisteredWidgets;
}

bool DockPalette::isTabFlotable()
{
    return d->mTabPalette->d->mFlotable;
}

void DockPalette::setTabFlotable(bool newFlotable)
{
    d->mTabPalette->d->mFlotable = newFlotable;
}

int DockPalette::getAdditionalVOffset() const
{
    return d->mTabPalette->d->mVerticalOffset;
}

void DockPalette::setAdditionalVOffset(int newOffset)
{
    d->mTabPalette->d->mVerticalOffset = newOffset;
}

DockPaletteType DockPalette::paletteType()
{
    return d->mPaletteType;
}

void DockPalette::onDocumentSet(DocumentProxy *documentProxy)
{
    Q_UNUSED(documentProxy);
}

DockOrientation DockPalette::orientation()
{
    return d->mOrientation;
}

void DockPalette::setOrientation(DockOrientation orientation)
{
    d->mOrientation = orientation;

    // 左右停靠时高度跟随容器，上下停靠时宽度跟随容器。
    if (orientation == DockOrientation::Left || orientation == DockOrientation::Right)
    {
        setMaximumHeight(parentWidget()->height());
        setMinimumHeight(maximumHeight());
    }
    else if (orientation == DockOrientation::Top || orientation == DockOrientation::Bottom)
    {
        setMaximumWidth(parentWidget()->width());
        setMinimumWidth(maximumWidth());
    }
}

void DockPalette::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    updateMaxWidth();
    if (parentWidget())
    {
        setMinimumHeight(parentWidget()->height());
    }
    // 左右停靠的面板始终贴在容器对应的边缘。
    QPoint origin;
    switch (d->mOrientation)
    {
        case DockOrientation::Right:
            origin.setX(parentWidget()->width() - this->width());
            origin.setY(0);
            break;
        case DockOrientation::Bottom:
        case DockOrientation::Top:
        case DockOrientation::Left:
        default:
            origin.setX(0);
            origin.setY(0);
            break;
    }
    move(origin.x(), origin.y());
    moveTabs();
}

void DockPalette::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
    // 进入面板时强制恢复箭头光标，可能刚从画布的十字光标移入。
    setCursor(Qt::ArrowCursor);
    emit mouseEntered();
}

void DockPalette::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    // 离开后取消光标覆盖，让画布重新决定光标形状。
    unsetCursor();
}

void DockPalette::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(d->mBackgroundBrush);
    QPainterPath path;
    path.setFillRule(Qt::WindingFill);

    int nbTabs = d->mTabWidgets.size();
    if (0 < nbTabs)
    {
        // 没有标签时不画背景，避免折叠后残留一条色带。
        if (d->mOrientation == DockOrientation::Left)
        {
            path.addRect(0.0, 0.0, width(), height());
        }
        else if (d->mOrientation == DockOrientation::Right)
        {
            path.addRect(0.0, 0.0, width(), height());
        }

        painter.drawPath(path);
    }
}

void DockPalette::setBackgroundBrush(const QBrush &brush)
{
    if (d->mBackgroundBrush != brush)
    {
        d->mBackgroundBrush = brush;
        update();
    }
}

int DockPalette::border()
{
    return 10;
}

int DockPalette::radius()
{
    return 5;
}

void DockPalette::updateMaxWidth()
{
    // 只有子类需要限制最大宽度时才覆盖这个方法。
}

int DockPalette::collapseWidth()
{
    return d->mCollapseWidth;
}

void DockPalette::tabClicked(int tabIndex)
{
    // 点击非当前标签则切换页面；再次点击当前标签则折叠或展开。
    if (d->mCurrentTab != tabIndex)
    {
        showTabWidget(tabIndex);
    }
    else
    {
        toggleCollapseExpand();
    }
    d->mTabPalette->update();
    emit pageSelectionChangedRequired();
}

void DockPalette::showTabWidget(int tabIndex)
{
    d->mpStackWidget->setCurrentIndex(tabIndex);
    d->mCurrentTab = tabIndex;

    // 记住当前页面名称，下载结束后用它判断是否恢复同一页。
    if (NULL != (dynamic_cast<DockPaletteWidget *>(d->mpStackWidget->widget(tabIndex))))
    {
        d->mCrntTabWidget = dynamic_cast<DockPaletteWidget *>(d->mpStackWidget->widget(tabIndex))->name();
    }
}

void DockPalette::toggleCollapseExpand()
{
    // 当前宽度小于阈值视为折叠状态，恢复上次宽度；否则记住宽度并收起到 0。
    if (width() < d->mCollapseWidth)
    {
        resize(d->mLastWidth, height());
    }
    else
    {
        d->mLastWidth = width();
        update();
        resize(0, height());
    }
}

void DockPalette::setTabsOrientation(DockTabOrientation orientation)
{
    d->mTabsOrientation = orientation;
}

void DockPalette::onToolbarPosUpdated()
{
    // 工具栏在顶部时标签从上方排列，在底部时从下方排列，避免被工具栏遮住。
    if (Settings::settings()->appToolBarPositionedAtTop->get().toBool())
    {
        setTabsOrientation(DockTabOrientation::Up);
    }
    else
    {
        setTabsOrientation(DockTabOrientation::Down);
    }
    moveTabs();
    update();
}

int DockPalette::customMargin()
{
    return 5;
}

void DockPalette::addTab(DockPaletteWidget *widget)
{
    // 只接收已注册且逻辑可见的页面，避免重复加入或误显示。
    if (!d->mTabWidgets.contains(widget) && widget->visibleState())
    {
        widget->setVisible(true);
        d->mTabWidgets.append(widget);
        d->mpStackWidget->addWidget(widget);
        d->mpStackWidget->setCurrentWidget(widget);
        resizeTabs();
        update();
    }
}

void DockPalette::removeTab(DockPaletteWidget *widget)
{
    int nWidget = d->mTabWidgets.indexOf(widget);
    if (nWidget >= 0)
    {
        d->mpStackWidget->removeWidget(widget);
        d->mTabWidgets.remove(nWidget);
        widget->hide();
        update();
    }
    resizeTabs();
    // 移除的页面可能位于当前页之前，下标至少回退一格且不能为负。
    d->mCurrentTab = qMax(d->mCurrentTab - 1, 0);
}

void DockPalette::onResizeRequest(QResizeEvent *event)
{
    resizeEvent(event);
}

int DockPalette::tabSpacing()
{
    return 2;
}

void DockPalette::onShowTabWidget(DockPaletteWidget *widget)
{
    if (d->mRegisteredWidgets.contains(widget))
    {
        widget->setVisibleState(true);
        addTab(widget);
        moveTabs();
    }
}

void DockPalette::onHideTabWidget(DockPaletteWidget *widget)
{
    if (d->mRegisteredWidgets.contains(widget))
    {
        widget->setVisibleState(false);
        removeTab(widget);
        moveTabs();
    }
}

void DockPalette::connectSignals()
{
    for (int i = 0; i < d->mRegisteredWidgets.size(); i++)
    {
        connect(d->mRegisteredWidgets.at(i), &DockPaletteWidget::showTab, this, &DockPalette::onShowTabWidget);
        connect(d->mRegisteredWidgets.at(i), &DockPaletteWidget::hideTab, this, &DockPalette::onHideTabWidget);
    }
}

void DockPalette::registerWidget(DockPaletteWidget *widget)
{
    if (!d->mRegisteredWidgets.contains(widget))
    {
        // 页面必须挂到面板下，才能被堆叠控件接管显示。
        widget->setParent(this);
        d->mRegisteredWidgets.append(widget);

        // 注册后默认隐藏，真正显示要等 addTab() 或 showTab()。
        widget->hide();
    }
}

void DockPalette::onAllDownloadsFinished()
{
    for (int i = 0; i < d->mTabWidgets.size(); i++)
    {
        DockPaletteWidget *pW = d->mTabWidgets.at(i);
        if (NULL != pW && d->mCrntTabWidget == pW->name())
        {
            d->mpStackWidget->setCurrentWidget(pW);
        }
    }
}

void DockPalette::moveTabs()
{
    // 左面板的标签条贴在面板右缘之外；右面板则贴在面板左缘之外。
    int x = width();
    if (d->mOrientation == DockOrientation::Right)
    {
        if (parentWidget())
        {
            x = parentWidget()->width() - width() - border() * 2;
        }
    }

    int y = border();
    if (DockTabOrientation::Down == d->mTabsOrientation)
    {
        if (d->mOrientation == DockOrientation::Right)
        {
            if (parentWidget())
            {
                y = parentWidget()->height() - border() - d->mTabWidgets.size() * kTabSize;
            }
        }
        else
        {
            y = height() - border() - d->mTabWidgets.size() * kTabSize;
        }
    }

    d->mHTab = y;

    d->mTabPalette->move(x, y + d->mTabPalette->d->mVerticalOffset);
}

void DockPalette::resizeTabs()
{
    int numTabs = d->mTabWidgets.size();
    // 标签条宽度固定为两倍 border，高度按标签数与间距累加。
    d->mTabPalette->setFixedSize(2 * border(), (numTabs * kTabSize) + qMax(numTabs - 1, 0) * tabSpacing());
}

QRect DockPalette::getTabPaletteRect()
{
    QRect  tabsRect    = d->mTabPalette->rect();
    QPoint topLeft     = d->mTabPalette->mapToParent(tabsRect.topLeft());
    QPoint bottomRight = d->mTabPalette->mapToParent(tabsRect.bottomRight());

    return QRect(topLeft, bottomRight);
}

void DockPalette::assignParent(QWidget *widget)
{
    setParent(widget);
    // 标签条是独立同级控件，换父控件时必须一起迁移。
    d->mTabPalette->setParent(widget);
}

void DockPalette::setVisible(bool visible)
{
    QWidget::setVisible(visible);
    // 标签条不在面板的子控件树里，需要单独同步显隐。
    d->mTabPalette->setVisible(visible);
}

bool DockPalette::switchMode(DockWidgetMode mode)
{
    // 先记住旧模式下打开的标签，供下次切回时恢复。
    d->mLastOpenedTabForMode.insert(d->mCurrentMode, d->mpStackWidget->currentIndex());
    d->mCurrentMode         = mode;
    bool hasVisibleElements = false;
    // 遍历全部已注册页面：当前模式可见的加入标签，其余移除。
    for (int i = 0; i < d->mRegisteredWidgets.size(); i++)
    {
        DockPaletteWidget *pNextWidget = d->mRegisteredWidgets.at(i);
        if (pNextWidget != NULL)
        {
            if (pNextWidget->visibleInMode(mode))
            {
                addTab(pNextWidget);
                hasVisibleElements = true;
            }
            else
            {
                removeTab(pNextWidget);
            }
        }
    }

    // 恢复该模式上次打开的标签；没有记录时 value() 返回 0。
    if (d->mRegisteredWidgets.size() > 0)
    {
        showTabWidget(d->mLastOpenedTabForMode.value(d->mCurrentMode));
    }

    update();

    return hasVisibleElements;
}

TabDockPalette::TabDockPalette(DockPalette *dockPalette, QWidget *parent) : QWidget(parent), d(new Private)
{
    d->dock     = dockPalette;

    int numTabs = dockPalette->d->mTabWidgets.size();
    resize(2 * dockPalette->border(), (numTabs * kTabSize) + qMax(numTabs - 1, 0) * dockPalette->tabSpacing());

    setAttribute(Qt::WA_TranslucentBackground);
}

void TabDockPalette::paintEvent(QPaintEvent *)
{
    int nTabs = d->dock->d->mTabWidgets.size();
    if (nTabs <= 0)
    {
        qDebug() << "页签数量不够";
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(d->dock->d->mBackgroundBrush);

    int yFrom = 0;
    for (int i = 0; i < nTabs; i++)
    {
        DockPaletteWidget *pCrntWidget = d->dock->d->mTabWidgets.at(i);
        QPainterPath       path;
        path.setFillRule(Qt::WindingFill);
        QPixmap iconPixmap;

        switch (d->dock->d->mOrientation)
        {
            case DockOrientation::Left:
                path.addRect(0, yFrom, width() / 2, kTabSize);
                path.addRoundedRect(0, yFrom, width(), kTabSize, d->dock->radius(), d->dock->radius());
                if (pCrntWidget)
                {
                    if (d->dock->d->mCollapseWidth >= d->dock->width())
                    {
                        // 收起态的箭头图标：朝外，提示可以展开
                        iconPixmap = pCrntWidget->iconToRight();
                    }
                    else
                    {
                        // 展开态的箭头图标：朝内，提示可以收起
                        iconPixmap = pCrntWidget->iconToLeft();
                    }
                }
                break;

            case DockOrientation::Right:
                path.addRect(width() / 2, yFrom, width() / 2, kTabSize);
                path.addRoundedRect(0, yFrom, width(), kTabSize, d->dock->radius(), d->dock->radius());
                if (pCrntWidget)
                {
                    if (d->dock->d->mCollapseWidth >= d->dock->width())
                    {
                        // 收起态的箭头图标：朝外，提示可以展开
                        iconPixmap = pCrntWidget->iconToLeft();
                    }
                    else
                    {
                        // 展开态的箭头图标：朝内，提示可以收起
                        iconPixmap = pCrntWidget->iconToRight();
                    }
                }
                break;

            case DockOrientation::Top:;
            case DockOrientation::Bottom:;
            default:
                break;
        }

        painter.save();
        // 非当前标签用半透明画刷压暗，形成前后层次。
        if (d->dock->d->mCurrentTab != i)
        {
            QColor color(0x7F, 0x7F, 0x7F, 0x3F);
            painter.setBrush(QBrush(color));
        }

        painter.drawPath(path);
        painter.drawPixmap(2, yFrom + 2, width() - 4, kTabSize - 4, iconPixmap);
        yFrom += (kTabSize + d->dock->tabSpacing());
        painter.restore();
    }
}
TabDockPalette::~TabDockPalette()
{
}

void TabDockPalette::mousePressEvent(QMouseEvent *event)
{
    d->dock->d->mClickTime.start();
    // 记下按下位置并先按“未改宽”处理：只有移动超过拖拽阈值才转为改宽。
    QPoint p                   = event->pos();
    d->dock->d->mMousePressPos = p;
    d->dock->d->mResized       = false;

    switch (d->dock->d->mOrientation)
    {
        case DockOrientation::Left:
            d->dock->d->mCanResize = true;
            break;
        case DockOrientation::Right:
            d->dock->d->mCanResize = true;
            break;
        case DockOrientation::Top:
            // 上下停靠暂不支持改尺寸
            break;
        case DockOrientation::Bottom:
            // 上下停靠暂不支持改尺寸
            break;
        default:
            break;
    }
}
void TabDockPalette::mouseMoveEvent(QMouseEvent *event)
{
    QPoint p = event->pos();

    // 超过系统拖拽阈值才算拖动，避免点击时手抖导致面板宽度跳变。
    if (d->dock->d->mCanResize && ((d->dock->d->mMousePressPos - p).manhattanLength() > QApplication::startDragDistance()))
    {
        // 标签条可浮动时，纵向大幅移动按平移标签处理。
        if (qAbs(d->dock->d->mMousePressPos.y() - p.y()) > 10 && d->mFlotable)
        {
            qDebug() << "Y 方向差值:" << d->dock->d->mMousePressPos.y() << p.y();
            d->mVerticalOffset += p.y() - d->dock->d->mMousePressPos.y();
            move(this->pos().x(), p.y());
        }

        switch (d->dock->d->mOrientation)
        {
            case DockOrientation::Left:
                // 左面板：鼠标 x 加上当前宽度即新的右边界。
                p.setX(p.x() + d->dock->width());
                if (p.x() < d->dock->collapseWidth() && p.x() >= d->dock->minimumWidth())
                {
                    d->dock->update();
                    d->dock->resize(0, d->dock->height());
                    d->dock->d->mResized = true;
                }
                else if (p.x() <= d->dock->maximumWidth() && p.x() >= d->dock->minimumWidth())
                {
                    d->dock->resize(p.x(), d->dock->height());
                    d->dock->d->mResized = true;
                }
                break;

            case DockOrientation::Right:
                // 右面板：用容器宽度减去鼠标位置换算成面板宽度。
                p.setX(p.x() - 2 * d->dock->border());
                if ((d->dock->x() + p.x() > d->dock->parentWidget()->width() - d->dock->collapseWidth()) &&
                    (d->dock->x() + p.x() < d->dock->parentWidget()->width()))
                {
                    d->dock->update();
                    d->dock->resize(0, d->dock->height());
                    d->dock->d->mResized = true;
                }
                else if ((d->dock->x() + p.x() >= d->dock->parentWidget()->width() - d->dock->maximumWidth()) &&
                         (d->dock->x() + p.x() <= d->dock->parentWidget()->width() - d->dock->minimumWidth()))
                {
                    d->dock->resize(d->dock->parentWidget()->width() - (d->dock->x() + p.x()), d->dock->height());
                    d->dock->d->mResized = true;
                }
                break;

            case DockOrientation::Top:
            case DockOrientation::Bottom:
                if (p.y() <= d->dock->maximumHeight())
                {
                    d->dock->resize(width(), p.y());
                    d->dock->d->mResized = true;
                }
                break;

            default:
                break;
        }
    }
}

void TabDockPalette::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    // 没有改宽且按下时间足够短，才按点击处理。
    if (!d->dock->d->mResized && d->dock->d->mClickTime.elapsed() < kMaxClickDurationMs)
    {
        int nbTabs     = d->dock->d->mTabWidgets.size();
        int clickedTab = 0;
        // 只有按下位置落在标签区域内才计算被点的标签。
        if (d->dock->d->mMousePressPos.x() >= 0 && d->dock->d->mMousePressPos.x() <= width() && d->dock->d->mMousePressPos.y() >= 0 &&
            d->dock->d->mMousePressPos.y() <= nbTabs * kTabSize + (nbTabs - 1) * d->dock->tabSpacing())
        {
            clickedTab = (d->dock->d->mMousePressPos.y()) / (kTabSize + d->dock->tabSpacing());
            d->dock->tabClicked(clickedTab);
        }
    }
    d->dock->d->mCanResize = false;
}
