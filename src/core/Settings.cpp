#include "core/Settings.h"

#include <QtWidgets>

#include "platform/PlatformUtils.h"
#include "utils/FileSystemUtils.h"
#include "utils/CryptoUtils.h"

#include "utils/Enums.h"
#include "core/Setting.h"

#include "whiteboard/ToolsManager.h"


QPointer<Settings> Settings::sSingleton = 0;

int Settings::pointerDiameter = 40;
int Settings::crossSize = 24;
int Settings::defaultCrossSize = 24;
int Settings::minCrossSize = 12;
int Settings::maxCrossSize = 96; //TODO: user-settable?
int Settings::colorPaletteSize = 5;
int Settings::objectFrameWidth = 20;
int Settings::boardMargin = 10;

QString Settings::documentGroupName = QString("Subject");
QString Settings::documentName = QString("Lesson");
QString Settings::documentSize = QString("Size");
QString Settings::documentIdentifer = QString("ID");
QString Settings::documentVersion = QString("Version");
QString Settings::documentUpdatedAt = QString("UpdatedAt");
QString Settings::documentPageCount = QString("PageCount");
QString Settings::documentDate = QString("date");

QString Settings::trashedDocumentGroupNamePrefix = QString("_Trash:");

QString Settings::documentNamespaceUri = "http://www.tianboard.org/2026/document";
QString Settings::applicationNamespaceUri = "http://www.tianboard.org/2026/application";

QString Settings::undoCommandTransactionName = "UndoTransaction";

const int Settings::sDefaultFontPixelSize = 36;
const char *Settings::sDefaultFontFamily = "Arial";

QString Settings::currentFileVersion = "4.8.0";

QBrush Settings::eraserBrushDarkBackground = QBrush(QColor(127, 127, 127, 80));
QBrush Settings::eraserBrushLightBackground = QBrush(QColor(127, 127, 127, 80));

QPen Settings::eraserPenDarkBackground = QPen(QColor(255, 255, 255, 127));
QPen Settings::eraserPenLightBackground = QPen(QColor(0, 0, 0, 127));

QColor Settings::markerCircleBrushColorDarkBackground = QColor(127, 127, 127, 80);
QColor Settings::markerCircleBrushColorLightBackground = QColor(127, 127, 127, 80);

QColor Settings::markerCirclePenColorDarkBackground = QColor(255, 255, 255, 127);
QColor Settings::markerCirclePenColorLightBackground = QColor(0, 0, 0, 127);

QColor Settings::penCircleBrushColorDarkBackground = QColor(127, 127, 127, 80);
QColor Settings::penCircleBrushColorLightBackground = QColor(127, 127, 127, 80);

QColor Settings::penCirclePenColorDarkBackground = QColor(255, 255, 255, 127);
QColor Settings::penCirclePenColorLightBackground = QColor(0, 0, 0, 127);

QColor Settings::documentSizeMarkColorDarkBackground = QColor(44, 44, 44, 200);
QColor Settings::documentSizeMarkColorLightBackground = QColor(241, 241, 241);

QColor Settings::paletteColor = QColor(127, 127, 127, 65);
QColor Settings::opaquePaletteColor = QColor(66, 66, 66, 200);

QColor Settings::documentViewLightColor = QColor(241, 241, 241);

QPointer<QSettings> Settings::sAppSettings = 0;

const int Settings::maxThumbnailWidth = 400;
const int Settings::defaultThumbnailWidth = 100;
const int Settings::defaultSortKind = 0;
const int Settings::defaultSortOrder = 0;
const int Settings::defaultSplitterLeftSize = 200;
const int Settings::defaultSplitterRightSize = 800;

const int Settings::defaultLibraryIconSize = 80;

const int Settings::defaultGipWidth = 150;
const int Settings::defaultSoundWidth = 50;
const int Settings::defaultImageWidth = 150;
const int Settings::defaultShapeWidth = 50;
const int Settings::defaultWidgetIconWidth = 110;
const int Settings::defaultVideoWidth = 80;

const int Settings::thumbnailSpacing = 20;
const int Settings::longClickInterval = 1200;

const qreal Settings::minScreenRatio = 1.33; // 800/600 or 1024/768

QStringList Settings::bitmapFileExtensions;
QStringList Settings::vectoFileExtensions;
QStringList Settings::imageFileExtensions;
QStringList Settings::widgetFileExtensions;
QStringList Settings::interactiveContentFileExtensions;

QColor Settings::treeViewBackgroundColor = QColor(209, 215, 226); //in synch with css tree view background

int Settings::objectInControlViewMargin = 100;

QString Settings::appPingMessage = "__tianboard_ping";

Settings* Settings::settings()
{
    if (!sSingleton) {
        sSingleton = new Settings(qApp);
    }
    return sSingleton;
}

void Settings::destroy()
{
    if (sSingleton)
        delete sSingleton;
    sSingleton = NULL;
}


QSettings* Settings::getAppSettings()
{
    if (!Settings::sAppSettings)
    {
        QString tmpSettings = QDir::tempPath() + "/" + qApp->applicationName() + ".config";
        QString appSettings = PlatformUtils::applicationResourcesDirectory() + "/etc/" + qApp->applicationName() + ".config";

        // 旧版本升级遗留的临时配置还在时，先读它而不是读正式配置
        if (QFile::exists(tmpSettings))
        {
            QFile::rename(tmpSettings, appSettings);
        }

        Settings::sAppSettings = new QSettings(appSettings, QSettings::IniFormat, 0);

        qDebug() << "sAppSettings location: " << appSettings;
    }

    return Settings::sAppSettings;
}


Settings::Settings(QObject *parent)
    : QObject(parent)
{
    InitKeyboardPaletteKeyBtnSizes();

    mAppSettings = Settings::getAppSettings();

    QString userSettingsFile = Settings::userDataDirectory() + "/"+qApp->applicationName()+"User.config";

    mUserSettings = new QSettings(userSettingsFile, QSettings::IniFormat, parent);

    init();
}


Settings::~Settings()
{
    delete mAppSettings;

    if(supportedKeyboardSizes)
        delete supportedKeyboardSizes;
}

