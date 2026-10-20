#include "web/TabWidget.h"

#include "web/BrowserWindow.h"
#include "web/UrlLineEdit.h"
#include "web/WebView.h"

#include "core/Application.h"
#include "core/ApplicationController.h"

#include <QtWidgets>
#include <QSvgWidget>
#include <QListView>
#include <QLabel>


TabBar::TabBar(QWidget *parent) : QTabBar(parent) {
    setObjectName("webBrowserTabBar");

    setContextMenuPolicy(Qt::CustomContextMenu);
    setAcceptDrops(true);
    connect(this, &QWidget::customContextMenuRequested, this, &TabBar::contextMenuRequested);

    QString alt = QLatin1String("Alt+%1");
    for (int i = 1; i <= 10; ++i) {
        int key = i;
        if (key == 10)
            key = 0;
        QShortcut *shortCut = new QShortcut(alt.arg(key), this);
        mTabShortcuts.append(shortCut);
        connect(shortCut, &QShortcut::activated, this, &TabBar::selectTabAction);
    }
    setTabsClosable(true);
    connect(this, &QTabBar::tabCloseRequested, this, qOverload<int>(&TabBar::closeTab));
    setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
    setMovable(true);
    setDocumentMode(false);
}

void TabBar::selectTabAction() {
    if (QShortcut *shortCut = qobject_cast<QShortcut *>(sender())) {
        int index = mTabShortcuts.indexOf(shortCut);
        if (index == 0)
            index = 10;
        setCurrentIndex(index);
    }
}

void TabBar::contextMenuRequested(const QPoint &position) {
    QMenu menu;
    menu.addAction(tr("New &Tab"), QKeySequence::AddTab, this, &TabBar::newTab);
    int index = tabAt(position);
    if (-1 != index) {
        QAction *action = menu.addAction(tr("Clone Tab"), this, qOverload<>(&TabBar::cloneTab));
        action->setData(index);

        menu.addSeparator();

        action = menu.addAction(tr("&Close Tab"), QKeySequence::Close, this, qOverload<>(&TabBar::closeTab));
        action->setData(index);

        action = menu.addAction(tr("Close &Other Tabs"), this, qOverload<>(&TabBar::closeOtherTabs));
        action->setData(index);

        menu.addSeparator();

        action =
            menu.addAction(tr("Reload Tab"), QKeySequence::Refresh, this, qOverload<>(&TabBar::reloadTab));
        action->setData(index);
    } else {
        menu.addSeparator();
    }
    menu.addAction(tr("Reload All Tabs"), this, &TabBar::reloadAllTabs);
    menu.exec(QCursor::pos());
}

void TabBar::cloneTab() {
    if (QAction *action = qobject_cast<QAction *>(sender())) {
        int index = action->data().toInt();
        emit cloneTab(index);
    }
}

void TabBar::closeTab() {
    if (QAction *action = qobject_cast<QAction *>(sender())) {
        int index = action->data().toInt();
        emit closeTab(index);
    }
}

void TabBar::closeOtherTabs() {
    if (QAction *action = qobject_cast<QAction *>(sender())) {
        int index = action->data().toInt();
        emit closeOtherTabs(index);
    }
}

void TabBar::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton)
        mDragStartPos = event->pos();
    QTabBar::mousePressEvent(event);
}

void TabBar::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() == Qt::LeftButton) {
        int diffX = event->pos().x() - mDragStartPos.x();
        int diffY = event->pos().y() - mDragStartPos.y();
        if ((event->pos() - mDragStartPos).manhattanLength() > QApplication::startDragDistance() &&
            diffX < 3 && diffX > -3 && diffY < -10) {
            QDrag *drag = new QDrag(this);
            QMimeData *mimeData = new QMimeData;
            QList<QUrl> urls;
            int index = tabAt(event->pos());
            QUrl url = tabData(index).toUrl();
            urls.append(url);
            mimeData->setUrls(urls);
            mimeData->setText(tabText(index));
            mimeData->setData(QLatin1String("action"), "tab-reordering");
            drag->setMimeData(mimeData);
            drag->exec();
        }
    }
    QTabBar::mouseMoveEvent(event);
}

// index 为 -1 时表示当前标签页
void TabWidget::reloadTab(int index) {
    if (index < 0)
        index = currentIndex();
    if (index < 0 || index >= count())
        return;

    QWidget *widget = this->widget(index);
    if (WebView *tab = qobject_cast<WebView *>(widget))
        tab->reload();
}

