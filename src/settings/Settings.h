/**
 * @file Settings.h
 * @brief 声明应用的设置单例、全部配置项、用户目录与代理访问入口。
 */

#pragma once

#include <QtCore>
#include <QtWidgets>
#include <QtNetwork>

#include "utils/Enums.h"
#include "settings/Setting.h"

/**
 * @brief 应用配置注册表。
 *
 * 位于 settings 层（设置注册表）：集中持有全部设置项，并负责用户目录与 HTTP 代理。
 * 单例由 settings() 获取、destroy() 释放；构造时加载应用级与用户级 INI 并建立各
 * Setting 对象，退出前通过 save()/closing() 落盘。上层绘图、资源库、录屏等模块只
 * 通过本类读写配置，不直接接触 QSettings。用户目录由标准位置推导，
 * 路径内的 {UserLoginName} 由装配层注入，避免设置层反向依赖平台层。
 * 所有访问均在 GUI 线程，无额外同步。
 */
class Settings : public QObject {
    Q_OBJECT

public:
    /** @brief 返回全局唯一设置实例，首次调用时创建。 */
    static Settings *settings();
    /** @brief 销毁单例并释放配置对象。 */
    static void destroy();

private:
    /** @brief 私有构造，保证单例。 */
    Settings(QObject *parent = 0);
    /** @brief 私有析构，保存后释放。 */
    virtual ~Settings();

public:
    QStringList *supportedKeyboardSizes; ///< 屏幕键盘可选的按键尺寸列表。
    /** @brief 构建屏幕键盘的按键尺寸候选表。 */
    void InitKeyboardPaletteKeyBtnSizes();
    /** @brief 校正已保存的按键尺寸，越界时回退到默认值。 */
    void ValidateKeyboardPaletteKeyBtnSize();
    /** @brief 程序退出前的收尾，保存尚未落盘的设置。 */
    void closing();
    /** @brief 立即把配置写入磁盘。 */
    void save();

    /** @brief 返回当前钢笔粗细档位索引。 */
    int penWidthIndex();

    /** @brief 返回当前钢笔的实际线宽。 */
    qreal currentPenWidth();

    /** @brief 返回当前钢笔颜色索引。 */
    int penColorIndex();
    /** @brief 返回当前钢笔颜色。 */
    QColor currentPenColor();
    /** @brief 返回指定背景下的钢笔颜色。 */
    QColor penColor(bool onDarkBackground);
    /** @brief 返回指定背景下的全部钢笔颜色。 */
    QList<QColor> penColors(bool onDarkBackground);

    // 荧光笔相关
    /** @brief 返回当前荧光笔粗细档位索引。 */
    int markerWidthIndex();
    /** @brief 返回当前荧光笔的实际线宽。 */
    qreal currentMarkerWidth();
    /** @brief 返回当前荧光笔颜色索引。 */
    int markerColorIndex();
    /** @brief 返回当前荧光笔颜色。 */
    QColor currentMarkerColor();
    /** @brief 返回指定背景下的荧光笔颜色。 */
    QColor markerColor(bool onDarkBackground);
    /** @brief 返回指定背景下的全部荧光笔颜色。 */
    QList<QColor> markerColors(bool onDarkBackground);

    // 橡皮相关
    /** @brief 返回当前橡皮粗细档位索引。 */
    int eraserWidthIndex();
    /** @brief 返回细档橡皮宽度。 */
    qreal eraserFineWidth();
    /** @brief 返回中档橡皮宽度。 */
    qreal eraserMediumWidth();
    /** @brief 返回粗档橡皮宽度。 */
    qreal eraserStrongWidth();
    /** @brief 返回当前橡皮的实际宽度。 */
    qreal currentEraserWidth();

    // 背景相关
    /** @brief 当前是否使用深色背景。 */
    bool isDarkBackground();
    /** @brief 返回当前页面背景样式。 */
    PageBackground pageBackground();
    /** @brief 切换深浅背景。 */
    void setDarkBackground(bool isDarkBackground);
    /** @brief 设置页面背景样式。 */
    void setPageBackground(PageBackground background);

    // 触控笔调色板相关
    /** @brief 触控笔调色板当前是否可见。 */
    bool isStylusPaletteVisible();