void Settings::InitKeyboardPaletteKeyBtnSizes()
{
    supportedKeyboardSizes = new QStringList();
    supportedKeyboardSizes->append("29x29");
    supportedKeyboardSizes->append("41x41");
}

void Settings::ValidateKeyboardPaletteKeyBtnSize()
{
    // if boardKeyboardPaletteKeyBtnSize is not initialized, or supportedKeyboardSizes not initialized or empty
    if( !boardKeyboardPaletteKeyBtnSize ||
        !supportedKeyboardSizes ||
        supportedKeyboardSizes->size() == 0 ) return;

    // get original size value
    QString origValue = boardKeyboardPaletteKeyBtnSize->get().toString();

    // parse available size values, for make sure original value is valid
    for(int i = 0; i < supportedKeyboardSizes->size(); i++)
    {
        int compareCode = QString::compare(origValue, supportedKeyboardSizes->at(i));
        if(compareCode == 0) return;
    }

    // if original value is invalid, than set it value to first value from avaliable list
    boardKeyboardPaletteKeyBtnSize->set(supportedKeyboardSizes->at(0));
}

void Settings::init()
{
    productWebUrl =  new Setting(this, "App", "ProductWebAddress", "http://www.baidu.com");

    softwareHomeUrl = productWebUrl->get().toString();

    documentSizes.insert(DocumentSizeRatio::Ratio4_3, QSize(1280, 960)); // 1.33
    documentSizes.insert(DocumentSizeRatio::Ratio16_9, QSize((960 / 9 * 16), 960)); // 1.77

    appToolBarPositionedAtTop = new Setting(this, "App", "ToolBarPositionedAtTop", true);
    appToolBarDisplayText = new Setting(this, "App", "ToolBarDisplayText", true);
    appHideSwapDisplayScreens = new Setting(this, "App", "HideSwapDisplayScreens", true);
    appToolBarOrientationVertical = new Setting(this, "App", "ToolBarOrientationVertical", false);
    appPreferredLanguage = new Setting(this,"App","PreferredLanguage", "");

    rightLibPaletteBoardModeWidth = new Setting(this, "Board", "RightLibPaletteBoardModeWidth", 270);
    rightLibPaletteBoardModeIsCollapsed = new Setting(this,"Board", "RightLibPaletteBoardModeIsCollapsed",true);
    rightLibPaletteDesktopModeWidth = new Setting(this, "Board", "RightLibPaletteDesktopModeWidth", 270);
    rightLibPaletteDesktopModeIsCollapsed = new Setting(this,"Board", "RightLibPaletteDesktopModeIsCollapsed",true);
    leftLibPaletteBoardModeWidth = new Setting(this, "Board", "LeftLibPaletteBoardModeWidth",270);
    leftLibPaletteBoardModeIsCollapsed = new Setting(this,"Board","LeftLibPaletteBoardModeIsCollapsed",true);
    leftLibPaletteDesktopModeWidth = new Setting(this, "Board", "LeftLibPaletteDesktopModeWidth",270);
    leftLibPaletteDesktopModeIsCollapsed = new Setting(this,"Board","LeftLibPaletteDesktopModeIsCollapsed",true);

    appLastSessionDocumentUUID = new Setting(this, "App", "LastSessionDocumentUUID", "");
    appLastSessionPageIndex = new Setting(this, "App", "LastSessionPageIndex", 0);
    appUseMultiscreen = new Setting(this, "App", "UseMultiscreenMode", true);

    appStartupHintsEnabled = new Setting(this,"App","EnableStartupHints",true);

    appStartMode = new Setting(this, "App", "StartMode", "");

    featureSliderPosition = new Setting(this, "Board", "FeatureSliderPosition", 40);

    boardPenFineWidth = new Setting(this, "Board", "PenFineWidth", 1.5);
    boardPenMediumWidth = new Setting(this, "Board", "PenMediumWidth", 3.0);
    boardPenStrongWidth = new Setting(this, "Board", "PenStrongWidth", 8.0);

    boardMarkerFineWidth = new Setting(this, "Board", "MarkerFineWidth", 12.0);
    boardMarkerMediumWidth = new Setting(this, "Board", "MarkerMediumWidth", 24.0);
    boardMarkerStrongWidth = new Setting(this, "Board", "MarkerStrongWidth", 48.0);

    boardPenPressureSensitive = new Setting(this, "Board", "PenPressureSensitive", true);
    boardMarkerPressureSensitive = new Setting(this, "Board", "MarkerPressureSensitive", false);

    boardUseHighResTabletEvent = new Setting(this, "Board", "UseHighResTabletEvent", true);

    boardInterpolatePenStrokes = new Setting(this, "Board", "InterpolatePenStrokes", true);
    boardSimplifyPenStrokes = new Setting(this, "Board", "SimplifyPenStrokes", true);
    boardSimplifyPenStrokesThresholdAngle = new Setting(this, "Board", "SimplifyPenStrokesThresholdAngle", 2);
    boardSimplifyPenStrokesThresholdWidthDifference = new Setting(this, "Board", "SimplifyPenStrokesThresholdWidthDifference", 2.0);

    boardInterpolateMarkerStrokes = new Setting(this, "Board", "InterpolateMarkerStrokes", true);
    boardSimplifyMarkerStrokes = new Setting(this, "Board", "SimplifyMarkerStrokes", true);

    boardKeyboardPaletteKeyBtnSize = new Setting(this, "Board", "KeyboardPaletteKeyBtnSize", "16x16");
    ValidateKeyboardPaletteKeyBtnSize();

    pageSize = new Setting(this, "Board", "DefaultPageSize", documentSizes.value(DocumentSizeRatio::Ratio4_3));

    boardCrossColorDarkBackground = new Setting(this, "Board", "CrossColorDarkBackground", "#C8C0C0C0");
    boardCrossColorLightBackground = new Setting(this, "Board", "CrossColorLightBackground", "#A5E1FF");

    QStringList gridLightBackgroundColors;
    gridLightBackgroundColors << "#000000" << "#FF0000" << "#004080" << "#008000" << "#FFDD00" << "#C87400" << "#800040" << "#008080" << "#A5E1FF";
    boardGridLightBackgroundColors = new ColorListSetting(this, "Board", "GridLightBackgroundColors", gridLightBackgroundColors, -1.0);

    QStringList gridDarkBackgroundColors;
    gridDarkBackgroundColors << "#FFFFFF" << "#FF3400" << "#66C0FF" << "#81FF5C" << "#FFFF00" << "#B68360" << "#FF497E" << "#8D69FF" << "#C8C0C0C0";
    boardGridDarkBackgroundColors = new ColorListSetting(this, "Board", "GridDarkBackgroundColors", gridDarkBackgroundColors, -1.0);

    QStringList penLightBackgroundColors;
    penLightBackgroundColors << "#000000" << "#FF0000" <<"#004080" << "#008000" << "#FFDD00" << "#C87400" << "#800040" << "#008080"  << "#5F2D0A" << "#FFFFFF";
    boardPenLightBackgroundColors = new ColorListSetting(this, "Board", "PenLightBackgroundColors", penLightBackgroundColors, 1.0);

    QStringList penDarkBackgroundColors;
    penDarkBackgroundColors << "#FFFFFF" << "#FF3400" <<"#66C0FF" << "#81FF5C" << "#FFFF00" << "#B68360" << "#FF497E" << "#8D69FF" << "#000000";
    boardPenDarkBackgroundColors = new ColorListSetting(this, "Board", "PenDarkBackgroundColors", penDarkBackgroundColors, 1.0);

    boardMarkerAlpha = new Setting(this, "Board", "MarkerAlpha", 0.5);

    QStringList markerLightBackgroundColors;
    markerLightBackgroundColors << "#E3FF00" << "#FF0000" <<"#004080" << "#008000" << "#C87400" << "#800040" << "#008080"  << "#000000";
    boardMarkerLightBackgroundColors = new ColorListSetting(this, "Board", "MarkerLightBackgroundColors", markerLightBackgroundColors, boardMarkerAlpha->get().toDouble());

    QStringList markerDarkBackgroundColors;
    markerDarkBackgroundColors << "#FFFF00" << "#FF4400" <<"#66C0FF" << "#81FF5C" << "#B68360" << "#FF497E" << "#8D69FF" << "#FFFFFF";
    boardMarkerDarkBackgroundColors = new ColorListSetting(this, "Board", "MarkerDarkBackgroundColors", markerDarkBackgroundColors, boardMarkerAlpha->get().toDouble());

    QStringList penLightBackgroundSelectedColors;
    QStringList penDarkBackgroundSelectedColors;

    QStringList markerLightBackgroundSelectedColors;
    QStringList markerDarkBackgroundSelectedColors;

    for (int i = 0; i < colorPaletteSize; i++)
    {
        penLightBackgroundSelectedColors << penLightBackgroundColors[i];
        penDarkBackgroundSelectedColors << penDarkBackgroundColors[i];
        markerLightBackgroundSelectedColors << markerLightBackgroundColors[i];
        markerDarkBackgroundSelectedColors << markerDarkBackgroundColors[i];
    }

    boardPenLightBackgroundSelectedColors = new ColorListSetting(this, "Board", "PenLightBackgroundSelectedColors", penLightBackgroundSelectedColors);
    boardPenDarkBackgroundSelectedColors = new ColorListSetting(this, "Board", "PenDarkBackgroundSelectedColors", penDarkBackgroundSelectedColors);

    boardMarkerLightBackgroundSelectedColors = new ColorListSetting(this, "Board", "MarkerLightBackgroundSelectedColors", markerLightBackgroundSelectedColors, boardMarkerAlpha->get().toDouble());
    boardMarkerDarkBackgroundSelectedColors = new ColorListSetting(this, "Board", "MarkerDarkBackgroundSelectedColors", markerDarkBackgroundSelectedColors, boardMarkerAlpha->get().toDouble());

    showEraserPreviewCircle = new Setting(this, "Board", "ShowEraserPreviewCircle", true);
    showMarkerPreviewCircle = new Setting(this, "Board", "ShowMarkerPreviewCircle", true);
    showPenPreviewCircle = new Setting(this, "Board", "ShowPenPreviewCircle", true);
    penPreviewFromSize = new Setting(this, "Board", "PenPreviewFromSize", 5);

    webUseExternalBrowser = new Setting(this, "Web", "UseExternalBrowser", false);

    bool defaultShowPageImmediatelyOnMirroredScreen = true;

#if defined(Q_OS_LINUX)
    // screen duplication is very slow on X11
    defaultShowPageImmediatelyOnMirroredScreen = false;
#endif

    webShowPageImmediatelyOnMirroredScreen = new Setting(this, "Web", "ShowPageImediatelyOnMirroredScreen", defaultShowPageImmediatelyOnMirroredScreen);

    webHomePage = new Setting(this, "Web", "Homepage", softwareHomeUrl);

    pageCacheSize = new Setting(this, "App", "PageCacheSize", 20);

    bitmapFileExtensions << "jpg" << "jpeg" <<  "png" <<  "tiff" << "tif" << "bmp" << "gif";
    vectoFileExtensions << "svg" <<  "svgz";
    imageFileExtensions << bitmapFileExtensions << vectoFileExtensions;

    widgetFileExtensions << "wdgt" << "wgt" << "pwgt";
    interactiveContentFileExtensions << widgetFileExtensions << "swf";

    boardZoomFactor = new Setting(this, "Board", "ZoomFactor", QVariant(1.41));

    if (boardZoomFactor->get().toDouble() <= 1.)
        boardZoomFactor->set(1.41);

    int defaultRefreshRateInFramePerSecond = 8;

#if defined(Q_OS_LINUX)
    // screen duplication is very slow on X11
    defaultRefreshRateInFramePerSecond = 2;
#endif

    mirroringRefreshRateInFps = new Setting(this, "Mirroring", "RefreshRateInFramePerSecond", QVariant(defaultRefreshRateInFramePerSecond));

    lastImportFilePath = new Setting(this, "Import", "LastImportFilePath", QVariant(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)));
    lastImportFolderPath = new Setting(this, "Import", "LastImportFolderPath", QVariant(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)));
    lastExportFilePath = new Setting(this, "Export", "LastExportFilePath", QVariant(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)));
    lastExportDirPath = new Setting(this, "Export", "LastExportDirPath", QVariant(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)));
    lastImportToLibraryPath = new Setting(this, "Library", "LastImportToLibraryPath", QVariant(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)));

    lastPicturePath = new Setting(this, "Library", "LastPicturePath", QVariant(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)));
    lastWidgetPath = new Setting(this, "Library", "LastWidgetPath", QVariant(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)));
    lastVideoPath = new Setting(this, "Library", "LastVideoPath", QVariant(QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)));

    appOnlineUserName = new Setting(this, "App", "OnlineUserName", "");

    boardShowToolsPalette = new Setting(this, "Board", "ShowToolsPalette", "false");
    magnifierDrawingMode = new Setting(this, "Board", "MagnifierDrawingMode", "0");
    autoSaveInterval = new Setting(this, "Board", "AutoSaveIntervalInMinutes", "3");

    svgViewBoxMargin = new Setting(this, "SVG", "ViewBoxMargin", "50");

    pdfMargin = new Setting(this, "PDF", "Margin", "20");
    pdfPageFormat = new Setting(this, "PDF", "PageFormat", "A4");
    pdfResolution = new Setting(this, "PDF", "Resolution", "300");

    recordingFramesPerSecond = new Setting(this, "Recording", "FramesPerSecond", 10);
    recordingVideoSize = new Setting(this, "Recording", "VideoSize", "Medium");
    recordingAudioRecordingDevice = new Setting(this, "Recording", "AudioRecordingDevice", "Default");

    recordingWindowsMediaBitsPerSecond = new Setting(this, "Recording", "WindowsMediaBitsPerSecond", 1700000);
    recordingQuickTimeQuality = new Setting(this, "Recording", "QuickTimeQuality", "High");

    communityUser = new Setting(this, "Community", "Username", "");
    communityPsw = new Setting(this, "Community", "Password", "");
    communityCredentialsPersistence = new Setting(this,"Community", "CredentialsPersistence",false);

    QStringList uris = ToolsManager::manager()->allToolIDs();

    favoritesNativeToolUris = new Setting(this, "App", "FavoriteToolURIs", uris);

    // removed in version 4.4.b.2
    mUserSettings->remove("Recording/RecordMicrophone");

    documentThumbnailWidth      = new Setting(this, "Document", "ThumbnailWidth", Settings::defaultThumbnailWidth);
    documentSortKind            = new Setting(this, "Document", "SortKind", Settings::defaultSortKind);
    documentSortOrder           = new Setting(this, "Document", "SortOrder", Settings::defaultSortOrder);
    documentSplitterLeftSize    = new Setting(this, "Document", "SplitterLeftSize", Settings::defaultSplitterLeftSize);
    documentSplitterRightSize   = new Setting(this, "Document", "SplitterRightSize", Settings::defaultSplitterRightSize);

    libraryShowDetailsForLocalItems = new Setting(this, "Library", "ShowDetailsForLocalItems", false);

    imageThumbnailWidth = new Setting(this, "Library", "ImageThumbnailWidth", Settings::defaultImageWidth);
    videoThumbnailWidth = new Setting(this, "Library", "VideoThumbnailWidth", Settings::defaultVideoWidth);
    shapeThumbnailWidth = new Setting(this, "Library", "ShapeThumbnailWidth", Settings::defaultShapeWidth);
    gipThumbnailWidth = new Setting(this, "Library", "ImageThumbnailWidth", Settings::defaultGipWidth);
    soundThumbnailWidth = new Setting(this, "Library", "SoundThumbnailWidth", Settings::defaultSoundWidth);;

    KeyboardLocale = new Setting(this, "Board", "StartupKeyboardLocale", 0);
    swapControlAndDisplayScreens = new Setting(this, "App", "SwapControlAndDisplayScreens", false);

    angleTolerance = new Setting(this, "App", "AngleTolerance", 4);
    historyLimit = new Setting(this, "Web", "HistoryLimit", 15);

    libIconSize = new Setting(this, "Library", "LibIconSize", defaultLibraryIconSize);

    useSystemOnScreenKeyboard = new Setting(this, "App", "UseSystemOnScreenKeyboard", true);

    showDateColumnOnAlphabeticalSort = new Setting(this, "Document", "ShowDateColumnOnAlphabeticalSort", false);
    emptyTrashForOlderDocuments = new Setting(this, "Document", "emptyTrashForOlderDocuments", false);
    emptyTrashDaysValue = new Setting(this, "Document", "emptyTrashDaysValue", 30);

    cleanNonPersistentSettings();
    checkNewSettings();
}


