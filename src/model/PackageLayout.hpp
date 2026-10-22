/**
 * @file PackageLayout.hpp
 * @brief 汇总 .tbz 文档包的磁盘布局：仓库顶层目录名、包内子目录名与 folders.xml 的词表。
 *
 * 这些字符串全部属于磁盘格式契约：取值决定文档包里的目录叫什么、目录树文件里的节点怎么写，
 * 改动会让既有文档读不出来。新增图元类型需要新子目录时，必须同时登记进
 * `PersistenceManager::mAllDirectories`，否则该目录不会被创建、也不会被打包。
 *
 * 为什么住在 model 而不是 core/PersistenceManager：这些名字是"文档域"的词汇，不是某个服务的
 * 内部细节。挂在服务类上时，画板为了知道图片放哪个子目录就得 include 持久化服务
 * （`GraphicsScene`/`GraphicsPixmapItem`/`BoardPaletteManager` 都为此反向认识 core）。
 * model 是唯一同时满足各方声明的落点：whiteboard 允许依赖 model 与 utils，content 允许依赖
 * model，core 允许依赖 model——搬过来只减少向上的边，不新造任何一条。
 */

#pragma once

#include <QString>

/// .tbz 包内的对象子目录：每类图形项各自的落盘位置。
namespace PackageLayout
{
    /// @brief 位图与矢量图形的子目录名。
    inline const QString imageDirectory   = "images";
    /// @brief 通用对象（脚本、导出附件等）的子目录名。
    inline const QString objectDirectory  = "objects";
    /// @brief 小部件包的子目录名。
    inline const QString widgetDirectory  = "widgets";
    /// @brief 录制进文档的视频子目录名。
    inline const QString videoDirectory   = "videos";
    /// @brief 录制进文档的音频子目录名。
    inline const QString audioDirectory   = "audios";
    /// @brief 随文档携带的任意文件子目录名。
    inline const QString fileDirectory    = "files";

    /// @brief 文档仓库根分组的内部名（显示文案由界面自己 tr()，这里不是）。
    inline const QString myDocumentsName  = "MyDocuments";
    /// @brief 模型/示例分组的内部名。
    inline const QString modelsName       = "Models";

    /// @brief 目录树自身的文件名与它的 XML 标签名、属性名。
    inline const QString foldersFile      = "folders.xml";
    inline const QString folderTag        = "folder";
    inline const QString nameAttribute    = "name";
} // namespace PackageLayout
