/**
 * @file GraphicsCompass.h
 * @brief 圆规图元声明。
 */

#pragma once
#include <QtWidgets>
#include <QtSvg>
#include <QtSvgWidgets>

#include "utils/Enums.h"
#include "whiteboard/Item.h"

class GraphicsScene;

/**
 * @brief 可移动、缩放、旋转的圆规图元，可用铅笔端在页面上画圆/弧。
 *
 * 以 QGraphicsRectItem 表示两脚外接矩形；悬停时显示缩放/关闭按钮，
 * 拖动铅笔端进入绘制状态并生成笔画。绘制过程中的角度与半径读数会实时显示。
 * 生命周期归场景所有；仅 GUI 线程使用，绘制发生在场景 paint 流程。
 */
class GraphicsCompass : public QObject, public QGraphicsRectItem, public Item
{
    Q_OBJECT

public:
    GraphicsCompass();
    virtual ~GraphicsCompass();

    /// @brief 图元类型标识。
    enum
    {
        Type = GraphicsItemType::CompassItemType
    };

    /// @brief 返回图元类型。
    virtual int type() const
    {
        return Type;
    }

    /// @brief 创建一份圆规并复制几何参数。
    virtual Item *deepCopy() const;

    /// @brief 复制矩形、位置、变换与状态到目标实例。
    virtual void copyItemParameters(Item *copy) const;

protected:
    /// @brief 绘制两脚、铰链、铅笔、读数与按钮。
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *styleOption, QWidget *widget);
    /// @brief 响应几何变化，维护缩放状态。
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    /// @brief 按下：判断命中缩放按钮还是绘制起点。
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 拖动：执行缩放或绘制圆/弧。
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 松开：结束缩放或绘制。
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 悬停进入时显示按钮。
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    /// @brief 悬停离开时隐藏按钮。
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    /// @brief 悬停移动时按区域切换光标。
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);

private slots:
    /// @brief 画笔颜色变化时刷新绘制颜色。
    void penColorChanged();
    /// @brief 线宽变化时刷新读数区域。
    void lineWidthChanged();