    // 文本相关
    /** @brief 返回当前文本字体族。 */
    QString fontFamily();
    /** @brief 设置文本字体族。 */
    void setFontFamily(const QString &family);
    /** @brief 返回文本字号（像素）。 */
    int fontPixelSize();
    /** @brief 设置文本字号（像素）。 */
    void setFontPixelSize(int pixelSize);
    /** @brief 返回文本字号（点）。 */
    int fontPointSize();
    /** @brief 设置文本字号（点）。 */
    void setFontPointSize(int pointSize);
    /** @brief 当前文本是否加粗。 */
    bool isBoldFont();
    /** @brief 设置文本加粗。 */
    void setBoldFont(bool bold);
    /** @brief 当前文本是否斜体。 */
    bool isItalicFont();
    /** @brief 设置文本斜体。 */
    void setItalicFont(bool italic);

    /** @brief 返回资源库图标尺寸。 */
    int libraryIconSize();
    /** @brief 设置资源库图标尺寸。 */
    void setLibraryIconsize(const int &size);

    /** @brief 首次运行时写入默认配置并建立用户目录。 */
    void init();

    // 路径设置里的 {UserLoginName} 靠这个展开。登录名只有操作系统知道，
    // 但由装配层（core/Application）在启动时推进来——设置层不该反过来认识
    // platform，否则每个读设置的模块都被迫为拿一个开关拖进整棵平台依赖。
    /** @brief 由装配层注入当前登录名，供路径通配符展开。 */
    static void setUserLoginName(const QString &loginName);
    /** @brief 返回装配层注入的登录名。 */
    static QString userLoginName();

    // 用户目录
    /** @brief 用户数据根目录。 */
    static QString userDataDirectory();
    /** @brief 用户文档目录。 */
    static QString userDocumentDirectory();
    /** @brief 收藏列表文件路径。 */
    static QString userFavoriteListFilePath();
    /** @brief 回收站目录。 */
    static QString userTrashDirPath();
    /** @brief 用户图片目录。 */
    static QString userImageDirectory();
    /** @brief 用户视频目录。 */
    static QString userVideoDirectory();
    /** @brief 用户音频目录。 */
    static QString userAudioDirectory();
    /** @brief 用户检索目录。 */
    static QString userSearchDirectory();
    /** @brief 用户动画目录。 */
    static QString userAnimationDirectory();
    /** @brief 用户互动内容目录。 */
    static QString userInteractiveDirectory();
    /** @brief 用户互动内容收藏目录。 */
    static QString userInteractiveFavoritesDirectory();
    /** @brief 用户录制文件目录。 */
    static QString userRecordingDirectory();

    /** @brief 用户 GIP 图片资源库目录。 */
    QString userGipLibraryDirectory();

    // 随程序安装发布的资源目录
    /** @brief 随程序发布的形状库目录。 */
    QString applicationShapeLibraryDirectory();
    /** @brief 随程序发布的图片库目录。 */
    QString applicationImageLibraryDirectory();
    /** @brief 随程序发布的互动内容目录。 */
    QString applicationInteractivesDirectory();
    /** @brief 随程序发布的定制资源目录。 */
    QString applicationCustomizationDirectory();
    /** @brief 随程序发布的字体目录。 */
    QString applicationCustomFontDirectory();
    /** @brief 随程序发布的音频库目录。 */
    QString applicationAudiosLibraryDirectory();
    /** @brief 随程序发布的视频库目录。 */
    QString applicationVideosLibraryDirectory();
    /** @brief 随程序发布的动画库目录。 */
    QString applicationAnimationsLibraryDirectory();

    /** @brief 返回配置了代理信息的 QNetworkProxy；未配置时返回直连代理。 */
    QNetworkProxy *httpProxy();

    static int pointerDiameter; ///< 指针光标直径。
    static int boardMargin;     ///< 画板四周留白。

    static QColor paletteColor;       ///< 调色板主色。
    static QColor opaquePaletteColor; ///< 不透明调色板色。

    static QColor documentViewLightColor; ///< 文档视图的浅色底。

