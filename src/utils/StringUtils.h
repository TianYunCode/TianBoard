/**
 * @file StringUtils.h
 * @brief 声明文件名排序、数字命名、UUID 与 ISO 时间格式化的字符串工具。
 */

#pragma once

#include <QtCore>

/**
 * @brief 字符串处理的静态工具集。
 *
 * 位于 utils 层（纯工具）：不依赖画板状态，所有函数可重入、无副作用。
 * 不可实例化；解析失败时返回原串或空值，不抛异常。
 */
class StringUtils {
private:
    /** @brief 私有构造，禁止实例化。 */
    StringUtils() {}
    /** @brief 私有析构。 */
    ~StringUtils() {}

public:
    /** @brief 按末段数字自然排序；无法解析的数字按 0 处理。 */
    static QStringList sortByLastDigit(const QStringList &source);

    /** @brief 给名称末尾的数字加一，保持原位数。 */
    static QString netxDigitizedName(const QString &source);

    /** @brief 输出带花括号的标准 UUID 字符串。 */
    static QString toCanonicalUuid(const QUuid &uuid);

    /** @brief 把时间转成 UTC 的 ISO 8601 字符串。 */
    static QString toUtcIsoDateTime(const QDateTime &dateTime);
    /** @brief 解析 UTC 的 ISO 日期时间串；失败时返回无效 QDateTime。 */
    static QDateTime fromUtcIsoDate(const QString &dateString);
};
