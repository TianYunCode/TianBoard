#pragma once

#include <QWidget>
#include <QPoint>

#include <memory>

typedef enum {
    eMinimizedLocation_None,
    eMinimizedLocation_Left,
    eMinimizedLocation_Top,
    eMinimizedLocation_Right,
    eMinimizedLocation_Bottom
} eMinimizedLocation;

class FloatingPalette : public QWidget {
    Q_OBJECT

public:
    FloatingPalette(Qt::Corner = Qt::TopLeftCorner, QWidget *parent = 0);
    virtual ~FloatingPalette();

    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

    void addAssociatedPalette(FloatingPalette *pOtherPalette);
    void removeAssociatedPalette(FloatingPalette *pOtherPalette);

    virtual void adjustSizeAndPosition(bool pUp = true);

    void setCustomPosition(bool pFlag);

    QSize preferredSize();

    void setBackgroundBrush(const QBrush &brush);
    void setGrip(bool newGrip);

    void setMinimizePermission(bool permission);

protected:
    virtual void enterEvent(QEvent *event);
    virtual void showEvent(QShowEvent *event);
    virtual void paintEvent(QPaintEvent *event);

    virtual int radius();
    virtual int border();
    virtual int gripSize();

    static const int sLayoutContentMargin = 12;
    static const int sLayoutSpacing = 15;
    void moveInsideParent(const QPoint &position);

    virtual int getParentRightOffset();

    eMinimizedLocation minimizedLocation();

    void setBackgroundAlpha(int alpha);
    void setGripVisible(bool visible);
    bool isMoving() const;
    void setCustomPositionFlag(bool pFlag);

private:
    void removeAllAssociatedPalette();
    void minimizePalette(const QPoint &pos);

    struct Private;
    std::unique_ptr<Private> d;

signals:
    void mouseEntered();
    void minimizeStart(eMinimizedLocation location);
    void maximizeStart();
    void maximized();
    void moving();
};