void TabBar::reloadTab() {
    if (QAction *action = qobject_cast<QAction *>(sender())) {
        int index = action->data().toInt();
        emit reloadTab(index);
    }
}

TabWidget::TabWidget(QWidget *parent)
    : QTabWidget(parent),
      mRecentlyClosedTabsMenu(0),
      mLineEditCompleter(0),
      mLineEdits(0),
      mTabBar(new TabBar(this)) {
    setObjectName("webBrowserTabWidget");
    mAddTabIcon = QPixmap(":/images/toolbar/plusBlack.png");

    setElideMode(Qt::ElideRight);

    connect(mTabBar, &TabBar::newTab, this, [this] { newTab(); });
    connect(mTabBar, qOverload<int>(&TabBar::closeTab), this, &TabWidget::closeTab);
    connect(mTabBar, qOverload<int>(&TabBar::cloneTab), this, &TabWidget::cloneTab);
    connect(mTabBar, qOverload<int>(&TabBar::closeOtherTabs), this, &TabWidget::closeOtherTabs);
    connect(mTabBar, qOverload<int>(&TabBar::reloadTab), this, &TabWidget::reloadTab);
    connect(mTabBar, &TabBar::reloadAllTabs, this, &TabWidget::reloadAllTabs);
    connect(mTabBar, &QTabBar::tabMoved, this, &TabWidget::moveTab);
    setTabBar(mTabBar);
    setDocumentMode(false);

    mRecentlyClosedTabsMenu = new QMenu(this);
    connect(mRecentlyClosedTabsMenu, &QMenu::aboutToShow, this, &TabWidget::aboutToShowRecentTabsMenu);
    connect(mRecentlyClosedTabsMenu, &QMenu::triggered, this, &TabWidget::aboutToShowRecentTriggeredAction);
    mRecentlyClosedTabsAction = new QAction(tr("Recently Closed Tabs"), this);
    mRecentlyClosedTabsAction->setMenu(mRecentlyClosedTabsMenu);
    mRecentlyClosedTabsAction->setEnabled(false);

    connect(this, &QTabWidget::currentChanged, this, &TabWidget::currentChanged);

    mLineEdits = new QStackedWidget(this);
    mLineEdits->setMinimumWidth(200);
    QSizePolicy spolicy = mLineEdits->sizePolicy();
    mLineEdits->setSizePolicy(QSizePolicy::Maximum, spolicy.verticalPolicy());
    mLineEdits->setStyleSheet("background-color:rgba(255,255,255,127)");
}

void TabWidget::clear() {
    // 清空最近关闭记录
    mRecentlyClosedTabs.clear();
    // 清空地址栏历史：重复写入当前文本可丢弃行编辑框内部的撤销记录
    for (int i = 0; i < mLineEdits->count(); ++i) {
        QLineEdit *qLineEdit = lineEdit(i);
        qLineEdit->setText(qLineEdit->text());
    }
}

void TabWidget::moveTab(int fromIndex, int toIndex) {
    QWidget *lineEdit = mLineEdits->widget(fromIndex);
    mLineEdits->removeWidget(lineEdit);
    mLineEdits->insertWidget(toIndex, lineEdit);
}

void TabWidget::addWebAction(QAction *action, QWebEnginePage::WebAction webAction) {
    if (!action)
        return;
    mWebActions.append(new WebActionMapper(action, webAction, this));
}

void TabWidget::currentChanged(int index) {
    WebView *webView = this->webView(index);
    if (!webView)
        return;

    Q_ASSERT(mLineEdits->count() == count());

    // 切页时把加载信号从旧页面摘下、绑到新页面，避免后台标签页也驱动进度与标题。
    WebView *oldWebView = this->webView(mLineEdits->currentIndex());
    if (oldWebView) {
        disconnect(oldWebView, &QWebEngineView::loadProgress, this, &TabWidget::loadProgress);
        disconnect(oldWebView, &QWebEngineView::loadFinished, this, &TabWidget::loadFinished);
    }

    connect(webView, &QWebEngineView::loadProgress, this, &TabWidget::loadProgress);
    connect(webView, &QWebEngineView::loadFinished, this, &TabWidget::loadFinished);

    // 工具栏动作只能指向一个页面，切页后统一改绑到新页面。
    for (int i = 0; i < mWebActions.count(); ++i) {
        WebActionMapper *mapper = mWebActions[i];
        mapper->updateCurrent(webView->page());
    }
    emit setCurrentTitle(webView->title());
    mLineEdits->setCurrentIndex(index);
    emit loadProgress(webView->progress());
    if (webView->url().isEmpty())
        mLineEdits->currentWidget()->setFocus();
    else
        webView->setFocus();
}

