# ======================================================================
# app — 可执行目标本体：main.cpp（消息处理器 + 单实例转发 + 启动）、
#       TianBoard.rc / resource.h（加速器与图标资源）、res/TianBoard.qrc。
#
# 这里只做装配：把各模块的 OBJECT 库并成一个 TianBoard.exe，再把运行期
# 目录铺到可执行文件旁边。不放业务代码——但有两样"只有装配层能干"的东西，
# 2026-10 起也住在这里：
#   * 下层模块所需实现的注入（drop 处理器工厂、文档格式服务），见 main.cpp；
#   * 四个 *SelfTest 与按命令行分发的 app/SelfTests.cpp——自检要同时伸手进
#     model/whiteboard/content/web/ui，换任何别的模块都会新造一组环，
#     core 那边只留一个通用钩子 Application::setSelfTestRun()。
#
# 运行期布局（FileSystemUtils::applicationResourcesDirectory() ==
# applicationDirPath()）：etc/ library/ customizations/ Language/ 必须与
# TianBoard.exe 同级，POST_BUILD 负责铺；tst_smoke 也按这个前提校验。
# ======================================================================

add_executable(TianBoard WIN32
    CookieSelfTest.cpp
    CookieSelfTest.h
    FormatSelfTest.cpp
    FormatSelfTest.h
    ImportSelfTest.cpp
    ImportSelfTest.h
    main.cpp
    resource.h
    SelfTests.cpp
    SelfTests.h
    ShortcutSelfTest.cpp
    ShortcutSelfTest.h
    SnapSelfTest.cpp
    SnapSelfTest.h
    TianBoard.rc
    ${CMAKE_SOURCE_DIR}/res/TianBoard.qrc
)

# 跨模块 include 一律走 "模块/文件.h"。main.cpp 是装配点：除了 core/ 与 utils/，
# 它还负责把 lower 模块需要的实现装上（见 installBoardDropHandlerFactory）。
target_include_directories(TianBoard PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}   # rc.exe 靠它找到 TianBoard.ico（不同生成器的工作目录不同，不依赖隐式行为）
)

# 全部模块对象库并进同一个可执行 —— 单进程、无 DLL、无运行时插件扫描。
# 依赖环（docs/architecture.md 的债表）在 OBJECT 库里不构成链接顺序问题；
# 逐个拆环后才改成真的 STATIC 库。
target_link_libraries(TianBoard PRIVATE
    tb_singleapp
    tb_api
    tb_utils
    tb_platform
    tb_settings
    tb_model
    tb_content
    tb_whiteboard
    tb_web
    tb_desktop
    tb_recording
    tb_ui
    tb_core
)

# tb_sdk 给 Qt/QuaZip 头与编译定义；下面这些是只有最终可执行才需要的
# 链接项（OBJECT 库的 PRIVATE 链接不会自动带过来）。advapi32 是凭据管理器
# （CredRead/CredWrite，见 src/platform/CredentialStore_win.cpp）要的，crypt32 留给
# 以后要哈希/加解密时再说。
target_link_libraries(TianBoard PRIVATE
    tb_sdk
    winmm wmvcore ole32 oleaut32 user32 gdi32 advapi32 ws2_32 crypt32 bcrypt
)

if(TARGET TianBoardTranslations)
    add_dependencies(TianBoard TianBoardTranslations)
endif()

# --------------------------------------------------------------------------
# 运行期目录：与可执行文件同级
# --------------------------------------------------------------------------
foreach(_dir etc library customizations)
    if(EXISTS ${CMAKE_SOURCE_DIR}/res/${_dir})
        add_custom_command(TARGET TianBoard POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/res/${_dir}
                $<TARGET_FILE_DIR:TianBoard>/${_dir})
    endif()
endforeach()

if(TIANBOARD_QM_FILES)
    # .qm 产出在**根**二进制目录的 Language/ 下（lrelease 规则在根 CMakeLists 里），
    # 这里的源路径必须用 CMAKE_BINARY_DIR，用 CMAKE_CURRENT_BINARY_DIR 会指到
    # build/src/app/Language（不存在）从而把 POST_BUILD 整批弄失败。
    add_custom_command(TARGET TianBoard POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_BINARY_DIR}/Language
            $<TARGET_FILE_DIR:TianBoard>/Language)
endif()

# --------------------------------------------------------------------------
# 部署 Qt 运行时：cmake --build build --config Release --target deploy
# --------------------------------------------------------------------------
find_program(TIANBOARD_WINDEPLOYQT_EXECUTABLE windeployqt
    HINTS ${Qt6_DIR}/../../../bin ${CMAKE_PREFIX_PATH}/bin)

if(TIANBOARD_WINDEPLOYQT_EXECUTABLE)
    add_custom_target(deploy
        COMMAND ${TIANBOARD_WINDEPLOYQT_EXECUTABLE} --no-translations --compiler-runtime
                $<TARGET_FILE:TianBoard>
        DEPENDS TianBoard
        COMMENT "windeployqt: 拷贝 Qt 运行时到 $<TARGET_FILE_DIR:TianBoard>")
endif()