    static QBrush eraserBrushDarkBackground;  ///< 深色背景下的橡皮画刷。
    static QBrush eraserBrushLightBackground; ///< 浅色背景下的橡皮画刷。

    static QPen eraserPenDarkBackground;  ///< 深色背景下的橡皮画笔。
    static QPen eraserPenLightBackground; ///< 浅色背景下的橡皮画笔。

    static QColor markerCircleBrushColorDarkBackground;  ///< 深色背景下荧光笔预览圆填充色。
    static QColor markerCircleBrushColorLightBackground; ///< 浅色背景下荧光笔预览圆填充色。

    static QColor markerCirclePenColorDarkBackground;  ///< 深色背景下荧光笔预览圆描边色。
    static QColor markerCirclePenColorLightBackground; ///< 浅色背景下荧光笔预览圆描边色。

    static QColor penCircleBrushColorDarkBackground;  ///< 深色背景下钢笔预览圆填充色。
    static QColor penCircleBrushColorLightBackground; ///< 浅色背景下钢笔预览圆填充色。

    static QColor penCirclePenColorDarkBackground;  ///< 深色背景下钢笔预览圆描边色。
    static QColor penCirclePenColorLightBackground; ///< 浅色背景下钢笔预览圆描边色。

    static QColor documentSizeMarkColorDarkBackground;  ///< 深色背景下文档尺寸标记色。
    static QColor documentSizeMarkColorLightBackground; ///< 浅色背景下文档尺寸标记色。

    // 背景网格
    static int crossSize;        ///< 当前网格间距。
    static int defaultCrossSize; ///< 默认网格间距。
    static int minCrossSize;     ///< 网格间距下限。
    static int maxCrossSize;     ///< 网格间距上限。

    static int colorPaletteSize; ///< 调色板色块数量。
    static int objectFrameWidth; ///< 对象选中框线宽。

    static QString documentGroupName; ///< 文档包内的组名键。
    static QString documentName;      ///< 文档名键。
    static QString documentSize;      ///< 文档尺寸键。
    static QString documentIdentifer; ///< 文档标识键。
    static QString documentVersion;   ///< 文档格式版本键。
    static QString documentUpdatedAt; ///< 文档更新时间键。
    static QString documentPageCount; ///< 文档页数键。

    static QString documentDate; ///< 文档日期键。

    static QString trashedDocumentGroupNamePrefix; ///< 回收站文档组名前缀。

    static QString currentFileVersion; ///< 当前读写器支持的文档格式版本。

    static QString documentNamespaceUri;    ///< 文档 XML 命名空间。
    static QString applicationNamespaceUri; ///< 应用 XML 命名空间。

    static QString undoCommandTransactionName; ///< 撤销事务名称。

    static const int maxThumbnailWidth;        ///< 缩略图最大宽度。
    static const int defaultThumbnailWidth;    ///< 默认缩略图宽度。
    static const int defaultSortKind;          ///< 默认排序字段。
    static const int defaultSortOrder;         ///< 默认排序方向。
    static const int defaultSplitterLeftSize;  ///< 左侧面板默认宽度。
    static const int defaultSplitterRightSize; ///< 右侧面板默认宽度。
    static const int defaultLibraryIconSize;   ///< 资源库默认图标尺寸。

    static const int defaultImageWidth;      ///< 图片对象默认宽度。
    static const int defaultShapeWidth;      ///< 形状对象默认宽度。
    static const int defaultWidgetIconWidth; ///< 小部件图标默认宽度。
    static const int defaultVideoWidth;      ///< 视频对象默认宽度。
    static const int defaultGipWidth;        ///< GIP 图片默认宽度。
    static const int defaultSoundWidth;      ///< 音频对象默认宽度。

    static const int thumbnailSpacing;  ///< 缩略图间距。
    static const int longClickInterval; ///< 长按判定时长（毫秒）。

    static const qreal minScreenRatio; ///< 屏幕最小宽高比。

    static QStringList bitmapFileExtensions;             ///< 位图后缀列表。
    static QStringList vectoFileExtensions;              ///< 矢量图后缀列表。
    static QStringList imageFileExtensions;              ///< 全部图片后缀列表。
    static QStringList widgetFileExtensions;             ///< 小部件后缀列表。
    static QStringList interactiveContentFileExtensions; ///< 互动内容后缀列表。

