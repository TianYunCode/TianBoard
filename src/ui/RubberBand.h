#pragma once

#include <QRubberBand>

class RubberBand : public QRubberBand
{
    Q_OBJECT

public:
    RubberBand(Shape s, QWidget * p = 0);
    virtual ~RubberBand();

private:
    enum enm_resizingMode
    {
        None,
        Top,
        TopLeft,
        TopRight,
        Bottom,
        BottomLeft,
        BottomRight,
        Left,
        Right
    };

    enm_resizingMode determineResizingMode(QPoint pos);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

private:
    QStyle* customStyle;
    enm_resizingMode mResizingMode;
    int mResizingBorderHeight;
    bool mMouseIsPressed;
    QPoint mLastPressedPoint;
    QPoint mLastMousePos;
};

