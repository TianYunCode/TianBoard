#pragma once

#include <QtCore>
#include <QtWidgets>
#include <QtNetwork>

#include "utils/Enums.h"
#include "settings/Setting.h"

class Settings : public QObject {
    Q_OBJECT

public:
    static Settings *settings();
    static void destroy();

private:
    Settings(QObject *parent = 0);
    virtual ~Settings();

public:
    QStringList *supportedKeyboardSizes;
    void InitKeyboardPaletteKeyBtnSizes();
    void ValidateKeyboardPaletteKeyBtnSize();
    void closing();
    void save();

    int penWidthIndex();

    qreal currentPenWidth();

    int penColorIndex();
    QColor currentPenColor();
    QColor penColor(bool onDarkBackground);
    QList<QColor> penColors(bool onDarkBackground);

    // Marker related
    int markerWidthIndex();
    qreal currentMarkerWidth();
    int markerColorIndex();
    QColor currentMarkerColor();
    QColor markerColor(bool onDarkBackground);
    QList<QColor> markerColors(bool onDarkBackground);

    // Eraser related
    int eraserWidthIndex();
    qreal eraserFineWidth();
    qreal eraserMediumWidth();
    qreal eraserStrongWidth();
    qreal currentEraserWidth();

    // Background related
    bool isDarkBackground();
    PageBackground pageBackground();
    void setDarkBackground(bool isDarkBackground);
    void setPageBackground(PageBackground background);

    // Stylus palette related
    bool isStylusPaletteVisible();

    // Text related
    QString fontFamily();
    void setFontFamily(const QString &family);
    int fontPixelSize();
    void setFontPixelSize(int pixelSize);
    int fontPointSize();
    void setFontPointSize(int pointSize);
    bool isBoldFont();
    void setBoldFont(bool bold);
    bool isItalicFont();
    void setItalicFont(bool italic);

    int libraryIconSize();
    void setLibraryIconsize(const int &size);

    void init();

    // 路径设置里的 {UserLoginName} 靠这个展开。登录名只有操作系统知道，
    // 但由装配层（core/Application）在启动时推进来——设置层不该反过来认识
    // platform，否则每个读设置的模块都被迫为拿一个开关拖进整棵平台依赖。
    static void setUserLoginName(const QString &loginName);
    static QString userLoginName();

    //user directories
    static QString userDataDirectory();
    static QString userDocumentDirectory();
    static QString userFavoriteListFilePath();
    static QString userTrashDirPath();
    static QString userImageDirectory();
    static QString userVideoDirectory();
    static QString userAudioDirectory();
    static QString userSearchDirectory();
    static QString userAnimationDirectory();
    static QString userInteractiveDirectory();
    static QString userInteractiveFavoritesDirectory();
    static QString userRecordingDirectory();

    QString userGipLibraryDirectory();

    //application directory
    QString applicationShapeLibraryDirectory();
    QString applicationImageLibraryDirectory();
    QString applicationInteractivesDirectory();
    QString applicationCustomizationDirectory();
    QString applicationCustomFontDirectory();
    QString applicationAudiosLibraryDirectory();
    QString applicationVideosLibraryDirectory();
    QString applicationAnimationsLibraryDirectory();
    QString applicationStartupHintsDirectory();

    QNetworkProxy *httpProxy();

    static int pointerDiameter;
    static int boardMargin;

    static QColor paletteColor;
    static QColor opaquePaletteColor;

    static QColor documentViewLightColor;

    static QBrush eraserBrushDarkBackground;
    static QBrush eraserBrushLightBackground;

    static QPen eraserPenDarkBackground;
    static QPen eraserPenLightBackground;

    static QColor markerCircleBrushColorDarkBackground;
    static QColor markerCircleBrushColorLightBackground;

    static QColor markerCirclePenColorDarkBackground;
    static QColor markerCirclePenColorLightBackground;

    static QColor penCircleBrushColorDarkBackground;
    static QColor penCircleBrushColorLightBackground;

    static QColor penCirclePenColorDarkBackground;
    static QColor penCirclePenColorLightBackground;

