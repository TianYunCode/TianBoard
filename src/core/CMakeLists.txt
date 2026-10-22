# ======================================================================
# core — 各类服务/控制器：DisplayManager（多屏）、LanguageManager（取词）、
#        PersistenceManager（.tbz 落盘）、DocumentManager（文档库）、
#        NetworkAccessManager/HttpGet/AutoSaver
#        （原 network/；ProxyDialog 归 ui/）、TextTools、IdleTimer、
#        ForeignObjectsHandler、CookieJar、ShortcutManager。
#        Application（QApplication 子类，单实例）与 ApplicationController（启动与模式
#        切换）的头文件在这里，实现住在 app/ —— 见下面"允许依赖"。
#        偏好设置对话框（PreferencesController + PreferencesDialog）不住这里——它是界面，
#        2026-10 搬去 ui/。
#        配置层 Settings/Setting 已经抽到 src/settings/；Enums.h 与 resource.h
#        也不在这里：前者是纯定义（utils/），后者归可执行目标（app/）。
#
# 允许依赖：utils、settings、model、content、whiteboard。对 ui 的出边已经是 0——
# Application 与 ApplicationController 这两个装配角色的**声明**留在 core（每个模块都
# 要从这里拿 Application::mainWindow / applicationController 当入口），**实现**搬到
# app/：它们的函数体本来就是在建主窗口、挂四个控制器、连动作，那是装配层的工作，
# 不是服务层的。留在 core 只会让"服务层认识界面"这条债永远删不掉。
# core 剩下的上边（→whiteboard 认图形项、→content 认读写器、→model 认文档代理）
# 见 docs/architecture.md 的债表。
# ======================================================================

add_library(tb_core OBJECT
    Application.h
    ApplicationController.h
    CookieJar.cpp
    CookieJar.h
    DisplayManager.cpp
    DisplayManager.h
    DocumentManager.cpp
    DocumentManager.h
    DownloadManager.cpp
    DownloadManager.h
    HttpGet.cpp
    HttpGet.h
    IDesktopAnnotation.h
    IWebController.h
    IdleTimer.cpp
    IdleTimer.h
    LanguageManager.cpp
    LanguageManager.h
    NetworkAccessManager.cpp
    NetworkAccessManager.h
    PersistenceManager.cpp
    PersistenceManager.h
    SceneCache.cpp
    SceneCache.h
    ShortcutManager.cpp
    ShortcutManager.h
)

# 跨模块 include 一律写成 "模块/文件.h"，由这一条 src/ 路径解析；
# 同目录裸 include 不再允许 —— CMAKE_INCLUDE_CURRENT_DIR 是关的，裸写编译不过；
# 规矩本身写在 docs/coding-style.md。
target_include_directories(tb_core PRIVATE ${CMAKE_SOURCE_DIR}/src)

# 依赖面：Qt 模块与编译定义都来自 tb_sdk（见根 CMakeLists.txt）
target_link_libraries(tb_core PRIVATE tb_sdk)
