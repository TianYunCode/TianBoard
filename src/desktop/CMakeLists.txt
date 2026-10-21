# ======================================================================
# desktop — 屏幕批注：DesktopAnnotationController（把整个桌面当一页板）、
#           DesktopPalette / DesktopPropertyPalette。
#           Win32 的窗口捕获实现（WindowCapture*、CustomCaptureWindow*）在
#           src/platform/，本模块只留平台无关的批注逻辑与调色板。
#
# 允许依赖：utils、model、settings、platform、core、ui、web、whiteboard。没有任何模块
# 反向 include desktop（只有装配点 app/main.cpp 认识实现类，那是可执行目标自己），
# 所以本模块不在任何环里，做成真正的 STATIC：越界 include 上层在编译期就失败。
# 2026-10-21 断掉最后两条 core → desktop：core 的总控器改经 core/IDesktopAnnotation.h
# 这份契约用批注层，实现类由装配点装的工厂创建。
# ======================================================================

add_library(tb_desktop STATIC
    DesktopAnnotationController.cpp
    DesktopAnnotationController.h
    DesktopPalette.cpp
    DesktopPalette.h
    DesktopPropertyPalette.cpp
    DesktopPropertyPalette.h
)

# 跨模块 include 一律写成 "模块/文件.h"，由这一条 src/ 路径解析；
# 同目录裸 include 不再允许 —— CMAKE_INCLUDE_CURRENT_DIR 是关的，裸写编译不过；
# 规矩本身写在 docs/coding-style.md。
target_include_directories(tb_desktop PRIVATE ${CMAKE_SOURCE_DIR}/src)

# 依赖面：Qt 模块与编译定义都来自 tb_sdk（见根 CMakeLists.txt）
target_link_libraries(tb_desktop PRIVATE tb_sdk)
