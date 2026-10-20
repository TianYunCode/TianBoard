/**
 * @file TextTools.h
 * @brief 声明处理粘贴 HTML 的静态字符串工具。
 */

#pragma once

#include <QString>

/**
 * @brief HTML 文本清洗工具：去除空字符或截取完整的文档片段。
 *
 * 详细说明：纯静态类，不需要实例；所有函数无状态、可在任意线程调用。
 */
class TextTools {
public:
    TextTools() {}
    virtual ~TextTools() {}

    /// 删除字符串中的所有 NUL 字符。
    static QString cleanHtmlCData(const QString &_html);
    /// 截取从 doctype 到 </html> 的片段；找不到标记时返回空串。
    static QString cleanHtml(const QString &_html);
};
