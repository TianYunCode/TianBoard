# QuaZip（.ubz 文档压缩）+ zlib，源码内联静态编译，替代原先 Qt5 预编译的 quazip.lib。
# 上游：QuaZip 1.5 (LGPL-2.1 with Qt exception) / zlib 1.3.1 (zlib license)
# 对外暴露 imported-style 别名 QuaZip::QuaZip，公开头文件目录含 quazip/ 与 zlib/。

add_library(tb_zlib STATIC
    zlib/adler32.c
    zlib/compress.c
    zlib/crc32.c
    zlib/deflate.c
    zlib/gzclose.c
    zlib/gzlib.c
    zlib/gzread.c
    zlib/gzwrite.c
    zlib/infback.c
    zlib/inffast.c
    zlib/inflate.c
    zlib/inftrees.c
    zlib/trees.c
    zlib/uncompr.c
    zlib/zutil.c
)

target_include_directories(tb_zlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/zlib)
target_compile_definitions(tb_zlib PRIVATE _CRT_SECURE_NO_WARNINGS)
set_target_properties(tb_zlib PROPERTIES POSITION_INDEPENDENT_CODE OFF)

add_library(tb_quazip STATIC
    quazip/JlCompress.cpp
    quazip/qioapi.cpp
    quazip/quaadler32.cpp
    quazip/quachecksum32.cpp
    quazip/quacrc32.cpp
    quazip/quagzipfile.cpp
    quazip/quaziodevice.cpp
    quazip/quazip.cpp
    quazip/quazipdir.cpp
    quazip/quazipfile.cpp
    quazip/quazipfileinfo.cpp
    quazip/quazipnewinfo.cpp
    quazip/unzip.c
    quazip/zip.c
)

target_link_libraries(tb_quazip PUBLIC Qt6::Core Qt6::Core5Compat tb_zlib)
target_include_directories(tb_quazip PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/quazip)
target_compile_definitions(tb_quazip PUBLIC QUAZIP_STATIC PRIVATE _CRT_SECURE_NO_WARNINGS)

add_library(QuaZip::QuaZip ALIAS tb_quazip)
