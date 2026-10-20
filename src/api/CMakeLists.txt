# ======================================================================
# api — 小部件与文档对外暴露的 API 门面（WebWidgetBoardAPI / WebWidgetMessageAPI）：
#        网页小部件经 QObject 消息桥接到画板能力。
#
# 目标依赖：utils、model、content、core、whiteboard。本模块只朝下认画板；
# 画板需要门面的那一件事（拖到小部件上的东西怎么进画板）经 whiteboard 自己的
# BoardDropHandler 接口回调过来，实现由装配点在启动时装上（见 app/main.cpp）。
#
# 已经删掉的 widget 门面 WebWidgetAPI/WebWidgetPreferenceAPI：Qt6 没有
# addToJavaScriptWindowObject，注册它的那条 connect 在移植时就断了线，
# 于是这两个类只剩"被一个从没接上的槽 new 出来"这一处引用。
# ======================================================================

add_library(tb_api OBJECT
    WebWidgetBoardAPI.cpp
    WebWidgetBoardAPI.h
    WebWidgetMessageAPI.cpp
    WebWidgetMessageAPI.h
)

# 跨模块 include 一律写成 "模块/文件.h"，由这一条 src/ 路径解析；
# 同目录裸 include 不再允许 —— CMAKE_INCLUDE_CURRENT_DIR 是关的，裸写编译不过；
# 规矩本身写在 docs/coding-style.md。
target_include_directories(tb_api PRIVATE ${CMAKE_SOURCE_DIR}/src)

# 依赖面：Qt 模块与编译定义都来自 tb_sdk（见根 CMakeLists.txt）
target_link_libraries(tb_api PRIVATE tb_sdk)
