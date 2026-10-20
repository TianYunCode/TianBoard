#include "whiteboard/IBoardDropHandler.h"

namespace {
IBoardDropHandlerFactory gFactory = nullptr;
}

void installBoardDropHandlerFactory(IBoardDropHandlerFactory factory) {
    gFactory = factory;
}

IBoardDropHandler *createBoardDropHandler(GraphicsScene *scene, GraphicsWidgetItem *widget) {
    return gFactory ? gFactory(scene, widget) : nullptr;
}
