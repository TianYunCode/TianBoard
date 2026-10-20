#include "whiteboard/BoardDropHandler.h"

namespace {
BoardDropHandlerFactory gFactory = nullptr;
}

void installBoardDropHandlerFactory(BoardDropHandlerFactory factory) {
    gFactory = factory;
}

BoardDropHandler *createBoardDropHandler(GraphicsScene *scene, GraphicsWidgetItem *widget) {
    return gFactory ? gFactory(scene, widget) : nullptr;
}
