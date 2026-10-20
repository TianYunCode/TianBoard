/**
 * @file TabWidget.h
 * @brief 声明网页标签页相关部件：可拖拽/带右键菜单的 TabBar、动作映射器 WebActionMapper
 *        以及管理 WebView 与地址栏的 TabWidget。
 */

#pragma once

#include <QtWidgets>
#include <QShortcut>
#include <QStackedWidget>
#include <QCompleter>
#include <QTabBar>
#include <QWebEnginePage>
#include <QTabWidget>
#include <QLineEdit>


/**
 * @brief 网页标签栏：在 QTabBar 上增加 Alt+数字 切页、右键菜单与向外拖拽网址。
 *
 * 自身不创建标签页，只发出 newTab/cloneTab/closeTab 等信号，由 TabWidget 执行。
 * 拖拽时把标签页 URL 放入 QMimeData 并标记 action=tab-reordering 以便外部识别。
 */
class TabBar : public QTabBar {
    Q_OBJECT;

public:
    /** @brief 构造标签栏：安装 Alt+1..0 快捷键与右键菜单。 */
    TabBar(QWidget *parent = 0);

protected:
    /** @brief 记录左键按下位置，作为拖拽起点。 */
    void mousePressEvent(QMouseEvent *event);
    /** @brief 按住左键向上拖动超过阈值时发起 URL 拖拽。 */
    void mouseMoveEvent(QMouseEvent *event);

signals:
    /** @brief 请求新建标签页。 */
    void newTab();
    /** @brief 请求克隆指定标签页。 */
    void cloneTab(int index);
    /** @brief 请求关闭指定标签页。 */
    void closeTab(int index);
    /** @brief 请求关闭除指定标签页外的其他页。 */
    void closeOtherTabs(int index);
    /** @brief 请求刷新指定标签页。 */
    void reloadTab(int index);
    /** @brief 请求刷新全部标签页。 */
    void reloadAllTabs();

private slots:
    /** @brief 处理 Alt+数字 快捷键，切换对应标签页。 */
    void selectTabAction();
    /** @brief 右键菜单“克隆标签页”槽。 */
    void cloneTab();
    /** @brief 右键菜单“关闭标签页”槽。 */
    void closeTab();
    /** @brief 右键菜单“关闭其他标签页”槽。 */
    void closeOtherTabs();
    /** @brief 右键菜单“刷新标签页”槽。 */
    void reloadTab();
    /** @brief 在鼠标位置弹出标签栏右键菜单。 */
    void contextMenuRequested(const QPoint &position);

private:
    /** @brief Alt+1..0 快捷键对象。 */
    QList<QShortcut *> mTabShortcuts;
    friend class TabWidget;

    /** @brief 左键按下的位置，用于判断拖拽阈值。 */
    QPoint mDragStartPos;
};

#include <QtWebEngineWidgets>


class WebView;

/**
 * @brief 工具栏动作映射器：把主窗口的 QAction 转发到当前页面同名的 WebAction。
 *
 * 一个映射器对应一个 WebAction（后退、前进、刷新、停止）。addChild 收集每个标签页
 * 页面动作，updateCurrent 在切页时把根动作的勾选/可用状态同步到当前页面动作。
 */
class WebActionMapper : public QObject {
    Q_OBJECT;

public:
    /**
     * @brief 构造映射器。
     * @param root 工具栏上的根动作。
     * @param webAction 要映射到的页面动作类型。
     * @param parent 父对象。
     */
    WebActionMapper(QAction *root, QWebEnginePage::WebAction webAction, QObject *parent);

    /** @brief 返回映射的页面动作类型。 */
    QWebEnginePage::WebAction webAction() const;

    /** @brief 登记一个标签页对应的页面动作，同步其状态变化。 */
    void addChild(QAction *action);

    /** @brief 切换到新的页面，重新绑定状态同步。 */
    void updateCurrent(QWebEnginePage *currentParent);

private slots:
    /** @brief 根动作被触发时触发当前页面的同名动作。 */
    void rootTriggered();
    /** @brief 页面动作状态变化时同步到根动作。 */
    void childChanged();
    /** @brief 根动作被销毁时清空引用。 */
    void rootDestroyed();
    /** @brief 当前页面被销毁时解除绑定。 */
    void currentDestroyed();

private:
    /** @brief 当前绑定的页面。 */
    QWebEnginePage *mCurrentParent;
    /** @brief 工具栏根动作。 */
    QAction *mRootAction;
    /** @brief 映射的页面动作类型。 */
    QWebEnginePage::WebAction mWebAction;
};


/**
 * @brief 网页标签页容器：每个标签页由 WebView 与 UrlLineEdit 成对组成，地址栏独占一个堆叠区。
 *
 * 位于 web 域的标签页层，由 BrowserWindow 创建并放入工具栏与主布局。它把 WebView 的
 * 加载进度、标题、图标、URL 变化转发给窗口，并通过 WebActionMapper 统一维护前进/后退
 * 等动作状态。关闭最后一个标签页时会自动补一个新页，保证窗口始终有内容。
 */
class TabWidget : public QTabWidget {
    Q_OBJECT

signals:
    /** @brief 请求加载用户输入的地址。 */
    void loadPage(const QString &url);

    /** @brief 当前标签页标题变化。 */
    void setCurrentTitle(const QString &url);
    /** @brief 当前标签页加载进度变化。 */
    void loadProgress(int progress);
    /** @brief 当前标签页加载结束。 */
    void loadFinished(bool pOk);
    /** @brief 页面请求改变窗口几何。 */
    void geometryChangeRequested(const QRect &geometry);

public:
    /** @brief 构造标签页容器，创建标签栏、地址栏堆叠区与最近关闭菜单。 */
    TabWidget(QWidget *parent = 0);

