#pragma once

#include <QString>

// 版本串比较器：把 "4.8"、"4.8.0"、"4.8.0.rc.2" 折算进同一个全序，用来判断文档里
// 记的格式版本比当前读写器的新还是旧。解析不出来的串一律得 0（=最旧），宁可放行
// 也不要对着一个读得懂的文档弹“版本过新”。
class Version
{
public:
    enum class Stage : unsigned { Alpha = 10, Beta = 11, ReleaseCandidate = 15, Release = 17 };

    explicit Version(const QString &string);
    ~Version();

    uint toUInt() const;

    bool operator < (const Version &otherVersion) const;
    bool operator <= (const Version &otherVersion) const;
    bool operator == (const Version &otherVersion) const;
    bool operator > (const Version &otherVersion) const;

private:
    QString mString;
};
