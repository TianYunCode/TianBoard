#pragma once

#include <QWidget>

#include <memory>

class BlackoutWidget : public QWidget
{
    Q_OBJECT

    public:
        BlackoutWidget(QWidget *parent = nullptr);
        ~BlackoutWidget() override;

        void setReturnControlsVisible(bool visible);

        void mousePressEvent(QMouseEvent *event) override;
        void keyPressEvent(QKeyEvent *event) override;

    signals:
        void activity();

    public slots:
        void doActivity();

    private:
        struct Private;
        std::unique_ptr<Private> d;
};