QWidget *TabWidget::lineEditStack() const {
    return mLineEdits;
}

QLineEdit *TabWidget::currentLineEdit() const {
    return lineEdit(mLineEdits->currentIndex());
}

WebView *TabWidget::currentWebView() const {
    return webView(currentIndex());
}

QLineEdit *TabWidget::lineEdit(int index) const {
    UrlLineEdit *urlLineEdit = qobject_cast<UrlLineEdit *>(mLineEdits->widget(index));
    if (urlLineEdit)
        return urlLineEdit->lineEdit();
    return 0;
}

WebView *TabWidget::webView(int index) const {
    QWidget *widget = this->widget(index);
    if (WebView *webView = qobject_cast<WebView *>(widget)) {
        return webView;
    } else {
        // 首个标签页只放了占位控件，真正的 WebView 延迟到第一次访问时创建，
        // 避免窗口刚建好就初始化昂贵的 WebEngine 页面。
        if (count() == 1) {
            TabWidget *that = const_cast<TabWidget *>(this);
            that->setUpdatesEnabled(false);
            that->newTab();
            that->closeTab(0);
            that->setUpdatesEnabled(true);
            return currentWebView();
        }
    }
    return 0;
}

int TabWidget::webViewIndex(WebView *webView) const {
    int index = indexOf(webView);
    return index;
}

WebView *TabWidget::newTab(bool makeCurrent) {
    // 每个标签页都配一个独立地址栏，二者在堆叠区中保持相同索引
    UrlLineEdit *urlLineEdit = new UrlLineEdit;

    QSizePolicy urlPolicy = urlLineEdit->sizePolicy();
    urlLineEdit->setSizePolicy(QSizePolicy::MinimumExpanding, urlPolicy.verticalPolicy());

    QLineEdit *lineEdit = urlLineEdit->lineEdit();
    QSizePolicy policy = lineEdit->sizePolicy();
    lineEdit->setSizePolicy(QSizePolicy::MinimumExpanding, policy.verticalPolicy());

    if (!mLineEditCompleter && count() > 0) {
        mLineEditCompleter = new QCompleter(this);
        // 补全列表项高度一致时 QListView 可以跳过逐项测量，弹出更快。
        QAbstractItemView *popup = mLineEditCompleter->popup();
        QListView *listView = qobject_cast<QListView *>(popup);
        if (listView)
            listView->setUniformItemSizes(true);
    }

    lineEdit->setCompleter(mLineEditCompleter);
    connect(lineEdit, &QLineEdit::returnPressed, this, &TabWidget::lineEditReturnPressed);
    mLineEdits->addWidget(urlLineEdit);
    mLineEdits->setSizePolicy(lineEdit->sizePolicy());

    // 首个标签页先只放一个底色占位控件，真正的 WebEngine 视图在需要时再建；
    // 临时断开 currentChanged，避免占位页触发地址栏与标题同步。
    if (count() == 0) {
        QWidget *emptyWidget = new QWidget;
        QPalette p = emptyWidget->palette();
        p.setColor(QPalette::Window, palette().color(QPalette::Base));
        emptyWidget->setPalette(p);
        emptyWidget->setAutoFillBackground(true);
        disconnect(this, &QTabWidget::currentChanged, this, &TabWidget::currentChanged);
        addTab(emptyWidget, tr("(Untitled)"));
        connect(this, &QTabWidget::currentChanged, this, &TabWidget::currentChanged);
        return 0;
    }

    // 之后的标签页直接创建 WebView，并把加载、标题、图标、URL 变化接到本容器
    WebView *webView = new WebView;
    urlLineEdit->setWebView(webView);
    connect(webView, &QWebEngineView::loadStarted, this, &TabWidget::webViewLoadStarted);
    connect(webView, &QWebEngineView::loadFinished, this, [this] { webViewIconChanged(); });
    connect(webView, &QWebEngineView::iconChanged, this, &TabWidget::webViewIconChanged);
    connect(webView, &QWebEngineView::titleChanged, this, &TabWidget::webViewTitleChanged);
    connect(webView, &QWebEngineView::urlChanged, this, &TabWidget::webViewUrlChanged);
    connect(webView->page(), &QWebEnginePage::windowCloseRequested, this, &TabWidget::windowCloseRequested);
    connect(webView->page(), &QWebEnginePage::geometryChangeRequested, this,
            &TabWidget::geometryChangeRequested);

    connect(webView, &WebTrapWebView::pixmapCaptured, Application::applicationController,
            [](const QPixmap &pixmap, bool pageMode) {
                Application::applicationController->addCapturedPixmap(pixmap, pageMode);
            });

    connect(webView, &WebTrapWebView::embedCodeCaptured, Application::applicationController,
            &ApplicationController::addCapturedEmbedCode);

    addTab(webView, tr("(Untitled)"));
    if (makeCurrent)
        setCurrentWidget(webView);

    // 登记工具栏动作对应的页面动作，切页时由 WebActionMapper 重新绑定
    for (int i = 0; i < mWebActions.count(); ++i) {
        WebActionMapper *mapper = mWebActions[i];
        mapper->addChild(webView->page()->action(mapper->webAction()));
    }

    if (count() == 1)
        currentChanged(currentIndex());

    return webView;
}

