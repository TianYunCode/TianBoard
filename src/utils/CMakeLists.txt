# ======================================================================
# utils — 纯工具：字符串 / 几何 / 版本 / 文件系统 / 全局类型定义
#
# 内容：StringUtils、GeometryUtils、Version、FileSystemUtils、Snapping、CookieRules、
# Globals，外加 Enums.h（全局枚举与 typedef，原先住在 core/，与同类的 Globals.h 放一起）。
# 平台相关的 PlatformUtils* 在 src/platform/（对外接口名字没变，仍是
# "platform/PlatformUtils.h"）。
#
# 只依赖 Qt：本模块对 core/whiteboard 的反向 include 已经清空
# （CoreGraphicsScene 归 whiteboard/，CryptoUtils 连同它的 OpenSSL 依赖一起删了——
# 那套"AES + 源码里写死的 key"只是混淆，代理凭据现在交给 Windows 凭据管理器，
# 见 src/platform/CredentialStore_win.cpp；两个 md5 助手没有任何调用点）。
# 所以它是第一个从 OBJECT 收紧成真静态库的模块：STATIC 意味着
# utils 里放不进任何指向别的模块的 include，越界会在编译期直接失败而不是等到链接期。
# ======================================================================

add_library(tb_utils STATIC
    CookieRules.cpp
    CookieRules.h
    FileSystemUtils.cpp
    FileSystemUtils.h
    GeometryUtils.cpp
    GeometryUtils.h
    Globals.h
    Snapping.cpp
    Snapping.h
    StringUtils.cpp
    StringUtils.h
    Version.cpp
    Version.h
    Enums.h
)

# 跨模块 include 一律写成 "模块/文件.h"，由这一条 src/ 路径解析；
# 同目录裸 include 不再允许 —— CMAKE_INCLUDE_CURRENT_DIR 是关的，裸写编译不过；
# 规矩本身写在 docs/coding-style.md。
target_include_directories(tb_utils PRIVATE ${CMAKE_SOURCE_DIR}/src)

# 依赖面：Qt 模块与编译定义都来自 tb_sdk（见根 CMakeLists.txt）
target_link_libraries(tb_utils PRIVATE tb_sdk)
