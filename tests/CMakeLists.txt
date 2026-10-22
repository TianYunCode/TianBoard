# ======================================================================
# tests — Qt Test 用例（ctest --test-dir build -C Debug）
#
# 单元测试只把被测的少量源文件单独编一份进测试可执行，因此可以脱离整个应用
# 栈构建；这也是 src/utils/、src/platform/ 与 src/whiteboard/PDFRenderer 那几个
# 文件必须保持“自包含”的原因（它们不 include 应用装配层）。产物与 TianBoard.exe
# 同目录：build/bin/。
# ======================================================================
find_package(Qt6 6.5 REQUIRED COMPONENTS Test)

function(tianboard_add_test name)
    cmake_parse_arguments(TB "" "" "SOURCES;LIBS" ${ARGN})
    add_executable(${name} ${name}.cpp ${TB_SOURCES})
    # 测试源与被测源现在都按 "模块/文件.h" 写 include，所以给 src/ 这一条。
    target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_compile_options(${name} PRIVATE /MP /utf-8)
    target_link_libraries(${name} PRIVATE Qt6::Test ${TB_LIBS})
    add_test(NAME ${name} COMMAND ${name})
endfunction()

tianboard_add_test(tst_stringutils
    SOURCES ${CMAKE_SOURCE_DIR}/src/utils/StringUtils.cpp
    LIBS    Qt6::Core Qt6::Core5Compat)

# 网格吸附只是纯几何：一个 QPointF 加一个格距算位移，脱离整个应用栈单独构建。
tianboard_add_test(tst_snapping
    SOURCES ${CMAKE_SOURCE_DIR}/src/utils/Snapping.cpp
    LIBS    Qt6::Core)

# Cookie 隐私规则也只是纯字符串运算：策略名映射与保留域匹配，脱离 QtWebEngine 单独构建。
tianboard_add_test(tst_cookierules
    SOURCES ${CMAKE_SOURCE_DIR}/src/utils/CookieRules.cpp
    LIBS    Qt6::Core)

# .tbz 的磁盘布局常量是纯数据（model/PackageLayout.hpp 里没有一行实现），所以这个用例
# 不链任何源文件。它钉的是字面量取值：往返自检存读用的是同一份常量，改了名对它自洽、
# 对用户盘上已有的文档却是灾难，只有钉在这里才会变红。
tianboard_add_test(tst_packagelayout
    LIBS    Qt6::Core)

tianboard_add_test(tst_geometryutils
    SOURCES ${CMAKE_SOURCE_DIR}/src/utils/GeometryUtils.cpp
    LIBS    Qt6::Core Qt6::Gui Qt6::Widgets)

tianboard_add_test(tst_version
    SOURCES ${CMAKE_SOURCE_DIR}/src/utils/Version.cpp
    LIBS    Qt6::Core)

# 凭据封装只依赖 Qt6::Core 与 advapi32（.cpp 里自己 comment 链接），所以同样能脱离
# 应用栈单独构建；它写的是当前用户的凭据管理器，target 用进程 PID 隔开并在收尾删掉。
tianboard_add_test(tst_credentialstore
    SOURCES ${CMAKE_SOURCE_DIR}/src/platform/CredentialStore_win.cpp
    LIBS    Qt6::Core)

tianboard_add_test(tst_pdfrenderer
    SOURCES ${CMAKE_SOURCE_DIR}/src/whiteboard/PDFRenderer.cpp
            ${CMAKE_SOURCE_DIR}/src/whiteboard/QPdfRenderer.cpp
    LIBS    Qt6::Core Qt6::Gui Qt6::Pdf)

# 资源测试直接读源码树里的 res/，不需要把整个 qrc 链进测试。
add_executable(tst_assets tst_assets.cpp)
target_compile_options(tst_assets PRIVATE /MP /utf-8)
target_compile_definitions(tst_assets PRIVATE
    TIANBOARD_RES_DIR="${CMAKE_SOURCE_DIR}/res")
target_link_libraries(tst_assets PRIVATE Qt6::Test Qt6::Gui Qt6::Svg)
add_test(NAME tst_assets COMMAND tst_assets)

# 冒烟测试直接拉起真正的应用程序，必须等主目标构建完才能跑。
add_executable(tst_smoke tst_smoke.cpp)
target_compile_options(tst_smoke PRIVATE /MP /utf-8)
target_link_libraries(tst_smoke PRIVATE Qt6::Test Qt6::Core)
add_dependencies(tst_smoke TianBoard)
add_test(NAME tst_smoke COMMAND tst_smoke)
set_tests_properties(tst_smoke PROPERTIES
    ENVIRONMENT "TIANBOARD_EXE=$<TARGET_FILE:TianBoard>"
    TIMEOUT 300)

