#include "model/Resources.h"
#include "utils/StaticMemory.h"

#include <QtWidgets>

#include "settings/Settings.h"
#include "utils/FileSystemUtils.h"

Resources *Resources::sSingleton = 0;

Resources::Resources(QObject *pParent) : QObject(pParent)
{
    // 刻意留空
}

Resources::~Resources()
{
    // 刻意留空
}

Resources *Resources::resources()
{
    if (!sSingleton)
    {
        sSingleton = new Resources(StaticMemory::cleaner());
        sSingleton->init();
        sSingleton->buildFontList();
    }

    return sSingleton;
}

void Resources::init()
{
    // 光标都从资源文件预构造一次，之后各视图直接复用，避免反复加载图片。
    penCursor           = QCursor(Qt::CrossCursor);
    eraserCursor        = QCursor(QPixmap(":/images/cursors/eraser.png"), 5, 25);
    markerCursor        = QCursor(QPixmap(":/images/cursors/marker.png"), 3, 30);
    pointerCursor       = QCursor(QPixmap(":/images/cursors/laser.png"), 2, 1);
    handCursor          = QCursor(Qt::OpenHandCursor);
    zoomInCursor        = QCursor(QPixmap(":/images/cursors/zoomIn.png"), 9, 9);
    zoomOutCursor       = QCursor(QPixmap(":/images/cursors/zoomOut.png"), 9, 9);
    arrowCursor         = QCursor(Qt::ArrowCursor);
    playCursor          = QCursor(QPixmap(":/images/cursors/play.png"), 6, 1);
    textCursor          = QCursor(Qt::ArrowCursor);
    rotateCursor        = QCursor(QPixmap(":/images/cursors/rotate.png"), 16, 16);
    drawLineRulerCursor = QCursor(QPixmap(":/images/cursors/drawRulerLine.png"), 3, 12);
}

void Resources::buildFontList()
{
    // 用户字体目录里的每个文件都注册进应用字体库，只收集字体族名供文本工具使用。
    QString     customFontDirectory = Settings::settings()->applicationCustomFontDirectory();
    QStringList fontFiles           = FileSystemUtils::allFiles(customFontDirectory);
    foreach (QString fontFile, fontFiles)
    {
        int fontId = QFontDatabase::addApplicationFont(fontFile);
        mCustomFontList << QFontDatabase::applicationFontFamilies(fontId);
    }
}
