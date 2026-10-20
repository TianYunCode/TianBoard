/**
 * @file GraphicsAxes.h
 * @brief 坐标轴图元声明。
 */

#pragma once

#include <QtWidgets>
#include <QtSvg>
#include <QtSvgWidgets>

#include "utils/Enums.h"
#include "whiteboard/Item.h"

class GraphicsScene;

/**
 * @brief 可移动、可四向伸缩的直角坐标轴图元，用于数学课画坐标系。
 *
 * 轴线交点就是图元原点：x 轴沿本地图元 y=0 向右、y 轴沿 x=0 向上，两端画箭头。
 * 刻度以“页面网格尺寸”为一个单位长度（不是物理厘米），因此换页、改网格都会跟着变；
 * 单位太窄时只标 1 与 5 的倍数，避免数字挤成一团。悬停时出现关闭与数字显隐按钮，
 * 获得焦点后出现移动把手，四边可拖动伸缩，方向键可 1 像素微调。
 * 生命周期归场景所有；仅 GUI 线程使用。
 */
class GraphicsAxes : public QObject, public QGraphicsPolygonItem, public Item
{
    Q_OBJECT

public:
    GraphicsAxes();
    virtual ~GraphicsAxes();

    /// @brief 图元类型标识。
    enum
    {
        Type = GraphicsItemType::AxesItemType
    };

    /// @brief 返回图元类型。
    virtual int type() const
    {
        return Type;
    }

    /// @brief 创建一份坐标轴并复制几何参数。
    virtual Item *deepCopy() const;

    /// @brief 复制位置、范围与变换到目标实例。
    virtual void copyItemParameters(Item *copy) const;

    /// @brief 设定坐标轴范围并重算本体多边形与把手位置。
    void setRect(qreal x, qreal y, qreal w, qreal h);

    /// @brief 当前坐标轴范围（本地坐标）。
    QRectF bounds() const;

    /// @brief 显示或隐藏刻度数字。
    void setShowNumbers(bool showNumbers);

    /// @brief 是否显示刻度数字。
    bool showNumbers() const;

    /// @brief 四个伸缩方向。
    enum Direction
    {
        Left = 0,
        Right,
        Bottom,
        Top
    };

protected:
    /// @brief 绘制轴线、箭头、刻度与按钮。
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *styleOption, QWidget *widget);
    /// @brief 可见性变化时把按钮挂回本体。
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    /// @brief 按下：判定命中的是哪一个伸缩边。
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 拖动：整体移动或按命中的边伸缩。
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 松开：结束伸缩，或响应关闭/数字按钮。
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 方向键微调位置。
    virtual void keyPressEvent(QKeyEvent *event);
    /// @brief 悬停进入时显示按钮。
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    /// @brief 悬停移动时按区域切换光标。
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
    /// @brief 悬停离开时隐藏按钮。
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

private:
    /// @brief 绘制两轴的刻度线与刻度数字。
    void paintGraduations(QPainter *painter);
    /// @brief 按给定矩形伸缩一条边。
    void setRect(const QRectF &rect);

    /// @brief 重建横竖两个缩放光标。
    void updateResizeCursor();
    /// @brief 当前伸缩方向对应的光标。
    QCursor resizeCursor() const;
    /// @brief 关闭/数字按钮的光标。
    QCursor closeCursor() const;
    /// @brief 本体拖动光标。
    QCursor moveCursor() const;
    /// @brief 按悬停位置选择光标。
    void selectCursor(QGraphicsSceneHoverEvent *event);

    /// @brief 左边的伸缩热区。
    QRectF resizeLeftRect() const;
    /// @brief 右边的伸缩热区。
    QRectF resizeRightRect() const;
    /// @brief 下边的伸缩热区。
    QRectF resizeBottomRect() const;
    /// @brief 上边的伸缩热区。
    QRectF resizeTopRect() const;
    /// @brief 关闭按钮热区。
    QRectF closeButtonRect() const;
    /// @brief 数字显隐按钮热区。
    QRectF numbersButtonRect() const;
    /// @brief 移动把手的摆放位置。
    QRectF moveToolButtonRect() const;
    /// @brief x 轴（本地坐标）。
    QLineF xAxis() const;
    /// @brief y 轴（本地坐标），正方向朝上。
    QLineF yAxis() const;
    /// @brief 所在场景。
    GraphicsScene *scene() const;
    /// @brief 随背景明暗切换的绘制色。
    QColor drawColor() const;
    /// @brief 刻度数字字体。
    QFont font() const;

    QGraphicsSvgItem *mCloseSvgItem;               ///< 关闭按钮图元
    QGraphicsSvgItem *mMoveToolSvgItem;            ///< 移动把手图元
    QGraphicsSvgItem *mNumbersSvgItem;             ///< 数字显隐按钮图元

    bool      mResizing;                           ///< 是否正在伸缩
    Direction mResizeDirection;                    ///< 当前伸缩方向
    bool      mShowButtons;                        ///< 是否显示按钮
    bool      mShowNumbers;                        ///< 是否显示刻度数字

    QCursor mResizeCursorH;                        ///< 横向缩放光标
    QCursor mResizeCursorV;                        ///< 纵向缩放光标

    qreal  mAntiScaleRatio;                        ///< 反缩放系数，使把手不随视图缩放变大变小
    qreal  mUnitLength;                            ///< 一个刻度单位的像素长度
    QRectF mBounds;                                ///< 坐标轴范围（本地坐标）

    static const QRect sDefaultRect;               ///< 新建时的默认范围：原点居中，四周各 200

    static const int    sMinLength        = 50;    ///< 收缩到只剩这么多就不再允许
    static const int    sArrowLength      = 12;    ///< 箭头沿轴方向的长度
    static const int    sArrowWidth       = 5;     ///< 箭头的半宽
    static const int    sMargin           = 5;     ///< 首末刻度距轴端的留白
    static const int    sItemWidth        = 30;    ///< 本体把手的宽度
    static const int    sDrawTransparency = 255;   ///< 轴线不透明度
    static const QColor sLightBackgroundDrawColor; ///< 浅色背景下的绘制色
    static const QColor sDarkBackgroundDrawColor;  ///< 深色背景下的绘制色
};
