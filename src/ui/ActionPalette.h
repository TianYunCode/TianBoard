/**
 * @file ActionPalette.h
 * @brief 提供由 QAction 驱动的浮动按钮调色板及其按钮控件。
 */

#pragma once

#include <QtWidgets>
#include <QPoint>
#include <QButtonGroup>
#include <QToolButton>

#include <memory>

#include "ui/FloatingPalette.h"

class ActionPaletteButton;

/**
 * @brief 把一组 QAction 排成按钮列表的浮动调色板。
 *
 * 界面上是一列（或一行）透明图标按钮，悬浮在白板区域之上，可拖动。所有按钮都由
 * 本类 new 出来并在析构时统一删除；QAction 本身仍归 MainWindow 的 ui 所有，这里
 * 只保存裸指针。BoardPaletteManager 在 setupPalettes() 中创建并持有工具、添加项、
 * 橡皮、翻页等实例；BackgroundPalette、KeyboardPalette、DocumentToolsPalette 和
 * FavoriteToolPalette 都以它为基类。
 */
class ActionPalette : public FloatingPalette
{
    Q_OBJECT

public:
    /**
     * @brief 用给定动作列表构造调色板。
     * @param actions 初始动作，按顺序各生成一个按钮
     * @param orientation 按钮排列方向
     * @param parent 父控件；为空时由 FloatingPalette 作为独立置顶窗口显示
     */
    ActionPalette(QList<QAction *> actions, Qt::Orientation orientation = Qt::Vertical, QWidget *parent = 0);
    /**
     * @brief 构造空调色板，之后可用 addAction()/setActions() 填充。
     * @param orientation 按钮排列方向
     * @param parent 父控件
     */
    ActionPalette(Qt::Orientation orientation, QWidget *parent = 0);
    /**
     * @brief 构造空调色板并指定初始停靠角。
     * @param corner 初始停靠角，供 FloatingPalette 定位使用
     * @param parent 父控件
     * @param orient 按钮排列方向
     */
    ActionPalette(Qt::Corner corner, QWidget *parent = 0, Qt::Orientation orient = Qt::Vertical);
    /**
     * @brief 构造空的竖向调色板。
     * @param parent 父控件
     */
    ActionPalette(QWidget *parent = 0);

    virtual ~ActionPalette();

    /**
     * @brief 设置所有按钮的图标尺寸，并记住该尺寸供后续新建按钮复用。
     * @param size 图标像素尺寸
     */
    void setButtonIconSize(const QSize &size);
    /**
     * @brief 设置按钮文字/图标布局，并按新样式刷新内容边距。
     * @param style Qt::ToolButtonStyle 取值
     */
    void setToolButtonStyle(Qt::ToolButtonStyle);

    /** @brief 返回当前托管的动作列表（不复制按钮）。 */
    QList<QAction *> actions();
    /**
     * @brief 重新按列表生成按钮。
     * @param actions 动作列表
     * @note 只清空动作到按钮的映射，不会删除已存在的按钮控件。
     */
    virtual void setActions(QList<QAction *> actions);
    /**
     * @brief 为当前按钮建立互斥的 QButtonGroup 并把点击编号转成 buttonGroupClicked()。
     * @note 重复调用会替换按钮组；通常只在初始化时调用一次。
     */
    void groupActions();
    /**
     * @brief 为单个动作创建按钮并加入布局。
     * @param action 要展示的动作
     */
    virtual void addAction(QAction *action);

    /**
     * @brief 是否在左上角绘制关闭图标。
     * @param closable true 时绘制，鼠标点击该区域会触发 close()
     */
    void setClosable(bool closable);
    /**
     * @brief 设置点击任一按钮后是否自动关闭。
     * @param autoClose true 表示点击后调用 close()
     */
    void setAutoClose(bool autoClose);

    /**
     * @brief 是否把关闭动作完全交给外部处理。
     * @param customCloseProcessing true 时 close() 只发 closed()，不隐藏自身；
     *        KeyboardPalette 借此让虚拟键盘的显隐始终由对应 QAction 驱动。
     */
    void setCustomCloseProcessing(bool customCloseProcessing);

