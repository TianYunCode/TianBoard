# ======================================================================
# settings — 用户配置的读写层：Settings（键与默认值集中的单例）+ Setting
#            （单条配置的代理，带 changed(QVariant) 信号，界面直接连它）。
#
# 从 core/ 抽出来是因为它两边都不该认识：
#   - 向下只用 utils/（FileSystemUtils、CryptoUtils、Enums）；
#   - 向上被 62 个外部文件 include，是几乎每个模块都要用的服务。
# 留在 core/ 里，core 就同时是“装配点”和“被所有人依赖的服务”，现存的双向环
# 有一大半是这么来的。历史上它还在构造期去拉 whiteboard/ToolsManager 要内置工具
# 全集当默认值，连带把带 tr() 的工具标签建在翻译器装好之前——那条边已经断了，
# 默认值由唯一的读方 ui/FavoriteToolPalette 自己兜。
#
# 它原来还读 platform 拿资源目录与登录名，两条都断掉了：资源目录是部署约定，
# 归 utils/FileSystemUtils；登录名由 core/Application 在启动时 setUserLoginName
# 推进来。于是本模块出边只剩 utils，做成真正的 STATIC：放不进任何指向上层的
# include，越界在编译期就失败，而不是等链接期靠符号缺失才发现。
# ======================================================================

add_library(tb_settings STATIC
    Setting.cpp
    Setting.h
    Settings.cpp
    Settings.h
)

target_include_directories(tb_settings PRIVATE ${CMAKE_SOURCE_DIR}/src)

# 依赖面：Qt 模块与编译定义来自 tb_sdk（见根 CMakeLists.txt），下层只有 utils。
target_link_libraries(tb_settings PRIVATE tb_sdk tb_utils)
