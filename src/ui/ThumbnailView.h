#pragma once

#include <QGraphicsView>
#include <QLabel>
#include <QHBoxLayout>
#include <QDebug>

#include <memory>

class GraphicsScene;

class ThumbnailView : public QGraphicsView {
    Q_OBJECT

public:
    ThumbnailView(GraphicsScene *scene, QWidget *parent = 0);
    virtual ~ThumbnailView();

private:
    struct Private;
    std::unique_ptr<Private> d;
};
