# ======================================================================
# ui — 界面控件与调色板：MainWindow（+ 手写表单 MainWindowUi.h）、各类
#      FloatingPalette / DockPalette / ToolWidget / 缩略图视图 / 消息框，
#      以及原先根下 widgets/ 的内容（已并进来）与从 network/ 移来的
#      ProxyDialog（它是对话框，不是服务）。屏幕键盘 KeyboardPalette* 与框选取图的
#      CustomCaptureWindow* 也从 src/platform/ 移到了这里——它们是界面，平台层只留
#      下"怎么把一次按键交给系统"这一件事（platform/KeyboardSender）。文档模式的控制器
#      DocumentController（+ 它的树视图/委托/重名对话框）也是 2026-10 从 model/ 搬进来的：
#      它建的是界面，model/ 只留纯文档域。
#      偏好设置那一套（PreferencesDialog + PreferencesController + BrushPropertiesFrame）
#      同一轮从 core/ 搬进来——一个 QDialog 与它的控制器不该住在服务层。
#
# 允许依赖：model、utils、platform。尚未成立的边：ui <-> core / whiteboard。
# （对 model 已是单向：model 不再 include 本模块；对 content 与 api 零出边——content
# 那两个是从没被使用的死 include，api 那个是 FeaturesWidget 里从不使用的 boardApi。）
# ======================================================================

add_library(tb_ui OBJECT
    ActionPalette.cpp
    ActionPalette.h
    BackgroundPalette.cpp
    BackgroundPalette.h
    BlackoutWidget.cpp
    BlackoutWidget.h
    CachePropertiesWidget.cpp
    CachePropertiesWidget.h
    CircleFrame.cpp
    CircleFrame.h
    ColorPicker.cpp
    ColorPicker.h
    CustomCaptureWindow.cpp
    CustomCaptureWindow.h
    DockDownloadWidget.cpp
    DockDownloadWidget.h
    DockPalette.cpp
    DockPalette.h
    DockPaletteWidget.cpp
    DockPaletteWidget.h
    DocumentController.cpp
    DocumentController.h
    DocumentReplaceDialog.cpp
    DocumentReplaceDialog.h
    DocumentNavigator.cpp
    DocumentNavigator.h
    DocumentsWidget.cpp
    DocumentsWidget.h
    DocumentThumbnailWidget.cpp
    DocumentThumbnailWidget.h
    DocumentToolsPalette.cpp
    DocumentToolsPalette.h
    DownloadWidget.cpp
    DownloadWidget.h
    FavoriteToolPalette.cpp
    FavoriteToolPalette.h
    FeaturesActionBar.cpp
    FeaturesActionBar.h
    FeaturesWidget.cpp
    FeaturesWidget.h
    FloatingPalette.cpp
    FloatingPalette.h
    IconButton.cpp
    IconButton.h
    KeyboardPalette.cpp
    KeyboardPalette.h
    LeftPalette.cpp
    LeftPalette.h
    Magnifer.cpp
    Magnifer.h
    MainWindow.cpp
    MainWindow.h
    MainWindowUi.h
    MessagesDialog.cpp
    MessagesDialog.h
    MessageWindow.cpp
    MessageWindow.h
    MousePressFilter.cpp
    MousePressFilter.h
    PageNavigationWidget.cpp
    PageNavigationWidget.h
    PreferencesController.cpp
    PreferencesController.h
    PropertyPalette.cpp
    PropertyPalette.h
    ProxyDialog.cpp
    ProxyDialog.h
    Resources.cpp
    Resources.h
    RightPalette.cpp
    RightPalette.h
    RubberBand.cpp
    RubberBand.h
    ScreenMirror.cpp
    ScreenMirror.h
    ShortcutSettingsWidget.cpp
    ShortcutSettingsWidget.h
    SpinningWheel.cpp
    SpinningWheel.h
    StylusPalette.cpp
    StylusPalette.h
    ThumbnailWidget.cpp
    ThumbnailWidget.h
    ToolbarButtonGroup.cpp
    ToolbarButtonGroup.h
    ToolWidget.cpp
    ToolWidget.h
    WebToolsPalette.cpp
    WebToolsPalette.h
    WidgetMirror.cpp
    WidgetMirror.h
    ZoomPalette.cpp
    ZoomPalette.h
)

# 跨模块 include 一律写成 "模块/文件.h"，由这一条 src/ 路径解析；
# 同目录裸 include 不再允许 —— CMAKE_INCLUDE_CURRENT_DIR 是关的，裸写编译不过；
# 规矩本身写在 docs/coding-style.md。
target_include_directories(tb_ui PRIVATE ${CMAKE_SOURCE_DIR}/src)

# 依赖面：Qt 模块与编译定义都来自 tb_sdk（见根 CMakeLists.txt）
target_link_libraries(tb_ui PRIVATE tb_sdk)
