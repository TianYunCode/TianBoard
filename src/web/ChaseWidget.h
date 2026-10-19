#pragma once

#include <QtWidgets>
#include <QWidget>

#include <memory>

class ChaseWidget : public QWidget {
    Q_OBJECT

public:
    ChaseWidget(QWidget *parent = 0, QPixmap pixmap = QPixmap(), bool pixmapEnabled = false);
    ~ChaseWidget() override;

    void setAnimated(bool value);
    void setPixmapEnabled(bool enable);
    QSize sizeHint() const;

protected:
    void paintEvent(QPaintEvent *event);
    void timerEvent(QTimerEvent *event);

private:
    int segmentCount() const;
    QColor colorForSegment(int segment) const;

    struct Private;
    std::unique_ptr<Private> d;
};