/**
 * @brief Returns the value for the *key* setting, or *defaultValue* if the key doesn't exist
 *
 * The value is also added to the local settings queue, to prevent future disk I/O when accessing
 * that same setting.
 *
 * If the value doesn't exist in the application settings (i.e it was not present in the config file),
 * it is also added there.
 */
QVariant Settings::value ( const QString & key, const QVariant & defaultValue)
{
    // Check first the settings queue, then the app settings, then the user settings.
    // If the key exists in neither of these, then defaultValue is returned.

    if (mSettingsQueue.contains(key))
        return mSettingsQueue.value(key);

    // If the setting doesn't exist in the App settings, add it there
    if (!sAppSettings->contains(key) && !(defaultValue == QVariant()))
        sAppSettings->setValue(key, defaultValue);

    // Get the value from the user settings (or if it doesn't exist there, from the app settings)
    QVariant val = mUserSettings->value(key, sAppSettings->value(key, defaultValue));

    // Add the value to the settings queue for faster access next time it is requested
    mSettingsQueue[key] = val;

    return val;
}


void Settings::setValue (const QString & key, const QVariant & value)
{
    // Save the setting to the queue only; a call to save() is necessary to persist the settings
    mSettingsQueue[key] = value;
}

/**
 * @brief Save all the queued settings to disk
 */
