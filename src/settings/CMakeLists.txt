# ======================================================================
# settings — 用户配置的读写层：Settings（键与默认值集中的单例）+ Setting
#            （单条配置的代理，带 changed(QVariant) 信号，界面直接连它）。
#
# 从 core/ 抽出来是因为它两边都不该认识：
#   - 向下只用 utils/（FileSystemUtils、CryptoUtils、Enums）与 platform/
#     （PlatformUtils）；
#   - 向上被 62 个外部文件 include，是几乎每个模块都要用的服务。
# 留在 core/ 里，core 就同时是“装配点”和“被所有人依赖的服务”，现存的双向环
# 有一大半是这么来的。历史上它还在构造期去拉 whiteboard/ToolsManager 要内置工具
# 全集当默认值，连带把带 tr() 的工具标签建在翻译器装好之前——那条边已经断了，
# 默认值由唯一的读方 ui/FavoriteToolPalette 自己兜。
#
# 本模块可以做成 STATIC（它对上层的出边为 0），但 platform/ 反向 include 它，
# 所以先留在 OBJECT，等 platform 那批界面依赖拆掉再收紧。
# ======================================================================

add_library(tb_settings OBJECT
    Setting.cpp
    Setting.h
    Settings.cpp
    Settings.h
)

target_include_directories(tb_settings PRIVATE ${CMAKE_SOURCE_DIR}/src)

# 依赖面：Qt 模块与编译定义来自 tb_sdk（见根 CMakeLists.txt）
target_link_libraries(tb_settings PRIVATE tb_sdk)
