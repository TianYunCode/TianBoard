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
#         PackageLayout.hpp（.tbz 包内的目录名与 folders.xml 的词表）2026-10-22 从
#         core/PersistenceManager 的静态成员里搬出来：这些是文档域的磁盘词汇，挂在服务类上
#         时画板为了知道图片放哪个子目录就得 include 持久化服务（GraphicsScene、
#         GraphicsPixmapItem、BoardPaletteManager 各背一条反向边）。
#
# 允许依赖：utils、settings —— 对 core 的出边已于 2026-10-22 归零，所以本模块是真的叶子：
# 页面操作与元数据通知改走 model/IDocumentStore（实现是 core 的 PersistenceManager，构造时
# 把自己装上），静态对象宿主改住 utils/StaticMemory，MIME 类型名与拖放页面载荷留在本模块
# （MimeTypes.hpp / MimeData，携带图形项副本的那个在 whiteboard，它得能释放副本）。
# 对 content 与 ui 也已经是零出边。没有任何模块反向 include model 里的低层，做成 STATIC
# 之后越界 include 上层在编译期就失败。
# ======================================================================

add_library(tb_model STATIC
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
    IDocumentStore.cpp
    IDocumentStore.h
    MimeData.cpp
    MimeData.h
    MimeTypes.hpp
    PackageLayout.hpp
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