# 主模式切换回归：应用带 --ui-smoke 自己依次切 文档 / 网页 / 画板 / 桌面批注 再退出。
# Debug 下 Qt 断言对话框会卡住事件循环，进程就不会按时退出，这条用例据此判红。
add_executable(tst_ui_modes tst_ui_modes.cpp)
target_compile_options(tst_ui_modes PRIVATE /MP /utf-8)
target_link_libraries(tst_ui_modes PRIVATE Qt6::Test Qt6::Core)
add_dependencies(tst_ui_modes TianBoard)
add_test(NAME tst_ui_modes COMMAND tst_ui_modes)
set_tests_properties(tst_ui_modes PROPERTIES
    ENVIRONMENT "TIANBOARD_EXE=$<TARGET_FILE:TianBoard>"
    TIMEOUT 300)

# 文档格式往返自检：应用带 --format-selftest 真存一页再绕开缓存读回来逐项比对。
# 判据是退出码（非 0 = 不一致的条数，75 = 已有实例在跑让路，测试据此跳过）。
add_executable(tst_format tst_format.cpp)
target_compile_options(tst_format PRIVATE /MP /utf-8)
target_link_libraries(tst_format PRIVATE Qt6::Test Qt6::Core)
add_dependencies(tst_format TianBoard)
add_test(NAME tst_format COMMAND tst_format)
set_tests_properties(tst_format PROPERTIES
    ENVIRONMENT "TIANBOARD_EXE=$<TARGET_FILE:TianBoard>"
    TIMEOUT 300)

# 网格吸附接线自检：应用带 --snap-selftest 跑场景侧那一层（纯几何另有 tst_snapping）。
add_executable(tst_snap tst_snap.cpp)
target_compile_options(tst_snap PRIVATE /MP /utf-8)
target_link_libraries(tst_snap PRIVATE Qt6::Test Qt6::Core)
add_dependencies(tst_snap TianBoard)
add_test(NAME tst_snap COMMAND tst_snap)
set_tests_properties(tst_snap PROPERTIES
    ENVIRONMENT "TIANBOARD_EXE=$<TARGET_FILE:TianBoard>"
    TIMEOUT 300)

# 快捷键自定义自检：应用带 --shortcut-selftest 跑注册表登记、出厂默认撞键检查，
# 再用一张自建表跑改绑→落盘→读回→恢复默认的完整回路（含快捷键页的按钮接线）。
add_executable(tst_shortcuts tst_shortcuts.cpp)
target_compile_options(tst_shortcuts PRIVATE /MP /utf-8)
target_link_libraries(tst_shortcuts PRIVATE Qt6::Test Qt6::Core)
add_dependencies(tst_shortcuts TianBoard)
add_test(NAME tst_shortcuts COMMAND tst_shortcuts)
set_tests_properties(tst_shortcuts PROPERTIES
    ENVIRONMENT "TIANBOARD_EXE=$<TARGET_FILE:TianBoard>"
    TIMEOUT 300)

# 导入重名裁决自检：应用带 --import-selftest 用桩裁决器走完取消/改名/替换三条路径，
# 查的是登记结果、磁盘上有没有残留没人认领的文档包，以及顶掉当前文档时界面有没有跟上。
add_executable(tst_import tst_import.cpp)
target_compile_options(tst_import PRIVATE /MP /utf-8)
target_link_libraries(tst_import PRIVATE Qt6::Test Qt6::Core)
add_dependencies(tst_import TianBoard)
add_test(NAME tst_import COMMAND tst_import)
set_tests_properties(tst_import PROPERTIES
    ENVIRONMENT "TIANBOARD_EXE=$<TARGET_FILE:TianBoard>"
    TIMEOUT 300)

# Cookie 隐私自检：应用带 --cookie-selftest 查镜像罐类型、三项设置名与退出清理计数。
add_executable(tst_cookies tst_cookies.cpp)
target_compile_options(tst_cookies PRIVATE /MP /utf-8)
target_link_libraries(tst_cookies PRIVATE Qt6::Test Qt6::Core)
add_dependencies(tst_cookies TianBoard)
add_test(NAME tst_cookies COMMAND tst_cookies)
set_tests_properties(tst_cookies PROPERTIES
    ENVIRONMENT "TIANBOARD_EXE=$<TARGET_FILE:TianBoard>"
    TIMEOUT 300)

# 语言包测试：加载 lrelease 产出的 .qm，验证上下文名与取词结果。
if(TARGET TianBoardTranslations)
    add_executable(tst_language tst_language.cpp)
    target_compile_options(tst_language PRIVATE /MP /utf-8)
    target_link_libraries(tst_language PRIVATE Qt6::Test Qt6::Core)
    add_dependencies(tst_language TianBoardTranslations)
    add_test(NAME tst_language COMMAND tst_language)
    set_tests_properties(tst_language PROPERTIES
        ENVIRONMENT
            "TIANBOARD_ZH_QM=${CMAKE_BINARY_DIR}/Language/TianBoard_zh_CN.qm;TIANBOARD_QT_ZH_QM=${CMAKE_BINARY_DIR}/Language/qtbase_zh_CN.qm")
endif()
