/**
 * @file Version.h
 * @brief 声明可比较的版本串解析器。
 */

#pragma once

#include <QString>

/**
 * @brief 版本串比较器。
 *
 * 把 "4.8"、"4.8.0"、"4.8.0.rc.2" 折算进同一个全序，用来判断文档里记录的格式版本
 * 比当前读写器的新还是旧。解析不出的串一律得 0（视为最旧），宁可放行也不要对着
 * 一个读得懂的文档弹“版本过新”。对象不可变，可在任意线程构造与比较。
 */
class Version {
public:
    /** @brief 版本阶段；数值越大越接近正式版。 */
    enum class Stage : unsigned {
        Alpha = 10,            ///< 内测版。
        Beta = 11,             ///< 公测版。
        ReleaseCandidate = 15, ///< 候选发布版。
        Release = 17           ///< 正式版。
    };

    /** @brief 解析版本串；无法解析的部分按 0 处理。 */
    explicit Version(const QString &string);
    /** @brief 析构。 */
    ~Version();

    /** @brief 返回折算后的可比较整数。 */
    uint toUInt() const;

    /** @brief 严格小于比较。 */
    bool operator<(const Version &otherVersion) const;
    /** @brief 小于等于比较。 */
    bool operator<=(const Version &otherVersion) const;
    /** @brief 相等比较。 */
    bool operator==(const Version &otherVersion) const;
    /** @brief 严格大于比较。 */
    bool operator>(const Version &otherVersion) const;

private:
    QString mString; ///< 原始版本串，供调试与再次解析。
};
