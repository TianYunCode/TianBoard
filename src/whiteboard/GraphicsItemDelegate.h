/**
 * @file GraphicsItemDelegate.h
 * @brief 声明图形项委托体系：选择框按钮、媒体控制条、工具栏与委托基类。
 */

#pragma once

#include <QtWidgets>
#include <QtSvg>
#include <QtSvgWidgets>
#include <QMimeData>
#include <QGraphicsVideoItem>

#include "utils/Enums.h"
#include "settings/Settings.h"

class QGraphicsSceneMouseEvent;
class QGraphicsItem;
class GraphicsScene;
class GraphicsProxyWidget;
class GraphicsDelegateFrame;
class GraphicsWidgetItem;
class GraphicsMediaItem;

/**
 * @brief 选择框上的 SVG 图标按钮。
 *
 * 按钮始终挂在 GraphicsDelegateFrame 上，按 WindowFrameSection 决定贴边位置。
 * 按下后开始计时，长按会发出 longClicked()，普通点击在释放时发出 clicked()；
 * 进度指示器用于提示长按操作正在进行。按钮不拥有被委托项。
 */
class DelegateButton : public QGraphicsSvgItem {
    Q_OBJECT

public:
    /**
     * @brief 构造按钮。
     * @param fileName SVG 资源路径。
     * @param pDelegated 被委托的图形项，用于判断点击目标，所有权不转移。
     * @param parent 父图形项，通常是选择框。
     * @param section 按钮在框架上的贴边位置。
     */
    DelegateButton(const QString &fileName, QGraphicsItem *pDelegated, QGraphicsItem *parent = 0,
                   Qt::WindowFrameSection section = Qt::TopLeftSection);

    virtual ~DelegateButton();

    enum { Type = GraphicsItemType::DelegateButtonType };
    /** @brief 返回图形项类型标识。 */
    virtual int type() const { return Type; }

    /** @brief 设为 true 时鼠标事件穿透到下层，按钮只作装饰。 */
    void setTransparentToMouseEvent(bool tr) { mIsTransparentToMouseEvent = tr; }

    /** @brief 更换按钮图标。 */
    void setFileName(const QString &fileName);

    /** @brief 设置长按进度指示器是否可用。 */
    void setShowProgressIndicator(bool pShow) { mShowProgressIndicator = pShow; }
    /** @brief 返回长按进度指示器是否可用。 */
    bool testShowProgresIndicator() const { return mShowProgressIndicator; }

    /** @brief 设置按钮贴边位置，父框架据此摆放。 */
    void setSection(Qt::WindowFrameSection section) { mButtonAlignmentSection = section; }
    /** @brief 返回按钮贴边位置。 */
    Qt::WindowFrameSection getSection() const { return mButtonAlignmentSection; }

protected:
    /** @brief 按下时记录时间并启动长按计时。 */
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    /** @brief 释放时根据按压时长发出 clicked() 或 longClicked()。 */
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    /** @brief 绘制图标与长按进度环。 */
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    /** @brief 推进长按进度，达到阈值后发出 longClicked()。 */
    void timerEvent(QTimerEvent *event);

    /** @brief 通知场景按钮状态已改变，触发重绘。 */
    void modified();

private slots:
    /** @brief 开始显示长按进度指示。 */
    void startShowProgress();

private:
    QGraphicsItem *mDelegated;

    QTime mPressedTime;
    bool mIsTransparentToMouseEvent;
    bool mIsPressed;
    int mProgressTimerId;
    int mPressProgres;
    bool mShowProgressIndicator;
    Qt::WindowFrameSection mButtonAlignmentSection;

signals:
    /** @brief 按钮被短按后发出。 */
    void clicked(bool checked = false);
    /** @brief 按钮被长按达到阈值后发出。 */
    void longClicked();
};

/**
 * @brief 媒体控制条上的七段数码管计时器。
 *
 * 用自绘的段码显示当前时间，作为 DelegateMediaControl 的子项贴在控制条上。
 * 不拥有任何外部对象。
 */
class MediaTimer : public QGraphicsRectItem {
public:
    /** @brief 构造计时器，初始不显示数字。 */
    MediaTimer(QGraphicsItem *parent = 0);
    ~MediaTimer();

    /** @brief 根据父控制条的位置和尺寸重新摆放自身。 */
    void positionHandles();
    /** @brief 绘制七段数码管。 */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    /** @brief 显示给定字符串（通常为 mm:ss 格式）。 */
    void display(const QString &str);
    /** @brief 设置可显示的数字位数。 */
    void setNumDigits(int nDigits);

private:
    static const char *getSegments(char);
    void drawString(const QString &s, QPainter &, QBitArray * = 0, bool = true);
    void drawDigit(const QPoint &, QPainter &, int, char, char = ' ');
    void drawSegment(const QPoint &, char, QPainter &, int, bool = false);
    void addPoint(QPolygon &, const QPoint &);
    void internalSetString(const QString &s);