    static QColor documentSizeMarkColorDarkBackground;
    static QColor documentSizeMarkColorLightBackground;

    // Background grid
    static int crossSize;
    static int defaultCrossSize;
    static int minCrossSize;
    static int maxCrossSize;

    static int colorPaletteSize;
    static int objectFrameWidth;

    static QString documentGroupName;
    static QString documentName;
    static QString documentSize;
    static QString documentIdentifer;
    static QString documentVersion;
    static QString documentUpdatedAt;
    static QString documentPageCount;

    static QString documentDate;

    static QString trashedDocumentGroupNamePrefix;

    static QString currentFileVersion;

    static QString documentNamespaceUri;
    static QString applicationNamespaceUri;

    static QString undoCommandTransactionName;

    static const int maxThumbnailWidth;
    static const int defaultThumbnailWidth;
    static const int defaultSortKind;
    static const int defaultSortOrder;
    static const int defaultSplitterLeftSize;
    static const int defaultSplitterRightSize;
    static const int defaultLibraryIconSize;

    static const int defaultImageWidth;
    static const int defaultShapeWidth;
    static const int defaultWidgetIconWidth;
    static const int defaultVideoWidth;
    static const int defaultGipWidth;
    static const int defaultSoundWidth;

    static const int thumbnailSpacing;
    static const int longClickInterval;

    static const qreal minScreenRatio;

    static QStringList bitmapFileExtensions;
    static QStringList vectoFileExtensions;
    static QStringList imageFileExtensions;
    static QStringList widgetFileExtensions;
    static QStringList interactiveContentFileExtensions;

    static QColor treeViewBackgroundColor;

    static int objectInControlViewMargin;

    static QString appPingMessage;

    Setting *productWebUrl;

    QString softwareHomeUrl;

    Setting *appToolBarPositionedAtTop;
    Setting *appToolBarDisplayText;
    Setting *appHideSwapDisplayScreens;
    Setting *appToolBarOrientationVertical;
    Setting *appPreferredLanguage;


    Setting *appLastSessionDocumentUUID;
    Setting *appLastSessionPageIndex;

    Setting *appUseMultiscreen;

    Setting *appStartupHintsEnabled;

    Setting *boardPenFineWidth;
    Setting *boardPenMediumWidth;
    Setting *boardPenStrongWidth;

    Setting *boardMarkerFineWidth;
    Setting *boardMarkerMediumWidth;
    Setting *boardMarkerStrongWidth;

    Setting *boardPenPressureSensitive;
    Setting *boardMarkerPressureSensitive;

    Setting *boardUseHighResTabletEvent;

    Setting *boardInterpolatePenStrokes;
    Setting *boardSimplifyPenStrokes;
    Setting *boardSimplifyPenStrokesThresholdAngle;
    Setting *boardSimplifyPenStrokesThresholdWidthDifference;
    Setting *boardInterpolateMarkerStrokes;
    Setting *boardSimplifyMarkerStrokes;

    Setting *boardKeyboardPaletteKeyBtnSize;

    Setting *appStartMode;

    Setting *featureSliderPosition;

    Setting *boardCrossColorDarkBackground;
    Setting *boardCrossColorLightBackground;

    ColorListSetting *boardGridLightBackgroundColors;
    ColorListSetting *boardGridDarkBackgroundColors;

    ColorListSetting *boardPenLightBackgroundColors;
    ColorListSetting *boardPenLightBackgroundSelectedColors;

    ColorListSetting *boardPenDarkBackgroundColors;
    ColorListSetting *boardPenDarkBackgroundSelectedColors;

    Setting *boardMarkerAlpha;

    ColorListSetting *boardMarkerLightBackgroundColors;
    ColorListSetting *boardMarkerLightBackgroundSelectedColors;

    ColorListSetting *boardMarkerDarkBackgroundColors;
    ColorListSetting *boardMarkerDarkBackgroundSelectedColors;

    Setting *showEraserPreviewCircle;
    Setting *showMarkerPreviewCircle;
    Setting *showPenPreviewCircle;
    Setting *penPreviewFromSize;

