#pragma once

#include <QWidget>

#include <memory>

class WidgetMirror : public QWidget
{
    Q_OBJECT

public:
    WidgetMirror(QWidget* sourceWidget, QWidget* parent = nullptr);
    ~WidgetMirror() override;

public slots:
    void setSourceWidget(QWidget *sourceWidget);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void paintEvent (QPaintEvent * event) override;

private:
    struct Private;
    std::unique_ptr<Private> d;

};