void TabWidget::reloadAllTabs() {
    for (int i = 0; i < count(); ++i) {
        QWidget *tabWidget = widget(i);
        if (WebView *tab = qobject_cast<WebView *>(tabWidget)) {
            tab->reload();
        }
    }
}

void TabWidget::lineEditReturnPressed() {
    if (QLineEdit *lineEdit = qobject_cast<QLineEdit *>(sender())) {
        emit loadPage(lineEdit->text());
        if (mLineEdits->currentWidget() == lineEdit)
            currentWebView()->setFocus();
    }
}

void TabWidget::windowCloseRequested() {
    WebPage *webPage = qobject_cast<WebPage *>(sender());
    WebView *webView = qobject_cast<WebView *>(webPage->parent());
    int index = webViewIndex(webView);
    if (index >= 0) {
        if (count() == 1)
            webView->webPage()->mainWindow()->close();
        else
            closeTab(index);
    }
}

void TabWidget::closeOtherTabs(int index) {
    if (-1 == index)
        return;
    for (int i = count() - 1; i > index; --i)
        closeTab(i);
    for (int i = index - 1; i >= 0; --i)
        closeTab(i);
}

// index 为 -1 时表示当前标签页
void TabWidget::cloneTab(int index) {
    if (index < 0)
        index = currentIndex();
    if (index < 0 || index >= count())
        return;
    WebView *tab = newTab(false);
    tab->setUrl(webView(index)->url());
}

// index 为 -1 时表示当前标签页
void TabWidget::closeTab(int index) {
    if (index < 0)
        index = currentIndex();
    if (index < 0 || index >= count())
        return;

    bool hasFocus = false;
    if (WebView *tab = webView(index)) {
        hasFocus = tab->hasFocus();

        mRecentlyClosedTabsAction->setEnabled(true);
        mRecentlyClosedTabs.prepend(tab->url());
        if (mRecentlyClosedTabs.size() >= TabWidget::sRecentlyClosedTabsSize)
            mRecentlyClosedTabs.removeLast();
    }

    QWidget *lineEdit = mLineEdits->widget(index);
    mLineEdits->removeWidget(lineEdit);
    lineEdit->deleteLater();
    QWidget *webView = widget(index);
    removeTab(index);
    // 用 deleteLater 延迟销毁：关闭动作可能来自页面自身的信号回调，
    // 在回调栈里直接删除 WebEngine 视图不安全。
    webView->deleteLater();

    if (hasFocus && count() > 0)
        currentWebView()->setFocus();

    if (count() == 0) {
        // 始终保留至少一个标签页，否则窗口会变成空白且没有地址栏可用。
        newTab();
        emit currentChanged(0);
    }
}

void TabWidget::webViewLoadStarted() {
    WebView *webView = qobject_cast<WebView *>(sender());
    int index = webViewIndex(webView);
    if (-1 != index) {
        QIcon icon(QLatin1String(":loading.gif"));
        setTabIcon(index, icon);
    }
}

void TabWidget::webViewIconChanged() {
    WebView *webView = qobject_cast<WebView *>(sender());
    int index = webViewIndex(webView);
    if (-1 != index) {
        QIcon icon = webView->icon();

        UrlLineEdit *urlLineEdit = qobject_cast<UrlLineEdit *>(mLineEdits->widget(index));

        if (urlLineEdit) {
            QLabel *label = new QLabel(urlLineEdit);
            label->setGeometry(0, 0, 32, 32);
            label->setPixmap(icon.pixmap(16, 16));
            label->setAlignment(Qt::AlignCenter);
            label->show();
            urlLineEdit->setLeftWidget(label);
        }
    }
}

