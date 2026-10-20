/**
 * @file GraphicsRuler.h
 * @brief 直尺图元声明。
 */

#pragma once

#include <QtWidgets>
#include <QtSvg>
#include <QtSvgWidgets>

#include "utils/Enums.h"
#include "whiteboard/Item.h"
#include "whiteboard/AbstractDrawRuler.h"

class GraphicsScene;


/**
 * @brief 可移动、可旋转、可沿尺边画线的直尺图元。
 *
 * 继承 AbstractDrawRuler 复用关闭按钮与配色，继承 QGraphicsRectItem 提供矩形
 * 本体。悬停时显示缩放/旋转手柄；沿长边拖动时通过 StartLine/DrawLine/EndLine
 * 产生一条直线笔画。生命周期归场景所有，仅 GUI 线程交互。
 */
class GraphicsRuler : public AbstractDrawRuler, public QGraphicsRectItem, public Item {
    Q_OBJECT

public:
    GraphicsRuler();
    virtual ~GraphicsRuler();

    /// @brief 图元类型标识。
    enum { Type = GraphicsItemType::RulerItemType };

    /// @brief 返回图元类型。
    virtual int type() const { return Type; }

    /// @brief 创建一份直尺并复制几何参数。
    virtual Item *deepCopy() const;
    /// @brief 复制矩形、位置、变换、笔宽与画线方向。
    virtual void copyItemParameters(Item *copy) const;

    /// @brief 开始沿尺边画线，记录起点。
    virtual void StartLine(const QPointF &position, qreal width);
    /// @brief 更新尺边画线的终点。
    virtual void DrawLine(const QPointF &position, qreal width);
    /// @brief 结束尺边画线。
    virtual void EndLine();

protected:
    /// @brief 绘制尺身、刻度、手柄与按钮。
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *styleOption, QWidget *widget);
    /// @brief 响应位置/选中变化，维护交互状态。
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    /// @brief 按下：判断命中缩放/旋转/画线区域并进入对应状态。
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 拖动：执行缩放、旋转或画线。
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 松开：结束当前操作并按需提交撤销。
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 悬停进入时显示手柄与关闭按钮。
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    /// @brief 悬停离开时隐藏手柄与关闭按钮。
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    /// @brief 悬停移动时按区域切换缩放/旋转/画线光标。
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
    /// @brief 绘制厘米刻度与数字。
    void paintGraduations(QPainter *painter);

private:
    bool mResizing; ///< 是否正在拖拽缩放
    bool mRotating; ///< 是否正在旋转

    /// @brief 绘制尺身半透明底色与边缘。
    void fillBackground(QPainter *painter);
    /// @brief 绘制旋转中心标记。
    void paintRotationCenter(QPainter *painter);
    /// @brief 按角度旋转直尺（度）。
    virtual void rotateAroundCenter(qreal angle);

    QGraphicsSvgItem *mRotateSvgItem; ///< 旋转手柄图元
    QGraphicsSvgItem *mResizeSvgItem; ///< 缩放手柄图元

    /// @brief 按当前方向与缩放选择缩放光标。
    void updateResizeCursor();
    /// @brief 当前缩放光标。
    QCursor resizeCursor() const { return mResizeCursor; }

    /// @brief 旋转中心，取尺身中心。
    virtual QPointF rotationCenter() const;
    /// @brief 缩放手柄矩形。
    virtual QRectF resizeButtonRect() const;
    /// @brief 关闭按钮矩形。
    virtual QRectF closeButtonRect() const;
    /// @brief 旋转手柄矩形。
    virtual QRectF rotateButtonRect() const;
    /// @brief 所在场景。
    virtual GraphicsScene *scene() const;

    QCursor mResizeCursor; ///< 缩放光标

    int drawLineDirection; ///< 沿尺边画线时相对长边的方向（1/-1）

    static const QRect sDefaultRect; ///< 新建直尺的默认几何

    static const int sMinLength = 150;   ///< 最短长度（约 3 cm）
    static const int sMaxLength = 35000; ///< 最长长度（约 700 cm）

    qreal mStrokeWidth; ///< 沿尺边画线时的笔宽
};
