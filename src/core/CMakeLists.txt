# ======================================================================
# core — 应用与各类服务/控制器：Application（QApplication 子类，单实例）、
#        ApplicationController（启动与模式切换）、DisplayManager/VirtualDesktop
#        （多屏）、LanguageManager（取词）、PersistenceManager（.tbz 落盘）、
#        DocumentManager（文档库）、NetworkAccessManager/HttpGet/AutoSaver
#        （原 network/；ProxyDialog 归 ui/）、MimeData、TextTools、IdleTimer、
#        ForeignObjectsHandler、CookieJar、ShortcutManager。
#        偏好设置对话框（PreferencesController + PreferencesDialog）不住这里——它是界面，
#        2026-10 搬去 ui/。
#        配置层 Settings/Setting 已经抽到 src/settings/；Enums.h 与 resource.h
#        也不在这里：前者是纯定义（utils/），后者归可执行目标（app/）。
#
# 允许依赖：几乎所有模块 —— 服务层本来就是装配点。拆环时要先把它对 ui/whiteboard 的
# 具体类依赖换成接口或构造期注入（见 docs/architecture.md 的债表与拆环第 9 步）：
# 剩下的上边主要来自 Application/ApplicationController 这两个装配角色，加上
# DocumentManager、PersistenceManager、DisplayManager、DownloadManager、
# NetworkAccessManager 五处"服务伸手拿界面"的漏点。
# ======================================================================

add_library(tb_core OBJECT
    Application.cpp
    Application.h
    ApplicationController.cpp
    ApplicationController.h
    AutoSaver.cpp
    AutoSaver.h
    CookieJar.cpp
    CookieJar.h
    CookieSelfTest.cpp
    CookieSelfTest.h
    DisplayManager.cpp
    DisplayManager.h
    DocumentManager.cpp
    DocumentManager.h
    DownloadManager.cpp
    DownloadManager.h
    ForeignObjectsHandler.cpp
    ForeignObjectsHandler.h
    FormatSelfTest.cpp
    FormatSelfTest.h
    HttpGet.cpp
    HttpGet.h
    IdleTimer.cpp
    IdleTimer.h
    LanguageManager.cpp
    LanguageManager.h
    MimeData.cpp
    MimeData.h
    NetworkAccessManager.cpp
    NetworkAccessManager.h
    PersistenceManager.cpp
    PersistenceManager.h
    SnapSelfTest.cpp
    SnapSelfTest.h
    SceneCache.cpp
    SceneCache.h
    ShortcutManager.cpp
    ShortcutManager.h
    ShortcutSelfTest.cpp
    ShortcutSelfTest.h
    TextTools.cpp
    TextTools.h
    VirtualDesktop.cpp
    VirtualDesktop.h
)

# 跨模块 include 一律写成 "模块/文件.h"，由这一条 src/ 路径解析；
# 同目录裸 include 不再允许 —— CMAKE_INCLUDE_CURRENT_DIR 是关的，裸写编译不过；
# 规矩本身写在 docs/coding-style.md。
target_include_directories(tb_core PRIVATE ${CMAKE_SOURCE_DIR}/src)

# 依赖面：Qt 模块与编译定义都来自 tb_sdk（见根 CMakeLists.txt）
target_link_libraries(tb_core PRIVATE tb_sdk)
