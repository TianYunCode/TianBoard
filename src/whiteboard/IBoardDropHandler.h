#pragma once

class QGraphicsSceneDragDropEvent;
class GraphicsScene;
class GraphicsWidgetItem;

// 拖放到网页小部件上的东西要怎么进画板，做这件事的是 api 的门面（它要
// DownloadManager，也要文档的对象目录）。画板反过来 include api 就把分层写反了，
// 所以接口留在这里、实现由装配点在启动时装上（见 app/main.cpp）。
class IBoardDropHandler {
public:
    virtual ~IBoardDropHandler() = default;

    virtual void processDropEvent(QGraphicsSceneDragDropEvent *event) = 0;
};

// 工厂造出来的对象自己认父对象（跟着场景回收），调用方不接管所有权。
using IBoardDropHandlerFactory = IBoardDropHandler *(*)(GraphicsScene * scene, GraphicsWidgetItem *widget);

void installBoardDropHandlerFactory(IBoardDropHandlerFactory factory);
IBoardDropHandler *createBoardDropHandler(GraphicsScene *scene, GraphicsWidgetItem *widget);
