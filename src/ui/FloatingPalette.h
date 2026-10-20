/**
 * @file FloatingPalette.h
 * @brief 提供可拖动、可最小化并自动收进容器边缘的浮动调色板基类。
 */

#pragma once

#include <QWidget>
#include <QPoint>

#include <memory>

/** 浮动调色板被拖到父控件哪条边时收起。 */
typedef enum
{
    eMinimizedLocation_None,
    eMinimizedLocation_Left,
    eMinimizedLocation_Top,
    eMinimizedLocation_Right,
    eMinimizedLocation_Bottom
} eMinimizedLocation;

/**
 * @brief 浮动调色板基类：圆角背景、拖动、贴边收起的公共实现。
 *
 * 界面是悬浮在白板上的半透明圆角面板。ActionPalette、PropertyPalette 等工具面板
 * 都继承它；BoardPaletteManager 创建的具体调色板以白板容器为父控件，无父控件时
 * 则表现为置顶独立窗口。拖动时会被限制在父控件范围内，靠近左右边缘会发出
 * minimizeStart() 让调用方收起，拖离边缘后再发出 maximizeStart() 恢复。
 * 同组调色板可通过 addAssociatedPalette() 建立关联，一起对齐宽度和位置。
 */
class FloatingPalette : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造调色板。
     * @param position 默认停靠角，决定未被用户拖动时贴左还是贴右
     * @param parent 父控件；为空时创建置顶、无边框的独立窗口
     */
    FloatingPalette(Qt::Corner = Qt::TopLeftCorner, QWidget *parent = 0);
    virtual ~FloatingPalette();

    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

    /**
     * @brief 关联另一个调色板，使二者宽度对齐、水平位置同步。
     * @param pOtherPalette 另一个调色板
     */
    void addAssociatedPalette(FloatingPalette *pOtherPalette);
    /**
     * @brief 解除与另一个调色板的关联。
     * @param pOtherPalette 另一个调色板
     */
    void removeAssociatedPalette(FloatingPalette *pOtherPalette);

    /**
     * @brief 按内容重新调整尺寸，并与关联调色板对齐。
     * @param pUp true 表示尺寸变化时向上扩展（保持底边），false 向下扩展
     */
    virtual void adjustSizeAndPosition(bool pUp = true);

    /**
     * @brief 标记位置是否由用户指定。
     * @param pFlag true 表示自定义位置，同时解除所有关联调色板
     */
    void setCustomPosition(bool pFlag);

    /** @brief 首选尺寸：sizeHint() 无效时退回子控件包围盒。 */
    QSize preferredSize();

    /**
     * @brief 设置背景画刷并重绘。
     * @param brush 新画刷
     */
    void setBackgroundBrush(const QBrush &brush);
    /**
     * @brief 是否绘制外圈边框握把。
     * @param newGrip true 绘制
     */
    void setGrip(bool newGrip);

    /**
     * @brief 是否允许贴边收起。
     * @param permission true 允许
     */
    void setMinimizePermission(bool permission);

protected:
    virtual void enterEvent(QEvent *event);
    virtual void showEvent(QShowEvent *event);
    virtual void paintEvent(QPaintEvent *event);

    /** @brief 圆角半径。 */
    virtual int radius();
    /** @brief 内容与边缘的距离。 */
    virtual int border();
    /** @brief 握把宽度（当前仅作保留接口）。 */
    virtual int gripSize();

    static const int sLayoutContentMargin = 12;
    static const int sLayoutSpacing       = 15;
    /**
     * @brief 把窗口移动到父控件内，并处理贴边收起逻辑。
     * @param position 目标位置（父控件坐标）
     */
    void moveInsideParent(const QPoint &position);

    /** @brief 子类可返回右侧额外占用的宽度，用于计算右贴边位置。 */
    virtual int getParentRightOffset();

    /** @brief 当前收起位置。 */
    eMinimizedLocation minimizedLocation();

    /**
     * @brief 把背景色改为指定透明度。
     * @param alpha 0–255
     */
    void setBackgroundAlpha(int alpha);
    /** @brief 只改握把标志，不触发重绘。 */
    void setGripVisible(bool visible);
    /** @brief 是否正在拖动。 */
    bool isMoving() const;
    /** @brief 只改自定义位置标志，不解除关联。 */
    void setCustomPositionFlag(bool pFlag);

private:
    /** @brief 解除与所有关联调色板的双向关联。 */
    void removeAllAssociatedPalette();
    /**
     * @brief 根据位置判断是否应发出收起/恢复信号。
     * @param pos 当前窗口位置
     */
    void minimizePalette(const QPoint &pos);

    struct Private;
    std::unique_ptr<Private> d;

signals:
    /** @brief 鼠标进入调色板。 */
    void mouseEntered();
    /**
     * @brief 调色板贴到某条边，请求收起。
     * @param location 贴边位置
     */
    void minimizeStart(eMinimizedLocation location);
    /** @brief 从收起状态被拖离边缘，请求恢复。 */
    void maximizeStart();
    /** @brief 已完成恢复。 */
    void maximized();
    /** @brief 正在拖动。 */
    void moving();
};
