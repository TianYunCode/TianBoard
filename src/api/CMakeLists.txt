# ======================================================================
# api — 小部件与文档对外暴露的 API 门面（WebWidgetAPI / WebWidgetBoardAPI /
#        WebWidgetMessageAPI）：网页小部件经 QObject 消息桥接到画板能力。
#
# 目标依赖：utils（WB.h、FileSystemUtils）、model、content、web、core、whiteboard。
# 尚未成立的边：api <-> whiteboard（GraphicsWidgetItem.cpp 反过来 include 本模块）。
# ======================================================================

add_library(tb_api OBJECT
    WebWidgetAPI.cpp
    WebWidgetAPI.h
    WebWidgetBoardAPI.cpp
    WebWidgetBoardAPI.h
    WebWidgetMessageAPI.cpp
    WebWidgetMessageAPI.h
)

# 跨模块 include 一律写成 "模块/文件.h"，由这一条 src/ 路径解析；
# 同目录裸 include 不再允许 —— CMAKE_INCLUDE_CURRENT_DIR 是关的，裸写编译不过；
# 规矩本身写在 docs/coding-style.md。
target_include_directories(tb_api PRIVATE ${CMAKE_SOURCE_DIR}/src)

# 依赖面：Qt 模块、OpenSSL 头、编译定义都来自 tb_sdk（见根 CMakeLists.txt）
target_link_libraries(tb_api PRIVATE tb_sdk)
