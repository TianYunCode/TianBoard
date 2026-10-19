#pragma once

#include <QLabel>

#include <memory>

class SqueezeLabel : public QLabel
{
    Q_OBJECT;

public:
    SqueezeLabel(QWidget *parent = 0);
    ~SqueezeLabel() override;

protected:
    void paintEvent(QPaintEvent *event);

private:
    struct Private;
    std::unique_ptr<Private> d;
};