void TabWidget::webViewTitleChanged(const QString &title) {
    WebView *webView = qobject_cast<WebView *>(sender());
    int index = webViewIndex(webView);
    if (-1 != index) {
        setTabText(index, title);
    }
    if (currentIndex() == index)
        emit setCurrentTitle(title);
}

void TabWidget::webViewUrlChanged(const QUrl &url) {
    WebView *webView = qobject_cast<WebView *>(sender());
    int index = webViewIndex(webView);
    if (-1 != index) {
        mTabBar->setTabData(index, url);
    }

    // 个别平台/时序下 URL 变化后视图不会自动重绘，显式 show() 促成一次刷新。
    webView->show();
}

void TabWidget::aboutToShowRecentTabsMenu() {
    mRecentlyClosedTabsMenu->clear();
    for (int i = 0; i < mRecentlyClosedTabs.count(); ++i) {
        QAction *action = new QAction(mRecentlyClosedTabsMenu);
        action->setData(mRecentlyClosedTabs.at(i));

        action->setText(mRecentlyClosedTabs.at(i).toString());
        mRecentlyClosedTabsMenu->addAction(action);
    }
}

void TabWidget::aboutToShowRecentTriggeredAction(QAction *action) {
    QUrl url = action->data().toUrl();
    loadUrlInCurrentTab(url);
}

void TabWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    if (!childAt(event->pos())
        // QTabWidget 自带 1 像素边框，这里把可双击区域限定在标签栏高度之内
        && event->pos().y() < (tabBar()->y() + tabBar()->height())) {
        newTab();
        return;
    }
    QTabWidget::mouseDoubleClickEvent(event);
}

void TabWidget::contextMenuEvent(QContextMenuEvent *event) {
    if (!childAt(event->pos())) {
        mTabBar->contextMenuRequested(event->pos());
        return;
    }
    QTabWidget::contextMenuEvent(event);
}

void TabWidget::mouseReleaseEvent(QMouseEvent *event) {
    QRect addTabRect = addTabButtonRect();

    if (addTabRect.contains(event->pos())) {
        newTab();
    } else if (event->button() == Qt::MiddleButton &&
               !childAt(event->pos())
               // QTabWidget 自带 1 像素边框，这里把中键粘贴区域限定在标签栏高度之内
               && event->pos().y() < (tabBar()->y() + tabBar()->height())) {
        QUrl url(QApplication::clipboard()->text(QClipboard::Selection));
        if (!url.isEmpty() && url.isValid() && !url.scheme().isEmpty()) {
            WebView *webView = newTab();
            webView->setUrl(url);
        }
    }
}

void TabWidget::loadUrlInCurrentTab(const QUrl &url) {
    WebView *webView = currentWebView();
    if (webView) {
        webView->load(url);
        webView->setFocus();
    }
}

void TabWidget::nextTab() {
    int next = currentIndex() + 1;
    if (next == count())
        next = 0;
    setCurrentIndex(next);
}

void TabWidget::previousTab() {
    int next = currentIndex() - 1;
    if (next < 0)
        next = count() - 1;
    setCurrentIndex(next);
}

static const qint32 TabWidgetMagic = 0xaa;

QByteArray TabWidget::saveState() const {
    int version = 1;
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    stream << qint32(TabWidgetMagic);
    stream << qint32(version);

    QStringList tabs;
    for (int i = 0; i < count(); ++i) {
        if (WebView *tab = qobject_cast<WebView *>(widget(i))) {
            tabs.append(tab->url().toString());
        } else {
            tabs.append(QString());
        }
    }
    stream << tabs;
    stream << currentIndex();
    return data;
}

bool TabWidget::restoreState(const QByteArray &state) {
    int version = 1;
    QByteArray sd = state;
    QDataStream stream(&sd, QIODevice::ReadOnly);
    if (stream.atEnd())
        return false;

    qint32 marker;
    qint32 v;
    stream >> marker;
    stream >> v;
    if (marker != TabWidgetMagic || v != version)
        return false;

    QStringList openTabs;
    stream >> openTabs;

    for (int i = 0; i < openTabs.count(); ++i) {
        if (i != 0)
            newTab();
        loadPage(openTabs.at(i));
    }

    int currentTab;
    stream >> currentTab;
    setCurrentIndex(currentTab);

    return true;
}

