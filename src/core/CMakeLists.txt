# ======================================================================
# core — 应用与各类服务/控制器：Application（QApplication 子类，单实例）、
#        ApplicationController（启动与模式切换）、Settings/Setting（配置）、
#        DisplayManager/VirtualDesktop（多屏）、LanguageManager（取词）、
#        PersistenceManager（.tbz 落盘）、DocumentManager（文档库）、
#        SceneCache（场景缓存）、样式与资源（Resources 在 ui/，此处是控制器）、
#        DownloadManager/DownloadThread、NetworkAccessManager/HttpGet/AutoSaver
#        （原 network/；ProxyDialog 归 ui/）、MimeData、TextTools、IdleTimer、
#        ForeignObjectsHandler。
#        WB.h 与 resource.h 不在这里：前者是纯定义（utils/），后者归可执行目标（app/）。
#
# 允许依赖：几乎所有模块 —— 服务层本来就是装配点。拆环时要先把它对 ui/
# whiteboard 的具体类依赖换成接口（见 docs/architecture.md 的债表）。
# ======================================================================

add_library(tb_core OBJECT
    Application.cpp
    Application.h
    ApplicationController.cpp
    ApplicationController.h
    AutoSaver.cpp
    AutoSaver.h
    DisplayManager.cpp
    DisplayManager.h
    DocumentManager.cpp
    DocumentManager.h
    DownloadManager.cpp
    DownloadManager.h
    DownloadThread.cpp
    DownloadThread.h
    ForeignObjectsHandler.cpp
    ForeignObjectsHandler.h
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
    PreferencesController.cpp
    PreferencesController.h
    SceneCache.cpp
    SceneCache.h
    Setting.cpp
    Setting.h
    Settings.cpp
    Settings.h
    TextTools.cpp
    TextTools.h
    VirtualDesktop.cpp
    VirtualDesktop.h
)

# 跨模块 include 一律写成 "模块/文件.h"，由这一条 src/ 路径解析；
# 同目录裸 include 不再允许 —— CMAKE_INCLUDE_CURRENT_DIR 是关的，裸写编译不过；
# 规矩本身写在 docs/coding-style.md。
target_include_directories(tb_core PRIVATE ${CMAKE_SOURCE_DIR}/src)

# 依赖面：Qt 模块、OpenSSL 头、编译定义都来自 tb_sdk（见根 CMakeLists.txt）
target_link_libraries(tb_core PRIVATE tb_sdk)
