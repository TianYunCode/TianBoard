# ======================================================================
# whiteboard — 画板：BoardView/BoardController/DrawingController/
#              BoardPaletteManager/FeaturesController + 图形场景与全部图形项
#              （Graphics*Item、Undo 命令）+ 绘图工具（尺规、量角器、窗帘、缓存）。
#              ToolWidget（浮动网页工具窗）与 Magnifier（放大镜）2026-10-21 从 ui/ 搬来：
#              它们只认 BoardView/GraphicsScene/GraphicsWidgetItem，而画板本身又要造它们。
#              原 board/ + domain/ + tools/ 三块合并（它们本就同属画板域，
#              分开只让 include 前缀显得比实际耦合更干净）；pdf/ 那三个类
#              （PDFRenderer/QPdfRenderer/PdfPageItem）也从 content/ 搬了进来——
#              PdfPageItem 是 GraphicsPDFItem 的基类，合起来是"一页 PDF 怎么变成
#              画布上的图形"，不是文档格式读写。
#
# 允许依赖：model、utils。尚未成立的边：whiteboard <-> core / ui / platform。
# 对 api、desktop、recording、content 与 web 已经是零出边：录制按钮的转发、桌面批注
# 宿主与浏览器跳转都挪到了 core 装配层，拖放门面改经本模块的 IBoardDropHandler 接口回调。
# 绘制热路径在本模块，改动要盯性能。
# ======================================================================

add_library(tb_whiteboard OBJECT
    AbstractDrawRuler.cpp
    AbstractDrawRuler.h
    BoardController.cpp
    BoardController.h
    IBoardDropHandler.cpp
    IBoardDropHandler.h
    BoardPaletteManager.cpp
    BoardPaletteManager.h
    BoardView.cpp
    BoardView.h
    CoreGraphicsScene.cpp
    CoreGraphicsScene.h
    DrawingController.cpp
    DrawingController.h
    FeaturesController.cpp
    FeaturesController.h
    GraphicsAxes.cpp
    GraphicsAxes.h
    GraphicsCache.cpp
    GraphicsCache.h
    GraphicsCompass.cpp
    GraphicsCompass.h
    GraphicsCurtainItem.cpp
    GraphicsCurtainItem.h
    GraphicsCurtainItemDelegate.cpp
    GraphicsCurtainItemDelegate.h
    GraphicsDelegateFrame.cpp
    GraphicsDelegateFrame.h
    GraphicsGroupContainerItem.cpp
    GraphicsGroupContainerItem.h
    GraphicsGroupContainerItemDelegate.cpp
    GraphicsGroupContainerItemDelegate.h
    GraphicsItemDelegate.cpp
    GraphicsItemDelegate.h
    GraphicsItemGroupUndoCommand.cpp
    GraphicsItemGroupUndoCommand.h
    GraphicsItemTransformUndoCommand.cpp
    GraphicsItemTransformUndoCommand.h
    GraphicsItemUndoCommand.cpp
    GraphicsItemUndoCommand.h
    GraphicsItemZLevelUndoCommand.cpp
    GraphicsItemZLevelUndoCommand.h
    GraphicsMediaItem.cpp
    GraphicsMediaItem.h
    GraphicsMediaItemDelegate.cpp
    GraphicsMediaItemDelegate.h
    GraphicsPDFItem.cpp
    GraphicsPDFItem.h
    GraphicsPixmapItem.cpp
    GraphicsPixmapItem.h
    GraphicsPolygonItem.cpp
    GraphicsPolygonItem.h
    GraphicsProtractor.cpp
    GraphicsProtractor.h
    GraphicsProxyWidget.cpp
    GraphicsProxyWidget.h
    GraphicsRuler.cpp
    GraphicsRuler.h
    GraphicsScene.cpp
    GraphicsScene.h
    GraphicsStroke.cpp
    GraphicsStroke.h
    GraphicsStrokesGroup.cpp
    GraphicsStrokesGroup.h
    GraphicsSvgItem.cpp
    GraphicsSvgItem.h
    GraphicsTextItem.cpp
    GraphicsTextItem.h
    GraphicsTextItemDelegate.cpp
    GraphicsTextItemDelegate.h
    GraphicsTextItemUndoCommand.cpp
    GraphicsTextItemUndoCommand.h
    GraphicsTriangle.cpp
    GraphicsTriangle.h
    GraphicsWidgetItem.cpp
    GraphicsWidgetItem.h
    GraphicsWidgetItemDelegate.cpp
    GraphicsWidgetItemDelegate.h
    Magnifier.cpp
    Magnifier.h
    MimeDataGraphicsItem.cpp
    MimeDataGraphicsItem.h
    Item.cpp
    Item.h
    PDFRenderer.cpp
    PDFRenderer.h
    PdfPageItem.cpp
    PdfPageItem.h
    PageSizeUndoCommand.cpp
    PageSizeUndoCommand.h
    QPdfRenderer.cpp
    QPdfRenderer.h
    ResizableGraphicsItem.cpp
    ResizableGraphicsItem.h
    SnapIndicator.cpp
    SnapIndicator.h
    SelectionFrame.cpp
    SelectionFrame.h
    ToolWidget.cpp
    ToolWidget.h
    ToolsManager.cpp
    ToolsManager.h
    UndoCommand.cpp
    UndoCommand.h
)

# 跨模块 include 一律写成 "模块/文件.h"，由这一条 src/ 路径解析；
# 同目录裸 include 不再允许 —— CMAKE_INCLUDE_CURRENT_DIR 是关的，裸写编译不过；
# 规矩本身写在 docs/coding-style.md。
target_include_directories(tb_whiteboard PRIVATE ${CMAKE_SOURCE_DIR}/src)

# 依赖面：Qt 模块与编译定义都来自 tb_sdk（见根 CMakeLists.txt）
target_link_libraries(tb_whiteboard PRIVATE tb_sdk)