    static char segments[][8];

    int ndigits;
    QString digitStr;
    QBitArray points;
    double val;

    uint shadow : 1;
    uint smallPoint : 1;

    int digitSpace;
    int xSegLen;
    int ySegLen;
    int segLen;
    int xAdvance;
    int xOffset;
    int yOffset;
};

/**
 * @brief 媒体项底部的播放控制条。
 *
 * 提供播放/暂停、进度拖动与时间显示，事件坐标限定在控制条矩形内。
 * 它直接引用被委托的 GraphicsMediaItem 来驱动播放器，不拥有该媒体项。
 */
class DelegateMediaControl : public QObject, public QGraphicsRectItem {
    Q_OBJECT

public:
    /**
     * @brief 构造控制条。
     * @param pDelegated 被委托的媒体项，不能为空，所有权不转移。
     * @param parent 父图形项，通常为媒体项。
     */
    DelegateMediaControl(GraphicsMediaItem *pDelegated, QGraphicsItem *parent = 0);

    virtual ~DelegateMediaControl() {}

    /** @brief 绘制控制条背景、按钮与进度。 */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    /** @brief 返回控制条的命中路径。 */
    QPainterPath shape() const;

    /** @brief 按下时判断是否点击了播放按钮或进度区域。 */
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    /** @brief 拖动进度时更新播放位置。 */
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    /** @brief 释放时结束拖动。 */
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    /** @brief 重新计算尺寸并触发重绘。 */
    virtual void update();

    /** @brief 按媒体项位置摆放控制条。 */
    void positionHandles();
    /** @brief 用当前播放时间刷新进度与时间显示。 */
    void updateTicker(qint64 time);
    /** @brief 媒体总时长变化时更新内部记录。 */
    void totalTimeChanged(qint64 newTotalTime);

signals:
    /** @brief 用户与控件发生交互时发出，用于延迟隐藏。 */
    void used();

protected:
    /** @brief 按鼠标位置跳转播放进度。 */
    void seekToMousePos(QPointF mousePos);

    GraphicsMediaItem *mDelegate;
    bool mDisplayCurrentTime;

    qint64 mCurrentTimeInMs;
    qint64 mTotalTimeInMs;

private:
    int mStartWidth;
    int mSeecAreaBorderHeight;

    QRectF mSeecArea;
    QRectF mLCDTimerArea;

    MediaTimer *lcdTimer;

    QString mDisplayFormat;
};

/**
 * @brief 媒体项的浮动工具栏容器。
 *
 * 当媒体项被选中时，在内容上方浮出一条工具栏，容纳播放等操作按钮。
 * 工具栏只负责布局与可见性，按钮由使用方通过 setItemsOnToolBar 注入。
 */
class GraphicsToolBarItem : public QObject, public QGraphicsRectItem {
public:
    /** @brief 构造工具栏，初始不可见且宽度为最小值。 */
    GraphicsToolBarItem(QGraphicsItem *parent = 0);
    virtual ~GraphicsToolBarItem() { ; }

    /** @brief 工具栏当前是否显示在画板上。 */
    bool isVisibleOnBoard() const { return mVisible; }
    /** @brief 设置工具栏是否显示在画板上。 */
    void setVisibleOnBoard(bool visible) { mVisible = visible; }
    /** @brief 工具栏是否处于让位移动状态（避免遮挡内容）。 */
    bool isShifting() const { return mShifting; }
    /** @brief 设置工具栏是否让位移动。 */
    void setShifting(bool shifting) { mShifting = shifting; }
    /** @brief 返回工具栏上承载的图形项。 */
    QList<QGraphicsItem *> itemsOnToolBar() const { return mItemsOnToolBar; }
    /** @brief 设置工具栏上承载的图形项。 */
    void setItemsOnToolBar(QList<QGraphicsItem *> itemsOnToolBar) { mItemsOnToolBar = itemsOnToolBar; }
    /** @brief 返回工具栏最小宽度。 */
    int minWidth() { return mMinWidth; }
    /** @brief 按承载项重新摆放工具栏及其子项。 */
    void positionHandles();
    /** @brief 触发工具栏重绘。 */
    void update();
    /** @brief 返回元素之间的间距。 */
    int getElementsPadding() { return mElementsPadding; }

private:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
    bool mShifting;
    bool mVisible;
    int mMinWidth;
    int mInitialHeight;
    int mElementsPadding;
    QList<QGraphicsItem *> mItemsOnToolBar;
};

