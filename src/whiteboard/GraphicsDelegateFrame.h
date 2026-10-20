/**
 * @file GraphicsDelegateFrame.h
 * @brief 声明图形项的选中框架：绘制边框、缩放/旋转把手并处理拖动缩放。
 */

#pragma once

#include <QtWidgets>
#include "utils/Enums.h"

class QGraphicsSceneMouseEvent;
class GraphicsItemDelegate;
class QGraphicsSvgItem;

/**
 * @brief 图形项的选择框与缩放/旋转交互层。
 *
 * 框架是一个矩形图形项，由 GraphicsItemDelegate 创建并作为其被委托项的兄弟/子项存在，
 * 绘制选中边框、四角/四边缩放把手和旋转按钮。鼠标事件先由框架接管，转换成对委托项
 * 的平移、缩放、旋转操作，再把结果写回被委托项。
 *
 * 坐标约定：框架通常与被委托项处于同一父坐标系，使用 mInitialTransform 记录拖动开始时的
 * 变换，并在整个拖动过程中以“起始状态 + 累计增量”的方式重建变换，避免逐帧累积误差。
 * 框架不拥有被委托项；QObject 基类仅用于 connect 信号槽（本类未声明 Q_OBJECT，因此不能
 * 定义自有信号槽）。
 */
class GraphicsDelegateFrame : public QGraphicsRectItem, public QObject
{
public:
    /**
     * @brief 构造框架。
     * @param pDelegate 所属委托，不能为空，框架不接管其所有权。
     * @param pRect 框架初始矩形（通常为被委托项的包围盒）。
     * @param pFrameWidth 边框宽度，随视图缩放会按 antiScale 调整。
     * @param respectRatio 缩放时是否保持宽高比。
     * @param hasTitleBar 是否在顶部保留标题栏高度（用于带标题的媒体项等）。
     */
    GraphicsDelegateFrame(GraphicsItemDelegate *pDelegate, QRectF pRect, qreal pFrameWidth, bool respectRatio = true, bool hasTitleBar = false);
    virtual ~GraphicsDelegateFrame();

    /** @brief 绘制选中边框；不绘制填充，仅描边。 */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

    /** @brief 返回边框与把手的可点击区域，用于精确命中测试。 */
    QPainterPath shape() const;

    /**
     * @brief 按下时判定命中的工具（移动/旋转/各方向缩放）并记录起始状态。
     * @note 命中旋转按钮时进入旋转模式，命中边角把手时进入对应缩放模式。
     */
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    /** @brief 返回缩放过程中保持不动的锚点（对角或对边），由当前缩放方向决定。 */
    QPointF getFixedPointFromPos();
    /** @brief 根据本次鼠标位移计算缩放增量向量。 */
    QSizeF getResizeVector(qreal moveX, qreal moveY);
    /**
     * @brief 按当前缩放方向调整被委托项的尺寸。
     * @return 实际应用的尺寸变化量。
     */
    QSizeF resizeDelegate(qreal moveX, qreal moveY);
    /** @brief 拖动过程中更新平移/缩放/旋转结果并应用到被委托项。 */
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    /** @brief 结束拖动，提交撤销步骤并复位内部状态。 */
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    /** @brief 依据被委托项的位置与尺寸重新摆放框架及把手。 */
    void positionHandles();
    /** @brief 设置框架可见性；不可见时同时隐藏全部把手与旋转按钮。 */
    void setVisible(bool visible);

    /** @brief 设置反缩放系数，使边框和把手在缩放视图中保持恒定屏幕尺寸。 */
    virtual void setAntiScale(qreal pAntiScale);

