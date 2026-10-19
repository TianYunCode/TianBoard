#pragma once

#include <QtWidgets>
#include <QWidget>

#include <memory>

class GraphicsWidgetItem;
class QWidget;
class GraphicsScene;
class QWebEngineView;

class ToolWidget : public QWidget
{
    Q_OBJECT

public:
    ToolWidget(const QUrl& pUrl, QWidget* pParent = 0);
    ToolWidget(GraphicsWidgetItem* pWidget, QWidget* pParent = 0);
    virtual ~ToolWidget();

    void remove();
    void centerOn(const QPoint& pos);

    QPoint naturalCenter() const;

    GraphicsWidgetItem *toolWidget() const;

protected:
    void initialize();
    virtual void paintEvent(QPaintEvent *event);

    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

    virtual bool eventFilter(QObject *obj, QEvent *event);

private:
    struct Private;
    std::unique_ptr<Private> d;
};

