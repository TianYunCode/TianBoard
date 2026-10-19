# ==========================================================================
# TianBoard — 交互白板应用（单可执行，Qt 6.9 + MSVC2022 + CMake）
#
# 结构（对齐参考项目 SPM_PTS：显式文件清单 + 每模块独立 CMakeLists +
# 产物钉死到一个 bin 目录）：
#
#   src/app/          main.cpp / TianBoard.rc / 启动装配 —— 唯一可执行目标
#   src/core/         应用、设置、显示器、语言、持久化、各类控制器、网络服务
#   src/model/        文档域：DocumentProxy、容器、模型、缩略图数据
#   src/whiteboard/   画板视图与控制器、图形场景、图形项、绘图工具
#   src/content/      文档格式读写与导入导出（.tbz / CFF / PDF / SVG / RDF）
#   src/api/          小部件与文档对外的 API 门面
#   src/web/          内嵌浏览器与网页控制器
#   src/desktop/      屏幕批注（平台捕获实现不在这里，见 src/platform/）
#   src/recording/    录屏（画面与声音采集，Windows Media 编码器）
#   src/ui/           全部界面控件与调色板
#   src/platform/     平台相关实现：平台工具、窗口捕获、键盘面板
#   src/utils/        纯工具：字符串/几何/版本/文件/WB.h 全局定义
#   third/            内联第三方（quazip+zlib、openssl 静态库、qtsingleapplication）
#   res/ tests/ docs/ packaging/ scripts/
#
# 模块一律做成 OBJECT 库（add_library(tb_<module> OBJECT ...)），全部并进
# TianBoard.exe。为什么不是一开始就 STATIC：现在的代码里 core↔ui、core↔whiteboard
# 等存在真实的双向 include（清单见 docs/architecture.md 的“依赖债”一节），
# 静态库需要无环的链接顺序，OBJECT 库不需要 —— 本阶段先拿到“每模块显式文件
# 清单 + 每模块 include 白名单”，把环拆掉之后再逐个收紧成 STATIC。
#
# 加新文件必须改对应模块的 CMakeLists.txt —— 这是刻意的（资源确定性）：
# 不再用 file(GLOB_RECURSE) 收源，避免出现“文件在树上、没进构建、也没人知道”。
#
# 构建：
#   cmake --preset vs2022-release            # 或 scripts/build.ps1（Ninja）
#   cmake --build --preset vs2022-release
#   ctest --test-dir build -C Release
# 产物：build/bin/TianBoard.exe（etc/ library/ customizations/ Language/ 与之同级）
# ==========================================================================
cmake_minimum_required(VERSION 3.22)

project(TianBoard
    VERSION 1.1.0
    DESCRIPTION "TianBoard - interactive whiteboard application"
    LANGUAGES C CXX
)

if(NOT MSVC)
    message(FATAL_ERROR "TianBoard 目前只支持 MSVC (Visual Studio 2022) 构建。")
endif()

# ==========================================================================
# 全局约定
# ==========================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 不关 CMAKE_INCLUDE_CURRENT_DIR：显式关掉它，跨目录 include 就必须写成
# "模块/文件.h" 并经过各模块 target_include_directories 的白名单才能解析；
# 开着它则同目录裸 include 能蒙混过关。规矩见 docs/coding-style.md。
set(CMAKE_INCLUDE_CURRENT_DIR OFF)

# 单配置生成器（Ninja）下不指定类型就是 RelWithDebInfo：带 pdb，现场崩溃能看栈。
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

