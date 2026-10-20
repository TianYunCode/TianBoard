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

# 语言包测试：加载 lrelease 产出的 .qm，验证上下文名与取词结果。
if(TARGET TianBoardTranslations)
    add_executable(tst_language tst_language.cpp)
    target_compile_options(tst_language PRIVATE /MP /utf-8)
    target_link_libraries(tst_language PRIVATE Qt6::Test Qt6::Core)
    add_dependencies(tst_language TianBoardTranslations)
    add_test(NAME tst_language COMMAND tst_language)
    set_tests_properties(tst_language PROPERTIES
        ENVIRONMENT "TIANBOARD_ZH_QM=${CMAKE_BINARY_DIR}/Language/TianBoard_zh_CN.qm")
endif()
