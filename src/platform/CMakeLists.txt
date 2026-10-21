# ======================================================================
# platform — 平台相关实现（Win32）：PlatformUtils（含键盘布局表）、窗口捕获
#            （WindowCapture*）、一次按键如何注入系统（KeyboardSender_win）、
#            凭据管理器（CredentialStore_win）、虚拟桌面枚举与窗口归属（VirtualDesktop，
#            2026-10-21 从 core/ 搬来：它只 include 自己的头加 Win32/Qt，是纯平台代码）。
#
# 从 utils/ 与 desktop/、ui/ 里挑出来的、只有这几个文件是真按平台分的；
# PlatformUtils.h 的对外接口名字保持原样，其余模块不感知底下是 Win32 还是别的。
#
# 屏幕键盘 KeyboardPalette* 和框选取图的 CustomCaptureWindow* 原来也住在这里，
# 现在归 ui/：它们是界面，继承 ActionPalette、要 MainWindow 定位、要 RubberBand
# 画框，把 platform 拽着反向认 core 与 ui。这个模块只留下不带界面的那几件事，
# 出边因此只剩 utils，做成真正的 STATIC：越界 include 上层在编译期就失败。
#
# 允许依赖：utils。尚未成立的边：无。
# ======================================================================

add_library(tb_platform STATIC
    CredentialStore.h
    CredentialStore_win.cpp
    KeyboardSender.h
    KeyboardSender_win.cpp
    PlatformUtils.cpp
    PlatformUtils.h
    PlatformUtils_win.cpp
    VirtualDesktop.cpp
    VirtualDesktop.h
    WindowCapture.h
    WindowCapture_win.cpp
    WindowCaptureDelegate_win.cpp
    WindowCaptureDelegate_win.h
)

# 跨模块 include 一律写成 "模块/文件.h"，由这一条 src/ 路径解析；
# 同目录裸 include 不再允许 —— CMAKE_INCLUDE_CURRENT_DIR 是关的，裸写编译不过；
# 规矩本身写在 docs/coding-style.md。
target_include_directories(tb_platform PRIVATE ${CMAKE_SOURCE_DIR}/src)

# 依赖面：Qt 模块与编译定义都来自 tb_sdk（见根 CMakeLists.txt），下层只有 utils。
target_link_libraries(tb_platform PRIVATE tb_sdk tb_utils)