void Settings::save()
{
    QHash<QString, QVariant>::const_iterator it = mSettingsQueue.constBegin();

    while (it != mSettingsQueue.constEnd()) {
        /*
         * We save the setting to the user settings if
         * a) it is different from the (non-null) value stored in the user settings, or
         * b) it doesn't currently exist in the user settings AND has changed from the app settings
        */
        if (mUserSettings->contains(it.key())
                && it.value() != mUserSettings->value(it.key()))
        {
            mUserSettings->setValue(it.key(), it.value());
        }

        else if (!mUserSettings->contains(it.key())
                 && it.value() != mAppSettings->value(it.key()))
        {
            mUserSettings->setValue(it.key(), it.value());
        }

        ++it;
    }

    // Force save to file
    mUserSettings->sync();

    qDebug() << "User settings saved";
}

int Settings::penWidthIndex()
{
    return value("Board/PenLineWidthIndex", 0).toInt();
}


void Settings::setPenWidthIndex(int index)
{
    if (index != penWidthIndex())
    {
        setValue("Board/PenLineWidthIndex", index);
    }
}


qreal Settings::currentPenWidth()
{
    qreal width = 0;

    switch (penWidthIndex())
    {
        case Width::Fine:
            width = boardPenFineWidth->get().toDouble();
            break;
        case Width::Medium:
            width = boardPenMediumWidth->get().toDouble();
            break;
        case Width::Strong:
            width = boardPenStrongWidth->get().toDouble();
            break;
        default:
            Q_ASSERT(false);
            //failsafe
            width = boardPenFineWidth->get().toDouble();
            break;
    }

    return width;
}


