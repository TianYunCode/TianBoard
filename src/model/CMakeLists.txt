# ======================================================================
# model — 文档域：DocumentProxy（一份 .tbz 文档）、DocumentContainer（页面容器）、
#         DocumentTreeModel（文档目录树）与 SortFilterProxyModel。
#         文档模式的控制器 DocumentController 在 ui/（它建的是文档库界面）。
#         原 document/；缩略图与元数据的“数据”在本模块，落盘格式在 content/。
#         ForeignObjectsHandler（把文档包内被引用的图片/媒体随页面一起搬运）2026-10-21
#         从 core/ 搬来：它做的是文档包内部的事，调用方是 core 的 PersistenceManager 与
#         ui 的 DocumentController，都是向下依赖。
#         Resources（工具光标 + 自定义字体清单）2026-10-21 从 ui/ 搬来：它自己的注释
#         就写着“不参与界面绘制”，却有 13 处跨模块 include 走它，把 whiteboard/core/
#         recording 各自对 ui 的边白白拖长一截。
#
# 允许依赖：utils、settings、core（DocumentProxy/树模型要用 Application 的静态内存清理器
# 与 PersistenceManager 的仓库常量）。对 content 与 ui 已经零出边。
# ======================================================================

add_library(tb_model OBJECT
    DocumentContainer.cpp
    DocumentContainer.h
    DocumentProxy.cpp
    DocumentProxy.h
    DocumentTreeModel.cpp
    DocumentTreeModel.h
    ForeignObjectsHandler.cpp
    ForeignObjectsHandler.h
    IDocumentFormatServices.cpp
    IDocumentFormatServices.h
    Resources.cpp
    Resources.h
    SortFilterProxyModel.cpp
    SortFilterProxyModel.h
)

# 跨模块 include 一律写成 "模块/文件.h"，由这一条 src/ 路径解析；
# 同目录裸 include 不再允许 —— CMAKE_INCLUDE_CURRENT_DIR 是关的，裸写编译不过；
# 规矩本身写在 docs/coding-style.md。
target_include_directories(tb_model PRIVATE ${CMAKE_SOURCE_DIR}/src)

# 依赖面：Qt 模块与编译定义都来自 tb_sdk（见根 CMakeLists.txt）
target_link_libraries(tb_model PRIVATE tb_sdk)