    /** @brief 框架当前的缩放模式。 */
    enum OperationMode
    {
        Scaling,             ///< 等比/自由缩放
        Resizing,            ///< 调整尺寸（可能改变宽高比）
        ResizingHorizontally ///< 仅水平方向调整
    };
    /** @brief 设置缩放模式，影响 resizeDelegate 对宽高的处理方式。 */
    void setOperationMode(OperationMode pMode)
    {
        mOperationMode = pMode;
    }
    /** @brief 是否正在缩放。 */
    bool isResizing()
    {
        return mResizing;
    }
    /**
     * @brief 把同组的其他框架按给定位移一起移动。
     * @param movingVector 本次移动向量。
     * @param bLinked 为 true 时按链接关系递归移动。
     */
    void moveLinkedItems(QLineF movingVector, bool bLinked = false);
    /** @brief 记录一组将被一起移动的框架，并保存它们的起始位置。 */
    void prepareFramesToMove(QList<GraphicsDelegateFrame *> framesToMove);
    /** @brief 收集与本框架链接的框架，准备联动移动。 */
    void prepareLinkedFrameToMove();
    /** @brief 返回与本框架链接的框架列表。 */
    QList<GraphicsDelegateFrame *> getLinkedFrames();

private:
    QRectF bottomRightResizeGripRect() const;
    QRectF bottomResizeGripRect() const;
    QRectF leftResizeGripRect() const;
    QRectF rightResizeGripRect() const;
    QRectF topResizeGripRect() const;
    QRectF rotateButtonBounds() const;

    inline bool resizingBottomRight() const
    {
        return mCurrentTool == ResizeBottomRight;
    }
    inline bool resizingBottom() const
    {
        return mCurrentTool == ResizeBottom;
    }
    inline bool resizingRight() const
    {
        return mCurrentTool == ResizeRight;
    }
    inline bool resizingLeft() const
    {
        return mCurrentTool == ResizeLeft;
    }
    inline bool resizingTop() const
    {
        return mCurrentTool == ResizeTop;
    }
    inline bool rotating() const
    {
        return mCurrentTool == Rotate;
    }
    inline bool moving() const
    {
        return mCurrentTool == Move;
    }
    void setCursorFromAngle(QString angle);
    bool canResizeBottomRight(qreal width, qreal height, qreal scaleFactor);

    QTransform buildTransform();
    void       updateResizeCursors();
    void       initializeTransform();

    enum FrameTool
    {
        None,              ///< 未命中任何工具
        Move,              ///< 移动被委托项
        Rotate,            ///< 绕中心旋转
        ResizeBottomRight, ///< 拖动右下角，固定左上角
        ResizeTop,         ///< 拖动上边，固定下边
        ResizeRight,       ///< 拖动右边，固定左边
        ResizeBottom,      ///< 拖动下边，固定上边
        ResizeLeft         ///< 拖动左边，固定右边
    };
    FrameTool toolFromPos(QPointF pos);
    void      refreshGeometry();

    FrameTool             mCurrentTool;
    GraphicsItemDelegate *mDelegate;

    bool  mVisible;
    qreal mFrameWidth;
    qreal mNominalFrameWidth;
    bool  mRespectRatio;

    qreal mAngle;
    qreal mAngleOffset;
    qreal mTotalScaleX;
    qreal mTotalScaleY;
    qreal mScaleX;
    qreal mScaleY;
    qreal mTranslateX;
    qreal mTranslateY;
    qreal mTotalTranslateX;
    qreal mTotalTranslateY;
    qreal mAngleTolerance;
    QRect mAngleRect;

    QPointF    mStartingPoint;
    QTransform mInitialTransform;
    QSizeF     mOriginalSize;
    QPointF    mFixedPoint;

    QGraphicsSvgItem *mBottomRightResizeGripSvgItem;
    QGraphicsSvgItem *mBottomResizeGripSvgItem;
    QGraphicsSvgItem *mLeftResizeGripSvgItem;
    QGraphicsSvgItem *mRightResizeGripSvgItem;
    QGraphicsSvgItem *mTopResizeGripSvgItem;
    QGraphicsSvgItem *mRotateButton;

    QGraphicsRectItem *mBottomRightResizeGrip;
    QGraphicsRectItem *mBottomResizeGrip;
    QGraphicsRectItem *mLeftResizeGrip;
    QGraphicsRectItem *mRightResizeGrip;
    QGraphicsRectItem *mTopResizeGrip;

    OperationMode mOperationMode;

    QGraphicsItem *delegated();
    bool           mFlippedX;
    bool           mFlippedY;
    bool           mMirrorX;
    bool           mMirrorY;
    bool           mResizing;
    bool           mMirroredXAtStart;
    bool           mMirroredYAtStart;
    qreal          mTitleBarHeight;
    qreal          mNominalTitleBarHeight;

    QList<GraphicsDelegateFrame *> mLinkedFrames;
};
