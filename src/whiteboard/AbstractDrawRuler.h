/**
 * @file AbstractDrawRuler.h
 * @brief 直尺、量角器、三角板等绘图尺具的公共基类声明。
 */

#pragma once

#include <QtWidgets>
#include <QGraphicsItem>
#include "utils/GeometryUtils.h"

class GraphicsScene;
class QGraphicsSvgItem;

/**
 * @brief 绘图尺具的公共基类，收敛移动、旋转、关闭按钮与配色等重复逻辑。
 *
 * 以多继承方式混入具体的 QGraphicsItem 派生类，因此本身不持有图元实体，
 * 只通过纯虚接口（scene()、rotateAroundCenter()、rotationCenter()、
 * closeButtonRect()、paintGraduations()）向派生类索取上下文。
 *
 * 生命周期：实例由对应的图元（GraphicsRuler/GraphicsProtractor/GraphicsTriangle）
 * 以成员或伴生对象方式持有，随图元一起被场景（GraphicsScene）回收；
 * 关闭按钮是图元的子图元，父子关系负责释放。
 *
 * 线程约束：所有接口只在 GUI 线程调用，绘制发生在 QGraphicsScene 的 paint 流程中。
 */
class AbstractDrawRuler : public QObject
{
    Q_OBJECT

public:
    AbstractDrawRuler();
    ~AbstractDrawRuler();

    /**
     * @brief 初始化宿主图元的交互标志，并创建关闭按钮子图元。
     * @param item 具体的尺具图元（直尺/量角器/三角板）。
     * @note 在派生类构造函数中调用一次；关闭按钮以 item 为父图元。
     */
    void create(QGraphicsItem &item);

    /// @brief 在尺边按下时记录起点，开始绘制辅助线；基类默认不做任何事。
    virtual void StartLine(const QPointF &position, qreal width);
    /// @brief 拖动过程中延伸辅助线；基类默认不做任何事。
    virtual void DrawLine(const QPointF &position, qreal width);
    /// @brief 松开后结束辅助线绘制；基类默认不做任何事。
    virtual void EndLine();

protected:
    /// @brief 供派生类 paint 调用的公共收尾：更新反缩放比例并摆放关闭按钮。
    void paint();

    /// @brief 尺具当前所在的场景；派生类负责从图元取 scene 并做类型转换。
    virtual GraphicsScene *scene() const             = 0;

    /// @brief 绕尺具中心旋转给定角度（度）。
    virtual void rotateAroundCenter(qreal angle)     = 0;

    /// @brief 旋转中心，坐标约定由派生类决定（本地坐标）。
    virtual QPointF rotationCenter() const           = 0;
    /// @brief 关闭按钮在本地坐标下的矩形区域。
    virtual QRectF closeButtonRect() const           = 0;
    /// @brief 绘制刻度与标注，由派生类实现具体的排版与单位。
    virtual void paintGraduations(QPainter *painter) = 0;

    bool              mShowButtons;    ///< 是否显示关闭等悬浮按钮
    QGraphicsSvgItem *mCloseSvgItem;   ///< 关闭按钮图元，父图元为尺具本身
    qreal             mAntiScaleRatio; ///< 屏幕像素到场景坐标的反缩放系数，保证按钮视觉大小恒定

    QPointF startDrawPosition;         ///< 沿尺边画线时的起点

    /// @brief 移动尺具时使用的光标。
    QCursor moveCursor() const;
    /// @brief 旋转尺具时使用的光标。
    QCursor rotateCursor() const;
    /// @brief 悬停关闭按钮时使用的光标。
    QCursor closeCursor() const;
    /// @brief 沿尺边画线时使用的光标。
    QCursor drawRulerLineCursor() const;

    /// @brief 前景（刻度、文字）颜色，随背景明暗切换。
    QColor drawColor() const;
    /// @brief 尺身中部填充色，随背景明暗切换。
    QColor middleFillColor() const;
    /// @brief 尺身边缘填充色，随背景明暗切换。
    QColor edgeFillColor() const;
    /// @brief 刻度文字字体。
    QFont font() const;

    static const QColor sLightBackgroundEdgeFillColor;   ///< 浅色背景下的边缘填充色
    static const QColor sLightBackgroundMiddleFillColor; ///< 浅色背景下的中部填充色
    static const QColor sLightBackgroundDrawColor;       ///< 浅色背景下的前景绘制色
    static const QColor sDarkBackgroundEdgeFillColor;    ///< 深色背景下的边缘填充色
    static const QColor sDarkBackgroundMiddleFillColor;  ///< 深色背景下的中部填充色
    static const QColor sDarkBackgroundDrawColor;        ///< 深色背景下的前景绘制色

    static const int sLeftEdgeMargin;                    ///< 尺身左侧留白（像素）
    static const int sDegreeToQtAngleUnit;               ///< 度到 Qt 角度单位的换算系数（16）
    static const int sRotationRadius;                    ///< 旋转手柄距边缘的半径（像素）
    static const int sFillTransparency;                  ///< 填充色透明度（0-255）
    static const int sDrawTransparency;                  ///< 前景色透明度（0-255）
    static const int sRoundingRadius;                    ///< 圆角半径，取左留白的一半
    qreal            sPixelsPerCentimeter;               ///< 每厘米对应的场景像素数，来自当前场景的网格尺寸
};
