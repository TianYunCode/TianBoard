#include "whiteboard/ToolsManager.h"


ToolsManager *ToolsManager::sManager = 0;

ToolsManager *ToolsManager::manager() {
    if (!sManager)
        sManager = new ToolsManager(Application::staticMemoryCleaner);
    return sManager;
}

void ToolsManager::destroy() {
    if (sManager)
        delete sManager;
    sManager = NULL;
}


ToolsManager::ToolsManager(QObject *parent) : QObject(parent) {
    mask.id = "tianboardtool://mask";
    mask.icon = QPixmap(":/images/toolPalette/maskTool.png");
    mask.label = tr("Mask");
    mask.version = "1.0";
    mToolsIcon.insert(mask.id, ":/images/toolPalette/maskTool.png");
    mDescriptors << mask;


    ruler.id = "tianboardtool://ruler";
    ruler.icon = QPixmap(":/images/toolPalette/rulerTool.png");
    ruler.label = tr("Ruler");
    ruler.version = "1.0";
    mToolsIcon.insert(ruler.id, ":/images/toolPalette/rulerTool.png");
    mDescriptors << ruler;


    compass.id = "tianboardtool://compass";
    compass.icon = QPixmap(":/images/toolPalette/compassTool.png");
    compass.label = tr("Compass");
    compass.version = "1.0";
    mToolsIcon.insert(compass.id, ":/images/toolPalette/compassTool.png");
    mDescriptors << compass;


    protractor.id = "tianboardtool://protractor";
    protractor.icon = QPixmap(":/images/toolPalette/protractorTool.png");
    protractor.label = tr("Protractor");
    protractor.version = "1.0";
    mToolsIcon.insert(protractor.id, ":/images/toolPalette/protractorTool.png");
    mDescriptors << protractor;


    triangle.id = "tianboardtool://triangle";
    triangle.icon = QPixmap(":/images/toolPalette/triangleTool.png");
    triangle.label = tr("Triangle");
    triangle.version = "1.0";
    mToolsIcon.insert(triangle.id, ":/images/toolPalette/triangleTool.png");
    mDescriptors << triangle;


    magnifier.id = "tianboardtool://magnifier";
    magnifier.icon = QPixmap(":/images/toolPalette/magnifierTool.png");
    magnifier.label = tr("Magnifier");
    magnifier.version = "1.0";
    mToolsIcon.insert(magnifier.id, ":/images/toolPalette/magnifierTool.png");
    mDescriptors << magnifier;


    cache.id = "tianboardtool://cache";
    cache.icon = QPixmap(":/images/toolPalette/cacheTool.png");
    cache.label = tr("Cache");
    cache.version = "1.0";
    mToolsIcon.insert(cache.id, ":/images/toolPalette/cacheTool.png");
    mDescriptors << cache;
}

ToolsManager::~ToolsManager() {
    // 刻意留空
}
