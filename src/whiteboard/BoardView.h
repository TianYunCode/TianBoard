/**
 * @file BoardView.h
 * @brief 画板视图（控制视图/演示视图）声明。
 */

#pragma once

#define CONTROLVIEW_OBJ_NAME "ControlView"

#include <QtWidgets>
#include <QGraphicsView>
#include <QRubberBand>

#include <memory>

#include "utils/Enums.h"
#include "whiteboard/GraphicsDelegateFrame.h"

class BoardController;
class GraphicsScene;
class GraphicsWidgetItem;
class RubberBand;

/**
 * @brief 画板的 QGraphicsView：负责输入事件分派、框选、拖放、平板事件与背景绘制。
 *
 * 控制视图接收鼠标/触控/手写笔并执行工具逻辑；演示视图通常只做展示与同步。
 * 两个视图由 BoardController 创建并持有，共享同一场景；视图不拥有场景。
 * 大量状态放在 d（Private）中，避免头文件暴露内部实现。
 *
 * 线程约束：所有事件处理都在 GUI 线程；表格/手写笔事件若来自系统线程也由
 * Qt 事件循环投递到 GUI 线程后处理。
 */
class BoardView : public QGraphicsView
{
    Q_OBJECT

public:
    /**
     * @param pController 画板控制器，不持有所有权。
     * @param pParent 父控件。
     * @param isControl 是否为控制视图。
     * @param isDesktop 是否处于桌面批注模式。
     */
    BoardView(BoardController *pController, QWidget *pParent = 0, bool isControl = false, bool isDesktop = false);
    /**
     * @brief 只显示指定 Z 层区间内图元的构造重载（用于分层渲染/演示）。
     * @param pStartLayer 起始层。
     * @param pEndLayer 结束层。
     */
    BoardView(BoardController *pController, int pStartLayer, int pEndLayer, QWidget *pParent = 0, bool isControl = false, bool isDesktop = false);
    virtual ~BoardView();

    /// @brief 当前场景（转换为 GraphicsScene）。
    GraphicsScene *scene();

    /// @brief 强制结束平板按键状态，避免手写笔丢失 release 事件后卡住。
    void forcedTabletRelease();

    /// @brief 按工具类型设置视图光标。
    void setToolCursor(int tool);

    /// @brief 用当前选中图元初始化橡皮框选。
    void rubberItems();
    /// @brief 按位移移动框选中的图元。
    void moveRubberedItems(QPointF movingVector);

    /// @brief 启用/禁用多选模式。
    void setMultiselection(bool enable);
    /// @brief 当前是否处于多选模式。
    bool isMultipleSelectionEnabled();

signals:
    /// @brief 视图尺寸变化，参数为原始 resize 事件。
    void resized(QResizeEvent *event);
    /// @brief 鼠标释放（供上层结束拖拽/长按状态）。
    void mouseReleased();

protected:
    /// @brief 图元是否被锁定。
    bool itemIsLocked(QGraphicsItem *item);
    /// @brief 是否为画板内容图元（而非尺规/控件）。
    bool isBoardItem(QGraphicsItem *item);
    /// @brief 是否为 C++ 实现的尺规工具图元。
    bool isCppTool(QGraphicsItem *item);
    /// @brief 处理图元选中逻辑（按 Ctrl/Shift 决定替换或追加）。
    void handleItemsSelection(QGraphicsItem *item);
    /// @brief 图元是否应接收本次鼠标按下事件。
    bool itemShouldReceiveMousePressEvent(QGraphicsItem *item);
    /// @brief 图元是否应接收被挂起后重放的鼠标按下事件。
    bool itemShouldReceiveSuspendedMousePressEvent(QGraphicsItem *item);
    /// @brief 图元是否有指定类型的祖先。
    bool itemHaveParentWithType(QGraphicsItem *item, int type);
    /// @brief 图元是否应随拖拽移动。
    bool itemShouldBeMoved(QGraphicsItem *item);
    /// @brief 从命中图元向上找到真正应接收按下的图元。
    QGraphicsItem *determineItemToPress(QGraphicsItem *item);
    /// @brief 从命中图元向上找到真正应被移动的图元。
    QGraphicsItem *determineItemToMove(QGraphicsItem *item);
    /// @brief 处理图元上的鼠标按下。
    void handleItemMousePress(QMouseEvent *event);
    /// @brief 处理图元上的鼠标移动。
    void handleItemMouseMove(QMouseEvent *event);

