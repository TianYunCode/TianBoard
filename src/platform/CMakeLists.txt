# ======================================================================
# platform — 平台相关实现（Win32）：PlatformUtils、窗口捕获（WindowCapture*、
#            CustomCaptureWindow）、屏幕键盘（KeyboardPalette*）、凭据管理器
#            （CredentialStore_win）。
#
# 从 utils/ 与 desktop/、ui/ 里挑出来的、只有这几个文件是真按平台分的；
# KeyboardPalette.h 与 PlatformUtils.h 的对外接口名字保持原样，其余模块
# 不感知底下是 Win32 还是别的。
#
# 允许依赖：utils。尚未成立的边：platform <-> core / ui / desktop / whiteboard
# （CustomCaptureWindow、KeyboardPalette、WindowCapture_win 直接认界面与控制器）。
# ======================================================================

add_library(tb_platform OBJECT
    CredentialStore.h
    CredentialStore_win.cpp
    CustomCaptureWindow.cpp
    CustomCaptureWindow.h
    KeyboardPalette.cpp
    KeyboardPalette.h
    KeyboardPalette_win.cpp
    PlatformUtils.cpp
    PlatformUtils.h
    PlatformUtils_win.cpp
    WindowCapture.h
    WindowCapture_win.cpp
    WindowCaptureDelegate_win.cpp
    WindowCaptureDelegate_win.h
)

# 跨模块 include 一律写成 "模块/文件.h"，由这一条 src/ 路径解析；
# 同目录裸 include 不再允许 —— CMAKE_INCLUDE_CURRENT_DIR 是关的，裸写编译不过；
# 规矩本身写在 docs/coding-style.md。
target_include_directories(tb_platform PRIVATE ${CMAKE_SOURCE_DIR}/src)

# 依赖面：Qt 模块与编译定义都来自 tb_sdk（见根 CMakeLists.txt）
target_link_libraries(tb_platform PRIVATE tb_sdk)
