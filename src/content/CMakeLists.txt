# ======================================================================
# content — 文档格式读写与导入导出：.tbz（IWB）子集、SVG/RDF 元数据、
#           Thumbnail 数据块、PDF 导入与导出、图片导入、Web 导出。
#           原 adaptors/ + pdf/（都是“文档格式”的读写件，故并入本模块）。
#           pdf/ 里的 PDFRenderer/QPdfRenderer/PdfPageItem 例外，已搬到
#           src/whiteboard/：PdfPageItem 是 GraphicsPDFItem 的基类，三个类合起来
#           是“一页 PDF 怎么变成画布上的图形”，没有任何格式读写以外的依赖。
#
# 允许依赖：utils、settings、platform、model、whiteboard（都是单向向下）。对 core 与 ui
# 已经是零出边：对 ui 靠"导出器只 emit exportWarnings，对话框父窗口问装配层装的 provider
# 要"；对 core 靠 model/IDocumentStore 这份契约（2026-10-22），仓库常量在 model/PackageLayout.hpp、
# 状态消息在 utils/UserNotices，画板自己的通知走 GraphicsScene/GraphicsMediaItem 的入口。
# 因此本模块不在任何环里，做成真正的 STATIC：越界 include 上层在编译期就失败。
# ======================================================================

add_library(tb_content STATIC
    DocumentFormatServices.cpp
    DocumentFormatServices.h
    ExportAdaptor.cpp
    ExportAdaptor.h
    ExportDocument.cpp
    ExportDocument.h
    ExportDocumentSetAdaptor.cpp
    ExportDocumentSetAdaptor.h
    ExportFullPDF.cpp
    ExportFullPDF.h
    ExportIwb.cpp
    ExportIwb.h
    ExportPDF.cpp
    ExportPDF.h
    ExportWeb.cpp
    ExportWeb.h
    ImportAdaptor.cpp
    ImportAdaptor.h
    ImportDocument.cpp
    ImportDocument.h
    ImportDocumentSetAdaptor.cpp
    ImportDocumentSetAdaptor.h
    ImportImage.cpp
    ImportImage.h
    ImportIwb.cpp
    ImportIwb.h
    ImportPDF.cpp
    ImportPDF.h
    IwbConstants.h
    IwbDocumentAdaptor.cpp
    IwbDocumentAdaptor.h
    IwbExport.h
    IwbSubsetAdaptor.cpp
    IwbSubsetAdaptor.h
    MetadataDcSubsetAdaptor.cpp
    MetadataDcSubsetAdaptor.h
    SvgSubsetAdaptor.cpp
    SvgSubsetAdaptor.h
    ThumbnailAdaptor.cpp
    ThumbnailAdaptor.h
)

# 跨模块 include 一律写成 "模块/文件.h"，由这一条 src/ 路径解析；
# 同目录裸 include 不再允许 —— CMAKE_INCLUDE_CURRENT_DIR 是关的，裸写编译不过；
# 规矩本身写在 docs/coding-style.md。
target_include_directories(tb_content PRIVATE ${CMAKE_SOURCE_DIR}/src)

# 依赖面：Qt 模块与编译定义都来自 tb_sdk（见根 CMakeLists.txt）
target_link_libraries(tb_content PRIVATE tb_sdk)