int Settings::penColorIndex()
{
    return value("Board/PenColorIndex", 0).toInt();
}


void Settings::setPenColorIndex(int index)
{
    if (index != penColorIndex())
    {
        setValue("Board/PenColorIndex", index);
    }
}


QColor Settings::currentPenColor()
{
    return penColor(isDarkBackground());
}


QColor Settings::penColor(bool onDarkBackground)
{
    QList<QColor> colors = penColors(onDarkBackground);
    return colors.at(penColorIndex());
}


QList<QColor> Settings::penColors(bool onDarkBackground)
{
    if (onDarkBackground)
    {
        return boardPenDarkBackgroundSelectedColors->colors();
    }
    else
    {
        return boardPenLightBackgroundSelectedColors->colors();
    }
}


int Settings::markerWidthIndex()
{
    return value("Board/MarkerLineWidthIndex", 0).toInt();
}


void Settings::setMarkerWidthIndex(int index)
{
    if (index != markerWidthIndex())
    {
        setValue("Board/MarkerLineWidthIndex", index);
    }
}


qreal Settings::currentMarkerWidth()
{
    qreal width = 0;

    switch (markerWidthIndex())
    {
        case Width::Fine:
            width = boardMarkerFineWidth->get().toDouble();
            break;
        case Width::Medium:
            width = boardMarkerMediumWidth->get().toDouble();
            break;
        case Width::Strong:
            width = boardMarkerStrongWidth->get().toDouble();
            break;
        default:
            Q_ASSERT(false);
            //failsafe
            width = boardMarkerFineWidth->get().toDouble();
            break;
    }

    return width;
}


int Settings::markerColorIndex()
{
    return value("Board/MarkerColorIndex", 0).toInt();
}


void Settings::setMarkerColorIndex(int index)
{
    if (index != markerColorIndex())
    {
        setValue("Board/MarkerColorIndex", index);
    }
}


QColor Settings::currentMarkerColor()
{
    return markerColor(isDarkBackground());
}


QColor Settings::markerColor(bool onDarkBackground)
{
    QList<QColor> colors = markerColors(onDarkBackground);
    return colors.at(markerColorIndex());
}


QList<QColor> Settings::markerColors(bool onDarkBackground)
{
    if (onDarkBackground)
    {
        return boardMarkerDarkBackgroundSelectedColors->colors();
    }
    else
    {
        return boardMarkerLightBackgroundSelectedColors->colors();
    }
}

//----------------------------------------//
// eraser

int Settings::eraserWidthIndex()
{
    return value("Board/EraserCircleWidthIndex", 1).toInt();
}

void Settings::setEraserWidthIndex(int index)
{
    setValue("Board/EraserCircleWidthIndex", index);
}

qreal Settings::eraserFineWidth()
{
    return value("Board/EraserFineWidth", 16).toDouble();
}

void Settings::setEraserFineWidth(qreal width)
{
    setValue("Board/EraserFineWidth", width);
}

qreal Settings::eraserMediumWidth()
{
    return value("Board/EraserMediumWidth", 64).toDouble();
}

void Settings::setEraserMediumWidth(qreal width)
{
    setValue("Board/EraserMediumWidth", width);
}

qreal Settings::eraserStrongWidth()
{
    return value("Board/EraserStrongWidth", 128).toDouble();
}

void Settings::setEraserStrongWidth(qreal width)
{
    setValue("Board/EraserStrongWidth", width);
}

qreal Settings::currentEraserWidth()
{
    qreal width = 0;

    switch (eraserWidthIndex())
    {
        case Width::Fine:
            width = eraserFineWidth();
            break;
        case Width::Medium:
            width = eraserMediumWidth();
            break;
        case Width::Strong:
            width = eraserStrongWidth();
            break;
        default:
            Q_ASSERT(false);
            //failsafe
            width = eraserFineWidth();
            break;
    }

    return width;
}

bool Settings::isDarkBackground()
{
    return value("Board/DarkBackground", 0).toBool();
}


PageBackground Settings::pageBackground()
{
    QString val = value("Board/PageBackground", 0).toString();

    if (val == "crossed")
        return PageBackground::crossed;
    else if (val == "ruled")
        return PageBackground::ruled;
    else
        return PageBackground::plain;
}


void Settings::setDarkBackground(bool isDarkBackground)
{
    setValue("Board/DarkBackground", isDarkBackground);
    emit colorContextChanged();
}


