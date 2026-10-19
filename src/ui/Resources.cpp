#include "ui/Resources.h"

#include <QtWidgets>

#include "core/Application.h"
#include "settings/Settings.h"
#include "utils/FileSystemUtils.h"


Resources *Resources::sSingleton = 0;

Resources::Resources(QObject *pParent) : QObject(pParent) {
    // NOOP
}

Resources::~Resources() {
    // NOOP
}

Resources *Resources::resources() {
    if (!sSingleton) {
        sSingleton = new Resources(Application::staticMemoryCleaner);
        sSingleton->init();
        sSingleton->buildFontList();
    }

    return sSingleton;
}

void Resources::init() {
    // Cursors
    penCursor = QCursor(Qt::CrossCursor);
    eraserCursor = QCursor(QPixmap(":/images/cursors/eraser.png"), 5, 25);
    markerCursor = QCursor(QPixmap(":/images/cursors/marker.png"), 3, 30);
    pointerCursor = QCursor(QPixmap(":/images/cursors/laser.png"), 2, 1);
    handCursor = QCursor(Qt::OpenHandCursor);
    zoomInCursor = QCursor(QPixmap(":/images/cursors/zoomIn.png"), 9, 9);
    zoomOutCursor = QCursor(QPixmap(":/images/cursors/zoomOut.png"), 9, 9);
    arrowCursor = QCursor(Qt::ArrowCursor);
    playCursor = QCursor(QPixmap(":/images/cursors/play.png"), 6, 1);
    textCursor = QCursor(Qt::ArrowCursor);
    rotateCursor = QCursor(QPixmap(":/images/cursors/rotate.png"), 16, 16);
    drawLineRulerCursor = QCursor(QPixmap(":/images/cursors/drawRulerLine.png"), 3, 12);
}

void Resources::buildFontList() {
    QString customFontDirectory = Settings::settings()->applicationCustomFontDirectory();
    QStringList fontFiles = FileSystemUtils::allFiles(customFontDirectory);
    foreach (QString fontFile, fontFiles) {
        int fontId = QFontDatabase::addApplicationFont(fontFile);
        mCustomFontList << QFontDatabase::applicationFontFamilies(fontId);
    }
}
