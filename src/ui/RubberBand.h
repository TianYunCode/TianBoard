/**
 * @file RubberBand.h
 * @brief 可调整大小的橡皮筋选框声明：用于框选对象或截取屏幕区域。
 */

#pragma once


#include <QRubberBand>

/**
 * @brief 带缩放手柄的橡皮筋选框。
 *
 * 界面上是一个半透明选区，四周 20 像素宽的边框可拖动来改变大小，内部拖动
 * 则可整体移动。由 BoardView 在框选模式下创建并持有（非缩放拖动时把位移
 * 转交给视图移动已选对象），CustomCaptureWindow 也用它来圈定截图区域。
 */
class RubberBand : public QRubberBand {
    Q_OBJECT

public:
    /// 指定形状创建选框，并换成 Windows 风格以保证边框绘制一致。
    RubberBand(Shape s, QWidget *p = 0);
    virtual ~RubberBand();

private:
    /// 鼠标所在位置对应的缩放方向；None 表示在选区内部。
    enum enm_resizingMode {
        None,        ///< 选区内部，整体拖动。
        Top,         ///< 上边。
        TopLeft,     ///< 左上角。
        TopRight,    ///< 右上角。
        Bottom,      ///< 下边。
        BottomLeft,  ///< 左下角。
        BottomRight, ///< 右下角。
        Left,        ///< 左边。
        Right        ///< 右边。
    };

    /// 根据坐标判断缩放方向，并顺手把光标换成对应方向的缩放图标。
    enm_resizingMode determineResizingMode(QPoint pos);
    /// 记录按下点与缩放方向；在选区内部按下时请求视图框选对象。
    virtual void mousePressEvent(QMouseEvent *event);
    /// 按缩放方向改变几何尺寸，或在 None 模式下移动选框与已选对象。
    virtual void mouseMoveEvent(QMouseEvent *event);
    /// 结束拖动并恢复默认光标。
    virtual void mouseReleaseEvent(QMouseEvent *event);

private:
    QStyle *customStyle;            ///< 自建的 windows 风格实例，析构时释放。
    enm_resizingMode mResizingMode; ///< 当前拖动对应的缩放方向。
    int mResizingBorderHeight;      ///< 可触发缩放的边框宽度（像素）。
    bool mMouseIsPressed;           ///< 是否处于按下拖动状态。
    QPoint mLastPressedPoint;       ///< 上一次按下点，用于计算整体位移。
    QPoint mLastMousePos;           ///< 上一次移动点，用于计算右侧/底部增量。
};