void Settings::setPageBackground(PageBackground background)
{
    QString val;

    if (background == PageBackground::crossed)
        val = "crossed";
    else if (background == PageBackground::ruled)
        val = "ruled";
    else
        val = "plain";

    setValue("Board/PageBackground", val);
}


void Settings::setPenPressureSensitive(bool sensitive)
{
    boardPenPressureSensitive->set(sensitive);
}

void Settings::setPenPreviewCircle(bool circle)
{
    showPenPreviewCircle->set(circle);
}

void Settings::setPenPreviewFromSize(int size)
{
    penPreviewFromSize->set(size);
}

void Settings::setMarkerPressureSensitive(bool sensitive)
{
    boardMarkerPressureSensitive->set(sensitive);
}


bool Settings::isStylusPaletteVisible()
{
    return value("Board/StylusPaletteIsVisible", true).toBool();
}


void Settings::setStylusPaletteVisible(bool visible)
{
    setValue("Board/StylusPaletteIsVisible", visible);
}


QString Settings::fontFamily()
{
    return value("Board/FontFamily", sDefaultFontFamily).toString();
}


void Settings::setFontFamily(const QString &family)
{
    setValue("Board/FontFamily", family);
}


int Settings::fontPixelSize()
{
    return value("Board/FontPixelSize", sDefaultFontPixelSize).toInt();
}


void Settings::setFontPixelSize(int pixelSize)
{
    setValue("Board/FontPixelSize", pixelSize);
}

int Settings::fontPointSize()
{
    return value("Board/FontPointSize", 12).toInt();
}

void Settings::setFontPointSize(int pointSize)
{
    setValue("Board/FontPointSize", pointSize);
}

bool Settings::isBoldFont()
{
    return value("Board/FontIsBold", false).toBool();
}


void Settings::setBoldFont(bool bold)
{
    setValue("Board/FontIsBold", bold);
}


bool Settings::isItalicFont()
{
    return value("Board/FontIsItalic", false).toBool();
}


void Settings::setItalicFont(bool italic)
{
    setValue("Board/FontIsItalic", italic);
}


QString Settings::userDataDirectory()
{
    static QString dataDirPath = "";
    if(dataDirPath.isEmpty()){
        if (getAppSettings() && getAppSettings()->contains("App/DataDirectory")) {
            qDebug() << "getAppSettings()->contains(App/DataDirectory):" << getAppSettings()->contains("App/DataDirectory");
            dataDirPath = getAppSettings()->value("App/DataDirectory").toString();
            dataDirPath = replaceWildcard(dataDirPath);

            if(checkDirectory(dataDirPath))
                return dataDirPath;
            else
                qCritical() << "Impossible to create datadirpath " << dataDirPath;

        }
        dataDirPath = FileSystemUtils::normalizeFilePath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
        if (qApp->organizationName().size() > 0)
            dataDirPath.replace(qApp->organizationName() + "/", "");
    }
    return dataDirPath;
}


QString Settings::userImageDirectory()
{
    static QString imageDirectory = "";
    if(imageDirectory.isEmpty()){
        if (sAppSettings && getAppSettings()->contains("App/UserImageDirectory")) {
            imageDirectory = getAppSettings()->value("App/UserImageDirectory").toString();

            imageDirectory = replaceWildcard(imageDirectory);
            if(checkDirectory(imageDirectory))
                return imageDirectory;
            else
                qCritical() << "failed to create image directory " << imageDirectory;
        }

        imageDirectory = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/" + qApp->applicationName();
        checkDirectory(imageDirectory);
    }
    return imageDirectory;
}


QString Settings::userVideoDirectory()
{
    static QString videoDirectory = "";
    if(videoDirectory.isEmpty()){
        if (sAppSettings && getAppSettings()->contains("App/UserVideoDirectory")) {
            videoDirectory = getAppSettings()->value("App/UserVideoDirectory").toString();
            videoDirectory = replaceWildcard(videoDirectory);
            if(checkDirectory(videoDirectory))
                return videoDirectory;
            else
                qCritical() << "failed to create video directory " << videoDirectory;
        }


        videoDirectory = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);

        if(videoDirectory.isEmpty())
            videoDirectory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/" + tr("My Movies");
        else
            videoDirectory = videoDirectory + "/" + qApp->applicationName();

        checkDirectory(videoDirectory);
    }
    return videoDirectory;
}

QString Settings::userAudioDirectory()
{
    static QString audioDirectory = "";
    if(audioDirectory.isEmpty()){
        if (sAppSettings && getAppSettings()->contains("App/UserAudioDirectory")) {
            audioDirectory = getAppSettings()->value("App/UserAudioDirectory").toString();

            audioDirectory = replaceWildcard(audioDirectory);
            if(checkDirectory(audioDirectory))
                return audioDirectory;
            else
                qCritical() << "failed to create image directory " << audioDirectory;
        }

        audioDirectory = QStandardPaths::writableLocation(QStandardPaths::MusicLocation) + "/" + qApp->applicationName();
        checkDirectory(audioDirectory);
    }
    return audioDirectory;
}


QString Settings::userRecordingDirectory()
{
    static QString dirPath = "";
    if(dirPath.isEmpty()){
        if (sAppSettings && getAppSettings()->contains("Recording/RecordingDirectory"))
        {
            dirPath = getAppSettings()->value("Recording/RecordingDirectory").toString();
            dirPath = replaceWildcard(dirPath);
            if(checkDirectory(dirPath))
                return dirPath;
            else
                qCritical() << "failed to create dir " << dirPath;

        }
        dirPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        checkDirectory(dirPath);
    }
    return dirPath;
}

QString Settings::userDocumentDirectory()
{
    static QString documentDirectory = "";
    if(documentDirectory.isEmpty()){
        documentDirectory = userDataDirectory() + "/document";
        checkDirectory(documentDirectory);
    }
    qDebug() << "userDocumentDirectory()" << documentDirectory;
    return documentDirectory;
}

