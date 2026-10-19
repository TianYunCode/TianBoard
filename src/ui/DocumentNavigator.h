#pragma once

#include <QResizeEvent>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QThread>

#include "model/DocumentProxy.h"
#include "model/DocumentContainer.h"
#include "ui/ThumbnailWidget.h"

#include <memory>

#define NO_PAGESELECTED -1

class DocumentNavigator : public QGraphicsView {
    Q_OBJECT
public:
    DocumentNavigator(QWidget *parent = 0, const char *name = "documentNavigator");
    ~DocumentNavigator();

    void setNbColumns(int nbColumns);
    int nbColumns();
    void setThumbnailMinWidth(int width);
    int thumbnailMinWidth();
    SceneThumbnailNavigPixmap *clickedThumbnail(const QPoint pos) const;

public slots:
    void onScrollToSelectedPage(int index);
    void generateThumbnails(DocumentContainer *source);
    void updateSpecificThumbnail(int iPage);

    void longPressTimeout();
    void mousePressAndHoldEvent();

protected:
    virtual void resizeEvent(QResizeEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);

    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dragMoveEvent(QDragMoveEvent *event);
    virtual void dropEvent(QDropEvent *event);

signals:
    void mousePressAndHoldEventRequired();

private:
    void refreshScene();
    int border();

    struct Private;
    std::unique_ptr<Private> d;
};