WebActionMapper::WebActionMapper(QAction *root, QWebEnginePage::WebAction webAction, QObject *parent)
    : QObject(parent), mCurrentParent(0), mRootAction(root), mWebAction(webAction) {
    if (!mRootAction)
        return;
    connect(mRootAction, &QAction::triggered, this, [this] { rootTriggered(); });
    connect(root, &QObject::destroyed, this, [this] { rootDestroyed(); });
    root->setEnabled(false);
}

void WebActionMapper::rootDestroyed() {
    mRootAction = 0;
}

void WebActionMapper::currentDestroyed() {
    updateCurrent(0);
}

void WebActionMapper::addChild(QAction *action) {
    if (!action)
        return;
    connect(action, &QAction::changed, this, &WebActionMapper::childChanged);
}

QWebEnginePage::WebAction WebActionMapper::webAction() const {
    return mWebAction;
}

void WebActionMapper::rootTriggered() {
    if (mCurrentParent) {
        QAction *gotoAction = mCurrentParent->action(mWebAction);
        gotoAction->trigger();
    }
}

void WebActionMapper::childChanged() {
    if (QAction *source = qobject_cast<QAction *>(sender())) {
        if (mRootAction && mCurrentParent && source->parent() == mCurrentParent) {
            mRootAction->setChecked(source->isChecked());
            mRootAction->setEnabled(source->isEnabled());
        }
    }
}

void WebActionMapper::updateCurrent(QWebEnginePage *currentParent) {
    if (mCurrentParent)
        disconnect(mCurrentParent, &QObject::destroyed, this, &WebActionMapper::currentDestroyed);

    mCurrentParent = currentParent;
    if (!mRootAction)
        return;
    if (!mCurrentParent) {
        mRootAction->setEnabled(false);
        mRootAction->setChecked(false);
        return;
    }
    QAction *source = mCurrentParent->action(mWebAction);
    mRootAction->setChecked(source->isChecked());
    mRootAction->setEnabled(source->isEnabled());
    connect(mCurrentParent, &QObject::destroyed, this, &WebActionMapper::currentDestroyed);
}


QRect TabWidget::addTabButtonRect() {
    QRect lastTabRect = tabBar()->tabRect(tabBar()->count() - 1);
    int x = lastTabRect.topRight().x();
    int y = 0;

    if (tabPosition() == QTabWidget::North)
        y = lastTabRect.topRight().y();
    else if (tabPosition() == QTabWidget::South)
        y = geometry().height() - lastTabRect.height();
    else
        qDebug() << "TabWidget::addTabButtonRect() - unsupported tab posion";

    // 下面的偏移与宽高和样式表中 QTabBar 的规则保持一致，改样式时需同步修改。
    QRect r(x + 3, y + 6, 25, lastTabRect.height() - 8);

    return r;
}


void TabWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);

    // 渐变与描边参数同样对应样式表中的 QTabBar 规则，保证“+”按钮和标签页外观一致。
    QLinearGradient linearGrad(QPointF(0, 0), QPointF(0, 1));
    linearGrad.setColorAt(0, QColor("#d3d3d3"));
    linearGrad.setColorAt(1, QColor("#dddddd"));
    painter.setBrush(linearGrad);

    QRect r = addTabButtonRect();

    painter.setPen(QColor("#888888"));
    painter.drawRoundedRect(r, 3, 3);

    painter.setPen(Qt::NoPen);
    painter.drawRect(QRect(r.x(), r.y() + r.height() / 2, r.width() + 1, r.height() / 2 + 1));

    painter.setPen(QColor("#888888"));
    painter.drawLine(r.x(), r.y() + r.height() / 2, r.x(), r.bottom());
    painter.drawLine(r.right() + 1, r.y() + r.height() / 2, r.right() + 1, r.bottom());

    if (tabPosition() == QTabWidget::South) {
        QPen pen = painter.pen();
        pen.setColor(QColor("#b3b3b3"));
        pen.setWidth(2);
        painter.setPen(pen);
        painter.drawLine(0, r.bottom() + 2, width(), r.bottom() + 2);
    }

    QPointF topLeft = r.center() - QPointF(mAddTabIcon.width() / 2, mAddTabIcon.height() / 2);
    painter.drawPixmap(topLeft, mAddTabIcon);

    painter.end();

    QTabWidget::paintEvent(event);
}