    /** @brief 内容边距，随是否可关闭而不同（关闭图标需要额外空间）。 */
    virtual int border();
    /** @brief 删除布局中的全部按钮控件并清空动作与按钮列表。 */
    virtual void clearLayout();
    /** @brief 返回当前按钮图标尺寸。 */
    QSize buttonSize();

    /**
     * @brief 查找某个动作对应的按钮。
     * @param action 已加入本调色板的动作
     * @return 对应按钮，未找到时为 nullptr
     */
    virtual ActionPaletteButton *getButtonFromAction(QAction *action);

public slots:
    /** @brief 发出 closed()；默认先隐藏自身，除非启用了自定义关闭处理。 */
    void close();

signals:
    /** @brief 关闭动作已发生；无论是否真的隐藏都会发出。 */
    void closed();
    /** @brief 分组按钮被点击，id 为按钮在组内的序号。 */
    void buttonGroupClicked(int id);
    /** @brief 调色板收到并放行了鼠标释放事件，供外部做额外处理。 */
    void customMouseReleased();

protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    /**
     * @brief 建立按钮布局方向并初始化按钮状态。
     * @param orientation 排列方向
     */
    virtual void init(Qt::Orientation orientation);

    /** @brief 按按钮样式调整布局边距并请求重绘；子类可覆盖以使用自己的布局。 */
    virtual void updateLayout();

    /**
     * @brief 新建一个 ActionPaletteButton 并接好点击/动作变化信号。
     * @param action 按钮绑定的动作
     * @param parent 按钮父控件
     * @return 新按钮；已登记到内部列表和动作映射
     */
    ActionPaletteButton *createPaletteButton(QAction *action, QWidget *parent);

    /** @brief 当前按钮列表（只读引用）。 */
    const QList<ActionPaletteButton *> &buttons() const;
    /** @brief 只清空按钮列表，不删除控件。 */
    void clearButtons();
    /** @brief 返回按钮组，未分组时为 nullptr。 */
    QButtonGroup *buttonGroup() const;
    /** @brief 替换按钮组指针，不接管其生命周期。 */
    void setButtonGroup(QButtonGroup *group);
    /** @brief 当前按钮文字/图标样式。 */
    Qt::ToolButtonStyle toolButtonStyle() const;
    /** @brief 仅把动作追加到内部列表，不创建按钮。 */
    void appendAction(QAction *action);
    /** @brief 清空内部动作列表，不触碰按钮。 */
    void clearActions();
    /** @brief 清空动作到按钮的映射。 */
    void clearButtonMap();
    /** @brief 复位按钮相关成员（尺寸、可关闭、自动关闭、按钮组与样式）。 */
    void initButtonState();

private:
    struct Private;
    std::unique_ptr<Private> d;

protected slots:
    /** @brief 按钮点击回调；仅在 autoClose 为真时关闭调色板。 */
    void buttonClicked();
    /** @brief 动作可见性变化时同步按钮可见性；按列表下标一一对应。 */
    void actionChanged();
};

/**
 * @brief 调色板内的透明工具按钮，直接绑定一个 QAction。
 *
 * 由 ActionPalette::createPaletteButton() 创建，父控件是调色板自身。图标、文本和
 * 可用状态都由 QAction 提供，因此外部只需维护动作。整个按钮矩形都可点击，且不
 * 参与键盘焦点，避免打断白板的输入焦点。
 */
class ActionPaletteButton : public QToolButton
{
    Q_OBJECT

public:
    /**
     * @brief 构造按钮并绑定动作。
     * @param action 要展示的动作
     * @param parent 父控件
     */
    ActionPaletteButton(QAction *action, QWidget *parent = 0);
    virtual ~ActionPaletteButton();

signals:
    /** @brief 按钮被双击。 */
    void doubleClicked();

protected:
    virtual void mouseDoubleClickEvent(QMouseEvent *event);
    /** @brief 始终返回 true，使图标外的空白区域也能响应点击。 */
    virtual bool hitButton(const QPoint &pos) const;
};
