/**
 * @file GraphicsTriangle.h
 * @brief 三角板图元声明。
 */

#pragma once

#include <QtWidgets>
#include <QtSvg>
#include <QtSvgWidgets>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsPolygonItem>

#include "utils/Enums.h"
#include "whiteboard/Item.h"
#include "whiteboard/AbstractDrawRuler.h"

class GraphicsScene;
class Item;

/**
 * @brief 直角三角形图元：支持四个直角朝向、双向缩放、旋转、翻转与沿斜边画线。
 *
 * 继承 AbstractDrawRuler 复用关闭按钮与配色，继承 QGraphicsPolygonItem 保存
 * 三角形顶点。悬停时显示缩放/翻转/旋转手柄，朝向导变化时重算顶点数组。
 * 生命周期归场景所有，仅 GUI 线程使用。
 */
class GraphicsTriangle : public AbstractDrawRuler, public QGraphicsPolygonItem, public Item
{
    Q_OBJECT

public:
    GraphicsTriangle();
    virtual ~GraphicsTriangle();

    /// @brief 图元类型标识。
    enum
    {
        Type = GraphicsItemType::TriangleItemType
    };

    /// @brief 返回图元类型。
    virtual int type() const
    {
        return Type;
    }

    /// @brief 创建一份三角板并复制几何参数。
    virtual Item *deepCopy(void) const;
    /// @brief 复制矩形、朝向、变换与保留的包围矩形。
    virtual void copyItemParameters(Item *copy) const;

    /// @brief 开始沿斜边画线。
    virtual void StartLine(const QPointF &scenePos, qreal width);
    /// @brief 更新沿斜边画线的终点。
    virtual void DrawLine(const QPointF &position, qreal width);
    /// @brief 结束沿斜边画线。
    virtual void EndLine();

    /// @brief 直角所在角。
    enum Orientation
    {
        BottomLeft = 0, ///< 直角在左下
        BottomRight,    ///< 直角在右下
        TopLeft,        ///< 直角在左上
        TopRight        ///< 直角在右上
    };

    /**
     * @brief 把持久化字符串转换为朝向。
     * @param str "BottomLeft"/"BottomRight"/"TopLeft"/"TopRight" 之一。
     * @return 未知字符串返回 sDefaultOrientation。
     */
    static Orientation orientationFromStr(const QStringView &str)
    {
        if (str == "BottomLeft")
        {
            return BottomLeft;
        }
        if (str == "BottomRight")
        {
            return BottomRight;
        }
        if (str == "TopLeft")
        {
            return TopLeft;
        }
        if (str == "TopRight")
        {
            return TopRight;
        }
        return sDefaultOrientation;
    }
    /// @brief 把朝向转换为持久化字符串；非法值返回空串。
    static QString orientationToStr(Orientation orientation)
    {
        QString result;
        if (orientation == 0)
        {
            result = "BottomLeft";
        }
        else if (orientation == 1)
        {
            result = "BottomRight";
        }
        else if (orientation == 2)
        {
            result = "TopLeft";
        }
        else if (orientation == 3)
        {
            result = "TopRight";
        }

        return result;
    }

    /// @brief 以矩形与朝向设置几何。
    void setRect(const QRectF &rect, Orientation orientation)
    {
        setRect(rect.x(), rect.y(), rect.width(), rect.height(), orientation);
    }
    /// @brief 以坐标尺寸与朝向设置几何，并重算三个顶点。
    void setRect(qreal x, qreal y, qreal w, qreal h, Orientation orientation);
    /// @brief 切换朝向并重算顶点。
    void setOrientation(Orientation orientation);
    /// @brief 当前朝向。
    Orientation getOrientation() const
    {
        return mOrientation;
    }
    /// @brief 三角形外接矩形。
    QRectF rect() const
    {
        return boundingRect();
    }

    /// @brief 所在场景。
    GraphicsScene *scene() const;

protected:
    /// @brief 按当前朝向与缩放选择光标。
    void updateResizeCursor();

