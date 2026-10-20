/**
 * @file DockPalette.h
 * @brief 提供可拖拽调整宽度、带标签页的左/右停靠面板及其标签条。
 */

#pragma once

class DocumentProxy;

#include <QElapsedTimer>
#include <QWidget>
#include <QMouseEvent>
#include <QBrush>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QEvent>
#include <QTime>
#include <QPoint>
#include <QPixmap>
#include <QMap>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QVector>

#include <memory>

#include "ui/DockPaletteWidget.h"

/** 停靠面板所在的一侧 */
enum class DockOrientation
{
    Left,
    Right,
    Top,
    Bottom
};

/** 标签朝向 */
enum class DockTabOrientation
{
    Up,
    Down
};

class DockPalette;

/**
 * @brief 停靠面板右侧（或左侧）的标签条，负责绘制标签并处理点击与拖拽改宽。
 *
 * 由 DockPalette 在构造时创建，作为面板的同级控件叠在面板边缘，不参与堆叠页面。
 * 它把鼠标按下位置交给 DockPalette 的私有状态，从而区分“点击切换标签”和
 * “拖动改变面板宽度”；标签图标与背景都取自所属 DockPalette。
 */
class TabDockPalette : public QWidget
{
    Q_OBJECT
    friend class DockPalette;

public:
    /**
     * @brief 构造标签条并绑定所属面板。
     * @param dockPalette 所属的停靠面板
     * @param parent 父控件
     */
    TabDockPalette(DockPalette *dockPalette, QWidget *parent = 0);
    ~TabDockPalette();

protected:
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void paintEvent(QPaintEvent *event);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

/** 左/右停靠面板 */
enum class DockPaletteType
{
    Left,
    Right
};

/**
 * @brief 停靠面板基类：管理一组 DockPaletteWidget 页面、标签条与可折叠的宽度。
 *
 * 界面上是贴在容器左缘或右缘的半透明竖条，右侧一列圆角标签；点击标签切换页面，
 * 再次点击当前标签折叠/展开，拖动标签条可改宽。BoardPaletteManager 创建
 * LeftPalette 与 RightPalette（都继承本类）作为容器 mContainer 的子控件持有，
 * 页面由 registerWidget() 交给它管理，切换应用模式时用 switchMode() 决定显隐。
 */
class DockPalette : public QWidget
{
    Q_OBJECT
    friend class TabDockPalette;

public:
    /**
     * @brief 构造停靠面板及其标签条。
     * @param paletteType 面板位于左侧还是右侧
     * @param parent 容器控件；为空时作为独立窗口
     * @param name 对象名
     */
    DockPalette(DockPaletteType paletteType, QWidget *parent = 0, const char *name = "DockPalette");
    ~DockPalette();

    /** @brief 当前停靠方向。 */
    DockOrientation orientation();
    /**
     * @brief 设置停靠方向，并按方向锁定高度或宽度。
     * @param orientation 新的停靠方向
     * @note 依赖 parentWidget() 存在；当前只完整支持 Left/Right。
     */
    void setOrientation(DockOrientation orientation);
    /**
     * @brief 设置标签条的上下朝向。
     * @param orientation Up 从顶部排列，Down 从底部排列
     */
    void setTabsOrientation(DockTabOrientation orientation);
    /**
     * @brief 切换到指定下标对应的页面。
     * @param tabIndex 堆叠页面下标
     */
    void showTabWidget(int tabIndex);
    /** @brief 返回标签条在父控件坐标系中的矩形。 */
    QRect getTabPaletteRect();

    /**
     * @brief 把面板与标签条一起挂到新的父控件下。
     * @param widget 新父控件
     */
    virtual void assignParent(QWidget *widget);
    /** @brief 同时显示/隐藏面板与标签条。 */
    virtual void setVisible(bool visible);

    virtual void paintEvent(QPaintEvent *event);
    virtual void enterEvent(QEvent *);
    virtual void leaveEvent(QEvent *);

    /**
     * @brief 设置面板背景画刷并重绘。
     * @param brush 新画刷
     */
    void setBackgroundBrush(const QBrush &brush);
    /**
     * @brief 注册页面：转交父控件、加入登记表并隐藏。
     * @param widget 页面
     * @note 只登记，不创建标签；显示需另行 addTab() 或收到 showTab()。
     */
    void registerWidget(DockPaletteWidget *widget);

