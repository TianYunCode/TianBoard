/**
 * @file GraphicsProtractor.h
 * @brief 量角器图元声明。
 */

#pragma once

#include <QtWidgets>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsView>
#include <QtSvg>
#include <QtSvgWidgets>

#include "utils/Enums.h"
#include "whiteboard/AbstractDrawRuler.h"
#include "whiteboard/Item.h"

#include "utils/GeometryUtils.h"


class GraphicsScene;


/**
 * @brief 半圆量角器图元：可移动、缩放、旋转，并可用角度标记读出夹角。
 *
 * 继承 AbstractDrawRuler 复用关闭按钮、配色与光标，继承 QGraphicsEllipseItem
 * 表示半圆本体。悬停时在周边显示复位/缩放/旋转/标记/关闭按钮，内部以
 * Tool 状态机分派鼠标事件。生命周期归场景所有，仅 GUI 线程使用。
 */
class GraphicsProtractor : public AbstractDrawRuler, public QGraphicsEllipseItem, public Item {
    Q_OBJECT

public:
    GraphicsProtractor();
    /// @brief 鼠标命中的操作区域。
    enum Tool {
        None,      ///< 未命中任何区域
        Move,      ///< 拖动本体
        Resize,    ///< 拖动缩放手柄
        Rotate,    ///< 拖动旋转手柄
        Reset,     ///< 复位按钮
        Close,     ///< 关闭按钮
        MoveMarker ///< 拖动角度标记
    };

    /// @brief 量角器起始角（度）。
    qreal angle() { return mStartAngle; }
    /// @brief 角度标记当前所在角（度）。
    qreal markerAngle() { return mCurrentAngle; }
    /**
     * @brief 设置起始角并同步到椭圆项的角度。
     * @param angle 角度（度）。
     * @note 内部按 1 度 = 16 Qt 角度单位换算。
     */
    void setAngle(qreal angle) {
        mStartAngle = angle;
        setStartAngle(mStartAngle * 16);
    }
    /// @brief 设置角度标记位置（度）。
    void setMarkerAngle(qreal angle) { mCurrentAngle = angle; }

    /// @brief 创建一份量角器并复制几何与角度参数。
    virtual Item *deepCopy() const;
    /// @brief 复制矩形、位置、变换与角度参数。
    virtual void copyItemParameters(Item *copy) const;

    /// @brief 图元类型标识。
    enum { Type = GraphicsItemType::ProtractorItemType };

    /// @brief 返回图元类型。
    virtual int type() const { return Type; }

protected:
    /// @brief 绘制半圆本体、刻度、按钮与角度标记。
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *styleOption, QWidget *widget);

    /// @brief 响应几何/选中变化，维护交互状态。
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    /// @brief 按下：识别命中区域并记录起点。
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 拖动：按当前工具移动、缩放、旋转或移动标记。
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 松开：结束当前操作并按需提交撤销。
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 悬停进入时显示按钮。
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    /// @brief 悬停离开时隐藏按钮。
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    /// @brief 悬停移动时按区域切换光标。
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
    /// @brief 拼出按钮与本体之外的半圆命中区域。
    virtual QPainterPath shape() const;
    /// @brief 图元外接矩形。
    QRectF boundingRect() const;
    /// @brief 绘制角度刻度。
    void paintGraduations(QPainter *painter);


private:
    /// @brief 绘制周边操作按钮。
    void paintButtons(QPainter *painter);
    /// @brief 绘制角度标记（指针与读数）。
    void paintAngleMarker(QPainter *painter);
    /// @brief 根据鼠标位置判断命中的操作区域。
    Tool toolFromPos(QPointF pos);
    /// @brief 当前反缩放系数。
    qreal antiScale() const;
    /// @brief 所在场景。
    GraphicsScene *scene() const;
    /// @brief 半圆填充画刷。
    QBrush fillBrush() const;

    /// @brief 按钮参考尺寸，按半径等比例缩放。
    QSizeF buttonSizeReference() const {
        return QSizeF(radius() / 10, mCloseSvgItem->boundingRect().height() * radius() /
                                         (10 * mCloseSvgItem->boundingRect().width()));
    }
    /// @brief 角度标记的参考尺寸，按半径等比例缩放。
    QSizeF markerSizeReference() const {
        return QSizeF(radius() / 10, mMarkerSvgItem->boundingRect().height() * radius() /
                                         (10 * mMarkerSvgItem->boundingRect().width()));
    }
    /// @brief 复位按钮矩形。
    QRectF resetButtonRect() const;

    /// @brief 关闭按钮矩形。
    QRectF closeButtonRect() const;
    /// @brief 缩放手柄矩形。
    QRectF resizeButtonRect() const;
    /// @brief 旋转手柄矩形。
    QRectF rotateButtonRect() const;
    /// @brief 角度标记按钮矩形。
    QRectF markerButtonRect() const;

    /// @brief 有效半径：外接矩形高度的一半再留出边缘空白。
    inline qreal radius() const { return rect().height() / 2 - 20; }

    QPointF mPreviousMousePos; ///< 上一次鼠标位置，用于增量计算
    Tool mCurrentTool;         ///< 当前命中的操作区域
    bool mShowButtons;         ///< 是否显示操作按钮
    qreal mCurrentAngle;       ///< 角度标记当前角度
    qreal mSpan;               ///< 当前跨度角
    qreal mStartAngle;         ///< 起始角
    qreal mScaleFactor;        ///< 缩放过程中的比例因子

    QGraphicsSvgItem *mResetSvgItem;  ///< 复位按钮图元
    QGraphicsSvgItem *mResizeSvgItem; ///< 缩放手柄图元
    QGraphicsSvgItem *mMarkerSvgItem; ///< 角度标记图元
    QGraphicsSvgItem *mRotateSvgItem; ///< 旋转手柄图元

    static const QRectF sDefaultRect; ///< 新建量角器的默认几何
    static const qreal minRadius;     ///< 最小半径，防止缩放过小无法操作

    /// @brief 绕中心旋转（度）。
    virtual void rotateAroundCenter(qreal angle);
    /// @brief 旋转中心，取半圆圆心。
    virtual QPointF rotationCenter() const;

    int sFillTransparency; ///< 填充透明度
    int sDrawTransparency; ///< 前景透明度
};
