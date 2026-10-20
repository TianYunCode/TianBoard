/**
 * @file SelectionFrame.h
 * @brief 选中框图元声明。
 */

#pragma once

#include <QGraphicsRectItem>
#include <QtWidgets>
#include "utils/Enums.h"

#include "whiteboard/GraphicsScene.h"

class DelegateButton;
class GraphicsItemDelegate;

/**
 * @brief 围绕选中图元的可交互边框：绘制描边、放置删除/复制/层级/旋转等按钮，
 * 并在拖动时整体移动或旋转被选图元。
 *
 * 由场景在选中集合变化时创建/更新（场景持有），按钮是它的子图元。
 * 移动与旋转在松开时通过委托的 commitUndoStep() 合并为一条宏命令；
 * 缩放时用反缩放系数保持按钮的屏幕尺寸恒定。
 *
 * 线程约束：所有交互、绘制与撤销操作都在 GUI 线程。
 */
class SelectionFrame : public QObject, public QGraphicsRectItem
{
    Q_OBJECT

public:
    enum
    {
        om_idle,
        om_moving,
        om_rotating
    } mOperationMode; ///< 当前鼠标操作模式（空闲/移动/旋转）
    /// @brief 图元类型标识。
    enum
    {
        Type = GraphicsItemType::SelectionFrameType
    };

    SelectionFrame();

    /// @brief 只绘制边框环带，内部挖空以免遮挡被选内容。
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    /// @brief 在矩形外扩一个描边宽度，容纳边框与按钮。
    QRectF boundingRect() const;
    /// @brief 命中区域与可见的边框环带一致。
    QPainterPath shape() const;

    /// @brief 设置边框画刷。
    void setLocalBrush(const QBrush &pBrush)
    {
        mLocalBrush = pBrush;
    }
    /// @brief 当前边框画刷。
    QBrush localBrush() const
    {
        return mLocalBrush;
    }
    /**
     * @brief 根据一组图元重建选中框：合并包围区域、汇总能力标志并放置按钮。
     * @param pGraphicsItems 当前选中的图元集合；其中任一图元被锁定时整框视为锁定。
     * @note 集合为空时隐藏选中框。
     */
    void setEnclosedItems(const QList<QGraphicsItem *> pGraphicsItems);
    /// @brief 被选图元几何变化后重算包围矩形并重新摆放按钮。
    void updateRect();
    /// @brief 按当前缩放反向缩放选中框，保持视觉厚度恒定。
    void updateScale();
    /// @brief 包围矩形是否为空（无有效选中内容）。
    bool isEmpty() const
    {
        return this->rect().isEmpty();
    }
    /// @brief 返回图元类型。
    virtual int type() const
    {
        return Type;
    }

protected:
    /// @brief 记录按下位置，并根据命中的按钮决定进入移动还是旋转模式。
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 按模式平移或旋转所有被选图元，并刷新选中框与按钮。
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 结束拖动，把各图元的变换合并为一条可撤销的宏命令。
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private slots:
    /// @brief 由场景的缩放通知更新反缩放系数。
    void setAntiScaleRatio(qreal pAntiscaleRatio)
    {
        mAntiscaleRatio = pAntiscaleRatio;
    }
    /// @brief 缩放变化时重算反缩放系数与按钮位置。
    void onZoomChanged(qreal pZoom);
    /// @brief 删除所有被选图元（可撤销）。
    void remove();
    /// @brief 复制所有被选图元，按原 Z 序重建相对层级。
    void duplicate();
    /// @brief 把选中图元上移一层。
    void increaseZlevelUp();
    /// @brief 把选中图元置于顶层。
    void increaseZlevelTop();
    /// @brief 把选中图元下移一层。
    void increaseZlevelDown();
    /// @brief 把选中图元置于底层。
    void increaseZlevelBottom();
    /// @brief 把选中图元组合为一个组容器。
    void groupItems();

private:
    /// @brief 为一次层级调整压入撤销命令。
    void addSelectionUndo(QList<QGraphicsItem *> items, ZLayerController::moveDestination dest);
    /// @brief 计算按钮位置并挂到选中框下。
    void placeButtons();
    /// @brief 摆放不参与常规行列排布的按钮（如旋转按钮）。
    void placeExceptionButton(DelegateButton *pButton, QTransform pTransform);
    /// @brief 把所有按钮从选中框上摘下并隐藏。
    void clearButtons();
    /// @brief 边框厚度按反缩放后的像素值。
    inline int adjThickness() const
    {
        return mThickness * mAntiscaleRatio;
    }
    /// @brief 把场景转换为 GraphicsScene。
    inline GraphicsScene *graphicsScene();
    /// @brief 用角度文本生成并设置旋转光标。
    void setCursorFromAngle(QString angle);

    /// @brief 按 ItemOwnZValue 排序图元，并剔除选中框自身。
    QList<QGraphicsItem *> sortedByZ(const QList<QGraphicsItem *> &pItems);
    /// @brief 按图元能力标志生成按钮集合，按钮懒创建并复用。
    QList<DelegateButton *> buttonsForFlags(GraphicsFlags fls);

    /// @brief 把内部委托列表还原成图元列表。
    QList<QGraphicsItem *> enclosedGraphicsItems();

private:
    int                           mThickness;      ///< 边框基准厚度
    qreal                         mAntiscaleRatio; ///< 反缩放系数
    QList<GraphicsItemDelegate *> mEnclosedtems;   ///< 被选图元的委托列表
    QBrush                        mLocalBrush;     ///< 边框画刷

    QPointF mPressedPos;                           ///< 按下位置
    QPointF mLastMovedPos;                         ///< 上一次移动位置
    QPointF mLastTranslateOffset;                  ///< 上一次累计平移量，避免重复位移
    qreal   mRotationAngle;                        ///< 当前累计旋转角度（调试用）

    bool mIsLocked;                                ///< 选中集合中是否有锁定图元

    QList<DelegateButton *> mButtons;              ///< 当前显示的按钮

    DelegateButton *mDeleteButton;                 ///< 删除按钮
    DelegateButton *mDuplicateButton;              ///< 复制按钮
    DelegateButton *mZOrderUpButton;               ///< 层级上移按钮
    DelegateButton *mZOrderDownButton;             ///< 层级下移按钮
    DelegateButton *mGroupButton;                  ///< 组合按钮

    DelegateButton *mRotateButton;                 ///< 旋转按钮
};
