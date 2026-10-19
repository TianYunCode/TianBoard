# ======================================================================
# model — 文档域：DocumentProxy（一份 .tbz 文档）、DocumentContainer（页面容器）、
#         DocumentController（文档库的增删改查）、SortFilterProxyModel。
#         原 document/；缩略图与元数据的“数据”在本模块，落盘格式在 content/。
#
# 允许依赖：utils。尚未成立的边：model <-> core / content / ui / whiteboard。
# ======================================================================

add_library(tb_model OBJECT
    DocumentContainer.cpp
    DocumentContainer.h
    DocumentController.cpp
    DocumentController.h
    DocumentProxy.cpp
    DocumentProxy.h
    SortFilterProxyModel.cpp
    SortFilterProxyModel.h
)

# 跨模块 include 一律写成 "模块/文件.h"，由这一条 src/ 路径解析；
# 同目录裸 include 不再允许 —— CMAKE_INCLUDE_CURRENT_DIR 是关的，裸写编译不过；
# 规矩本身写在 docs/coding-style.md。
target_include_directories(tb_model PRIVATE ${CMAKE_SOURCE_DIR}/src)

# 依赖面：Qt 模块、OpenSSL 头、编译定义都来自 tb_sdk（见根 CMakeLists.txt）
target_link_libraries(tb_model PRIVATE tb_sdk)