    Setting *webUseExternalBrowser;
    Setting *webShowPageImmediatelyOnMirroredScreen;

    Setting *webHomePage;

    Setting *pageCacheSize;

    Setting *boardZoomFactor;

    Setting *mirroringRefreshRateInFps;

    Setting *lastImportFilePath;
    Setting *lastImportFolderPath;

    Setting *lastExportFilePath;
    Setting *lastExportDirPath;

    Setting *lastImportToLibraryPath;

    Setting *lastPicturePath;
    Setting *lastWidgetPath;
    Setting *lastVideoPath;

    Setting *appOnlineUserName;

    Setting *boardShowToolsPalette;

    QMap<DocumentSizeRatio::Enum, QSize> documentSizes;

    Setting *svgViewBoxMargin;
    Setting *pdfMargin;
    Setting *pdfPageFormat;
    Setting *pdfResolution;

    Setting *recordingFramesPerSecond;
    Setting *recordingVideoSize;
    Setting *recordingWindowsMediaBitsPerSecond;
    Setting *recordingAudioRecordingDevice;
    Setting *recordingQuickTimeQuality;

    Setting *favoritesNativeToolUris;

    Setting *documentThumbnailWidth;
    Setting *documentSortKind;
    Setting *documentSortOrder;
    Setting *documentSplitterLeftSize;
    Setting *documentSplitterRightSize;
    Setting *imageThumbnailWidth;
    Setting *videoThumbnailWidth;
    Setting *shapeThumbnailWidth;
    Setting *gipThumbnailWidth;
    Setting *soundThumbnailWidth;

    Setting *libraryShowDetailsForLocalItems;

    Setting *rightLibPaletteBoardModeWidth;
    Setting *rightLibPaletteBoardModeIsCollapsed;
    Setting *rightLibPaletteDesktopModeWidth;
    Setting *rightLibPaletteDesktopModeIsCollapsed;
    Setting *leftLibPaletteBoardModeWidth;
    Setting *leftLibPaletteBoardModeIsCollapsed;
    Setting *leftLibPaletteDesktopModeWidth;
    Setting *leftLibPaletteDesktopModeIsCollapsed;


    Setting *pageSize;

    Setting *KeyboardLocale;
    Setting *swapControlAndDisplayScreens;

    Setting *angleTolerance;
    Setting *historyLimit;

    Setting *libIconSize;

    Setting *useSystemOnScreenKeyboard;

    Setting *showDateColumnOnAlphabeticalSort;

    Setting *emptyTrashForOlderDocuments;
    Setting *emptyTrashDaysValue;

    Setting *magnifierDrawingMode;
    Setting *autoSaveInterval;

public slots:
    void setPenWidthIndex(int index);
    void setPenColorIndex(int index);

    void setMarkerWidthIndex(int index);
    void setMarkerColorIndex(int index);

    void setEraserWidthIndex(int index);
    void setEraserFineWidth(qreal width);
    void setEraserMediumWidth(qreal width);
    void setEraserStrongWidth(qreal width);

    void setStylusPaletteVisible(bool visible);

    void setPenPressureSensitive(bool sensitive);
    void setPenPreviewCircle(bool sensitive);
    void setPenPreviewFromSize(int size);
    void setMarkerPressureSensitive(bool sensitive);

    QVariant value(const QString &key, const QVariant &defaultValue = QVariant());
    void setValue(const QString &key, const QVariant &value);

    void colorChanged() { emit colorContextChanged(); }

signals:
    void colorContextChanged();

private:
    QSettings *mAppSettings;
    QSettings *mUserSettings;

    QHash<QString, QVariant> mSettingsQueue;

    static const int sDefaultFontPixelSize;
    static const char *sDefaultFontFamily;

    static QSettings *getAppSettings();

    static QPointer<QSettings> sAppSettings;
    static QPointer<Settings> sSingleton;
    static QString sUserLoginName;

    static bool checkDirectory(QString &dirPath);
    static QString replaceWildcard(QString &path);

    void removeSetting(const QString &setting);
    void checkNewSettings();
};
