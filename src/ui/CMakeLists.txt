# ======================================================================
# ui — 界面控件与调色板：MainWindow（+ 手写表单 MainWindowUi.h）、各类
#      FloatingPalette / DockPalette / ToolWidget / 缩略图视图 / 消息框，
#      以及原先根下 widgets/ 的内容（已并进来）与从 network/ 移来的
#      ProxyDialog（它是对话框，不是服务）。
#      屏幕键盘 KeyboardPalette* 在 src/platform/。
#
# 允许依赖：model、utils、api。尚未成立的边：ui <-> core / whiteboard / content /
# model / platform。
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
    DockDownloadWidget.cpp
    DockDownloadWidget.h
    DockPalette.cpp
    DockPalette.h
    DockPaletteWidget.cpp
    DockPaletteWidget.h
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
    SpinningWheel.cpp
    SpinningWheel.h
    StylusPalette.cpp
    StylusPalette.h
    ThumbnailView.cpp
    ThumbnailView.h
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
