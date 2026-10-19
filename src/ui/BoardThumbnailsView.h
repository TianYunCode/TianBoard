#pragma once

#include <QResizeEvent>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMouseEvent>

#include "model/DocumentContainer.h"
#include "ui/ThumbnailWidget.h"

#include <memory>

class BoardThumbnailsView : public QGraphicsView
{
    Q_OBJECT
public:
    BoardThumbnailsView(QWidget* parent=0, const char* name="BoardThumbnailsView");
    ~BoardThumbnailsView() override;

public slots:
    void ensureVisibleThumbnail(int index);
    void centerOnThumbnail(int index);

    void clearThumbnails();
    void initThumbnails(DocumentContainer* source);
    void addThumbnail(DocumentContainer* source, int i);
    void moveThumbnail(int from, int to);
    void removeThumbnail(int i);
    void updateThumbnails();

    void longPressTimeout();
    void mousePressAndHoldEvent(QPoint pos);

protected:
    virtual void resizeEvent(QResizeEvent *event);

    virtual void dragEnterEvent(QDragEnterEvent* event);
    virtual void dragMoveEvent(QDragMoveEvent* event);
    virtual void dropEvent(QDropEvent* event);

    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

signals:
    void mousePressAndHoldEventRequired(QPoint pos);
    void moveThumbnailRequired(int from, int to);

private:
    DraggableThumbnailView* createThumbnail(DocumentContainer* source, int i);
    void updateThumbnailsPos();

    struct Private;
    std::unique_ptr<Private> d;
};

