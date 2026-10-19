# ======================================================================
# utils — 纯工具：字符串 / 几何 / 版本 / 文件系统 / 加解密 / 全局类型定义
#
# 内容：StringUtils、GeometryUtils、Version、FileSystemUtils、Globals、
# CoreGraphicsScene、CryptoUtils，外加 Enums.h（全局枚举与 typedef，原先住在
# core/，与同类的 Globals.h 放一起）。平台相关的 PlatformUtils* 在
# src/platform/（对外接口名字没变，仍是 "platform/PlatformUtils.h"）。
#
# 本应只依赖 Qt + OpenSSL 头。尚未成立的边（依赖债，拆环顺序见
# docs/architecture.md）：
#   utils -> core      CryptoUtils.h / FileSystemUtils.cpp include core/Application.h
#   utils -> whiteboard CoreGraphicsScene.cpp include 两个图形项
# ======================================================================

add_library(tb_utils OBJECT
    CoreGraphicsScene.cpp
    CoreGraphicsScene.h
    CryptoUtils.cpp
    CryptoUtils.h
    FileSystemUtils.cpp
    FileSystemUtils.h
    GeometryUtils.cpp
    GeometryUtils.h
    Globals.h
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

# 依赖面：Qt 模块、OpenSSL 头、编译定义都来自 tb_sdk（见根 CMakeLists.txt）
target_link_libraries(tb_utils PRIVATE tb_sdk)