# ---- 产物目录：钉死到 build/bin ----
# 与 SPM_PTS 同法。多配置生成器（VS）默认会再分 Debug/Release 子目录，这里逐个
# 配置覆盖掉，保证 Debug 与 Release 都落在同一个 bin/ —— 应用按“可执行文件所在
# 目录”解析 etc/ library/ customizations/ Language/，测试（tst_smoke）按
# TIANBOARD_EXE 定位，路径只有一处，部署与打包脚本也只需要认这一处。
# 产物按"每个 binary 目录 + 每个配置"分开放：${CMAKE_BINARY_DIR}/bin/<CONFIG>/。
# 之前把三种输出统一钉死到源目录下的单个 build/bin 是有 bug 的做法：静态库属于
# ARCHIVE 输出，多配置生成器（VS）下 Debug 与 Release 的 tb_quazip.lib 同名互相
# 覆盖，实测踩过一次 —— Release 构建之后紧接着的 Debug 链接拿到 Release 的 .obj，
# 报 LNK1319 / LNK2038 RuntimeLibrary 不匹配。而且两个不同的 binary 目录
# （build-debug/ 与 build-release/，或隔离构建目录）也会共用同一处互相污染。
# 现在按配置分目录、且相对 CMAKE_BINARY_DIR 而非 CMAKE_SOURCE_DIR，隔离构建目录
# 天然互不影响，不再需要 -DTIANBOARD_BIN_DIR 手工改道。
# 其余一切（res 拷贝、windeployqt、tst_smoke 的 TIANBOARD_EXE）都跟着
# $<TARGET_FILE_DIR:TianBoard> 走，只有这一处定义产物目录。
set(TIANBOARD_BIN_DIR "${CMAKE_BINARY_DIR}/bin" CACHE PATH
    "产物根目录：每个配置再分子目录，可执行、测试可执行、运行期 res 拷贝与 Qt 运行时部署都落这里")

if(CMAKE_CONFIGURATION_TYPES)
    foreach(_cfg ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER "${_cfg}" _CFG)
        foreach(_kind RUNTIME LIBRARY ARCHIVE)
            set(CMAKE_${_kind}_OUTPUT_DIRECTORY_${_CFG} "${TIANBOARD_BIN_DIR}/${_cfg}")
        endforeach()
    endforeach()
else()
    foreach(_kind RUNTIME LIBRARY ARCHIVE)
        set(CMAKE_${_kind}_OUTPUT_DIRECTORY "${TIANBOARD_BIN_DIR}/${CMAKE_BUILD_TYPE}")
    endforeach()
endif()

# ---- MSVC：/utf-8（源码里有中文注释）+ /MP（并行编译）----
# Release 也要 pdb：/Zi 产调试信息，/DEBUG 才把 .pdb 链接出来。RelWithDebInfo
# 本身已经带这两样，这里兜住纯 Release 配置。
add_compile_options(/utf-8 /MP)
add_compile_options($<$<CONFIG:Release>:/Zi>)
add_link_options($<$<CONFIG:Release>:/DEBUG>)

# CMAKE_DEBUG_POSTFIX 不再设置：本工程没有任何库目标（模块是 OBJECT 库，
# 产物并进可执行），而 build/bin 里各配置同名，加后缀只会留下并排的旧副本。

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core Gui Widgets Xml Network
    Svg SvgWidgets PrintSupport Pdf
    Multimedia MultimediaWidgets
    WebChannel WebEngineCore WebEngineWidgets Qml
    Core5Compat
)
find_package(Qt6 6.5 OPTIONAL_COMPONENTS LinguistTools)

add_subdirectory(third/quazip)

# --------------------------------------------------------------------------
# qtsingleapplication（third/ 里的内联第三方，厂商包没有 CMake 文件，
# 清单因此写在这里而不是 third/singleapp/CMakeLists.txt —— third/ 保持原样）。
#
# qtlockedfile.cpp / qtlockedfile_win.cpp 不能单独列：qtlocalpeer.cpp 把它们
# 原文 #include 进自己的命名空间里编译，再列一遍就是重复定义。
# --------------------------------------------------------------------------
add_library(tb_singleapp OBJECT
    third/singleapp/qtlocalpeer.cpp
    third/singleapp/qtsingleapplication.cpp
    third/singleapp/qtlocalpeer.h
    third/singleapp/qtlockedfile.h
    third/singleapp/qtsingleapplication.h
)
# 这两条是第三方内部引用方式决定的：裸同名 include 认自己所在目录，
# "third/singleapp/qtlockedfile.h" 那种长写法认仓库根。
target_include_directories(tb_singleapp PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third/singleapp
)
target_link_libraries(tb_singleapp PRIVATE tb_sdk)