/**
 * @brief 图形项委托基类：把交互、选择框和菜单从图形项中分离出来。
 *
 * 每个可交互的图形项持有一个委托对象。委托负责：
 * - 创建并摆放 GraphicsDelegateFrame（选中边框）与一圈 DelegateButton；
 * - 拦截鼠标、滚轮、键盘事件，实现移动、缩放、旋转、删除、复制、层级调整；
 * - 维护右键菜单与锁定/显示状态；
 * - 通过 startUndoStep()/commitUndoStep() 把一次拖动并入撤销栈。
 *
 * 所有权：委托由被委托的图形项创建并持有（构造时传入 parent），委托不删除 mDelegated；
 * 框架与按钮由委托创建，统一在 freeControls()/析构中释放。
 * 坐标：所有几何操作都交给框架，委托自身不做坐标换算。
 * z 值：委托不直接改 z 值，层级变化通过场景的层级命令完成。
 */
class GraphicsItemDelegate : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造委托。
     * @param pDelegated 被委托的图形项，不能为空，所有权不转移。
     * @param parent 父 QObject，通常为空，由图形项在析构时删除委托。
     * @param fls 框架能力标志（可旋转、可翻转、菜单类型等）。
     */
    GraphicsItemDelegate(QGraphicsItem *pDelegated, QObject *parent = 0, GraphicsFlags fls = GraphicsFlags());

    virtual ~GraphicsItemDelegate();

    /** @brief 创建选择框与按钮；已存在时先释放。 */
    virtual void createControls();
    /** @brief 释放选择框与全部按钮。 */
    virtual void freeControls();
    /** @brief 显示选择框与按钮，并按当前缩放摆放。 */
    virtual void showControls();
    /** @brief 返回控件是否已创建。 */
    virtual bool controlsExist() const;

    /**
     * @brief 处理按下：默认开始撤销步骤并选中被委托项。
     * @return true 表示事件已被消费，图形项不再处理。
     */
    virtual bool mousePressEvent(QGraphicsSceneMouseEvent *event);
    /** @brief 处理移动：未创建框架时返回 false，否则交给框架。 */
    virtual bool mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    /** @brief 处理释放：结束拖动并提交撤销步骤。 */
    virtual bool mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    /** @brief 处理滚轮事件，默认不消费。 */
    virtual bool wheelEvent(QGraphicsSceneWheelEvent *event);

    /** @brief 鼠标进入时显示选择框。 */
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    /** @brief 鼠标离开时按需隐藏选择框。 */
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

    /** @brief 键盘按下，默认不消费。 */
    virtual bool keyPressEvent(QKeyEvent *event);
    /** @brief 键盘释放，默认不消费。 */
    virtual bool keyReleaseEvent(QKeyEvent *event);

    /**
     * @brief 图形项状态变化钩子。
     * @return 处理后的值；子类可修改后返回。
     */
    virtual QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value);
    /** @brief 返回被委托项所属的 GraphicsScene，失败时返回空。 */
    virtual GraphicsScene *owningScene();
    /** @brief 在图形项绘制之后绘制选择框等装饰。 */
    virtual void postpaint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    /** @brief 调试输出。 */
    void printMessage(const QString &mess) { qDebug() << mess; }

    /** @brief 返回被委托的图形项。 */
    QGraphicsItem *delegated();

    /** @brief 摆放选择框与按钮；无框架时自动创建。 */
    virtual void positionHandles();
    /** @brief 显示或隐藏层级调整按钮。 */
    void setZOrderButtonsVisible(bool visible);

    /**
     * @brief 开始一个撤销步骤。
     * @note 会记录当前变换、位置、尺寸与 z 值，供 commitUndoStep() 生成命令。
     */
    void startUndoStep();
    /** @brief 提交自 startUndoStep() 以来的变化；无变化时不生成命令。 */
    void commitUndoStep();

    /** @brief 返回选择框，可能为空。 */
    GraphicsDelegateFrame *frame() { return mFrame; }

    /** @brief 返回被委托项是否处于锁定状态。 */
    bool isLocked() const;

    /** @brief 返回拖动时携带的 MIME 数据。 */
    QMimeData *mimeData() { return mMimeData; }
    /** @brief 设置拖动时携带的 MIME 数据，委托接管其所有权。 */
    void setMimeData(QMimeData *mimeData);
    /** @brief 设置拖动时显示的缩略图。 */
    void setDragPixmap(const QPixmap &pix) { mDragPixmap = pix; }

    /** @brief 设置锁定状态，并递归应用到组合子项。 */
    void setLocked(bool pLocked);
    /** @brief 统一设置选择框按钮的可见性。 */
    void setButtonsVisible(bool visible);

    /** @brief 返回媒体工具栏，可能为空。 */
    GraphicsToolBarItem *getToolBarItem() const { return mToolBarItem; }

    /** @brief 返回当前反缩放系数，用于保持控件屏幕尺寸。 */
    qreal antiScaleRatio() const { return mAntiScaleRatio; }
    /** @brief 控件刷新入口，默认重新摆放。 */
    virtual void update() { positionHandles(); }

    /** @brief 返回框架能力标志。 */
    GraphicsFlags frameFlags() const { return mFlags; }
    /** @brief 测试是否设置了某个能力标志。 */
    bool testFrameFlags(GraphicsFlags pf) const { return mFlags & pf; }
    /** @brief 替换全部能力标志并刷新框架。 */
    void setFrameFlags(GraphicsFlags pf);
    /** @brief 设置或清除单个能力标志。 */
    void setFrameFlag(GraphicsFlags pf, bool set = true);

    /** @brief 显示媒体工具栏；基类为空实现。 */
    virtual void showToolBar(bool autohide = true) { Q_UNUSED(autohide); }

