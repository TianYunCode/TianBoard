# ======================================================================
# recording — 录屏：RecordingController（画面 + 声音采集）、录制调色板、
#             Windows Media 编码器与波形录音（编码器与波形录音原来是子目录，
#             现已扁平并入本模块）。
#
# 允许依赖：utils、model。尚未成立的边：recording <-> core / ui / whiteboard /
# web / platform。
# ======================================================================

add_library(tb_recording OBJECT
    AbstractVideoEncoder.cpp
    AbstractVideoEncoder.h
    RecordingController.cpp
    RecordingController.h
    RecordingPalette.cpp
    RecordingPalette.h
    WaveRecorder.cpp
    WaveRecorder.h
    WindowsMediaFile.cpp
    WindowsMediaFile.h
    WindowsMediaVideoEncoder.cpp
    WindowsMediaVideoEncoder.h
)

# 跨模块 include 一律写成 "模块/文件.h"，由这一条 src/ 路径解析；
# 同目录裸 include 不再允许 —— CMAKE_INCLUDE_CURRENT_DIR 是关的，裸写编译不过；
# 规矩本身写在 docs/coding-style.md。
target_include_directories(tb_recording PRIVATE ${CMAKE_SOURCE_DIR}/src)

# 依赖面：Qt 模块与编译定义都来自 tb_sdk（见根 CMakeLists.txt）
target_link_libraries(tb_recording PRIVATE tb_sdk)