private:
    /// @brief 绘制角度读数。
    void paintAngleDisplay(QPainter *painter);
    /// @brief 绘制半径读数。
    void paintRadiusDisplay(QPainter *painter);
    /// @brief 绕针尖旋转指定角度。
    void rotateAroundNeedle(qreal angle);
    /// @brief 根据当前夹角与半径构造并绘制圆弧。
    void drawArc();
    /// @brief 更新缩放光标。
    void updateResizeCursor();
    /// @brief 更新绘制光标。
    void updateDrawCursor();
    /// @brief 绘制针尖中心十字。
    void paintCenterCross();

    /// @brief 移动光标。
    QCursor moveCursor() const;
    /// @brief 缩放光标。
    QCursor resizeCursor() const;
    /// @brief 旋转光标。
    QCursor rotateCursor() const;
    /// @brief 关闭按钮光标。
    QCursor closeCursor() const;
    /// @brief 绘制状态光标。
    QCursor drawCursor() const;
    /// @brief 铰链区域。
    QRectF hingeRect() const;
    /// @brief 针尖位置（本地坐标）。
    QPointF needlePosition() const;
    /// @brief 铅笔端位置（本地坐标）。
    QPointF pencilPosition() const;
    /// @brief 关闭按钮矩形。
    QRectF closeButtonRect() const;
    /// @brief 缩放按钮矩形。
    QRectF resizeButtonRect() const;
    /// @brief 图元命中区域。
    virtual QPainterPath shape() const;
    /// @brief 针尖处图形的命中区域。
    QPainterPath needleShape() const;
    /// @brief 铅笔端图形的命中区域。
    QPainterPath pencilShape() const;
    /// @brief 针尖底部图形的命中区域。
    QPainterPath needleBaseShape() const;
    /// @brief 针尖臂的命中区域。
    QPainterPath needleArmShape() const;
    /// @brief 铰链的命中区域。
    QPainterPath hingeShape() const;
    /// @brief 铅笔底部图形的命中区域。
    QPainterPath pencilBaseShape() const;
    /// @brief 铅笔臂的命中区域。
    QPainterPath pencilArmShape() const;
    /// @brief 所在场景。
    GraphicsScene *scene() const;
    /// @brief 前景绘制色，随背景明暗切换。
    QColor drawColor() const;
    /// @brief 中部填充色。
    QColor middleFillColor() const;
    /// @brief 边缘填充色。
    QColor edgeFillColor() const;
    /// @brief 读数文字字体。
    QFont font() const;
    /// @brief 两脚当前夹角（度）。
    qreal angleInDegrees() const;

    QCursor           mOuterCursor;                               ///< 外框光标
    QCursor           mResizeCursor;                              ///< 缩放光标
    QCursor           mDrawCursor;                                ///< 绘制光标
    bool              mResizing;                                  ///< 是否正在缩放
    bool              mRotating;                                  ///< 是否正在旋转
    bool              mDrawing;                                   ///< 是否正在绘制圆/弧
    bool              mShowButtons;                               ///< 是否显示按钮
    qreal             mSpanAngleInDegrees;                        ///< 两脚夹角（度）
    QPointF           mSceneArcStartPoint;                        ///< 圆弧起点（场景坐标）
    bool              mDrewCircle;                                ///< 是否已画出闭合圆
    QGraphicsSvgItem *mCloseSvgItem;                              ///< 关闭按钮图元
    QGraphicsSvgItem *mResizeSvgItem;                             ///< 缩放按钮图元
    qreal             mAntiScaleRatio;                            ///< 反缩放系数
    bool              mDrewCenterCross;                           ///< 是否已绘制针尖十字

    static const int sNeedleLength        = 12;                   ///< 针尖长度
    static const int sNeedleWidth         = 3;                    ///< 针尖宽度

    static const int sNeedleBaseLength    = 9;                    ///< 针尖底座长度
    static const int sNeedleBaseWidth     = 9;                    ///< 针尖底座宽度

    static const int sNeedleArmLeftWidth  = 12;                   ///< 针臂左缘宽度
    static const int sNeedleArmRigthWidth = 16;                   ///< 针臂右缘宽度

    static const int sPencilLength        = 12;                   ///< 铅笔长度
    static const int sPencilWidth         = 2;                    ///< 铅笔宽度

    static const int sPencilBaseLength    = 9;                    ///< 铅笔底座长度
    static const int sPencilBaseWidth     = 9;                    ///< 铅笔底座宽度

    static const int sPencilArmLeftWidth  = 16;                   ///< 铅笔臂左缘宽度
    static const int sPencilArmRightWidth = 12;                   ///< 铅笔臂右缘宽度

    static const int sCornerRadius        = 2;                    ///< 圆角半径

    static const QRect  sDefaultRect;                             ///< 新建圆规的默认几何
    static const int    sMinRadius;                               ///< 最小半径
    static const int    sDegreeToQtAngleUnit = 16;                ///< 度到 Qt 角度单位的换算系数
    static const int    sFillTransparency    = 127;               ///< 填充透明度
    static const int    sDrawTransparency    = 192;               ///< 前景透明度
    static const QColor sLightBackgroundEdgeFillColor;            ///< 浅色背景边缘填充色
    static const QColor sLightBackgroundMiddleFillColor;          ///< 浅色背景中部填充色
    static const QColor sLightBackgroundDrawColor;                ///< 浅色背景前景色
    static const QColor sDarkBackgroundEdgeFillColor;             ///< 深色背景边缘填充色
    static const QColor sDarkBackgroundMiddleFillColor;           ///< 深色背景中部填充色
    static const QColor sDarkBackgroundDrawColor;                 ///< 深色背景前景色
    static const int    sDisplayRadiusOnPencilArmMinLength = 300; ///< 铅笔臂显示半径读数的最小臂长
    static const int    sDisplayRadiusUnitMinLength        = 250; ///< 显示半径单位的最小臂长
};