    static QColor treeViewBackgroundColor; ///< 树形视图背景色。

    static int objectInControlViewMargin; ///< 控件视图中对象的边距。

    static QString appPingMessage; ///< 用于单实例唤醒的进程间消息。

    Setting *productWebUrl; ///< 产品主页地址。

    QString softwareHomeUrl; ///< 软件主页地址（非设置项，启动时固定）。

    Setting *appToolBarPositionedAtTop;     ///< 工具栏是否停靠顶部。
    Setting *appToolBarDisplayText;         ///< 工具栏按钮是否显示文字。
    Setting *appHideSwapDisplayScreens;     ///< 是否隐藏“交换显示屏幕”按钮。
    Setting *appToolBarOrientationVertical; ///< 工具栏是否竖向排列。
    Setting *appPreferredLanguage;          ///< 首选界面语言。


    Setting *appLastSessionDocumentUUID; ///< 上次会话打开的文档 UUID。
    Setting *appLastSessionPageIndex;    ///< 上次会话停留的页码。

    Setting *appUseMultiscreen; ///< 是否使用多屏模式。


    Setting *boardPenFineWidth;   ///< 细档钢笔宽度。
    Setting *boardPenMediumWidth; ///< 中档钢笔宽度。
    Setting *boardPenStrongWidth; ///< 粗档钢笔宽度。

    Setting *boardMarkerFineWidth;   ///< 细档荧光笔宽度。
    Setting *boardMarkerMediumWidth; ///< 中档荧光笔宽度。
    Setting *boardMarkerStrongWidth; ///< 粗档荧光笔宽度。

    Setting *boardPenPressureSensitive;    ///< 钢笔是否响应压感。
    Setting *boardMarkerPressureSensitive; ///< 荧光笔是否响应压感。

    Setting *boardUseHighResTabletEvent; ///< 是否使用高精度数位板事件。

    Setting *boardInterpolatePenStrokes;                      ///< 钢笔笔画是否插值。
    Setting *boardSimplifyPenStrokes;                         ///< 钢笔笔画是否简化。
    Setting *boardSimplifyPenStrokesThresholdAngle;           ///< 简化时的角度阈值。
    Setting *boardSimplifyPenStrokesThresholdWidthDifference; ///< 简化时的笔宽差阈值。
    Setting *boardInterpolateMarkerStrokes;                   ///< 荧光笔笔画是否插值。
    Setting *boardSimplifyMarkerStrokes;                      ///< 荧光笔笔画是否简化。

    Setting *boardKeyboardPaletteKeyBtnSize; ///< 屏幕键盘按键尺寸。

    Setting *appStartMode; ///< 启动时的界面模式。

    Setting *featureSliderPosition; ///< 功能滑块位置。

    Setting *boardCrossColorDarkBackground;  ///< 深色背景网格线颜色。
    Setting *boardCrossColorLightBackground; ///< 浅色背景网格线颜色。

    ColorListSetting *boardGridLightBackgroundColors; ///< 浅色背景网格色候选。
    ColorListSetting *boardGridDarkBackgroundColors;  ///< 深色背景网格色候选。

    ColorListSetting *boardPenLightBackgroundColors;         ///< 浅色背景钢笔色候选。
    ColorListSetting *boardPenLightBackgroundSelectedColors; ///< 浅色背景选中的钢笔色。

    ColorListSetting *boardPenDarkBackgroundColors;         ///< 深色背景钢笔色候选。
    ColorListSetting *boardPenDarkBackgroundSelectedColors; ///< 深色背景选中的钢笔色。

    Setting *boardMarkerAlpha; ///< 荧光笔透明度。

    ColorListSetting *boardMarkerLightBackgroundColors;         ///< 浅色背景荧光笔色候选。
    ColorListSetting *boardMarkerLightBackgroundSelectedColors; ///< 浅色背景选中的荧光笔色。

    ColorListSetting *boardMarkerDarkBackgroundColors;         ///< 深色背景荧光笔色候选。
    ColorListSetting *boardMarkerDarkBackgroundSelectedColors; ///< 深色背景选中的荧光笔色。

