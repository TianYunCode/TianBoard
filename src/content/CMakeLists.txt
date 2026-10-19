# ======================================================================
# content — 文档格式读写与导入导出：.tbz（IWB）子集、SVG/RDF 元数据、
#           Thumbnail 数据块、PDF 渲染与导出、图片导入、Web 导出。
#           原 adaptors/ + pdf/（PDFRenderer/QPdfRenderer/PdfPageItem 都是
#           “文档格式”的读写件，故并入本模块）。
#
# 允许依赖：model、utils、api。尚未成立的边：content <-> core / ui / whiteboard /
# platform（ExportAdaptor 直接拿 MainWindow 弹进度框）。
# ======================================================================

add_library(tb_content OBJECT
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
    PdfPageItem.cpp
    PdfPageItem.h
    PDFRenderer.cpp
    PDFRenderer.h
    QPdfRenderer.cpp
    QPdfRenderer.h
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