QString Settings::userFavoriteListFilePath()
{
    static QString filePath = "";
    if(filePath.isEmpty()){
        QString dirPath = userDataDirectory() + "/libraryPalette";
        checkDirectory(dirPath);
        filePath = dirPath + "/favorite.dat";
    }
    return filePath;
}

QString Settings::userTrashDirPath()
{
    static QString trashPath = "";
    if(trashPath.isEmpty()){
        trashPath = userDataDirectory() + "/libraryPalette/trash";
        checkDirectory(trashPath);
    }
    return trashPath;
}


QString Settings::userGipLibraryDirectory()
{
    static QString dirPath = "";
    if(dirPath.isEmpty()){
        dirPath = userDataDirectory() + "/library/gips";
        checkDirectory(dirPath);
    }
    return dirPath;
}


QString Settings::applicationShapeLibraryDirectory()
{
    QString defaultRelativePath = QString("./library/shape");

    QString configPath = value("Library/ShapeDirectory", QVariant(defaultRelativePath)).toString();

    if (configPath.startsWith(".")) {
        return PlatformUtils::applicationResourcesDirectory() + configPath.right(configPath.size() - 1);
    }
    else {
        return configPath;
    }
}

QString Settings::applicationCustomizationDirectory()
{
    QString defaultRelativePath = QString("/customizations");
    return PlatformUtils::applicationResourcesDirectory() + defaultRelativePath;

}

QString Settings::applicationCustomFontDirectory()
{
    QString defaultFontDirectory = "/fonts";
    return applicationCustomizationDirectory() + defaultFontDirectory;
}

QString Settings::userSearchDirectory()
{
    static QString dirPath = "";
    if(dirPath.isEmpty()){
        dirPath = PlatformUtils::applicationResourcesDirectory() + "/library/search";
        checkDirectory(dirPath);
    }
    return dirPath;
}

QString Settings::applicationImageLibraryDirectory()
{
    QString defaultRelativePath = QString("./library/pictures");

    QString configPath = value("Library/ImageDirectory", QVariant(defaultRelativePath)).toString();

    if (configPath.startsWith(".")) {
        return PlatformUtils::applicationResourcesDirectory() + configPath.right(configPath.size() - 1);
    }
    else {
        return configPath;
    }
}

QString Settings::userAnimationDirectory()
{
    static QString animationDirectory = "";
    if(animationDirectory.isEmpty()){
        animationDirectory = userDataDirectory() + "/animationUserDirectory";
        checkDirectory(animationDirectory);
    }
    return animationDirectory;
}

QString Settings::userInteractiveDirectory()
{
    static QString interactiveDirectory = "";
    if(interactiveDirectory.isEmpty()){
        if (sAppSettings && getAppSettings()->contains("App/UserInteractiveContentDirectory")) {
            interactiveDirectory = getAppSettings()->value("App/UserInteractiveContentDirectory").toString();
            interactiveDirectory = replaceWildcard(interactiveDirectory);
            if(checkDirectory(interactiveDirectory))
                return interactiveDirectory;
            else
                qCritical() << "failed to create directory " << interactiveDirectory;
        }
        interactiveDirectory = userDataDirectory() + "/interactive content";
        checkDirectory(interactiveDirectory);
    }
    return interactiveDirectory;
}


QString Settings::applicationInteractivesDirectory()
{
    QString defaultRelativePath = QString("./library/interactivities");

    QString configPath = value("Library/InteractivitiesDirectory", QVariant(defaultRelativePath)).toString();

    if (configPath.startsWith(".")) {
        return PlatformUtils::applicationResourcesDirectory() + configPath.right(configPath.size() - 1);
    }
    else {
        return configPath;
    }
}


QString Settings::applicationAudiosLibraryDirectory()
{
    QString defaultRelativePath = QString("./library/audios");

    QString configPath = value("Library/AudiosDirectory", QVariant(defaultRelativePath)).toString();

    if (configPath.startsWith(".")) {
        return PlatformUtils::applicationResourcesDirectory() + configPath.right(configPath.size() - 1);
    }
    else {
        return configPath;
    }
}

QString Settings::applicationVideosLibraryDirectory()
{
    QString defaultRelativePath = QString("./library/videos");

    QString configPath = value("Library/VideosDirectory", QVariant(defaultRelativePath)).toString();

    if (configPath.startsWith(".")) {
        return PlatformUtils::applicationResourcesDirectory() + configPath.right(configPath.size() - 1);
    }
    else {
        return configPath;
    }
}

QString Settings::applicationAnimationsLibraryDirectory()
{
    QString defaultRelativePath = QString("./library/animations");

    QString configPath = value("Library/AnimationsDirectory", QVariant(defaultRelativePath)).toString();

    if (configPath.startsWith(".")) {
        return PlatformUtils::applicationResourcesDirectory() + configPath.right(configPath.size() - 1);
    }
    else {
        return configPath;
    }
}

QString Settings::applicationStartupHintsDirectory()
{
    QString defaultRelativePath = QString("./startupHints");

    QString configPath = value("StartupHintsDirectory", QVariant(defaultRelativePath)).toString();

    if (configPath.startsWith(".")) {
        return PlatformUtils::applicationResourcesDirectory() + configPath.right(configPath.size() - 1);
    }
    else
        return configPath;
}

QString Settings::userInteractiveFavoritesDirectory()
{
    static QString dirPath = "";
    if(dirPath.isEmpty()){
        if (sAppSettings && getAppSettings()->contains("App/UserInteractiveFavoritesDirectory")) {
            dirPath = getAppSettings()->value("App/UserInteractiveFavoritesDirectory").toString();
            dirPath = replaceWildcard(dirPath);
            if(checkDirectory(dirPath))
                return dirPath;
            else
                qCritical() << "failed to create directory " << dirPath;
        }

        dirPath = userDataDirectory() + "/interactive favorites";
        checkDirectory(dirPath);
    }
    return dirPath;
}