    /** @brief 清空最近关闭记录与地址栏历史。 */
    void clear();

    /** @brief 登记一个需要随当前页面切换而同步状态的工具栏动作。 */
    void addWebAction(QAction *action, QWebEnginePage::WebAction webAction);

    /** @brief 返回地址栏堆叠控件，供工具栏插入。 */
    QWidget *lineEditStack() const;

    /** @brief 返回当前标签页的地址栏行编辑框。 */
    QLineEdit *currentLineEdit() const;

    /** @brief 返回当前标签页的 WebView。 */
    WebView *currentWebView() const;

    /** @brief 返回指定标签页的 WebView。 */
    WebView *webView(int index) const;

    /** @brief 返回指定标签页的地址栏行编辑框。 */
    QLineEdit *lineEdit(int index) const;

    /** @brief 返回 WebView 所在标签页索引。 */
    int webViewIndex(WebView *webView) const;

    /** @brief 序列化标签页列表与当前页，用于会话恢复。 */
    QByteArray saveState() const;

    /** @brief 从 saveState 的数据恢复标签页，版本或魔数不符时返回 false。 */
    bool restoreState(const QByteArray &state);

    /** @brief 返回标签栏。 */
    TabBar *tabBar() { return mTabBar; }

    /** @brief 返回地址栏堆叠控件。 */
    QStackedWidget *lineEdits() { return mLineEdits; }

    /**
     * @brief 设置地址栏堆叠区可见性。
     * @note 当前实现先显示再隐藏，实际效果总是隐藏，保持原行为不做改动。
     */
    void setLineEditStackVisible(bool visible) {
        mLineEdits->setVisible(visible);
        mLineEdits->hide();
    }

protected:
    /** @brief 双击标签栏空白处新建标签页。 */
    void mouseDoubleClickEvent(QMouseEvent *event);
    /** @brief 空白处右键时转发给标签栏菜单。 */
    void contextMenuEvent(QContextMenuEvent *event);
    /** @brief 点击“+”按钮新建页，中键点击空白处用剪贴板 URL 新建页。 */
    void mouseReleaseEvent(QMouseEvent *event);
    /** @brief 自绘右上角“新建标签页”按钮。 */
    void paintEvent(QPaintEvent *event);
    /** @brief 计算“新建标签页”按钮的矩形。 */
    QRect addTabButtonRect();

public slots:
    /** @brief 在当前标签页加载 URL 并聚焦页面。 */
    void loadUrlInCurrentTab(const QUrl &url);
    /** @brief 新建标签页，makeCurrent 决定是否立即切换过去。 */
    WebView *newTab(bool makeCurrent = true);
    /** @brief 克隆指定标签页的 URL，-1 表示当前页。 */
    void cloneTab(int index = -1);
    /** @brief 关闭指定标签页，-1 表示当前页。 */
    void closeTab(int index = -1);
    /** @brief 关闭除指定页外的所有标签页。 */
    void closeOtherTabs(int index);
    /** @brief 刷新指定标签页，-1 表示当前页。 */
    void reloadTab(int index = -1);
    /** @brief 刷新所有标签页。 */
    void reloadAllTabs();
    /** @brief 切换到下一个标签页（循环）。 */
    void nextTab();
    /** @brief 切换到上一个标签页（循环）。 */
    void previousTab();

private slots:
    /** @brief 当前标签页变化时重绑加载信号与工具栏动作。 */
    void currentChanged(int index);
    /** @brief 填充“最近关闭的标签页”菜单。 */
    void aboutToShowRecentTabsMenu();
    /** @brief 重新打开最近关闭的标签页。 */
    void aboutToShowRecentTriggeredAction(QAction *action);
    /** @brief 加载开始时给标签页换上加载图标。 */
    void webViewLoadStarted();
    /** @brief 图标变化时更新地址栏左侧图标。 */
    void webViewIconChanged();
    /** @brief 标题变化时更新标签文本与窗口标题。 */
    void webViewTitleChanged(const QString &title);
    /** @brief URL 变化时记录到标签数据并确保视图可见。 */
    void webViewUrlChanged(const QUrl &url);
    /** @brief 地址栏回车时加载文本中的地址。 */
    void lineEditReturnPressed();
    /** @brief 页面请求关闭窗口：只剩一页时关窗口，否则关标签页。 */
    void windowCloseRequested();
    /** @brief 标签拖动后同步地址栏堆叠区的顺序。 */
    void moveTab(int fromIndex, int toIndex);

private:
    /** @brief “最近关闭的标签页”菜单动作。 */
    QAction *mRecentlyClosedTabsAction;

    /** @brief “最近关闭的标签页”菜单。 */
    QMenu *mRecentlyClosedTabsMenu;
    /** @brief 最近关闭列表的最大长度。 */
    static const int sRecentlyClosedTabsSize = 10;
    /** @brief 最近关闭的 URL 列表。 */
    QList<QUrl> mRecentlyClosedTabs;
    /** @brief 工具栏动作映射器集合。 */
    QList<WebActionMapper *> mWebActions;

    /** @brief 地址栏补全器。 */
    QCompleter *mLineEditCompleter;
    /** @brief 地址栏堆叠控件，与标签页一一对应。 */
    QStackedWidget *mLineEdits;
    /** @brief 标签栏。 */
    TabBar *mTabBar;
    /** @brief “新建标签页”按钮图标。 */
    QPixmap mAddTabIcon;
};
