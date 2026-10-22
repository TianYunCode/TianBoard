/**
 * @file MimeTypes.hpp
 * @brief 文档库拖放与剪贴板用的 MIME 类型名。
 */

#pragma once

#include <QString>

/**
 * @brief 应用自己的拖放格式名。
 *
 * 详细说明：这四个字符串只出现在运行期的 QMimeData 与剪贴板里，不写进 .tbz，所以改动不会
 * 让既有文档读不出来；但它们必须两端一致——发起拖放的一方与解析的一方读的是同一个常量。
 * 挂在 model 而不是 Application 上：文档库的树模型（model）、画板（whiteboard）与界面（ui）
 * 都要用，挂在应用对象上就等于谁想认出一个 MIME 类型都得先 include 最上层。取值沿用
 * `application/vnd.tianboard-*`。
 */
namespace MimeTypes
{
    /// @brief 文档库条目（整份文档或分组）的拖放类型。
    inline const QString boardDocument      = "application/vnd.tianboard-document";
    /// @brief 单个页面的拖放类型。
    inline const QString boardPage          = "application/vnd.tianboard-page";
    /// @brief 页面内图形条目的拖放类型。
    inline const QString boardPageItem      = "application/vnd.tianboard-page-item";
    /// @brief 页面缩略图的拖放类型。
    inline const QString boardPageThumbnail = "application/vnd.tianboard-thumbnail";
} // namespace MimeTypes