QNetworkProxy* Settings::httpProxy()
{
    QNetworkProxy* proxy = 0;

    if (mAppSettings->value("Proxy/Enabled", false).toBool()) {

        proxy = new QNetworkProxy();

        if (mAppSettings->value("Proxy/Type", "HTTP").toString() == "Socks5")
            proxy->setType(QNetworkProxy::Socks5Proxy);
        else
            proxy->setType(QNetworkProxy::HttpProxy);

        proxy->setHostName(mAppSettings->value("Proxy/HostName").toString());
        proxy->setPort(mAppSettings->value("Proxy/Port", 1080).toInt());
        proxy->setUser(mAppSettings->value("Proxy/UserName").toString());
        proxy->setPassword(mAppSettings->value("Proxy/Password").toString());
    }

    return proxy;
}


void Settings::setPassword(const QString& id, const QString& password)
{
    QString encrypted = CryptoUtils::instance()->symetricEncrypt(password);

    mUserSettings->setValue(QString("Vault/") + id, encrypted);
}


void Settings::removePassword(const QString& id)
{
    mUserSettings->remove(QString("Vault/") + id);
}


QString Settings::password(const QString& id)
{
    QString encrypted = mUserSettings->value(QString("Vault/") + id).toString();

    QString result = "";

    if (encrypted.length() > 0)
        result =  CryptoUtils::instance()->symetricDecrypt(encrypted);

    return result;
}


QString Settings::proxyUsername()
{
    QString idUsername = "http.proxy.user";
    return password(idUsername);
}


void Settings::setProxyUsername(const QString& username)
{
    QString idUsername = "http.proxy.user";

    if (username.length() > 0)
        setPassword(idUsername, username);
    else
        removePassword(idUsername);
}


QString Settings::proxyPassword()
{
    QString idPassword = "http.proxy.pass";
    return password(idPassword);
}


void Settings::setProxyPassword(const QString& password)
{
    QString idPassword = "http.proxy.pass";

    if (password.length() > 0)
       setPassword(idPassword, password);
    else
        removePassword(idPassword);

}

QString Settings::communityUsername()
{
    return communityUser->get().toString();
}

void Settings::setCommunityUsername(const QString &username)
{
    communityUser->set(QVariant(username));
}

QString Settings::communityPassword()
{
    return communityPsw->get().toString();
}

void Settings::setCommunityPassword(const QString &password)
{
    communityPsw->set(QVariant(password));
}

void Settings::setCommunityPersistence(const bool persistence)
{
    communityCredentialsPersistence->set(QVariant(persistence));
}

int Settings::libraryIconSize(){
    return libIconSize->get().toInt();
}

void Settings::setLibraryIconsize(const int& size){
    libIconSize->set(QVariant(size));
}

bool Settings::checkDirectory(QString& dirPath)
{
    bool result = true;
    QDir dir(dirPath);
    if(!dir.exists())
        result = dir.mkpath(dirPath);
    return result;
}

QString Settings::replaceWildcard(QString& path)
{
    QString result(path);

    if (result.startsWith("{Documents}")) {
        result = result.replace("{Documents}", QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
    }
    else if(result.startsWith("{Home}")) {
        result = result.replace("{Home}", QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    }
    else if(result.startsWith("{Desktop}")) {
        result = result.replace("{Desktop}", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
    }

    if(result.contains("{UserLoginName}") && PlatformUtils::osUserLoginName().length() > 0) {
        result = result.replace("{UserLoginName}", PlatformUtils::osUserLoginName());
    }

    return result;
}

void Settings::closing()
{
    save();
    cleanNonPersistentSettings();
}

void Settings::cleanNonPersistentSettings()
{
    if(!communityCredentialsPersistence->get().toBool()){
        communityPsw->set(QVariant(""));
        communityUser->set(QVariant(""));
    }

}

/**
 * @brief Permanently remove a setting, from local memory and config files
 * @param setting The setting to remove
 */
void Settings::removeSetting(const QString &setting)
{
    if (sAppSettings->contains(setting))
        sAppSettings->remove(setting);

    if (mUserSettings->contains(setting))
        mUserSettings->remove(setting);

    if (mSettingsQueue.contains(setting))
        mSettingsQueue.remove(setting);
}

void Settings::checkNewSettings()
{
    /*
     * Some settings were modified in new versions and TianBoard can crash
     * if an old settings file is used. This function checks these settings and
     * if necessary, resets them to the values initialized in Settings::init().
     *
     * Thus this method can be removed when it is no longer deemed useful.
     */

    // OB 1.3 introduced an extra pen color;  for simplicity, if the old settings
    // are still present (i.e only 4 selected pen colors), we just reset all color settings.
    // Having too few colors actually causes TianBoard to crash, hence these measures.

    QList<ColorListSetting*> colorSettings;
    colorSettings << boardPenDarkBackgroundSelectedColors
                  << boardPenLightBackgroundSelectedColors
                  << boardMarkerDarkBackgroundSelectedColors
                  << boardMarkerLightBackgroundSelectedColors;

    foreach (ColorListSetting* setting, colorSettings) {
        if (setting->colors().size() < 5)
            setting->reset();
    }

    colorSettings.clear();

    // Next we check whether all the new colors were added; i.e if some default
    // colors are not also in the config file, we add them to it.

    // This is not nearly as critical as the above issue, but the users (or admins) seemingly
    // can't be trusted to delete or modify their own config file when upgrading, so they might
    // wonder why they don't have the new colors in their app.

    colorSettings << boardPenDarkBackgroundColors
                  << boardPenLightBackgroundColors
                  << boardMarkerDarkBackgroundColors
                  << boardMarkerLightBackgroundColors;

    foreach (ColorListSetting* setting, colorSettings) {
        QStringList defaultColors = qvariant_cast<QStringList>(setting->defaultValue());
        QStringList currentColors(qvariant_cast<QStringList>(setting->get())); // copy

        foreach (QString c, defaultColors) {
            if (!currentColors.contains(c))
                currentColors.append(QString(c));
        }

        setting->set(currentColors);
    }


    // A typo was corrected in version 1.3
    removeSetting("Board/useSystemOnScreenKeybard");


    // CrossedBackground changed in 1.4 (no longer a bool but an enum; can be crossed or ruled)
    removeSetting("Board/CrossedBackground");

}