    /// @brief 绘制三角形、刻度、手柄与按钮。
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *styleOption, QWidget *widget);
    /// @brief 图元命中区域。
    virtual QPainterPath shape() const;

    /// @brief 绕中心旋转（度）。
    virtual void rotateAroundCenter(qreal angle);

    /// @brief 旋转中心，取三角形外接矩形中心。
    virtual QPointF rotationCenter() const;

    /// @brief 关闭按钮矩形。
    virtual QRectF closeButtonRect() const;
    /// @brief 第一缩放手柄（直角边一侧）的多边形。
    QPolygonF resize1Polygon() const;
    /// @brief 第二缩放手柄（斜边一侧）的多边形。
    QPolygonF resize2Polygon() const;
    /// @brief 水平翻转按钮矩形。
    QRectF hFlipRect() const;
    /// @brief 垂直翻转按钮矩形。
    QRectF vFlipRect() const;
    /// @brief 旋转手柄矩形。
    QRectF rotateRect() const;

    /// @brief 兼容旧接口的包围矩形。
    QRectF bounding_Rect() const;

    /// @brief 第一缩放手柄光标。
    QCursor resizeCursor1() const;
    /// @brief 第二缩放手柄光标。
    QCursor resizeCursor2() const;

    /// @brief 翻转按钮光标。
    QCursor flipCursor() const;

    /// @brief 按下：判断命中缩放/翻转/旋转/画线区域。
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 拖动：执行缩放、翻转、旋转或画线。
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 松开：结束当前操作并按需提交撤销。
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    /// @brief 悬停进入时显示手柄与按钮。
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    /// @brief 悬停离开时隐藏手柄与按钮。
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    /// @brief 悬停移动时按区域切换光标。
    virtual void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
    /// @brief 绘制厘米刻度。
    void paintGraduations(QPainter *painter);

private:
    QCursor mResizeCursor1; ///< 第一缩放手柄光标
    QCursor mResizeCursor2; ///< 第二缩放手柄光标

    /// @brief 计算旋转手柄所用的变换。
    QTransform calculateRotationTransform();
    qreal      angle; ///< 旋转累计角度（度）
    /// @brief 绕指定中心应用旋转变换。
    void rotateAroundCenter(QTransform &transform, QPointF center);

    bool  mResizing1;                             ///< 是否正在拖动第一缩放手柄
    bool  mResizing2;                             ///< 是否正在拖动第二缩放手柄
    bool  mRotating;                              ///< 是否正在旋转
    QRect lastRect;                               ///< 上一次几何矩形，用于增量缩放

    QPoint lastPos;                               ///< 上一次鼠标位置

    QRectF bounds_rect;                           ///< 最近一次保存的包围矩形

    QGraphicsSvgItem *mHFlipSvgItem;              ///< 水平翻转按钮图元
    QGraphicsSvgItem *mVFlipSvgItem;              ///< 垂直翻转按钮图元
    QGraphicsSvgItem *mRotateSvgItem;             ///< 旋转手柄图元

    static const QRect       sDefaultRect;        ///< 新建三角板的默认几何
    static const Orientation sDefaultOrientation; ///< 默认朝向

    Orientation mOrientation;                     ///< 当前朝向

    QPointF A1, B1, C1, A2, B2, C2;               ///< 两套缩放基准顶点（斜边两端与直角顶点）
    qreal   C;                                    ///< 中间计算的边长/比例系数
    qreal   W1, H1;                               ///< 第一缩放手柄的参考宽高
    QPointF CC;                                   ///< 参考顶点
    /// @brief 按给定矩形与当前朝向计算三个顶点。
    void calculatePoints(const QRectF &rect);

    bool mShouldPaintInnerTriangle;      ///< 是否绘制内侧刻度三角形

    static const int d            = 70;  ///< 刻度边距
    static const int sArrowLength = 30;  ///< 朝向箭头长度
    static const int sMinWidth    = 240; ///< 最小宽度
    static const int sMinHeight   = 120; ///< 最小高度
    qreal            mStrokeWidth;       ///< 沿斜边画线时的笔宽

    /// @brief 判断矩形是否落在三角形内部（strict 控制边界处理）。
    bool contains(const QRectF &rect, bool strict = true);
};
