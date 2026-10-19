#pragma once

#include <QtWidgets>
#include <QFrame>

#include <memory>

class CircleFrame : public QFrame {
public:
    CircleFrame(QWidget *parent);
    virtual ~CircleFrame();

    qreal currentPenWidth() const;
    void setCurrentPenWidth(qreal width);

    qreal maxPenWidth() const;
    void setMaxPenWidth(qreal width);

protected:
    virtual void paintEvent(QPaintEvent *event);

private:
    struct Private;
    std::unique_ptr<Private> d;
};