    /// @brief 事件总入口，先处理平板/手势等特殊情况。
    virtual bool event(QEvent *e);

    /// @brief 键盘按下：删除、撤销、方向键微调等快捷键。
    virtual void keyPressEvent(QKeyEvent *event);
    /// @brief 键盘释放。
    virtual void keyReleaseEvent(QKeyEvent *event);
    /// @brief 手写笔事件：压感绘制、橡皮擦除。
    virtual void tabletEvent(QTabletEvent *event);
    /// @brief 双击：进入文本编辑或激活网页小部件。
    virtual void mouseDoubleClickEvent(QMouseEvent *event);
    /// @brief 鼠标按下：开始绘制、选择、平移或缩放。
    virtual void mousePressEvent(QMouseEvent *event);
    /// @brief 鼠标移动：绘制中继、框选、平移。
    virtual void mouseMoveEvent(QMouseEvent *event);
    /// @brief 鼠标释放：结束绘制/选择并提交撤销。
    virtual void mouseReleaseEvent(QMouseEvent *event);
    /// @brief 滚轮：缩放或滚动。
    virtual void wheelEvent(QWheelEvent *event);
    /// @brief 离开视图时结束悬停相关状态。
    virtual void leaveEvent(QEvent *event);

    /// @brief 失焦时结束进行中的操作，避免状态卡死。
    virtual void focusOutEvent(QFocusEvent *event);

    /// @brief 自定义图元绘制顺序/跳过不可见图元。
    virtual void drawItems(QPainter *painter, int numItems, QGraphicsItem *items[], const QStyleOptionGraphicsItem options[]);

    /// @brief 拖放落下：按 MIME 类型插入图片/文件/URL。
    virtual void dropEvent(QDropEvent *event);
    /// @brief 拖放移动：决定是否接受。
    virtual void dragMoveEvent(QDragMoveEvent *event);

    /// @brief 尺寸变化时同步场景矩形并发出 resized()。
    virtual void resizeEvent(QResizeEvent *event);

    /// @brief 绘制页面背景（纯色/网格/图片）。
    virtual void drawBackground(QPainter *painter, const QRectF &rect);

private:
    /// @brief 构造期公共初始化：视口、渲染提示、拖放与滚动条设置。
    void init();

    /// @brief 指定图元是否需要在本视图中绘制（分层视图过滤）。
    bool shouldDisplayItem(QGraphicsItem *item);

    /// @brief 从 MIME 数据解析出可插入的 URL 列表。
    QList<QUrl> processMimeData(const QMimeData *pMimeData);

    /// @brief 判断坐标是否离谱（超出场景极大范围），用于过滤异常事件。
    bool isAbsurdPoint(QPoint point);

    /// @brief 图元是否有被选中的祖先（组内图元选中判定）。
    static bool hasSelectedParents(QGraphicsItem *item);

    struct Private;             ///< 内部实现（pimpl）
    std::unique_ptr<Private> d; ///< 私有状态

private slots:
    /// @brief 设置项变化后的视图刷新。
    void settingChanged(QVariant newValue);

public slots:
    /// @brief 虚拟键盘显示状态变化时调整视图边距/焦点。
    void virtualKeyboardActivated(bool b);
    /// @brief 长按触发（如右键菜单或特殊工具）。
    void longPressEvent();
};