signals:
    /** @brief “在扩展屏显示”状态变化时发出。 */
    void showOnDisplayChanged(bool shown);

public slots:
    /**
     * @brief 删除被委托项。
     * @param canUndo 是否纳入撤销栈。
     */
    virtual void remove(bool canUndo = true);
    /** @brief 在鼠标位置弹出右键菜单。 */
    void showMenu();

    /** @brief 设置是否在扩展屏显示。 */
    virtual void showHide(bool show);
    /** @brief 设置锁定状态。 */
    virtual void lock(bool lock);
    /** @brief 复制被委托项并放到场景中。 */
    virtual void duplicate();

    /** @brief 上移一层。 */
    void increaseZLevelUp();
    /** @brief 下移一层。 */
    void increaseZLevelDown();
    /** @brief 移到最上层。 */
    void increaseZlevelTop();
    /** @brief 移到最下层。 */
    void increaseZlevelBottom();

    /** @brief 视图缩放变化时调整框架的反缩放系数。 */
    void onZoomChanged();

protected:
    /** @brief 创建选择框上的按钮，子类可扩展。 */
    virtual void buildButtons();
    /** @brief 删除按钮并清空列表。 */
    virtual void freeButtons();
    /** @brief 向菜单添加通用动作，子类可扩展。 */
    virtual void decorateMenu(QMenu *menu);
    /** @brief 根据当前状态刷新菜单项的勾选与可用性。 */
    virtual void updateMenuActionState();

    /** @brief 递归设置子项的显示状态。 */
    void showHideRecurs(const QVariant &pShow, QGraphicsItem *pItem);
    /** @brief 递归设置子项的锁定状态。 */
    void setLockedRecurs(const QVariant &pLock, QGraphicsItem *pItem);

    /** @brief 返回当前按钮列表。 */
    QList<DelegateButton *> buttons() { return mButtons; }
    QGraphicsItem *mDelegated;

    // 选择框左上角的按钮
    DelegateButton *mDeleteButton;
    DelegateButton *mDuplicateButton;
    DelegateButton *mMenuButton;

    // 选择框左下角的按钮
    DelegateButton *mZOrderUpButton;
    DelegateButton *mZOrderDownButton;

    QMenu *mMenu;

    QAction *mLockAction;
    QAction *mShowOnDisplayAction;
    QAction *mSetAsBackgroundAction;
    QAction *mGotoContentSourceAction;

    GraphicsDelegateFrame *mFrame;
    qreal mFrameWidth;
    qreal mAntiScaleRatio;

    QList<DelegateButton *> mButtons;
    QList<DelegateButton *> mToolBarButtons;
    GraphicsToolBarItem *mToolBarItem;

protected slots:
    /** @brief 把图像类对象设为页面背景。 */
    virtual void setAsBackground();
    /** @brief 跳转到内容的原始来源（如网页链接）。 */
    virtual void gotoContentSource();

private:
    void updateFrame();
    void updateButtons(bool showUpdated = false);

    QPointF mOffset;
    QTransform mPreviousTransform;
    QPointF mPreviousPosition;
    QPointF mDragStartPosition;
    qreal mPreviousZValue;
    QSizeF mPreviousSize;
    QMimeData *mMimeData;
    QPixmap mDragPixmap;

    bool mMoved;
    GraphicsFlags mFlags;
};