    Setting *showEraserPreviewCircle; ///< 是否显示橡皮预览圆。
    Setting *showMarkerPreviewCircle; ///< 是否显示荧光笔预览圆。
    Setting *showPenPreviewCircle;    ///< 是否显示钢笔预览圆。
    Setting *penPreviewFromSize;      ///< 显示钢笔预览的最小笔宽。

    Setting *webUseExternalBrowser;                  ///< 网页对象是否用外部浏览器打开。
    Setting *webShowPageImmediatelyOnMirroredScreen; ///< 镜像屏是否立即显示网页。

    Setting *webHomePage; ///< 网页对象默认主页。

    Setting *pageCacheSize; ///< 页面缓存大小。

    Setting *boardZoomFactor; ///< 缩放步进系数。

    Setting *mirroringRefreshRateInFps; ///< 镜像刷新率（帧/秒）。

    Setting *lastImportFilePath;   ///< 上次导入文件的路径。
    Setting *lastImportFolderPath; ///< 上次导入目录的路径。

    Setting *lastExportFilePath; ///< 上次导出文件的路径。
    Setting *lastExportDirPath;  ///< 上次导出目录的路径。

    Setting *lastImportToLibraryPath; ///< 上次导入资源库的路径。

    Setting *lastPicturePath; ///< 上次选择图片的路径。
    Setting *lastWidgetPath;  ///< 上次选择小部件的路径。
    Setting *lastVideoPath;   ///< 上次选择视频的路径。

    Setting *appOnlineUserName; ///< 在线功能使用的用户名。

    Setting *boardShowToolsPalette; ///< 是否显示工具面板。

    QMap<DocumentSizeRatio::Enum, QSize> documentSizes; ///< 各宽高比对应的页面尺寸。

    Setting *svgViewBoxMargin; ///< SVG 视框留白。
    Setting *pdfMargin;        ///< PDF 页边距。
    Setting *pdfPageFormat;    ///< PDF 页面格式。
    Setting *pdfResolution;    ///< PDF 渲染分辨率。

    Setting *recordingFramesPerSecond;           ///< 录制帧率。
    Setting *recordingVideoSize;                 ///< 录制画面尺寸。
    Setting *recordingWindowsMediaBitsPerSecond; ///< Windows Media 录制码率。
    Setting *recordingAudioRecordingDevice;      ///< 录制使用的音频设备。
    Setting *recordingQuickTimeQuality;          ///< QuickTime 录制质量。

    Setting *favoritesNativeToolUris; ///< 收藏的内置工具 URI 列表。

    Setting *documentThumbnailWidth;    ///< 文档缩略图宽度。
    Setting *documentSortKind;          ///< 文档排序字段。
    Setting *documentSortOrder;         ///< 文档排序方向。
    Setting *documentSplitterLeftSize;  ///< 文档面板左侧宽度。
    Setting *documentSplitterRightSize; ///< 文档面板右侧宽度。
    Setting *imageThumbnailWidth;       ///< 图片缩略图宽度。
    Setting *videoThumbnailWidth;       ///< 视频缩略图宽度。
    Setting *shapeThumbnailWidth;       ///< 形状缩略图宽度。
    Setting *gipThumbnailWidth;         ///< GIP 缩略图宽度。
    Setting *soundThumbnailWidth;       ///< 音频缩略图宽度。

    Setting *libraryShowDetailsForLocalItems; ///< 本地资源是否显示详情。

    Setting *rightLibPaletteBoardModeWidth;         ///< 白板模式右侧资源面板宽度。
    Setting *rightLibPaletteBoardModeIsCollapsed;   ///< 白板模式右侧资源面板是否收起。
    Setting *rightLibPaletteDesktopModeWidth;       ///< 桌面模式右侧资源面板宽度。
    Setting *rightLibPaletteDesktopModeIsCollapsed; ///< 桌面模式右侧资源面板是否收起。
    Setting *leftLibPaletteBoardModeWidth;          ///< 白板模式左侧资源面板宽度。
    Setting *leftLibPaletteBoardModeIsCollapsed;    ///< 白板模式左侧资源面板是否收起。
    Setting *leftLibPaletteDesktopModeWidth;        ///< 桌面模式左侧资源面板宽度。
    Setting *leftLibPaletteDesktopModeIsCollapsed;  ///< 桌面模式左侧资源面板是否收起。