    /**
     * @brief 把页面加入堆叠控件并生成标签。
     * @param widget 已注册且 visibleState() 为真的页面
     */
    void addTab(DockPaletteWidget *widget);
    /**
     * @brief 从堆叠控件移除页面并删除其标签。
     * @param widget 要移除的页面
     */
    void removeTab(DockPaletteWidget *widget);

    /** @brief 把所有已注册页面的 showTab/hideTab 信号接到本面板。 */
    void connectSignals();

    /**
     * @brief 切换到指定界面模式，按页面的 visibleInMode() 增删标签。
     * @param mode 目标模式
     * @return 是否存在任何可见页面
     */
    bool switchMode(DockWidgetMode mode);

    /** @brief 返回所有已注册页面（含当前隐藏的）。 */
    QVector<DockPaletteWidget *> GetWidgetsList();

public:
    /** @brief 标签条是否允许上下拖动。 */
    bool isTabFlotable();
    /** @brief 设置标签条是否允许上下拖动。 */
    void setTabFlotable(bool newFlotable);
    /** @brief 标签条相对默认位置的纵向偏移。 */
    int getAdditionalVOffset() const;
    /** @brief 设置标签条的纵向偏移。 */
    void setAdditionalVOffset(int newOffset);

    /** @brief 返回本面板是左侧还是右侧。 */
    DockPaletteType paletteType();

public slots:
    /**
     * @brief 响应页面的 showTab()：置位状态、加入标签并重新排布。
     * @param widget 请求显示的页面
     */
    void onShowTabWidget(DockPaletteWidget *widget);
    /**
     * @brief 响应页面的 hideTab()：清除状态、移除标签并重新排布。
     * @param widget 请求隐藏的页面
     */
    void onHideTabWidget(DockPaletteWidget *widget);
    /** @brief 所有下载结束后切回之前打开且仍存在的标签页。 */
    void onAllDownloadsFinished();
    /**
     * @brief 文档集合变化时的钩子。
     * @param documentProxy 新的当前文档
     * @note 基类不处理，由子类按需覆盖。
     */
    virtual void onDocumentSet(DocumentProxy *documentProxy);

signals:
    /** @brief 鼠标进入面板。 */
    void mouseEntered();
    /** @brief 标签点击后请求刷新页面选择。 */
    void pageSelectionChangedRequired();

protected:
    /** @brief 面板内容距边缘的留白，也是标签条宽度的一半。 */
    virtual int border();
    /** @brief 标签圆角半径。 */
    virtual int radius();
    /** @brief 标签文字/内容的额外边距。 */
    virtual int customMargin();
    /** @brief 子类可在此更新最大宽度。 */
    virtual void updateMaxWidth();
    virtual void resizeEvent(QResizeEvent *event);
    /** @brief 触发折叠的宽度阈值。 */
    virtual int collapseWidth();

    /** @brief 当前界面模式。 */
    DockWidgetMode currentMode();
    /** @brief 最近一次非折叠状态下的宽度。 */
    int lastWidth();
    /** @brief 记录非折叠宽度。 */
    void setLastWidth(int width);
    /** @brief 设置折叠阈值。 */
    void setCollapseWidth(int width);
    /** @brief 是否处于可拖拽改宽的状态（由标签条按下/释放维护）。 */
    bool canResize();
    /** @brief 记录某个模式下最后打开的标签下标。 */
    void setLastOpenedTabForMode(DockWidgetMode mode, int tabIndex);

private slots:
    /** @brief 工具栏位置设置变化时同步标签上下朝向。 */
    void onToolbarPosUpdated();
    /** @brief 把外部 resize 请求转交给 resizeEvent()。 */
    void onResizeRequest(QResizeEvent *event);

private:
    /**
     * @brief 处理标签点击：不同标签则切换，同一标签则折叠/展开。
     * @param tabIndex 被点击的标签下标
     */
    void tabClicked(int tabIndex);
    /** @brief 标签之间的间距。 */
    int tabSpacing();
    /** @brief 在折叠与展开之间切换，并记住/恢复宽度。 */
    void toggleCollapseExpand();
    /** @brief 按方向和标签数量重新定位标签条。 */
    void moveTabs();
    /** @brief 按标签数量调整标签条尺寸。 */
    void resizeTabs();

    struct Private;
    std::unique_ptr<Private> d;
};
