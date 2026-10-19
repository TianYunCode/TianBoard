# ======================================================================
# web — 内嵌浏览器（QWebEngine）与网页控制器：WebController、
#       browser/ 下的浏览器窗口与标签页控件（原 web/browser/，扁平并入）。
#
# 允许依赖：utils、content（导入导出走同一套文档格式）。尚未成立的边：
# web <-> core / ui / whiteboard / recording。
# ======================================================================

add_library(tb_web OBJECT
    BrowserWindow.cpp
    BrowserWindow.h
    ChaseWidget.cpp
    ChaseWidget.h
    EditTableView.cpp
    EditTableView.h
    ModelMenu.cpp
    ModelMenu.h
    SearchLineEdit.cpp
    SearchLineEdit.h
    SqueezeLabel.cpp
    SqueezeLabel.h
    TabWidget.cpp
    TabWidget.h
    ToolBarSearch.cpp
    ToolBarSearch.h
    UrlLineEdit.cpp
    UrlLineEdit.h
    WebController.cpp
    WebController.h
    WebDownloadManager.cpp
    WebDownloadManager.h
    WebKitUtils.cpp
    WebKitUtils.h
    WebTrapWebView.cpp
    WebTrapWebView.h
    WebView.cpp
    WebView.h
)

# 跨模块 include 一律写成 "模块/文件.h"，由这一条 src/ 路径解析；
# 同目录裸 include 不再允许 —— CMAKE_INCLUDE_CURRENT_DIR 是关的，裸写编译不过；
# 规矩本身写在 docs/coding-style.md。
target_include_directories(tb_web PRIVATE ${CMAKE_SOURCE_DIR}/src)

# 依赖面：Qt 模块、OpenSSL 头、编译定义都来自 tb_sdk（见根 CMakeLists.txt）
target_link_libraries(tb_web PRIVATE tb_sdk)