    Setting *pageSize; ///< 页面尺寸。

    Setting *KeyboardLocale;               ///< 屏幕键盘语言。
    Setting *swapControlAndDisplayScreens; ///< 是否交换控制屏与显示屏。

    Setting *angleTolerance; ///< 角度识别容差。
    Setting *historyLimit;   ///< 撤销历史上限。

    Setting *libIconSize; ///< 资源库图标尺寸。

    Setting *useSystemOnScreenKeyboard; ///< 是否使用系统屏幕键盘。

    Setting *showDateColumnOnAlphabeticalSort; ///< 字母排序时是否显示日期列。

    Setting *emptyTrashForOlderDocuments; ///< 是否自动清理旧文档。
    Setting *emptyTrashDaysValue;         ///< 自动清理的天数阈值。

    Setting *magnifierDrawingMode; ///< 放大镜绘制模式。
    Setting *autoSaveInterval;     ///< 自动保存间隔。

public slots:
    /** @brief 设置钢笔粗细档位索引。 */
    void setPenWidthIndex(int index);
    /** @brief 设置钢笔颜色索引。 */
    void setPenColorIndex(int index);

    /** @brief 设置荧光笔粗细档位索引。 */
    void setMarkerWidthIndex(int index);
    /** @brief 设置荧光笔颜色索引。 */
    void setMarkerColorIndex(int index);

    /** @brief 设置橡皮粗细档位索引。 */
    void setEraserWidthIndex(int index);
    /** @brief 设置细档橡皮宽度。 */
    void setEraserFineWidth(qreal width);
    /** @brief 设置中档橡皮宽度。 */
    void setEraserMediumWidth(qreal width);
    /** @brief 设置粗档橡皮宽度。 */
    void setEraserStrongWidth(qreal width);

    /** @brief 显示或隐藏触控笔调色板。 */
    void setStylusPaletteVisible(bool visible);

    /** @brief 设置钢笔是否响应压感。 */
    void setPenPressureSensitive(bool sensitive);
    /** @brief 开关钢笔预览圆。 */
    void setPenPreviewCircle(bool sensitive);
    /** @brief 设置显示钢笔预览的最小笔宽。 */
    void setPenPreviewFromSize(int size);
    /** @brief 设置荧光笔是否响应压感。 */
    void setMarkerPressureSensitive(bool sensitive);

    /** @brief 按完整键名读取配置值，不存在时返回 defaultValue。 */
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant());
    /** @brief 按完整键名写入配置值（先缓存，随后统一落盘）。 */
    void setValue(const QString &key, const QVariant &value);

    /** @brief 颜色环境变化时转发信号，供调色板刷新。 */
    void colorChanged() { emit colorContextChanged(); }

signals:
    /** @brief 深/浅背景或颜色集合发生变化。 */
    void colorContextChanged();

private:
    QSettings *mAppSettings;  ///< 应用级配置（只读默认值）。
    QSettings *mUserSettings; ///< 用户级配置（可写覆盖）。

    QHash<QString, QVariant> mSettingsQueue; ///< 待写入的用户配置缓存。

    static const int sDefaultFontPixelSize; ///< 默认字体像素大小。
    static const char *sDefaultFontFamily;  ///< 默认字体族。

    /** @brief 打开（必要时创建）应用级配置文件。 */
    static QSettings *getAppSettings();

    static QPointer<QSettings> sAppSettings; ///< 应用级配置的共享指针。
    static QPointer<Settings> sSingleton;    ///< 单例指针。
    static QString sUserLoginName;           ///< 装配层注入的登录名。

    /** @brief 确保目录存在，必要时修正路径；成功返回 true。 */
    static bool checkDirectory(QString &dirPath);
    /** @brief 展开路径中的用户目录通配符。 */
    static QString replaceWildcard(QString &path);

    /** @brief 从用户配置中移除指定键。 */
    void removeSetting(const QString &setting);
    /** @brief 为新版本引入的设置项补齐默认值。 */
    void checkNewSettings();
};