# ==========================================================================
# tb_sdk — 全模块共用的外部依赖面（Qt 模块、OpenSSL 头、编译定义）
#
# 各模块 target_link_libraries(tb_<module> PRIVATE tb_sdk) 即得；它不带任何
# TianBoard 自己的头文件路径 —— 那部分留给各模块的 target_include_directories，
# 这样“谁能 include 谁”仍然逐模块可见。
# ==========================================================================
add_library(tb_sdk INTERFACE)
target_link_libraries(tb_sdk INTERFACE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Xml Qt6::Network
    Qt6::Svg Qt6::SvgWidgets Qt6::PrintSupport Qt6::Pdf
    Qt6::Multimedia Qt6::MultimediaWidgets
    Qt6::WebChannel Qt6::WebEngineCore Qt6::WebEngineWidgets Qt6::Qml
    Qt6::Core5Compat
    QuaZip::QuaZip
)
# OpenSSL 1.1.1 的头是 CryptoUtils/FileSystemUtils 的公开依赖，故进接口；
# 静态库只在最终可执行上链接（见 src/app/CMakeLists.txt）。
target_include_directories(tb_sdk INTERFACE ${CMAKE_SOURCE_DIR}/third/openssl/include)
# 这一条只为一个 include 服务：core/Application.h 里
#   #include "third/singleapp/qtsingleapplication.h"
# 内联第三方保持仓库根相对引用（third/ 不动，它的引用方式也不动）。
# 该头被 10 个模块间接 include，所以路径给在整个 SDK 面上；src/ 的模块前缀规矩
# 不受影响 —— 仓库根下已经没有模块目录了。
target_include_directories(tb_sdk INTERFACE ${CMAKE_SOURCE_DIR})
target_compile_definitions(tb_sdk INTERFACE
    UNICODE _UNICODE WIN32
    _CRT_SECURE_NO_WARNINGS _SCL_SECURE_NO_WARNINGS
)

# ==========================================================================
# 模块（OBJECT 库）。被依赖的先加，纯为可读性，OBJECT 库之间没有链接顺序要求。
# ==========================================================================
add_subdirectory(src/utils)
add_subdirectory(src/api)
add_subdirectory(src/platform)
add_subdirectory(src/model)
add_subdirectory(src/content)
add_subdirectory(src/whiteboard)
add_subdirectory(src/web)
add_subdirectory(src/desktop)
add_subdirectory(src/recording)
add_subdirectory(src/ui)
add_subdirectory(src/core)

# ==========================================================================
# 翻译：lrelease res/i18n/*.ts -> <builddir>/Language/TianBoard_<locale>.qm
# 运行时按 <appdir>/Language/TianBoard_<locale>.qm 取词，所以由 src/app 的
# POST_BUILD 把它拷到可执行文件旁边。
# ==========================================================================
find_program(TIANBOARD_LRELEASE_EXECUTABLE lrelease HINTS ${Qt6_DIR}/../../../bin)

set(TIANBOARD_QM_FILES)
if(Qt6LinguistTools_FOUND AND TIANBOARD_LRELEASE_EXECUTABLE)
    file(GLOB TIANBOARD_TS_FILES ${CMAKE_SOURCE_DIR}/res/i18n/*.ts)
    foreach(_ts ${TIANBOARD_TS_FILES})
        get_filename_component(_lang ${_ts} NAME_WE)
        string(REPLACE "lang_" "" _lang ${_lang})
        set(_qm ${CMAKE_CURRENT_BINARY_DIR}/Language/TianBoard_${_lang}.qm)
        add_custom_command(OUTPUT ${_qm}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/Language
            COMMAND ${TIANBOARD_LRELEASE_EXECUTABLE} ${_ts} -qm ${_qm}
            DEPENDS ${_ts}
            COMMENT "lrelease: ${_lang} -> TianBoard_${_lang}.qm"
            VERBATIM)
        list(APPEND TIANBOARD_QM_FILES ${_qm})
    endforeach()
    add_custom_target(TianBoardTranslations DEPENDS ${TIANBOARD_QM_FILES})
endif()

# 可执行目标（依赖上面所有模块对象库 + TianBoardTranslations，见 src/app）
add_subdirectory(src/app)

# ==========================================================================
# 自测：ctest --test-dir build -C Debug
# ==========================================================================
option(TIANBOARD_BUILD_TESTS "构建 tests/ 下的 Qt Test 用例" ON)
enable_testing()
if(TIANBOARD_BUILD_TESTS)
    add_subdirectory(tests)
endif()
