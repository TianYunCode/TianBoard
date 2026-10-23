#include <QtWidgets>
#include <QToolTip>
#include <QStackedLayout>

#include "ui/MainWindow.h"
#include "ui/DownloadWidget.h"
#include "core/LanguageManager.h"
#include "core/ShortcutManager.h"

#include "ui/MainWindowUi.h"

// 这一大段 setupUi/retranslateUi 由原生 Designer 表单逐条移植而来，动作名与资源
// 路径沿用 .ui 中的命名，方便和翻译文件、样式表对应。
void MainWindowUi::setupUi(QMainWindow *mw)
{
    if (mw->objectName().isEmpty())
    {
        mw->setObjectName("MainWindow");
    }
    mw->resize(1342, 223);
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/images/TianBoardIcon.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    mw->setWindowIcon(icon);
    mw->setIconSize(QSize(32, 32));
    mw->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mw->setUnifiedTitleAndToolBarOnMac(false);
    actionStylus = new QAction(mw);
    actionStylus->setObjectName("actionStylus");
    actionStylus->setCheckable(true);
    actionStylus->setChecked(true);
    QIcon icon1;
    icon1.addFile(QString::fromUtf8(":/images/toolbar/stylus.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionStylus->setIcon(icon1);
    actionBackgrounds = new QAction(mw);
    actionBackgrounds->setObjectName("actionBackgrounds");
    actionBackgrounds->setCheckable(true);
    QIcon icon2;
    icon2.addFile(QString::fromUtf8(":/images/toolbar/background.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionBackgrounds->setIcon(icon2);
    actionUndo = new QAction(mw);
    actionUndo->setObjectName("actionUndo");
    QIcon icon3;
    icon3.addFile(QString::fromUtf8(":/images/toolbar/undoOn.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionUndo->setIcon(icon3);
    actionRedo = new QAction(mw);
    actionRedo->setObjectName("actionRedo");
    QIcon icon4;
    icon4.addFile(QString::fromUtf8(":/images/toolbar/redoOn.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionRedo->setIcon(icon4);
    actionBack = new QAction(mw);
    actionBack->setObjectName("actionBack");
    QIcon icon5;
    icon5.addFile(QString::fromUtf8(":/images/toolbar/previousPageOn.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionBack->setIcon(icon5);
    actionForward = new QAction(mw);
    actionForward->setObjectName("actionForward");
    QIcon icon6;
    icon6.addFile(QString::fromUtf8(":/images/toolbar/nextPageOn.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionForward->setIcon(icon6);
    actionDocument = new QAction(mw);
    actionDocument->setObjectName("actionDocument");
    actionDocument->setCheckable(false);
    QIcon icon7;
    icon7.addFile(QString::fromUtf8(":/images/toolbar/documents.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionDocument->setIcon(icon7);
    actionWeb = new QAction(mw);
    actionWeb->setObjectName("actionWeb");
    QIcon icon8;
    icon8.addFile(QString::fromUtf8(":/images/toolbar/web.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionWeb->setIcon(icon8);
    actionLineSmall = new QAction(mw);
    actionLineSmall->setObjectName("actionLineSmall");
    actionLineSmall->setCheckable(true);
    actionLineSmall->setChecked(true);
    QIcon icon9;
    icon9.addFile(QString::fromUtf8(":/images/toolbar/smallPen.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionLineSmall->setIcon(icon9);
    actionLineMedium = new QAction(mw);
    actionLineMedium->setObjectName("actionLineMedium");
    actionLineMedium->setCheckable(true);
    QIcon icon10;
    icon10.addFile(QString::fromUtf8(":/images/toolbar/mediumPen.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionLineMedium->setIcon(icon10);
    actionLineLarge = new QAction(mw);
    actionLineLarge->setObjectName("actionLineLarge");
    actionLineLarge->setCheckable(true);
    QIcon icon11;
    icon11.addFile(QString::fromUtf8(":/images/toolbar/largePen.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionLineLarge->setIcon(icon11);
    actionQuit = new QAction(mw);
    actionQuit->setObjectName("actionQuit");
    QIcon icon12;
    icon12.addFile(QString::fromUtf8(":/images/toolbar/quitOn.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionQuit->setIcon(icon12);
    actionQuit->setShortcutContext(Qt::WindowShortcut);
    actionQuit->setMenuRole(QAction::NoRole);
    actionEraserSmall = new QAction(mw);
    actionEraserSmall->setObjectName("actionEraserSmall");
    actionEraserSmall->setCheckable(true);
    QIcon icon13;
    icon13.addFile(QString::fromUtf8(":/images/toolbar/smallEraser.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionEraserSmall->setIcon(icon13);
    actionEraserMedium = new QAction(mw);
    actionEraserMedium->setObjectName("actionEraserMedium");
    actionEraserMedium->setCheckable(true);
    actionEraserMedium->setChecked(true);
    QIcon icon14;
    icon14.addFile(QString::fromUtf8(":/images/toolbar/mediumEraser.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionEraserMedium->setIcon(icon14);
    actionEraserLarge = new QAction(mw);
    actionEraserLarge->setObjectName("actionEraserLarge");
    actionEraserLarge->setCheckable(true);
    QIcon icon15;
    icon15.addFile(QString::fromUtf8(":/images/toolbar/largeEraser.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionEraserLarge->setIcon(icon15);
    actionColor0 = new QAction(mw);
    actionColor0->setObjectName("actionColor0");
    QIcon icon16;
    icon16.addFile(QString::fromUtf8(":/images/toolbar/color.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionColor0->setIcon(icon16);
    actionColor1 = new QAction(mw);
    actionColor1->setObjectName("actionColor1");
    actionColor1->setIcon(icon16);
    actionColor2 = new QAction(mw);
    actionColor2->setObjectName("actionColor2");
    actionColor2->setIcon(icon16);
    actionColor3 = new QAction(mw);
    actionColor3->setObjectName("actionColor3");
    actionColor3->setIcon(icon16);
    actionColor4 = new QAction(mw);
    actionColor4->setObjectName("actionColor4");
    actionColor4->setIcon(icon16);
    actionWebBack = new QAction(mw);
    actionWebBack->setObjectName("actionWebBack");
    QIcon icon17;
    icon17.addFile(QString::fromUtf8(":/images/toolbar/previous.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionWebBack->setIcon(icon17);
    actionWebForward = new QAction(mw);
    actionWebForward->setObjectName("actionWebForward");
    QIcon icon18;
    icon18.addFile(QString::fromUtf8(":/images/toolbar/next.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionWebForward->setIcon(icon18);
    actionWebReload = new QAction(mw);
    actionWebReload->setObjectName("actionWebReload");
    QIcon icon19;
    icon19.addFile(QString::fromUtf8(":/images/toolbar/reload.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionWebReload->setIcon(icon19);
    actionHome = new QAction(mw);
    actionHome->setObjectName("actionHome");
    QIcon icon20;
    icon20.addFile(QString::fromUtf8(":/images/toolbar/home.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionHome->setIcon(icon20);
    actionBoard = new QAction(mw);
    actionBoard->setObjectName("actionBoard");
    QIcon icon23;
    icon23.addFile(QString::fromUtf8(":/images/toolbar/board.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionBoard->setIcon(icon23);
    actionErase = new QAction(mw);
    actionErase->setObjectName("actionErase");
    actionErase->setCheckable(true);
    QIcon icon24;
    icon24.addFile(QString::fromUtf8(":/images/toolbar/clearPage.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionErase->setIcon(icon24);
    actionPreferences = new QAction(mw);
    actionPreferences->setObjectName("actionPreferences");
    QIcon icon25;
    icon25.addFile(QString::fromUtf8(":/images/toolbar/settings.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionPreferences->setIcon(icon25);
    actionMenu = new QAction(mw);
    actionMenu->setObjectName("actionMenu");
    QIcon icon27;
    icon27.addFile(QString::fromUtf8(":/images/toolbar/menu.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionMenu->setIcon(icon27);
    actionDesktop = new QAction(mw);
    actionDesktop->setObjectName("actionDesktop");
    QIcon icon28;
    icon28.addFile(QString::fromUtf8(":/images/toolbar/display.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionDesktop->setIcon(icon28);
    actionWebBigger = new QAction(mw);
    actionWebBigger->setObjectName("actionWebBigger");
    QIcon icon29;
    icon29.addFile(QString::fromUtf8(":/images/stylusPalette/zoomInOn.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionWebBigger->setIcon(icon29);
    actionWebSmaller = new QAction(mw);
    actionWebSmaller->setObjectName("actionWebSmaller");
    QIcon icon30;
    icon30.addFile(QString::fromUtf8(":/images/stylusPalette/zoomOutOn.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionWebSmaller->setIcon(icon30);
    actionNewFolder = new QAction(mw);
    actionNewFolder->setObjectName("actionNewFolder");
    QIcon icon31;
    icon31.addFile(QString::fromUtf8(":/images/toolbar/newFolder.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionNewFolder->setIcon(icon31);
    actionNewDocument = new QAction(mw);
    actionNewDocument->setObjectName("actionNewDocument");
    QIcon icon32;
    icon32.addFile(QString::fromUtf8(":/images/toolbar/newDocument.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionNewDocument->setIcon(icon32);
    actionImport = new QAction(mw);
    actionImport->setObjectName("actionImport");
    QIcon icon33;
    icon33.addFile(QString::fromUtf8(":/images/toolbar/import.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionImport->setIcon(icon33);
    actionExport = new QAction(mw);
    actionExport->setObjectName("actionExport");
    QIcon icon34;
    icon34.addFile(QString::fromUtf8(":/images/toolbar/export.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionExport->setIcon(icon34);
    actionOpen = new QAction(mw);
    actionOpen->setObjectName("actionOpen");
    QIcon icon35;
    icon35.addFile(QString::fromUtf8(":/images/toolbar/open.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionOpen->setIcon(icon35);
    actionDuplicate = new QAction(mw);
    actionDuplicate->setObjectName("actionDuplicate");
    QIcon icon36;
    icon36.addFile(QString::fromUtf8(":/images/toolbar/duplicate.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionDuplicate->setIcon(icon36);
    actionDelete = new QAction(mw);
    actionDelete->setObjectName("actionDelete");
    QIcon icon37;
    icon37.addFile(QString::fromUtf8(":/images/toolbar/deleteDocument.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionDelete->setIcon(icon37);
    actionAddToWorkingDocument = new QAction(mw);
    actionAddToWorkingDocument->setObjectName("actionAddToWorkingDocument");
    QIcon icon38;
    icon38.addFile(QString::fromUtf8(":/images/toolbar/addToWorkingDoc.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionAddToWorkingDocument->setIcon(icon38);
    actionDocumentAdd = new QAction(mw);
    actionDocumentAdd->setObjectName("actionDocumentAdd");
    QIcon icon39;
    icon39.addFile(QString::fromUtf8(":/images/toolbar/addToPage.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionDocumentAdd->setIcon(icon39);
    actionRename = new QAction(mw);
    actionRename->setObjectName("actionRename");
    QIcon icon40;
    icon40.addFile(QString::fromUtf8(":/images/toolbar/rename.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionRename->setIcon(icon40);
    actionRename->setMenuRole(QAction::TextHeuristicRole);
    actionDocumentTools = new QAction(mw);
    actionDocumentTools->setObjectName("actionDocumentTools");
    QIcon icon41;
    icon41.addFile(QString::fromUtf8(":/images/toolbar/tools.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionDocumentTools->setIcon(icon41);
    actionWebTools = new QAction(mw);
    actionWebTools->setObjectName("actionWebTools");
    actionWebTools->setCheckable(true);
    actionWebTools->setIcon(icon41);
    actionMultiScreen = new QAction(mw);
    actionMultiScreen->setObjectName("actionMultiScreen");
    actionMultiScreen->setCheckable(true);
    actionMultiScreen->setChecked(true);
    actionMultiScreen->setIcon(icon28);
    actionWidePageSize = new QAction(mw);
    actionWidePageSize->setObjectName("actionWidePageSize");
    actionWidePageSize->setCheckable(true);
    actionWidePageSize->setIcon(icon39);
    actionWidePageSize->setIconVisibleInMenu(false);
    actionRegularPageSize = new QAction(mw);
    actionRegularPageSize->setObjectName("actionRegularPageSize");
    actionRegularPageSize->setCheckable(true);
    actionRegularPageSize->setIcon(icon39);
    actionRegularPageSize->setIconVisibleInMenu(false);
    actionCustomPageSize = new QAction(mw);
    actionCustomPageSize->setObjectName("actionCustomPageSize");
    actionCustomPageSize->setCheckable(true);
    actionCustomPageSize->setEnabled(false);
    actionCustomPageSize->setIcon(icon39);
    actionCustomPageSize->setIconVisibleInMenu(false);
    actionStopLoading = new QAction(mw);
    actionStopLoading->setObjectName("actionStopLoading");
    actionStopLoading->setIcon(icon37);
    actionCut = new QAction(mw);
    actionCut->setObjectName("actionCut");
    QIcon icon42;
    icon42.addFile(QString::fromUtf8(":/images/toolbar/cut.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionCut->setIcon(icon42);
    actionCut->setShortcutContext(Qt::ApplicationShortcut);
    actionCopy = new QAction(mw);
    actionCopy->setObjectName("actionCopy");
    QIcon icon43;
    icon43.addFile(QString::fromUtf8(":/images/toolbar/copy.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionCopy->setIcon(icon43);
    actionCopy->setShortcutContext(Qt::ApplicationShortcut);
    actionPaste = new QAction(mw);
    actionPaste->setObjectName("actionPaste");
    QIcon icon44;
    icon44.addFile(QString::fromUtf8(":/images/toolbar/paste.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionPaste->setIcon(icon44);
    actionPaste->setShortcutContext(Qt::ApplicationShortcut);
    actionPaste->setMenuRole(QAction::TextHeuristicRole);
    actionSleep = new QAction(mw);
    actionSleep->setObjectName("actionSleep");
    QIcon icon45;
    icon45.addFile(QString::fromUtf8(":/images/extraPalette/blackout.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    icon45.addFile(QString::fromUtf8(":/images/extraPalette/blackoutOn.png"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
    actionSleep->setIcon(icon45);
    actionVirtualKeyboard = new QAction(mw);
    actionVirtualKeyboard->setObjectName("actionVirtualKeyboard");
    actionVirtualKeyboard->setCheckable(true);
    QIcon icon46;
    icon46.addFile(QString::fromUtf8(":/images/extraPalette/keyboard.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    icon46.addFile(QString::fromUtf8(":/images/extraPalette/keyboardOn.png"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
    actionVirtualKeyboard->setIcon(icon46);
    actionPlainLightBackground = new QAction(mw);
    actionPlainLightBackground->setObjectName("actionPlainLightBackground");
    actionPlainLightBackground->setCheckable(true);
    QIcon icon47;
    icon47.addFile(QString::fromUtf8(":/images/backgroundPalette/background1.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    icon47.addFile(QString::fromUtf8(":/images/backgroundPalette/background1On.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
    actionPlainLightBackground->setIcon(icon47);
    actionCrossedLightBackground = new QAction(mw);
    actionCrossedLightBackground->setObjectName("actionCrossedLightBackground");
    actionCrossedLightBackground->setCheckable(true);
    QIcon icon48;
    icon48.addFile(QString::fromUtf8(":/images/backgroundPalette/background2.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    icon48.addFile(QString::fromUtf8(":/images/backgroundPalette/background2On.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
    actionCrossedLightBackground->setIcon(icon48);
    actionRuledLightBackground = new QAction(mw);
    actionRuledLightBackground->setObjectName("actionRuledLightBackground");
    actionRuledLightBackground->setCheckable(true);
    QIcon icon49;
    icon49.addFile(QString::fromUtf8(":/images/backgroundPalette/background5.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    icon49.addFile(QString::fromUtf8(":/images/backgroundPalette/background5On.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
    actionRuledLightBackground->setIcon(icon49);
    actionPlainDarkBackground = new QAction(mw);
    actionPlainDarkBackground->setObjectName("actionPlainDarkBackground");
    actionPlainDarkBackground->setCheckable(true);
    QIcon icon50;
    icon50.addFile(QString::fromUtf8(":/images/backgroundPalette/background3.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    icon50.addFile(QString::fromUtf8(":/images/backgroundPalette/background3On.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
    actionPlainDarkBackground->setIcon(icon50);
    actionCrossedDarkBackground = new QAction(mw);
    actionCrossedDarkBackground->setObjectName("actionCrossedDarkBackground");
    actionCrossedDarkBackground->setCheckable(true);
    QIcon icon51;
    icon51.addFile(QString::fromUtf8(":/images/backgroundPalette/background4.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    icon51.addFile(QString::fromUtf8(":/images/backgroundPalette/background4On.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
    actionCrossedDarkBackground->setIcon(icon51);
    actionRuledDarkBackground = new QAction(mw);
    actionRuledDarkBackground->setObjectName("actionRuledDarkBackground");
    actionRuledDarkBackground->setCheckable(true);
    QIcon icon52;
    icon52.addFile(QString::fromUtf8(":/images/backgroundPalette/background6.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    icon52.addFile(QString::fromUtf8(":/images/backgroundPalette/background6On.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
    actionRuledDarkBackground->setIcon(icon52);
    actionRecording = new QAction(mw);
    actionRecording->setObjectName("actionRecording");
    actionRecording->setCheckable(true);
    QIcon icon53;
    icon53.addFile(QString::fromUtf8(":/images/toolbar/record.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionRecording->setIcon(icon53);
    actionRecordingStart = new QAction(mw);
    actionRecordingStart->setObjectName("actionRecordingStart");
    actionRecordingStart->setCheckable(true);
    QIcon icon54;
    icon54.addFile(QString::fromUtf8(":/images/toolbar/record.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    icon54.addFile(QString::fromUtf8(":/images/toolbar/stop.png"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
    actionRecordingStart->setIcon(icon54);
    actionEraseItems = new QAction(mw);
    actionEraseItems->setObjectName("actionEraseItems");
    QIcon icon55;
    icon55.addFile(QString::fromUtf8(":/images/erasePalette/eraseItem.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionEraseItems->setIcon(icon55);
    actionEraseAnnotations = new QAction(mw);
    actionEraseAnnotations->setObjectName("actionEraseAnnotations");
    QIcon icon56;
    icon56.addFile(QString::fromUtf8(":/images/erasePalette/eraseAnnotation.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionEraseAnnotations->setIcon(icon56);
    actionClearPage = new QAction(mw);
    actionClearPage->setObjectName("actionClearPage");
    QIcon icon57;
    icon57.addFile(QString::fromUtf8(":/images/erasePalette/eraseAll.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionClearPage->setIcon(icon57);
    actionAdd = new QAction(mw);
    actionAdd->setObjectName("actionAdd");
    actionAdd->setIcon(icon39);
    actionPen = new QAction(mw);
    actionPen->setObjectName("actionPen");
    actionPen->setCheckable(true);
    actionPen->setChecked(true);
    QIcon icon58;
    icon58.addFile(QString::fromUtf8(":/images/stylusPalette/pen.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    icon58.addFile(QString::fromUtf8(":/images/stylusPalette/penOn.png"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
    actionPen->setIcon(icon58);
    actionEraser = new QAction(mw);
    actionEraser->setObjectName("actionEraser");
    actionEraser->setCheckable(true);
    QIcon icon59;
    icon59.addFile(QString::fromUtf8(":/images/stylusPalette/eraser.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    icon59.addFile(QString::fromUtf8(":/images/stylusPalette/eraserOn.png"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
    actionEraser->setIcon(icon59);
    actionMarker = new QAction(mw);
    actionMarker->setObjectName("actionMarker");
    actionMarker->setCheckable(true);
    QIcon icon60;
    icon60.addFile(QString::fromUtf8(":/images/stylusPalette/marker.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    icon60.addFile(QString::fromUtf8(":/images/stylusPalette/markerOn.png"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
    actionMarker->setIcon(icon60);
    actionSelector = new QAction(mw);
    actionSelector->setObjectName("actionSelector");
    actionSelector->setCheckable(true);
    QIcon icon61;
    icon61.addFile(QString::fromUtf8(":/images/stylusPalette/arrow.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    icon61.addFile(QString::fromUtf8(":/images/stylusPalette/arrowOn.png"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
    actionSelector->setIcon(icon61);
    actionHand = new QAction(mw);
    actionHand->setObjectName("actionHand");
    actionHand->setCheckable(true);
    QIcon icon62;
    icon62.addFile(QString::fromUtf8(":/images/stylusPalette/hand.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    icon62.addFile(QString::fromUtf8(":/images/stylusPalette/handOn.png"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
    actionHand->setIcon(icon62);
    actionZoomIn = new QAction(mw);
    actionZoomIn->setObjectName("actionZoomIn");
    actionZoomIn->setCheckable(true);
    QIcon icon63;
    icon63.addFile(QString::fromUtf8(":/images/stylusPalette/zoomIn.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    icon63.addFile(QString::fromUtf8(":/images/stylusPalette/zoomInOn.png"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
    actionZoomIn->setIcon(icon63);
    actionZoomOut = new QAction(mw);
    actionZoomOut->setObjectName("actionZoomOut");
    actionZoomOut->setCheckable(true);
    QIcon icon64;
    icon64.addFile(QString::fromUtf8(":/images/stylusPalette/zoomOut.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    icon64.addFile(QString::fromUtf8(":/images/stylusPalette/zoomOutOn.png"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
    actionZoomOut->setIcon(icon64);
    actionPointer = new QAction(mw);
    actionPointer->setObjectName("actionPointer");
    actionPointer->setCheckable(true);
    QIcon icon65;
    icon65.addFile(QString::fromUtf8(":/images/stylusPalette/laser.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    icon65.addFile(QString::fromUtf8(":/images/stylusPalette/laserOn.png"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
    actionPointer->setIcon(icon65);
    actionLine = new QAction(mw);
    actionLine->setObjectName("actionLine");
    actionLine->setCheckable(true);
    QIcon icon66;
    icon66.addFile(QString::fromUtf8(":/images/stylusPalette/line.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    icon66.addFile(QString::fromUtf8(":/images/stylusPalette/lineOn.png"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
    actionLine->setIcon(icon66);
    actionText = new QAction(mw);
    actionText->setObjectName("actionText");
    actionText->setCheckable(true);
    QIcon icon67;
    icon67.addFile(QString::fromUtf8(":/images/stylusPalette/text.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    icon67.addFile(QString::fromUtf8(":/images/stylusPalette/textOn.png"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
    actionText->setIcon(icon67);
    actionCapture = new QAction(mw);
    actionCapture->setObjectName("actionCapture");
    actionCapture->setCheckable(true);
    QIcon icon68;
    icon68.addFile(QString::fromUtf8(":/images/stylusPalette/captureArea.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    icon68.addFile(QString::fromUtf8(":/images/stylusPalette/captureAreaOn.png"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
    actionCapture->setIcon(icon68);
    actionSnap = new QAction(mw);
    actionSnap->setObjectName("actionSnap");
    actionSnap->setCheckable(true);
    QIcon iconSnap;
    iconSnap.addFile(QString::fromUtf8(":/images/stylusPalette/snap.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    iconSnap.addFile(QString::fromUtf8(":/images/stylusPalette/snapOn.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
    actionSnap->setIcon(iconSnap);
    actionAddItemToCurrentPage = new QAction(mw);
    actionAddItemToCurrentPage->setObjectName("actionAddItemToCurrentPage");
    QIcon icon69;
    icon69.addFile(QString::fromUtf8(":/images/addItemToCurrentPage.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionAddItemToCurrentPage->setIcon(icon69);
    actionAddItemToNewPage = new QAction(mw);
    actionAddItemToNewPage->setObjectName("actionAddItemToNewPage");
    QIcon icon70;
    icon70.addFile(QString::fromUtf8(":/images/addItemToNewPage.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionAddItemToNewPage->setIcon(icon70);
    actionAddItemToLibrary = new QAction(mw);
    actionAddItemToLibrary->setObjectName("actionAddItemToLibrary");
    QIcon icon71;
    icon71.addFile(QString::fromUtf8(":/images/addItemToLibrary.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionAddItemToLibrary->setIcon(icon71);
    actionPages = new QAction(mw);
    actionPages->setObjectName("actionPages");
    actionPages->setCheckable(false);
    actionPages->setIcon(icon32);
    actionNewPage = new QAction(mw);
    actionNewPage->setObjectName("actionNewPage");
    actionNewPage->setIcon(icon69);
    actionDuplicatePage = new QAction(mw);
    actionDuplicatePage->setObjectName("actionDuplicatePage");
    actionDuplicatePage->setIcon(icon70);
    actionImportPage = new QAction(mw);
    actionImportPage->setObjectName("actionImportPage");
    actionImportPage->setIcon(icon71);
    actionRecordingPause = new QAction(mw);
    actionRecordingPause->setObjectName("actionRecordingPause");
    actionRecordingPause->setCheckable(true);
    actionRecordingPause->setEnabled(false);
    QIcon icon72;
    icon72.addFile(QString::fromUtf8(":/images/toolbar/pause.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionRecordingPause->setIcon(icon72);
    actionRecordingConfig = new QAction(mw);
    actionRecordingConfig->setObjectName("actionRecordingConfig");
    actionRecordingConfig->setIcon(icon25);
    actionWebTrap = new QAction(mw);
    actionWebTrap->setObjectName("actionWebTrap");
    actionWebTrap->setCheckable(true);
    QIcon icon73;
    icon73.addFile(QString::fromUtf8(":/images/extraPalette/webTrap.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    icon73.addFile(QString::fromUtf8(":/images/extraPalette/webTrapEnabled.png"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
    actionWebTrap->setIcon(icon73);
    actionWebCustomCapture = new QAction(mw);
    actionWebCustomCapture->setObjectName("actionWebCustomCapture");
    QIcon icon74;
    icon74.addFile(QString::fromUtf8(":/images/toolbar/captureArea.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionWebCustomCapture->setIcon(icon74);
    actionWebShowHideOnDisplay = new QAction(mw);
    actionWebShowHideOnDisplay->setObjectName("actionWebShowHideOnDisplay");
    actionWebShowHideOnDisplay->setCheckable(true);
    QIcon icon77;
    icon77.addFile(QString::fromUtf8(":/images/toolbar/eyeClosed.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    icon77.addFile(QString::fromUtf8(":/images/toolbar/eyeOpened.png"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
    actionWebShowHideOnDisplay->setIcon(icon77);
    actionWebShowHideOnDisplay->setAutoRepeat(false);
    actionWebShowHideOnDisplay->setMenuRole(QAction::TextHeuristicRole);
    actionEraseDesktopAnnotations = new QAction(mw);
    actionEraseDesktopAnnotations->setObjectName("actionEraseDesktopAnnotations");
    actionEraseDesktopAnnotations->setIcon(icon56);
    actionHideApplication = new QAction(mw);
    actionHideApplication->setObjectName("actionHideApplication");
    QIcon icon79;
    icon79.addFile(QString::fromUtf8(":/images/toolbar/hide.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionHideApplication->setIcon(icon79);
    actionPlay = new QAction(mw);
    actionPlay->setObjectName("actionPlay");
    actionPlay->setCheckable(true);
    QIcon icon80;
    icon80.addFile(QString::fromUtf8(":/images/stylusPalette/handPlay.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    icon80.addFile(QString::fromUtf8(":/images/stylusPalette/handPlayOn.png"), QSize(), QIcon::Mode::Normal, QIcon::State::On);
    actionPlay->setIcon(icon80);
    actionEraseBackground = new QAction(mw);
    actionEraseBackground->setObjectName("actionEraseBackground");
    QIcon icon81;
    icon81.addFile(QString::fromUtf8(":/images/erasePalette/eraseBackground.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionEraseBackground->setIcon(icon81);
    actionDefaultGridSize = new QAction(mw);
    actionDefaultGridSize->setObjectName("actionDefaultGridSize");
    QIcon icon83;
    icon83.addFile(QString::fromUtf8(":/images/backgroundPalette/resetDefaultGridSize.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
    actionDefaultGridSize->setIcon(icon83);
    centralwidget = new QWidget(mw);
    centralwidget->setObjectName("centralwidget");
    mw->setCentralWidget(centralwidget);
    // 三条工具栏都停在顶部，并用 toolbar break 强制各占一行；
    // 每行末尾重复放入 Board/Web/Document/Desktop/Menu，保证任何模式下都能切换。
    boardToolBar = new QToolBar(mw);
    boardToolBar->setObjectName("boardToolBar");
    boardToolBar->setContextMenuPolicy(Qt::PreventContextMenu);
    boardToolBar->setMovable(false);
    boardToolBar->setAllowedAreas(Qt::BottomToolBarArea | Qt::TopToolBarArea);
    boardToolBar->setIconSize(QSize(48, 32));
    boardToolBar->setFloatable(false);
    mw->addToolBar(Qt::ToolBarArea::TopToolBarArea, boardToolBar);
    webToolBar = new QToolBar(mw);
    webToolBar->setObjectName("webToolBar");
    webToolBar->setContextMenuPolicy(Qt::PreventContextMenu);
    webToolBar->setMovable(false);
    webToolBar->setAllowedAreas(Qt::BottomToolBarArea | Qt::TopToolBarArea);
    webToolBar->setIconSize(QSize(48, 32));
    webToolBar->setFloatable(false);
    mw->addToolBar(Qt::ToolBarArea::TopToolBarArea, webToolBar);
    mw->insertToolBarBreak(webToolBar);
    documentToolBar = new QToolBar(mw);
    documentToolBar->setObjectName("documentToolBar");
    documentToolBar->setContextMenuPolicy(Qt::PreventContextMenu);
    documentToolBar->setMovable(false);
    documentToolBar->setAllowedAreas(Qt::BottomToolBarArea | Qt::TopToolBarArea);
    documentToolBar->setIconSize(QSize(48, 32));
    documentToolBar->setFloatable(false);
    mw->addToolBar(Qt::ToolBarArea::TopToolBarArea, documentToolBar);
    mw->insertToolBarBreak(documentToolBar);

    boardToolBar->addAction(actionStylus);
    boardToolBar->addAction(actionBackgrounds);
    boardToolBar->addSeparator();
    boardToolBar->addAction(actionUndo);
    boardToolBar->addAction(actionRedo);
    boardToolBar->addSeparator();
    boardToolBar->addAction(actionPages);
    boardToolBar->addAction(actionBack);
    boardToolBar->addAction(actionForward);
    boardToolBar->addSeparator();
    boardToolBar->addAction(actionErase);
    boardToolBar->addAction(actionBoard);
    boardToolBar->addAction(actionWeb);
    boardToolBar->addAction(actionDocument);
    boardToolBar->addAction(actionDesktop);
    boardToolBar->addAction(actionMenu);
    webToolBar->addAction(actionWebTools);
    webToolBar->addSeparator();
    webToolBar->addAction(actionWebBack);
    webToolBar->addAction(actionWebForward);
    webToolBar->addAction(actionWebReload);
    webToolBar->addAction(actionStopLoading);
    webToolBar->addAction(actionHome);
    webToolBar->addSeparator();
    webToolBar->addAction(actionWebBigger);
    webToolBar->addAction(actionWebSmaller);
    webToolBar->addSeparator();
    webToolBar->addAction(actionBoard);
    webToolBar->addAction(actionWeb);
    webToolBar->addAction(actionDocument);
    webToolBar->addAction(actionDesktop);
    webToolBar->addAction(actionMenu);
    documentToolBar->addAction(actionVirtualKeyboard);
    documentToolBar->addSeparator();
    documentToolBar->addAction(actionNewDocument);
    documentToolBar->addAction(actionNewFolder);
    documentToolBar->addSeparator();
    documentToolBar->addAction(actionImport);
    documentToolBar->addAction(actionExport);
    documentToolBar->addAction(actionRename);
    documentToolBar->addSeparator();
    documentToolBar->addAction(actionDuplicate);
    documentToolBar->addAction(actionDelete);
    documentToolBar->addSeparator();
    documentToolBar->addAction(actionOpen);
    documentToolBar->addAction(actionAddToWorkingDocument);
    documentToolBar->addAction(actionDocumentAdd);
    documentToolBar->addAction(actionBoard);
    documentToolBar->addAction(actionWeb);
    documentToolBar->addAction(actionDocument);
    documentToolBar->addAction(actionDesktop);
    documentToolBar->addAction(actionMenu);

    // 默认快捷键集中在这里，一处一份：写的是按键名而不是自然语言，所以不进 retranslateUi——
    // 那边每次换语言都会重跑，会把用户自己改绑的结果冲掉。用户改动存在 Shortcut/<objectName>
    // 里，由 ShortcutManager 在注册时读回来。Cut/Copy/Paste 用 Qt 标准键序，让平台决定组合。
    actionStylus->setShortcut(QKeySequence(QStringLiteral("Ctrl+T")));
    actionUndo->setShortcut(QKeySequence(QStringLiteral("Ctrl+Z")));
    actionRedo->setShortcut(QKeySequence(QStringLiteral("Ctrl+Y")));
    actionBack->setShortcut(QKeySequence(QStringLiteral("PgUp")));
    actionForward->setShortcut(QKeySequence(QStringLiteral("PgDown")));
    actionDocument->setShortcut(QKeySequence(QStringLiteral("Ctrl+D")));
    actionWeb->setShortcut(QKeySequence(QStringLiteral("Ctrl+W")));
    actionWebBack->setShortcut(QKeySequence(QStringLiteral("Left")));
    actionWebForward->setShortcut(QKeySequence(QStringLiteral("Right")));
    actionBoard->setShortcut(QKeySequence(QStringLiteral("Ctrl+B")));
    actionDesktop->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+H")));
    actionWebBigger->setShortcut(QKeySequence(QStringLiteral("Ctrl++")));
    actionWebSmaller->setShortcut(QKeySequence(QStringLiteral("Ctrl+-")));
    actionOpen->setShortcut(QKeySequence(QStringLiteral("Ctrl+O")));
    actionDelete->setShortcut(QKeySequence(QStringLiteral("Del")));
    actionPen->setShortcut(QKeySequence(QStringLiteral("Ctrl+I")));
    actionEraser->setShortcut(QKeySequence(QStringLiteral("Ctrl+E")));
    actionMarker->setShortcut(QKeySequence(QStringLiteral("Ctrl+M")));
    actionSelector->setShortcut(QKeySequence(QStringLiteral("Ctrl+F")));
    actionPointer->setShortcut(QKeySequence(QStringLiteral("Ctrl+G")));
    actionLine->setShortcut(QKeySequence(QStringLiteral("Ctrl+J")));
    actionText->setShortcut(QKeySequence(QStringLiteral("Ctrl+K")));
    actionSnap->setShortcut(QKeySequence(QStringLiteral("Ctrl+S")));
    actionHideApplication->setShortcut(QKeySequence(QStringLiteral("Ctrl+H")));
    actionCut->setShortcuts(QKeySequence::Cut);
    actionCopy->setShortcuts(QKeySequence::Copy);
    actionPaste->setShortcuts(QKeySequence::Paste);
    retranslateUi(mw);
}

void MainWindowUi::retranslateUi(QMainWindow *mw)
{
    mw->setWindowTitle(QCoreApplication::translate("MainWindow", "TianBoard", nullptr));
    actionStylus->setText(QCoreApplication::translate("MainWindow", "Stylus", nullptr));
    actionStylus->setIconText(QCoreApplication::translate("MainWindow", "Stylus", nullptr));
    actionBackgrounds->setText(QCoreApplication::translate("MainWindow", "Backgrounds", nullptr));
#if QT_CONFIG(tooltip)
    actionBackgrounds->setToolTip(QCoreApplication::translate("MainWindow", "Change Background", nullptr));
#endif // QT_CONFIG(tooltip)
    actionUndo->setText(QCoreApplication::translate("MainWindow", "Undo", nullptr));
    actionRedo->setText(QCoreApplication::translate("MainWindow", "Redo", nullptr));
    actionBack->setText(QCoreApplication::translate("MainWindow", "Previous", nullptr));
#if QT_CONFIG(tooltip)
    actionBack->setToolTip(QCoreApplication::translate("MainWindow", "Previous Page", nullptr));
#endif // QT_CONFIG(tooltip)
    actionForward->setText(QCoreApplication::translate("MainWindow", "Next", nullptr));
#if QT_CONFIG(tooltip)
    actionForward->setToolTip(QCoreApplication::translate("MainWindow", "Next Page", nullptr));
#endif // QT_CONFIG(tooltip)
    actionDocument->setText(QCoreApplication::translate("MainWindow", "Documents", nullptr));
#if QT_CONFIG(tooltip)
    actionDocument->setToolTip(QCoreApplication::translate("MainWindow", "Manage Documents", nullptr));
#endif // QT_CONFIG(tooltip)
    actionWeb->setText(QCoreApplication::translate("MainWindow", "Web", nullptr));
#if QT_CONFIG(tooltip)
    actionWeb->setToolTip(QCoreApplication::translate("MainWindow", "Web Browsing", nullptr));
#endif // QT_CONFIG(tooltip)
    actionLineSmall->setText(QCoreApplication::translate("MainWindow", "Line", nullptr));
#if QT_CONFIG(tooltip)
    actionLineSmall->setToolTip(QCoreApplication::translate("MainWindow", "Small Line", nullptr));
#endif // QT_CONFIG(tooltip)
    actionLineMedium->setText(QCoreApplication::translate("MainWindow", "Line", nullptr));
#if QT_CONFIG(tooltip)
    actionLineMedium->setToolTip(QCoreApplication::translate("MainWindow", "Medium Line", nullptr));
#endif // QT_CONFIG(tooltip)
    actionLineLarge->setText(QCoreApplication::translate("MainWindow", "Line", nullptr));
#if QT_CONFIG(tooltip)
    actionLineLarge->setToolTip(QCoreApplication::translate("MainWindow", "Large Line", nullptr));
#endif // QT_CONFIG(tooltip)
    actionQuit->setText(QCoreApplication::translate("MainWindow", "Quit", nullptr));
#if QT_CONFIG(tooltip)
    actionQuit->setToolTip(QCoreApplication::translate("MainWindow", "Quit TianBoard", nullptr));
#endif // QT_CONFIG(tooltip)
    actionEraserSmall->setText(QCoreApplication::translate("MainWindow", "Eraser", nullptr));
#if QT_CONFIG(tooltip)
    actionEraserSmall->setToolTip(QCoreApplication::translate("MainWindow", "Smalle Eraser", nullptr));
#endif // QT_CONFIG(tooltip)
    actionEraserMedium->setText(QCoreApplication::translate("MainWindow", "Eraser", nullptr));
#if QT_CONFIG(tooltip)
    actionEraserMedium->setToolTip(QCoreApplication::translate("MainWindow", "Medium Eraser", nullptr));
#endif // QT_CONFIG(tooltip)
    actionEraserLarge->setText(QCoreApplication::translate("MainWindow", "Eraser", nullptr));
#if QT_CONFIG(tooltip)
    actionEraserLarge->setToolTip(QCoreApplication::translate("MainWindow", "Large Eraser", nullptr));
#endif // QT_CONFIG(tooltip)
    actionColor0->setText(QCoreApplication::translate("MainWindow", "Color", nullptr));
#if QT_CONFIG(tooltip)
    actionColor0->setToolTip(QCoreApplication::translate("MainWindow", "Color", nullptr));
#endif // QT_CONFIG(tooltip)
    actionColor1->setText(QCoreApplication::translate("MainWindow", "Color", nullptr));
    actionColor2->setText(QCoreApplication::translate("MainWindow", "Color", nullptr));
    actionColor3->setText(QCoreApplication::translate("MainWindow", "Color", nullptr));
    actionColor4->setText(QCoreApplication::translate("MainWindow", "Color", nullptr));
    actionWebBack->setText(QCoreApplication::translate("MainWindow", "Back", nullptr));
#if QT_CONFIG(tooltip)
    actionWebBack->setToolTip(QCoreApplication::translate("MainWindow", "Previous Page", nullptr));
#endif // QT_CONFIG(tooltip)
    actionWebForward->setText(QCoreApplication::translate("MainWindow", "Forward", nullptr));
#if QT_CONFIG(tooltip)
    actionWebForward->setToolTip(QCoreApplication::translate("MainWindow", "Next Page", nullptr));
#endif // QT_CONFIG(tooltip)
    actionWebReload->setText(QCoreApplication::translate("MainWindow", "Reload", nullptr));
#if QT_CONFIG(tooltip)
    actionWebReload->setToolTip(QCoreApplication::translate("MainWindow", "Reload Current Page", nullptr));
#endif // QT_CONFIG(tooltip)
    actionHome->setText(QCoreApplication::translate("MainWindow", "Home", nullptr));
#if QT_CONFIG(tooltip)
    actionHome->setToolTip(QCoreApplication::translate("MainWindow", "Load Home Page", nullptr));
#endif // QT_CONFIG(tooltip)
    actionBoard->setText(QCoreApplication::translate("MainWindow", "Board", nullptr));
#if QT_CONFIG(tooltip)
    actionBoard->setToolTip(QCoreApplication::translate("MainWindow", "Display Board", nullptr));
#endif // QT_CONFIG(tooltip)
    actionErase->setText(QCoreApplication::translate("MainWindow", "Erase", nullptr));
#if QT_CONFIG(tooltip)
    actionErase->setToolTip(QCoreApplication::translate("MainWindow", "Erase Content", nullptr));
#endif // QT_CONFIG(tooltip)
    actionPreferences->setText(QCoreApplication::translate("MainWindow", "Preferences", nullptr));
#if QT_CONFIG(tooltip)
    actionPreferences->setToolTip(QCoreApplication::translate("MainWindow", "Display Preferences", nullptr));
#endif // QT_CONFIG(tooltip)
    actionMenu->setText(QCoreApplication::translate("MainWindow", "TianBoard", nullptr));
#if QT_CONFIG(tooltip)
    actionMenu->setToolTip(QCoreApplication::translate("MainWindow", "TianBoard", nullptr));
#endif // QT_CONFIG(tooltip)
    actionDesktop->setText(QCoreApplication::translate("MainWindow", "Show Desktop", nullptr));
#if QT_CONFIG(tooltip)
    actionDesktop->setToolTip(QCoreApplication::translate("MainWindow", "Show Computer Desktop", nullptr));
#endif // QT_CONFIG(tooltip)
    actionWebBigger->setText(QCoreApplication::translate("MainWindow", "Bigger", nullptr));
#if QT_CONFIG(tooltip)
    actionWebBigger->setToolTip(QCoreApplication::translate("MainWindow", "Zoom In", nullptr));
#endif // QT_CONFIG(tooltip)
    actionWebSmaller->setText(QCoreApplication::translate("MainWindow", "Smaller", nullptr));
#if QT_CONFIG(tooltip)
    actionWebSmaller->setToolTip(QCoreApplication::translate("MainWindow", "Zoom Out", nullptr));
#endif // QT_CONFIG(tooltip)
    actionNewFolder->setText(QCoreApplication::translate("MainWindow", "New Folder", nullptr));
#if QT_CONFIG(tooltip)
    actionNewFolder->setToolTip(QCoreApplication::translate("MainWindow", "Create a New Folder", nullptr));
#endif // QT_CONFIG(tooltip)
    actionNewDocument->setText(QCoreApplication::translate("MainWindow", "New Document", nullptr));
#if QT_CONFIG(tooltip)
    actionNewDocument->setToolTip(QCoreApplication::translate("MainWindow", "Create a New Document", nullptr));
#endif // QT_CONFIG(tooltip)
    actionImport->setText(QCoreApplication::translate("MainWindow", "Import", nullptr));
#if QT_CONFIG(tooltip)
    actionImport->setToolTip(QCoreApplication::translate("MainWindow", "Import a Document", nullptr));
#endif // QT_CONFIG(tooltip)
    actionExport->setText(QCoreApplication::translate("MainWindow", "Export", nullptr));
#if QT_CONFIG(tooltip)
    actionExport->setToolTip(QCoreApplication::translate("MainWindow", "Export a Document", nullptr));
#endif // QT_CONFIG(tooltip)
    actionOpen->setText(QCoreApplication::translate("MainWindow", "Open in Board", nullptr));
#if QT_CONFIG(tooltip)
    actionOpen->setToolTip(QCoreApplication::translate("MainWindow", "Open Page in Board", nullptr));
#endif // QT_CONFIG(tooltip)
    actionDuplicate->setText(QCoreApplication::translate("MainWindow", "Duplicate", nullptr));
#if QT_CONFIG(tooltip)
    actionDuplicate->setToolTip(QCoreApplication::translate("MainWindow", "Duplicate Selected Content", nullptr));
#endif // QT_CONFIG(tooltip)
    actionDelete->setText(QCoreApplication::translate("MainWindow", "Delete", nullptr));
#if QT_CONFIG(tooltip)
    actionDelete->setToolTip(QCoreApplication::translate("MainWindow", "Delete Selected Content", nullptr));
#endif // QT_CONFIG(tooltip)
    actionAddToWorkingDocument->setText(QCoreApplication::translate("MainWindow", "Add to Working Document", nullptr));
#if QT_CONFIG(tooltip)
    actionAddToWorkingDocument->setToolTip(QCoreApplication::translate("MainWindow", "Add Selected Content to Open Document", nullptr));
#endif // QT_CONFIG(tooltip)
    actionDocumentAdd->setText(QCoreApplication::translate("MainWindow", "Add", nullptr));
#if QT_CONFIG(tooltip)
    actionDocumentAdd->setToolTip(QCoreApplication::translate("MainWindow", "Add Content to Document", nullptr));
#endif // QT_CONFIG(tooltip)
    actionRename->setText(QCoreApplication::translate("MainWindow", "Rename", nullptr));
#if QT_CONFIG(tooltip)
    actionRename->setToolTip(QCoreApplication::translate("MainWindow", "Rename Content", nullptr));
#endif // QT_CONFIG(tooltip)
    actionDocumentTools->setText(QCoreApplication::translate("MainWindow", "Tools", nullptr));
#if QT_CONFIG(tooltip)
    actionDocumentTools->setToolTip(QCoreApplication::translate("MainWindow", "Display Tools", nullptr));
#endif // QT_CONFIG(tooltip)
    actionWebTools->setText(QCoreApplication::translate("MainWindow", "Tools", nullptr));
#if QT_CONFIG(tooltip)
    actionWebTools->setToolTip(QCoreApplication::translate("MainWindow", "Display Tools", nullptr));
#endif // QT_CONFIG(tooltip)
    actionMultiScreen->setText(QCoreApplication::translate("MainWindow", "Multi Screen", nullptr));
    actionWidePageSize->setText(QCoreApplication::translate("MainWindow", "Wide Size (16/9)", nullptr));
    actionWidePageSize->setIconText(QCoreApplication::translate("MainWindow", "Wide Size (16/9)", nullptr));
#if QT_CONFIG(tooltip)
    actionWidePageSize->setToolTip(QCoreApplication::translate("MainWindow", "Use Document Wide Size (16/9)", nullptr));
#endif // QT_CONFIG(tooltip)
    actionRegularPageSize->setText(QCoreApplication::translate("MainWindow", "Regular Size (4/3)", nullptr));
    actionRegularPageSize->setIconText(QCoreApplication::translate("MainWindow", "Regular Size (4/3)", nullptr));
#if QT_CONFIG(tooltip)
    actionRegularPageSize->setToolTip(QCoreApplication::translate("MainWindow", "Use Document Regular Size (4/3)", nullptr));
#endif // QT_CONFIG(tooltip)
    actionCustomPageSize->setText(QCoreApplication::translate("MainWindow", "Custom Size", nullptr));
    actionCustomPageSize->setIconText(QCoreApplication::translate("MainWindow", "Custom Size", nullptr));
#if QT_CONFIG(tooltip)
    actionCustomPageSize->setToolTip(QCoreApplication::translate("MainWindow", "Use Custom Document Size", nullptr));
#endif // QT_CONFIG(tooltip)
    actionStopLoading->setText(QCoreApplication::translate("MainWindow", "Stop Loading", nullptr));
#if QT_CONFIG(tooltip)
    actionStopLoading->setToolTip(QCoreApplication::translate("MainWindow", "Stop Loading Web Page", nullptr));
#endif // QT_CONFIG(tooltip)
    actionCut->setText(QCoreApplication::translate("MainWindow", "Cut", nullptr));
    actionCopy->setText(QCoreApplication::translate("MainWindow", "Copy", nullptr));
    actionPaste->setText(QCoreApplication::translate("MainWindow", "Paste", nullptr));
    actionSleep->setText(QCoreApplication::translate("MainWindow", "Sleep", nullptr));
#if QT_CONFIG(tooltip)
    actionSleep->setToolTip(QCoreApplication::translate("MainWindow", "Put Presentation to Sleep", nullptr));
#endif // QT_CONFIG(tooltip)
    actionVirtualKeyboard->setText(QCoreApplication::translate("MainWindow", "Virtual Keyboard", nullptr));
#if QT_CONFIG(tooltip)
    actionVirtualKeyboard->setToolTip(QCoreApplication::translate("MainWindow", "Display Virtual Keyboard", nullptr));
#endif // QT_CONFIG(tooltip)
    actionPlainLightBackground->setText(QCoreApplication::translate("MainWindow", "Plain Light Background", nullptr));
    actionPlainLightBackground->setIconText(QCoreApplication::translate("MainWindow", "Light", nullptr));
#if QT_CONFIG(tooltip)
    actionPlainLightBackground->setToolTip(QCoreApplication::translate("MainWindow", "Plain Light Background", nullptr));
#endif // QT_CONFIG(tooltip)
    actionCrossedLightBackground->setText(QCoreApplication::translate("MainWindow", "Grid Light Background", nullptr));
    actionCrossedLightBackground->setIconText(QCoreApplication::translate("MainWindow", "Light", nullptr));
#if QT_CONFIG(tooltip)
    actionCrossedLightBackground->setToolTip(QCoreApplication::translate("MainWindow", "Grid Light Background", nullptr));
#endif // QT_CONFIG(tooltip)
    actionRuledLightBackground->setText(QCoreApplication::translate("MainWindow", "Ruled Light Background", nullptr));
    actionRuledLightBackground->setIconText(QCoreApplication::translate("MainWindow", "Light", nullptr));
#if QT_CONFIG(tooltip)
    actionRuledLightBackground->setToolTip(QCoreApplication::translate("MainWindow", "Ruled Light Background", nullptr));
#endif // QT_CONFIG(tooltip)
    actionPlainDarkBackground->setText(QCoreApplication::translate("MainWindow", "Plain Dark Background", nullptr));
    actionPlainDarkBackground->setIconText(QCoreApplication::translate("MainWindow", "Dark", nullptr));
#if QT_CONFIG(tooltip)
    actionPlainDarkBackground->setToolTip(QCoreApplication::translate("MainWindow", "Plain Dark Background", nullptr));
#endif // QT_CONFIG(tooltip)
    actionCrossedDarkBackground->setText(QCoreApplication::translate("MainWindow", "Grid Dark Background", nullptr));
    actionCrossedDarkBackground->setIconText(QCoreApplication::translate("MainWindow", "Dark", nullptr));
#if QT_CONFIG(tooltip)
    actionCrossedDarkBackground->setToolTip(QCoreApplication::translate("MainWindow", "Grid Dark Background", nullptr));
#endif // QT_CONFIG(tooltip)
    actionRuledDarkBackground->setText(QCoreApplication::translate("MainWindow", "Ruled Dark Background", nullptr));
    actionRuledDarkBackground->setIconText(QCoreApplication::translate("MainWindow", "Dark", nullptr));
#if QT_CONFIG(tooltip)
    actionRuledDarkBackground->setToolTip(QCoreApplication::translate("MainWindow", "Ruled Dark Background", nullptr));
#endif // QT_CONFIG(tooltip)
    actionRecording->setText(QCoreApplication::translate("MainWindow", "Screen Recording", nullptr));
#if QT_CONFIG(tooltip)
    actionRecording->setToolTip(QCoreApplication::translate("MainWindow", "Record the Screen to Video", nullptr));
#endif // QT_CONFIG(tooltip)
    actionRecordingStart->setText(QCoreApplication::translate("MainWindow", "Record", nullptr));
#if QT_CONFIG(tooltip)
    actionRecordingStart->setToolTip(QCoreApplication::translate("MainWindow", "Start Screen Recording", nullptr));
#endif // QT_CONFIG(tooltip)
    actionEraseItems->setText(QCoreApplication::translate("MainWindow", "Erase Items", nullptr));
#if QT_CONFIG(tooltip)
    actionEraseItems->setToolTip(QCoreApplication::translate("MainWindow", "Erase All Items", nullptr));
#endif // QT_CONFIG(tooltip)
    actionEraseAnnotations->setText(QCoreApplication::translate("MainWindow", "Erase Annotations", nullptr));
#if QT_CONFIG(tooltip)
    actionEraseAnnotations->setToolTip(QCoreApplication::translate("MainWindow", "Erase All Annotations", nullptr));
#endif // QT_CONFIG(tooltip)
    actionClearPage->setText(QCoreApplication::translate("MainWindow", "Clear Page", nullptr));
#if QT_CONFIG(tooltip)
    actionClearPage->setToolTip(QCoreApplication::translate("MainWindow", "Clear All Elements", nullptr));
#endif // QT_CONFIG(tooltip)
    actionAdd->setText(QCoreApplication::translate("MainWindow", "Add", nullptr));
    actionPen->setText(QCoreApplication::translate("MainWindow", "Pen", nullptr));
#if QT_CONFIG(tooltip)
    actionPen->setToolTip(QCoreApplication::translate("MainWindow", "Annotate Document", nullptr));
#endif // QT_CONFIG(tooltip)
    actionEraser->setText(QCoreApplication::translate("MainWindow", "Eraser", nullptr));
#if QT_CONFIG(tooltip)
    actionEraser->setToolTip(QCoreApplication::translate("MainWindow", "Erase Annotation", nullptr));
#endif // QT_CONFIG(tooltip)
    actionMarker->setText(QCoreApplication::translate("MainWindow", "Marker", nullptr));
#if QT_CONFIG(tooltip)
    actionMarker->setToolTip(QCoreApplication::translate("MainWindow", "Highlight ", nullptr));
#endif // QT_CONFIG(tooltip)
    actionSelector->setText(QCoreApplication::translate("MainWindow", "Selector", nullptr));
#if QT_CONFIG(tooltip)
    actionSelector->setToolTip(QCoreApplication::translate("MainWindow", "Select And Modify Objects", nullptr));
#endif // QT_CONFIG(tooltip)
    actionHand->setText(QCoreApplication::translate("MainWindow", "Hand", nullptr));
#if QT_CONFIG(tooltip)
    actionHand->setToolTip(QCoreApplication::translate("MainWindow", "Scroll Page", nullptr));
#endif // QT_CONFIG(tooltip)
    actionZoomIn->setText(QCoreApplication::translate("MainWindow", "Zoom In", nullptr));
    actionZoomOut->setText(QCoreApplication::translate("MainWindow", "Zoom Out", nullptr));
    actionPointer->setText(QCoreApplication::translate("MainWindow", "Laser Pointer", nullptr));
#if QT_CONFIG(tooltip)
    actionPointer->setToolTip(QCoreApplication::translate("MainWindow", "Virtual Laser Pointer", nullptr));
#endif // QT_CONFIG(tooltip)
    actionLine->setText(QCoreApplication::translate("MainWindow", "Line", nullptr));
#if QT_CONFIG(tooltip)
    actionLine->setToolTip(QCoreApplication::translate("MainWindow", "Draw Lines", nullptr));
#endif // QT_CONFIG(tooltip)
    actionText->setText(QCoreApplication::translate("MainWindow", "Text", nullptr));
#if QT_CONFIG(tooltip)
    actionText->setToolTip(QCoreApplication::translate("MainWindow", "Write Text", nullptr));
#endif // QT_CONFIG(tooltip)
    actionSnap->setText(QCoreApplication::translate("MainWindow", "Snap to Grid", nullptr));
#if QT_CONFIG(tooltip)
    actionSnap->setToolTip(QCoreApplication::translate("MainWindow", "Snap Items To The Page Grid", nullptr));
#endif // QT_CONFIG(tooltip)
    actionCapture->setText(QCoreApplication::translate("MainWindow", "Capture", nullptr));
#if QT_CONFIG(tooltip)
    actionCapture->setToolTip(QCoreApplication::translate("MainWindow", "Capture Part of the Screen", nullptr));
#endif // QT_CONFIG(tooltip)
    actionAddItemToCurrentPage->setText(QCoreApplication::translate("MainWindow", "Add To Current Page", nullptr));
    actionAddItemToCurrentPage->setIconText(QCoreApplication::translate("MainWindow", "Add To Current Page", nullptr));
#if QT_CONFIG(tooltip)
    actionAddItemToCurrentPage->setToolTip(QCoreApplication::translate("MainWindow", "Add Item To Current Page", nullptr));
#endif // QT_CONFIG(tooltip)
    actionAddItemToNewPage->setText(QCoreApplication::translate("MainWindow", "Add To New Page", nullptr));
#if QT_CONFIG(tooltip)
    actionAddItemToNewPage->setToolTip(QCoreApplication::translate("MainWindow", "Add Item To New Page", nullptr));
#endif // QT_CONFIG(tooltip)
    actionAddItemToLibrary->setText(QCoreApplication::translate("MainWindow", "Add To Library", nullptr));
#if QT_CONFIG(tooltip)
    actionAddItemToLibrary->setToolTip(QCoreApplication::translate("MainWindow", "Add Item To Library", nullptr));
#endif // QT_CONFIG(tooltip)
    actionPages->setText(QCoreApplication::translate("MainWindow", "Pages", nullptr));
#if QT_CONFIG(tooltip)
    actionPages->setToolTip(QCoreApplication::translate("MainWindow", "Create a New Page", nullptr));
#endif // QT_CONFIG(tooltip)
    actionNewPage->setText(QCoreApplication::translate("MainWindow", "New Page", nullptr));
#if QT_CONFIG(tooltip)
    actionNewPage->setToolTip(QCoreApplication::translate("MainWindow", "Create a New Page", nullptr));
#endif // QT_CONFIG(tooltip)
    actionDuplicatePage->setText(QCoreApplication::translate("MainWindow", "Duplicate Page", nullptr));
#if QT_CONFIG(tooltip)
    actionDuplicatePage->setToolTip(QCoreApplication::translate("MainWindow", "Duplicate the Current Page", nullptr));
#endif // QT_CONFIG(tooltip)
    actionImportPage->setText(QCoreApplication::translate("MainWindow", "Import Page", nullptr));
#if QT_CONFIG(tooltip)
    actionImportPage->setToolTip(QCoreApplication::translate("MainWindow", "Import an External Page", nullptr));
#endif // QT_CONFIG(tooltip)
    actionRecordingPause->setText(QCoreApplication::translate("MainWindow", "Pause", nullptr));
#if QT_CONFIG(tooltip)
    actionRecordingPause->setToolTip(QCoreApplication::translate("MainWindow", "Pause Screen Recording", nullptr));
#endif // QT_CONFIG(tooltip)
    actionRecordingConfig->setText(QCoreApplication::translate("MainWindow", "Recording Config", nullptr));
#if QT_CONFIG(tooltip)
    actionRecordingConfig->setToolTip(QCoreApplication::translate("MainWindow", "Configure Screen Recording", nullptr));
#endif // QT_CONFIG(tooltip)
    actionWebTrap->setText(QCoreApplication::translate("MainWindow", "Web Trap", nullptr));
#if QT_CONFIG(tooltip)
    actionWebTrap->setToolTip(QCoreApplication::translate("MainWindow", "Trap Web Content", nullptr));
#endif // QT_CONFIG(tooltip)
    actionWebCustomCapture->setText(QCoreApplication::translate("MainWindow", "Custom Capture", nullptr));
#if QT_CONFIG(tooltip)
    actionWebCustomCapture->setToolTip(QCoreApplication::translate("MainWindow", "Capture Part of the Screen", nullptr));
#endif // QT_CONFIG(tooltip)
    actionWebShowHideOnDisplay->setText(QCoreApplication::translate("MainWindow", "Show on Display", nullptr));
#if QT_CONFIG(tooltip)
    actionWebShowHideOnDisplay->setToolTip(QCoreApplication::translate("MainWindow", "Show Main Screen on Display Screen", nullptr));
#endif // QT_CONFIG(tooltip)
    actionEraseDesktopAnnotations->setText(QCoreApplication::translate("MainWindow", "Erase Annotations", nullptr));
#if QT_CONFIG(tooltip)
    actionEraseDesktopAnnotations->setToolTip(QCoreApplication::translate("MainWindow", "Erase all Annotations", nullptr));
#endif // QT_CONFIG(tooltip)
    actionHideApplication->setText(QCoreApplication::translate("MainWindow", "Hide TianBoard", nullptr));
#if QT_CONFIG(tooltip)
    actionHideApplication->setToolTip(QCoreApplication::translate("MainWindow", "Hide TianBoard Application", nullptr));
#endif // QT_CONFIG(tooltip)
    actionPlay->setText(QCoreApplication::translate("MainWindow", "Play", nullptr));
#if QT_CONFIG(tooltip)
    actionPlay->setToolTip(QCoreApplication::translate("MainWindow", "Interact with items", nullptr));
#endif // QT_CONFIG(tooltip)
    actionEraseBackground->setText(QCoreApplication::translate("MainWindow", "Erase Background", nullptr));
#if QT_CONFIG(tooltip)
    actionEraseBackground->setToolTip(QCoreApplication::translate("MainWindow", "Remove the backgound", nullptr));
#endif // QT_CONFIG(tooltip)
    actionDefaultGridSize->setText(QCoreApplication::translate("MainWindow", "Reset grid size", nullptr));
#if QT_CONFIG(tooltip)
    actionDefaultGridSize->setToolTip(QCoreApplication::translate("MainWindow", "Reset grid size", nullptr));
#endif // QT_CONFIG(tooltip)
    boardToolBar->setWindowTitle(QCoreApplication::translate("MainWindow", "Board", nullptr));
    webToolBar->setWindowTitle(QCoreApplication::translate("MainWindow", "Web", nullptr));
    documentToolBar->setWindowTitle(QCoreApplication::translate("MainWindow", "Documents", nullptr));
}

struct MainWindow::Private
{
    MainWindowUi    ui;
    QStackedLayout *stackedLayout{nullptr};
    QWidget        *boardWidget{nullptr};
    QWidget        *webWidget{nullptr};
    QWidget        *documentsWidget{nullptr};
    DownloadWidget *downloadWidget{nullptr};
};

MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags flags) : QMainWindow(parent, flags), d(new Private)
{
    d->ui.setupUi(this);

    // 换语言时把这套从旧 Designer 表单移植过来的文案重新取一遍词。
    connect(LanguageManager::instance(), &LanguageManager::languageChanged, this,
        [this](const QString &)
        {
            d->ui.retranslateUi(this);
        });

    d->downloadWidget = new DownloadWidget();
    d->downloadWidget->setWindowModality(Qt::ApplicationModal);

    // 提示框颜色固定成浅黄底黑字，避免跟随系统主题后与白色板面混在一起看不清。
    QPalette toolTipPalette;
    toolTipPalette.setColor(QPalette::ToolTipBase, QColor("#FFFFDC"));
    toolTipPalette.setColor(QPalette::ToolTipText, Qt::black);
    QToolTip::setPalette(toolTipPalette);

    QWidget *central = new QWidget(this);
    central->setMinimumWidth(200);
    d->stackedLayout = new QStackedLayout(central);
    setCentralWidget(central);

    d->ui.actionPreferences->setShortcut(QKeySequence(Qt::ALT | Qt::Key_Return));
    // Alt+F4 是系统保留组合键，动作上设置的快捷键不会生效，只能在这里显式登记一次。
    d->ui.actionQuit->setShortcut(QKeySequence(Qt::ALT | Qt::Key_F4));

    // 默认值全部就位之后再登记：ShortcutManager 以这一刻的快捷键为“默认”，
    // 随后把用户存在配置里的覆盖读回来盖上去。
    registerShortcuts();
}

void MainWindow::registerShortcuts()
{
    ShortcutManager *shortcuts = ShortcutManager::instance();
    MainWindowUi    &ui        = d->ui;

    shortcuts->registerActions(ShortcutManager::Group::Common, {
                                                                   ui.actionStylus,
                                                                   ui.actionBoard,
                                                                   ui.actionWeb,
                                                                   ui.actionDocument,
                                                                   ui.actionDesktop,
                                                                   ui.actionVirtualKeyboard,
                                                                   ui.actionHideApplication,
                                                                   ui.actionPreferences,
                                                                   ui.actionCut,
                                                                   ui.actionCopy,
                                                                   ui.actionPaste,
                                                                   ui.actionQuit,
                                                               });

    shortcuts->registerActions(ShortcutManager::Group::Board, {
                                                                  ui.actionUndo,
                                                                  ui.actionRedo,
                                                                  ui.actionBack,
                                                                  ui.actionForward,
                                                                  ui.actionNewPage,
                                                                  ui.actionDuplicatePage,
                                                                  ui.actionImportPage,
                                                                  ui.actionAdd,
                                                                  ui.actionClearPage,
                                                                  ui.actionEraseItems,
                                                                  ui.actionEraseAnnotations,
                                                                  ui.actionEraseBackground,
                                                              });

    shortcuts->registerActions(ShortcutManager::Group::Tool, {
                                                                 ui.actionPen,
                                                                 ui.actionEraser,
                                                                 ui.actionMarker,
                                                                 ui.actionSelector,
                                                                 ui.actionPlay,
                                                                 ui.actionHand,
                                                                 ui.actionZoomIn,
                                                                 ui.actionZoomOut,
                                                                 ui.actionPointer,
                                                                 ui.actionLine,
                                                                 ui.actionText,
                                                                 ui.actionCapture,
                                                                 ui.actionSnap,
                                                             });

    shortcuts->registerActions(ShortcutManager::Group::LineColor, {
                                                                      ui.actionLineSmall,
                                                                      ui.actionLineMedium,
                                                                      ui.actionLineLarge,
                                                                      ui.actionEraserSmall,
                                                                      ui.actionEraserMedium,
                                                                      ui.actionEraserLarge,
                                                                      ui.actionColor0,
                                                                      ui.actionColor1,
                                                                      ui.actionColor2,
                                                                      ui.actionColor3,
                                                                      ui.actionColor4,
                                                                  });

    shortcuts->registerActions(ShortcutManager::Group::Background, {
                                                                       ui.actionBackgrounds,
                                                                       ui.actionPlainLightBackground,
                                                                       ui.actionCrossedLightBackground,
                                                                       ui.actionRuledLightBackground,
                                                                       ui.actionPlainDarkBackground,
                                                                       ui.actionCrossedDarkBackground,
                                                                       ui.actionRuledDarkBackground,
                                                                       ui.actionDefaultGridSize,
                                                                   });

    shortcuts->registerActions(ShortcutManager::Group::Web, {
                                                                ui.actionWebBack,
                                                                ui.actionWebForward,
                                                                ui.actionWebReload,
                                                                ui.actionHome,
                                                                ui.actionWebBigger,
                                                                ui.actionWebSmaller,
                                                                ui.actionStopLoading,
                                                            });

    shortcuts->registerActions(ShortcutManager::Group::Document, {
                                                                     ui.actionNewDocument,
                                                                     ui.actionNewFolder,
                                                                     ui.actionImport,
                                                                     ui.actionExport,
                                                                     ui.actionRename,
                                                                     ui.actionDuplicate,
                                                                     ui.actionDelete,
                                                                     ui.actionOpen,
                                                                     ui.actionAddToWorkingDocument,
                                                                     ui.actionDocumentAdd,
                                                                 });
}

MainWindow::~MainWindow()
{
    // 下载窗口是无父的顶层窗体（要独立于主窗口显示并把整应用设成模态），
    // 因此不会随主窗口的控件树回收，必须在这里显式释放。
    delete d->downloadWidget;
    d->downloadWidget = nullptr;
}

MainWindowUi &MainWindow::ui()
{
    return d->ui;
}

void MainWindow::addBoardWidget(QWidget *pWidget)
{
    if (!d->boardWidget)
    {
        d->boardWidget = pWidget;
        d->stackedLayout->addWidget(d->boardWidget);
    }
}

void MainWindow::switchToBoardWidget()
{
    if (d->boardWidget)
    {
        d->stackedLayout->setCurrentWidget(d->boardWidget);
    }
}

void MainWindow::addWebWidget(QWidget *pWidget)
{
    if (!d->webWidget)
    {
        d->webWidget = pWidget;
        // BrowserWindow 关闭时 deleteLater()；不置空的话下一次 addWebWidget 会以为
        // 还挂着一个网页控件，switchToWebWidget 也会对已删除的对象 setCurrentWidget。
        connect(pWidget, &QObject::destroyed, this,
            [this]
            {
                d->webWidget = nullptr;
            });
        d->stackedLayout->addWidget(d->webWidget);
    }
}

void MainWindow::switchToWebWidget()
{
    if (d->webWidget)
    {
        qDebug() << "从 StackedLayout 弹出来，高:" << d->webWidget->height() << "宽:" << d->webWidget->width();
        d->stackedLayout->setCurrentWidget(d->webWidget);
    }
}

void MainWindow::addDocumentsWidget(QWidget *pWidget)
{
    if (!d->documentsWidget)
    {
        d->documentsWidget = pWidget;
        d->stackedLayout->addWidget(d->documentsWidget);
    }
}

void MainWindow::switchToDocumentsWidget()
{
    if (d->documentsWidget)
    {
        d->stackedLayout->setCurrentWidget(d->documentsWidget);
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    QMainWindow::keyPressEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    event->ignore();
    emit closeEvent_Signal(event);
}

void MainWindow::onExportDone()
{
    // HACK：导出时打开文件保存对话框会让部分工具栏按钮被禁用，
    //       这里统一恢复，避免导出结束后界面停留在半禁用状态。
    d->ui.actionExport->setEnabled(true);
    d->ui.actionNewDocument->setEnabled(true);
    d->ui.actionRename->setEnabled(true);
    d->ui.actionDuplicate->setEnabled(true);
    d->ui.actionDelete->setEnabled(true);
    d->ui.actionOpen->setEnabled(true);
    d->ui.actionDocumentAdd->setEnabled(true);
}

bool MainWindow::yesNoQuestion(QString windowTitle, QString text)
{
    QMessageBox messageBox;
    messageBox.setParent(this);
    messageBox.setWindowTitle(windowTitle);
    messageBox.setText(text);
    QPushButton *yesButton = messageBox.addButton(tr("Yes"), QMessageBox::YesRole);
    messageBox.addButton(tr("No"), QMessageBox::NoRole);
    messageBox.setIcon(QMessageBox::Question);
    messageBox.setWindowFlags(Qt::Dialog);

    messageBox.exec();
    return messageBox.clickedButton() == yesButton;
}

void MainWindow::oneButtonMessageBox(QString windowTitle, QString text, QMessageBox::Icon type)
{
    QMessageBox messageBox;
    messageBox.setParent(this);
    messageBox.setWindowFlags(Qt::Dialog);
    messageBox.setWindowTitle(windowTitle);
    messageBox.setText(text);
    messageBox.addButton(tr("Ok"), QMessageBox::YesRole);
    messageBox.setIcon(type);
    messageBox.exec();
}

void MainWindow::warning(QString windowTitle, QString text)
{
    oneButtonMessageBox(windowTitle, text, QMessageBox::Warning);
}

void MainWindow::information(QString windowTitle, QString text)
{
    oneButtonMessageBox(windowTitle, text, QMessageBox::Information);
}
