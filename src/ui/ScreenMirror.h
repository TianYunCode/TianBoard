#pragma once

#include <QWidget>

#include <memory>

class ScreenMirror : public QWidget
{
    Q_OBJECT

public:
    ScreenMirror(QWidget* parent = nullptr);
    ~ScreenMirror() override;

    void paintEvent (QPaintEvent * event) override;
    void timerEvent(QTimerEvent *event) override;

public slots:

    void setSourceWidget(QWidget *sourceWidget);

    void setSourceRect(const QRect& pRect);

    void start();

    void stop();

private:

    void grabPixmap();

    struct Private;
    std::unique_ptr<Private> d;

};

