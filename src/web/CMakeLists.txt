# ======================================================================
# web — 内嵌浏览器（QWebEngine）与网页控制器：WebController、
#       browser/ 下的浏览器窗口与标签页控件（原 web/browser/，扁平并入）。
#
# 允许依赖：utils、settings、platform、core、ui、whiteboard（网页内容要能变成画板图形项）。
# 没有任何模块反向 include web（2026-10-21 断掉最后两条 core → web：Application 建控制器改走
# 装配点装的工厂，模式切换/剪贴板/镜像连线改经 core/IWebController.h 这份契约；recording 与
# desktop 也一并改吃契约），所以本模块不在任何环里，做成真正的 STATIC：越界 include 上层在
# 编译期就失败。（画板反向认 web 的那 7 处更早就已断：4 处死 include，3 处合成
# core/ApplicationController::openInternetUrl。）
# ======================================================================

add_library(tb_web STATIC
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

# 依赖面：Qt 模块与编译定义都来自 tb_sdk（见根 CMakeLists.txt）
target_link_libraries(tb_web PRIVATE tb_sdk)
