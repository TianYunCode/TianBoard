#pragma once

#include <QtWidgets>
#include <QWidget>

#include <memory>

class MagnifierParams {
public:
    int x;
    int y;
    qreal zoom;
    qreal sizePercentFromScene;
};

class Magnifier : public QWidget {
    Q_OBJECT

public:
    enum DrawingMode { circular = 0, rectangular, modesCount };

public:
    Magnifier(QWidget *parent = 0, bool isInteractive = false);
    ~Magnifier();

    void setSize(qreal percentFromScene);
    void createMask();
    void setZoom(qreal zoom);
    qreal zoom() const;

    void setGrabView(QWidget *view);
    void setMoveView(QWidget *view);
    void setDrawingMode(int mode);

    void grabPoint();
    void grabPoint(const QPoint &point);
    void grabNMove(const QPoint &pGrab, const QPoint &pMove, bool needGrab = true, bool needMove = true);

signals:
    void magnifierMoved_Signal(QPoint newPos);
    void magnifierClose_Signal();
    void magnifierZoomIn_Signal();
    void magnifierZoomOut_Signal();
    void magnifierResized_Signal(qreal newPercentSize);
    void magnifierDrawingModeChange_Signal(int mode);

public slots:
    void slot_refresh();

private:
    void calculateButtonsPositions();

protected:
    void paintEvent(QPaintEvent *);

    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

private:
    struct Private;
    std::unique_ptr<Private> d;
};
